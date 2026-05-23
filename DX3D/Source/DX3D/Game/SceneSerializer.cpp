#include <DX3D/Game/SceneSerializer.h>
#include <Game/Kepler/OrbitData.h>
#include <fstream>
#include <algorithm>
#include <vector>

namespace dx3d {

	SceneSerializer::SceneSerializer(Registry& registry, TransformSystem& tSys, OrbitSystem& oSys)
		: m_registry(registry), m_transformSystem(tSys), m_orbitSystem(oSys) {
	}

	int SceneSerializer::CalculateSimDepth(Entity entity) const {
		int depth = 0;
		Entity current = entity;


		while (m_orbitSystem.hasOrbit(current)) {
			const auto& orbit = m_orbitSystem.getOrbit(current);
			if (orbit.ParentEntity == Entity::Null) {
				break;
			}
			++depth;
			current = orbit.ParentEntity;
		}
		return depth;
	}

	uint32_t SceneSerializer::GetEntityIdFromOrbitEntity(Entity entity) const {
		return entity == Entity::Null ? static_cast<uint32_t>(-1) : entity.id;
	}

	bool SceneSerializer::Serialize(const std::string& filepath) {
		nlohmann::json sceneRoot;
		sceneRoot["entities"] = nlohmann::json::array();

		struct NodeWrapper {
			Entity entity;
			int simDepth;
			nlohmann::json jsonPayload;
		};

		std::vector<NodeWrapper> sortingPayload;
		const auto& activeEntities = m_transformSystem.getRawEntities();
		const auto& denseTransforms = m_transformSystem.getRawData();

		for (size_t i = 0; i < activeEntities.size(); ++i) {
			Entity e = activeEntities[i];
			NodeWrapper node;
			node.entity = e;
			node.simDepth = 0;

			node.jsonPayload["fileId"] = e.id;
			bool hasOrbit = m_orbitSystem.hasOrbit(e);
			node.jsonPayload["hasOrbit"] = hasOrbit;

			if (hasOrbit) {
				size_t orbitIndex = m_orbitSystem.getSparseIndex(e);
				node.simDepth = CalculateSimDepth(e);
				const Simulator::OrbitData& orbit = m_orbitSystem.getOrbit(e);

				// Populate 100% of spatial variables, bases, masses, and state arrays
				node.jsonPayload["orbit"] = {
					// Masses and Constants
					{"attractorMass", orbit.AttractorMass},
					{"bodyMass", orbit.BodyMass},
					{"gravConst", orbit.GravConst},
					{"mg", orbit.MG},

					// Primary Orbital Elements
					{"semiMajorAxis", orbit.SemiMajorAxis},
					{"semiMinorAxis", orbit.SemiMinorAxis},
					{"eccentricity", orbit.Eccentricity},
					{"focalParameter", orbit.FocalParameter},

					// Anomalies and Time
					{"period", orbit.Period},
					{"meanMotion", orbit.MeanMotion},
					{"trueAnomaly", orbit.TrueAnomaly},
					{"meanAnomalyEpoch", orbit.MeanAnomaly}, // Mapped to MeanAnomaly per spec
					{"eccentricAnomaly", orbit.EccentricAnomaly},

					// Spatial Geometric Bases
					{"centerPoint", {orbit.CenterPoint.x, orbit.CenterPoint.y, orbit.CenterPoint.z}},
					{"orbitNormal", {orbit.OrbitNormal.x, orbit.OrbitNormal.y, orbit.OrbitNormal.z}},
					{"semiMajorAxisBasis", {orbit.SemiMajorAxisBasis.x, orbit.SemiMajorAxisBasis.y, orbit.SemiMajorAxisBasis.z}},
					{"semiMinorAxisBasis", {orbit.SemiMinorAxisBasis.x, orbit.SemiMinorAxisBasis.y, orbit.SemiMinorAxisBasis.z}},

					{"periapsis", {orbit.Periapsis.x, orbit.Periapsis.y, orbit.Periapsis.z}},
					{"apoapsis", {orbit.Apoapsis.x, orbit.Apoapsis.y, orbit.Apoapsis.z}},
					{"periapsisDistance", orbit.PeriapsisDistance},
					{"apoapsisDistance", orbit.ApoapsisDistance},

					// State Vectors & Hierarchies
					{"parentFileId", GetEntityIdFromOrbitEntity(orbit.ParentEntity)},
					{"positionRelativeToAttractor", {orbit.positionRelativeToAttractor.x, orbit.positionRelativeToAttractor.y, orbit.positionRelativeToAttractor.z}},
					{"absoluteWorldPosition", {orbit.absoluteWorldPosition.x, orbit.absoluteWorldPosition.y, orbit.absoluteWorldPosition.z}},
					{"orbitalVelocity", {orbit.velocityRelativeToAttractor.x, orbit.velocityRelativeToAttractor.y, orbit.velocityRelativeToAttractor.z}},
					{"attractorDistance", orbit.AttractorDistance},

					// Auxiliary State & Rendering Configuration Parameters
					{"orbitCompressionRatio", orbit.OrbitCompressionRatio},
					{"orbitNormalDotEclipticNormal", orbit.OrbitNormalDotEclipticNormal},
					{"sphereOfInfluenceRadius", orbit.SphereOfInfluenceRadius},
					{"isFrozen", orbit.isFrozen},
					{"freezeColor", orbit.freezeColor},
					{"orbitColor", {orbit.orbitColor.x, orbit.orbitColor.y, orbit.orbitColor.z, orbit.orbitColor.w}}
				};
			}
			else {
				// If hasOrbit is false, fall back to writing explicit absolute transform definitions
				Vec3d p = denseTransforms[i].getPosition();
				Vec3d s = denseTransforms[i].getScale();
				node.jsonPayload["transform"] = {
					{"position", {p.x, p.y, p.z}},
					// FIXME
					{"rotation", {denseTransforms[i].getQuaternion().x, denseTransforms[i].getQuaternion().y, denseTransforms[i].getQuaternion().z, denseTransforms[i].getQuaternion().w}},
					{"scale", {s.x, s.y, s.z}}
				};
			}
			sortingPayload.push_back(node);
		}
		std::sort(sortingPayload.begin(), sortingPayload.end(), [](const NodeWrapper& a, const NodeWrapper& b) {
			return a.simDepth < b.simDepth;
			});

		for (const auto& node : sortingPayload) {
			sceneRoot["entities"].push_back(node.jsonPayload);
		}

		std::ofstream outFile(filepath);
		if (!outFile.is_open()) return false;

		outFile << sceneRoot.dump(4);
		return true;
	}

	bool SceneSerializer::Deserialize(const std::string& filepath) {
		std::ifstream inFile(filepath);
		if (!inFile.is_open()) return false;

		nlohmann::json sceneRoot;
		inFile >> sceneRoot;

		// Clean out runtime structures cleanly to prevent cache fragmentation alignment drift
		m_registry.clear();
		m_transformSystem.clear();
		m_orbitSystem.clear();

		std::unordered_map<uint32_t, Entity> fileToRuntimeMap;

		// --- PASS 1: Node Instantiation ---
		for (const auto& jEnt : sceneRoot["entities"]) {
			Entity liveEntity = m_registry.create();
			uint32_t fileId = jEnt["fileId"];
			fileToRuntimeMap[fileId] = liveEntity;

			Transform t;
			Vec3d tPos;
			Vec3d tRot;
			double w;
			Vec3d tScale;
			if (!jEnt["hasOrbit"]) {
				tPos.x = jEnt["transform"]["position"][0];
				tPos.y = jEnt["transform"]["position"][1];
				tPos.z = jEnt["transform"]["position"][2];

				tRot.x = jEnt["transform"]["rotation"][0];
				tRot.y = jEnt["transform"]["rotation"][1];
				tRot.z = jEnt["transform"]["rotation"][2];
				w = jEnt["transform"]["rotation"][3];

				tScale.x = jEnt["transform"]["scale"][0];
				tScale.y = jEnt["transform"]["scale"][1];
				tScale.z = jEnt["transform"]["scale"][2];
			}
			t.setPosition(tPos);
			t.setQuaternion(tRot.x, tRot.y, tRot.z, w);
			t.setScale(tScale);
			m_transformSystem.assignTransform(liveEntity, t);
		}

		// --- PASS 2: Component & Mathematical Reconstruction ---
		for (const auto& jEnt : sceneRoot["entities"]) {
			if (jEnt["hasOrbit"]) {
				uint32_t fileId = jEnt["fileId"];
				Entity liveEntity = fileToRuntimeMap[fileId];

				Simulator::OrbitData loadedOrbit;

				// Masses and Constants
				loadedOrbit.AttractorMass = jEnt["orbit"]["attractorMass"];
				loadedOrbit.BodyMass = jEnt["orbit"]["bodyMass"];
				loadedOrbit.GravConst = jEnt["orbit"]["gravConst"];
				loadedOrbit.MG = jEnt["orbit"]["mg"];

				// Primary Orbital Elements
				loadedOrbit.SemiMajorAxis = jEnt["orbit"]["semiMajorAxis"];
				loadedOrbit.SemiMinorAxis = jEnt["orbit"]["semiMinorAxis"];
				loadedOrbit.Eccentricity = jEnt["orbit"]["eccentricity"];
				loadedOrbit.FocalParameter = jEnt["orbit"]["focalParameter"];

				// Anomalies and Time
				loadedOrbit.Period = jEnt["orbit"]["period"];
				loadedOrbit.MeanMotion = jEnt["orbit"]["meanMotion"];
				loadedOrbit.TrueAnomaly = jEnt["orbit"]["trueAnomaly"];
				loadedOrbit.MeanAnomaly = jEnt["orbit"]["meanAnomalyEpoch"]; // Maps from "meanAnomalyEpoch"
				loadedOrbit.EccentricAnomaly = jEnt["orbit"]["eccentricAnomaly"];

				// Spatial Geometric Bases (Parsed directly out of JSON arrays)
				loadedOrbit.CenterPoint.x = jEnt["orbit"]["centerPoint"][0];
				loadedOrbit.CenterPoint.y = jEnt["orbit"]["centerPoint"][1];
				loadedOrbit.CenterPoint.z = jEnt["orbit"]["centerPoint"][2];

				loadedOrbit.OrbitNormal.x = jEnt["orbit"]["orbitNormal"][0];
				loadedOrbit.OrbitNormal.y = jEnt["orbit"]["orbitNormal"][1];
				loadedOrbit.OrbitNormal.z = jEnt["orbit"]["orbitNormal"][2];

				loadedOrbit.SemiMajorAxisBasis.x = jEnt["orbit"]["semiMajorAxisBasis"][0];
				loadedOrbit.SemiMajorAxisBasis.y = jEnt["orbit"]["semiMajorAxisBasis"][1];
				loadedOrbit.SemiMajorAxisBasis.z = jEnt["orbit"]["semiMajorAxisBasis"][2];

				loadedOrbit.SemiMinorAxisBasis.x = jEnt["orbit"]["semiMinorAxisBasis"][0];
				loadedOrbit.SemiMinorAxisBasis.y = jEnt["orbit"]["semiMinorAxisBasis"][1];
				loadedOrbit.SemiMinorAxisBasis.z = jEnt["orbit"]["semiMinorAxisBasis"][2];

				loadedOrbit.Periapsis.x = jEnt["orbit"]["periapsis"][0];
				loadedOrbit.Periapsis.y = jEnt["orbit"]["periapsis"][1];
				loadedOrbit.Periapsis.z = jEnt["orbit"]["periapsis"][2];

				loadedOrbit.Apoapsis.x = jEnt["orbit"]["apoapsis"][0];
				loadedOrbit.Apoapsis.y = jEnt["orbit"]["apoapsis"][1];
				loadedOrbit.Apoapsis.z = jEnt["orbit"]["apoapsis"][2];

				loadedOrbit.PeriapsisDistance = jEnt["orbit"]["periapsisDistance"];
				loadedOrbit.ApoapsisDistance = jEnt["orbit"]["apoapsisDistance"];

				// State Vectors
				loadedOrbit.positionRelativeToAttractor.x = jEnt["orbit"]["positionRelativeToAttractor"][0];
				loadedOrbit.positionRelativeToAttractor.y = jEnt["orbit"]["positionRelativeToAttractor"][1];
				loadedOrbit.positionRelativeToAttractor.z = jEnt["orbit"]["positionRelativeToAttractor"][2];

				loadedOrbit.absoluteWorldPosition.x = jEnt["orbit"]["absoluteWorldPosition"][0];
				loadedOrbit.absoluteWorldPosition.y = jEnt["orbit"]["absoluteWorldPosition"][1];
				loadedOrbit.absoluteWorldPosition.z = jEnt["orbit"]["absoluteWorldPosition"][2];

				loadedOrbit.velocityRelativeToAttractor.x = jEnt["orbit"]["orbitalVelocity"][0]; // Maps from "orbitalVelocity"
				loadedOrbit.velocityRelativeToAttractor.y = jEnt["orbit"]["orbitalVelocity"][1];
				loadedOrbit.velocityRelativeToAttractor.z = jEnt["orbit"]["orbitalVelocity"][2];

				loadedOrbit.AttractorDistance = jEnt["orbit"]["attractorDistance"];

				// Auxiliary State & Rendering Configuration Parameters
				loadedOrbit.OrbitCompressionRatio = jEnt["orbit"]["orbitCompressionRatio"];
				loadedOrbit.OrbitNormalDotEclipticNormal = jEnt["orbit"]["orbitNormalDotEclipticNormal"];
				loadedOrbit.SphereOfInfluenceRadius = jEnt["orbit"]["sphereOfInfluenceRadius"];
				loadedOrbit.isFrozen = jEnt["orbit"]["isFrozen"];
				loadedOrbit.freezeColor = jEnt["orbit"]["freezeColor"];

				loadedOrbit.orbitColor.x = jEnt["orbit"]["orbitColor"][0];
				loadedOrbit.orbitColor.y = jEnt["orbit"]["orbitColor"][1];
				loadedOrbit.orbitColor.z = jEnt["orbit"]["orbitColor"][2];
				loadedOrbit.orbitColor.w = jEnt["orbit"]["orbitColor"][3];

				// Force a line rendering path update on layout recovery
				loadedOrbit.visualDirty = true;

				// Parent relational allocation (ParentOrbitIndex will be patched afterwards via tracking maps)
				uint32_t parentFileId = jEnt["orbit"]["parentFileId"];
				if (parentFileId != static_cast<uint32_t>(-1) && fileToRuntimeMap.count(parentFileId)) {
					Entity liveParentEntity = fileToRuntimeMap[parentFileId];
					loadedOrbit.ParentEntity = liveParentEntity;
				}
				else {
					loadedOrbit.ParentEntity = Entity::Null; // Fallback back to local master frame root
				}

				m_orbitSystem.assignOrbitToEntity(liveEntity, loadedOrbit);

				// Coordinate sync immediately copies analytical position parameters onto spatial coordinates
				if (m_transformSystem.hasTransform(liveEntity)) {
					m_transformSystem.getTransform(liveEntity).setPosition(loadedOrbit.absoluteWorldPosition);
				}
			}
		}

		return true;
	}

}