#pragma once
#include <Game/Kepler/OrbitData.h>
#include <cmath>
#include <numbers>
#include <vector>
#include <algorithm>

namespace Simulator::Kepler
{
	constexpr double PI_2 = 6.28318530717958647692;

	inline double SolveKeplersEquationElliptical(double M, double e)
	{
		double E = M; // Initial guess
		for (int i = 0; i < 15; ++i)
		{
			double dE = (E - e * std::sin(E) - M) / (1.0 - e * std::cos(E));
			E -= dE;
			if (std::abs(dE) < 1e-7) break;
		}
		return E;
	}

	inline double SolveKeplersEquationHyperbolic(double M, double e)
	{
		double H = M; // Initial guess
		for (int i = 0; i < 15; ++i)
		{
			double dH = (e * std::sinh(H) - H - M) / (e * std::cosh(H) - 1.0);
			H -= dH;
			if (std::abs(dH) < 1e-7) break;
		}
		return H;
	}

	inline dx3d::Vec3d GetCentralPositionAtEccentricAnomaly(const OrbitData& d, double eccentricAnomaly)
	{
		if (d.Eccentricity < 1.0)
		{
			dx3d::Vec3d r(
				std::sin(eccentricAnomaly) * d.SemiMinorAxis,
				-std::cos(eccentricAnomaly) * d.SemiMajorAxis,
				0.0
			);
			return -d.SemiMinorAxisBasis * r.x - d.SemiMajorAxisBasis * r.y;
		}
		else if (d.Eccentricity > 1.0)
		{
			dx3d::Vec3d r(
				std::sinh(eccentricAnomaly) * d.SemiMinorAxis,
				std::cosh(eccentricAnomaly) * d.SemiMajorAxis,
				0.0
			);
			return -d.SemiMinorAxisBasis * r.x - d.SemiMajorAxisBasis * r.y;
		}
		else
		{
			double cosE = std::cos(eccentricAnomaly);
			dx3d::Vec3d r(
				d.PeriapsisDistance * std::sin(eccentricAnomaly) / (1.0 + cosE),
				d.PeriapsisDistance * cosE / (1.0 + cosE),
				0.0
			);
			return -d.SemiMinorAxisBasis * r.x + d.SemiMajorAxisBasis * r.y;
		}
	}

	inline dx3d::Vec3d GetFocalPositionAtEccentricAnomaly(const OrbitData& d, double eccentricAnomaly)
	{
		return GetCentralPositionAtEccentricAnomaly(d, eccentricAnomaly) + d.CenterPoint;
	}

	inline dx3d::Vec3d GetVelocityAtTrueAnomaly(const OrbitData& d, double trueAnomaly)
	{
		if (d.FocalParameter <= 0.0) return dx3d::Vec3d();

		double sqrtMGp = std::sqrt(d.AttractorMass * d.GravConst / d.FocalParameter);
		double vX = sqrtMGp * (d.Eccentricity + std::cos(trueAnomaly));
		double vY = sqrtMGp * std::sin(trueAnomaly);

		return -d.SemiMinorAxisBasis * vX - d.SemiMajorAxisBasis * vY;
	}

	inline void SetPositionAndVelocityByAnomaly(OrbitData& d)
	{
		d.positionRelativeToAttractor = GetFocalPositionAtEccentricAnomaly(d, d.EccentricAnomaly);
		d.velocityRelativeToAttractor = GetVelocityAtTrueAnomaly(d, d.TrueAnomaly);
	}

	inline void UpdateOrbitAnomaliesByTime(OrbitData& d, double deltaTime)
	{
		if (d.Eccentricity < 1.0)
		{
			d.MeanAnomaly += d.MeanMotion * deltaTime;
			d.MeanAnomaly = std::fmod(d.MeanAnomaly, PI_2);
			if (d.MeanAnomaly < 0.0) d.MeanAnomaly += PI_2;

			d.EccentricAnomaly = SolveKeplersEquationElliptical(d.MeanAnomaly, d.Eccentricity);

			double cosE = std::cos(d.EccentricAnomaly);
			d.TrueAnomaly = std::acos(std::clamp((cosE - d.Eccentricity) / (1.0 - d.Eccentricity * cosE), -1.0, 1.0));
			if (d.MeanAnomaly > std::numbers::pi) d.TrueAnomaly = PI_2 - d.TrueAnomaly;
		}
		else if (d.Eccentricity > 1.0)
		{
			d.MeanAnomaly += d.MeanMotion * deltaTime;
			d.EccentricAnomaly = SolveKeplersEquationHyperbolic(d.MeanAnomaly, d.Eccentricity);

			d.TrueAnomaly = std::atan2(
				std::sqrt(d.Eccentricity * d.Eccentricity - 1.0) * std::sinh(d.EccentricAnomaly),
				d.Eccentricity - std::cosh(d.EccentricAnomaly)
			);
		}

		SetPositionAndVelocityByAnomaly(d);
	}

	inline void ComputeSphereOfInfluence(OrbitData& d)
	{
		if (d.AttractorMass > 0.0 && d.BodyMass > 0.0)
		{
			d.SphereOfInfluenceRadius = d.SemiMajorAxis * std::pow(d.BodyMass / d.AttractorMass, 2.0 / 5.0);
		}
	}

	inline void CalculateOrbitStateFromElements(OrbitData& d)
	{
		d.MG = d.AttractorMass * d.GravConst;
		d.OrbitNormal = -dx3d::Vec3d::Cross(d.SemiMajorAxisBasis, d.SemiMinorAxisBasis).normalized();

		// Assuming Ecliptic Normal is standard Up (0, 1, 0)
		dx3d::Vec3d eclipticNormal(0.0, 1.0, 0.0);
		d.OrbitNormalDotEclipticNormal = dx3d::Vec3d::Dot(d.OrbitNormal, eclipticNormal);

		if (d.Eccentricity < 1.0)
		{
			d.OrbitCompressionRatio = 1.0 - d.Eccentricity * d.Eccentricity;
			d.CenterPoint = -d.SemiMajorAxisBasis * d.SemiMajorAxis * d.Eccentricity;
			d.Period = PI_2 * std::sqrt(std::pow(d.SemiMajorAxis, 3) / d.MG);
			d.MeanMotion = PI_2 / d.Period;
			d.Apoapsis = d.CenterPoint - d.SemiMajorAxisBasis * d.SemiMajorAxis;
			d.Periapsis = d.CenterPoint + d.SemiMajorAxisBasis * d.SemiMajorAxis;
			d.PeriapsisDistance = d.Periapsis.magnitude();
			d.ApoapsisDistance = d.Apoapsis.magnitude();
		}
		else if (d.Eccentricity > 1.0)
		{
			double inf = std::numeric_limits<double>::infinity();
			d.CenterPoint = d.SemiMajorAxisBasis * d.SemiMajorAxis * d.Eccentricity;
			d.Period = inf;
			d.MeanMotion = std::sqrt(d.MG / std::pow(d.SemiMajorAxis, 3));
			d.Apoapsis = dx3d::Vec3d(inf, inf, inf);
			d.Periapsis = d.CenterPoint - d.SemiMajorAxisBasis * d.SemiMajorAxis;
			d.PeriapsisDistance = d.Periapsis.magnitude();
			d.ApoapsisDistance = inf;
		}
		else
		{
			double inf = std::numeric_limits<double>::infinity();
			d.CenterPoint = dx3d::Vec3d();
			d.Period = inf;
			d.MeanMotion = std::sqrt(d.MG * 0.5 / std::pow(d.PeriapsisDistance, 3));
			d.Apoapsis = dx3d::Vec3d(inf, inf, inf);
			d.PeriapsisDistance = d.SemiMajorAxis;
			d.SemiMajorAxis = 0.0;
			d.Periapsis = -d.SemiMajorAxisBasis * d.PeriapsisDistance;
			d.ApoapsisDistance = inf;
		}

		d.positionRelativeToAttractor = GetFocalPositionAtEccentricAnomaly(d, d.EccentricAnomaly);

		double comp = d.Eccentricity < 1.0
			? (1.0 - d.Eccentricity * d.Eccentricity)
			: (d.Eccentricity * d.Eccentricity - 1.0);
		d.FocalParameter = d.SemiMajorAxis * comp;

		d.velocityRelativeToAttractor = GetVelocityAtTrueAnomaly(d, d.TrueAnomaly);
		d.AttractorDistance = d.positionRelativeToAttractor.magnitude();
		ComputeSphereOfInfluence(d);

		d.isPathDirty = true;
	}

	// Helper: Angle between two vectors in radians
	inline double AngleRad(const dx3d::Vec3d& from, const dx3d::Vec3d& to)
	{
		double dot = dx3d::Vec3d::Dot(from.normalized(), to.normalized());
		return std::acos(std::clamp(dot, -1.0, 1.0));
	}

	// Helper: Convert True Anomaly to Eccentric Anomaly
	inline double ConvertTrueToEccentricAnomaly(double trueAnomaly, double eccentricity)
	{
		if (eccentricity < 1.0)
		{
			// Elliptical
			double eCos = eccentricity * std::cos(trueAnomaly);
			double sinE = (std::sqrt(1.0 - eccentricity * eccentricity) * std::sin(trueAnomaly)) / (1.0 + eCos);
			double cosE = (eccentricity + std::cos(trueAnomaly)) / (1.0 + eCos);
			return std::atan2(sinE, cosE);
		}
		else if (eccentricity > 1.0)
		{
			// Hyperbolic
			double coshH = (eccentricity + std::cos(trueAnomaly)) / (1.0 + eccentricity * std::cos(trueAnomaly));
			// acosh returns positive; sign matches true anomaly
			double H = std::acosh(std::max(1.0, coshH));
			return (trueAnomaly < 0 || trueAnomaly > std::numbers::pi) ? -H : H;
		}
		else
		{
			// Parabolic
			return std::tan(trueAnomaly * 0.5);
		}
	}

	inline void CalculateOrbitStateFromOrbitalVectors(OrbitData& d)
	{
		d.MG = d.AttractorMass * d.GravConst;
		d.AttractorDistance = d.positionRelativeToAttractor.magnitude();

		dx3d::Vec3d h = dx3d::Vec3d::Cross(d.positionRelativeToAttractor, d.velocityRelativeToAttractor);
		d.OrbitNormal = h.normalized();

		dx3d::Vec3d ecc;
		dx3d::Vec3d eclipticUp(0.0, 1.0, 0.0); // Assuming EclipticConstants.EclipticUp is Y-up

		// If the object is falling straight down (or perfectly straight up), cross product is 0
		if (d.OrbitNormal.sqrMagnitude() < 0.99)
		{
			d.OrbitNormal = dx3d::Vec3d::Cross(d.positionRelativeToAttractor, eclipticUp).normalized();
			ecc = dx3d::Vec3d();
		}
		else
		{
			ecc = dx3d::Vec3d::Cross(d.velocityRelativeToAttractor, h) / d.MG -
				(d.positionRelativeToAttractor / d.AttractorDistance);
		}

		d.OrbitNormalDotEclipticNormal = dx3d::Vec3d::Dot(d.OrbitNormal, eclipticUp);
		d.FocalParameter = h.sqrMagnitude() / d.MG;
		d.Eccentricity = ecc.magnitude();

		d.SemiMinorAxisBasis = dx3d::Vec3d::Cross(h, -ecc).normalized();
		if (d.SemiMinorAxisBasis.sqrMagnitude() < 0.99)
		{
			d.SemiMinorAxisBasis = dx3d::Vec3d::Cross(d.OrbitNormal, d.positionRelativeToAttractor).normalized();
		}

		d.SemiMajorAxisBasis = dx3d::Vec3d::Cross(d.OrbitNormal, d.SemiMinorAxisBasis).normalized();

		double inf = std::numeric_limits<double>::infinity();

		// Branching based on orbit shape
		if (d.Eccentricity < 1.0) // Elliptical
		{
			d.OrbitCompressionRatio = 1.0 - d.Eccentricity * d.Eccentricity;
			d.SemiMajorAxis = d.FocalParameter / d.OrbitCompressionRatio;
			d.SemiMinorAxis = d.SemiMajorAxis * std::sqrt(d.OrbitCompressionRatio);
			d.CenterPoint = -ecc * d.SemiMajorAxis;

			double p = std::sqrt(std::pow(d.SemiMajorAxis, 3) / d.MG);
			d.Period = PI_2 * p;
			d.MeanMotion = 1.0 / p;

			d.Apoapsis = d.CenterPoint - d.SemiMajorAxisBasis * d.SemiMajorAxis;
			d.Periapsis = d.CenterPoint + d.SemiMajorAxisBasis * d.SemiMajorAxis;
			d.PeriapsisDistance = d.Periapsis.magnitude();
			d.ApoapsisDistance = d.Apoapsis.magnitude();

			d.TrueAnomaly = AngleRad(d.positionRelativeToAttractor, d.SemiMajorAxisBasis);
			if (dx3d::Vec3d::Dot(dx3d::Vec3d::Cross(d.positionRelativeToAttractor, -d.SemiMajorAxisBasis), d.OrbitNormal) < 0.0)
			{
				d.TrueAnomaly = PI_2 - d.TrueAnomaly;
			}

			d.EccentricAnomaly = ConvertTrueToEccentricAnomaly(d.TrueAnomaly, d.Eccentricity);
			d.MeanAnomaly = d.EccentricAnomaly - d.Eccentricity * std::sin(d.EccentricAnomaly);
		}
		else if (d.Eccentricity > 1.0) // Hyperbolic
		{
			d.OrbitCompressionRatio = d.Eccentricity * d.Eccentricity - 1.0;
			d.SemiMajorAxis = d.FocalParameter / d.OrbitCompressionRatio;
			d.SemiMinorAxis = d.SemiMajorAxis * std::sqrt(d.OrbitCompressionRatio);
			d.CenterPoint = ecc * d.SemiMajorAxis;

			d.Period = inf;
			d.MeanMotion = std::sqrt(d.MG / std::pow(d.SemiMajorAxis, 3));

			d.Apoapsis = dx3d::Vec3d(inf, inf, inf);
			d.Periapsis = d.CenterPoint - d.SemiMajorAxisBasis * d.SemiMajorAxis;
			d.PeriapsisDistance = d.Periapsis.magnitude();
			d.ApoapsisDistance = inf;

			d.TrueAnomaly = AngleRad(d.positionRelativeToAttractor, ecc);
			if (dx3d::Vec3d::Dot(dx3d::Vec3d::Cross(d.positionRelativeToAttractor, -d.SemiMajorAxisBasis), d.OrbitNormal) < 0.0)
			{
				d.TrueAnomaly = -d.TrueAnomaly;
			}

			d.EccentricAnomaly = ConvertTrueToEccentricAnomaly(d.TrueAnomaly, d.Eccentricity);
			d.MeanAnomaly = std::sinh(d.EccentricAnomaly) * d.Eccentricity - d.EccentricAnomaly;
		}
		else // Parabolic
		{
			d.OrbitCompressionRatio = 0.0;
			d.SemiMajorAxis = 0.0;
			d.SemiMinorAxis = 0.0;
			d.PeriapsisDistance = h.sqrMagnitude() / d.MG;
			d.CenterPoint = dx3d::Vec3d();
			d.Periapsis = -d.SemiMinorAxisBasis * d.PeriapsisDistance;

			d.Period = inf;
			d.MeanMotion = std::sqrt(d.MG / std::pow(d.PeriapsisDistance, 3));

			d.Apoapsis = dx3d::Vec3d(inf, inf, inf);
			d.ApoapsisDistance = inf;

			d.TrueAnomaly = AngleRad(d.positionRelativeToAttractor, ecc);
			if (dx3d::Vec3d::Dot(dx3d::Vec3d::Cross(d.positionRelativeToAttractor, -d.SemiMajorAxisBasis), d.OrbitNormal) < 0.0)
			{
				d.TrueAnomaly = -d.TrueAnomaly;
			}

			d.EccentricAnomaly = ConvertTrueToEccentricAnomaly(d.TrueAnomaly, d.Eccentricity);
			d.MeanAnomaly = std::sinh(d.EccentricAnomaly) * d.Eccentricity - d.EccentricAnomaly;
		}

		ComputeSphereOfInfluence(d);
		
		d.isPathDirty = true;
	}

	inline dx3d::Vec3d GetCentralPositionAtTrueAnomaly(const OrbitData& d, double trueAnomaly)
    {
        double ecc = ConvertTrueToEccentricAnomaly(trueAnomaly, d.Eccentricity);
        return GetCentralPositionAtEccentricAnomaly(d, ecc);
    }

    inline dx3d::Vec3d GetFocalPositionAtTrueAnomaly(const OrbitData& d, double trueAnomaly)
    {
        return GetCentralPositionAtTrueAnomaly(d, trueAnomaly) + d.CenterPoint;
    }

    inline double CalcTrueAnomalyForDistance(const OrbitData& d, double distance)
    {
        if (d.Eccentricity <= 0.0) return std::numbers::pi;
        
        // Derived from the polar orbital equation: r = p / (1 + e * cos(nu))
        double cosNu = (d.FocalParameter / distance - 1.0) / d.Eccentricity;
        return std::acos(std::clamp(cosNu, -1.0, 1.0));
    }

    inline void GenerateEllipticOrbitPoints(const OrbitData& d, std::vector<dx3d::Vec3d>& orbitPoints, int orbitPointsCount, const dx3d::Vec3d& origin)
    {
        orbitPoints.resize(orbitPointsCount);

        for (int i = 0; i < orbitPointsCount; ++i)
        {
            double eccentricAnomaly = i * PI_2 / (orbitPointsCount - 1);
            orbitPoints[i] = GetFocalPositionAtEccentricAnomaly(d, eccentricAnomaly) + origin;
        }
    }

    inline void GenerateHyperbolicOrbitPoints(const OrbitData& d, std::vector<dx3d::Vec3d>& orbitPoints, int orbitPointsCount, const dx3d::Vec3d& origin, double maxDistance)
    {
        if (maxDistance < d.PeriapsisDistance)
        {
            orbitPoints.clear();
            return;
        }

        double maxAngle = CalcTrueAnomalyForDistance(d, maxDistance);
        orbitPoints.resize(orbitPointsCount);

        for (int i = 0; i < orbitPointsCount; ++i)
        {
            double trueAnomaly = -maxAngle + i * (2.0 * maxAngle) / (orbitPointsCount - 1);
            orbitPoints[i] = GetFocalPositionAtTrueAnomaly(d, trueAnomaly) + origin;
        }
    }

    inline void GetOrbitPoints(const OrbitData& d, std::vector<dx3d::Vec3d>& orbitPoints, int orbitPointsCount, const dx3d::Vec3d& gravitySourceOrigin, double maxDistance = 500.0)
    {
        if (orbitPointsCount < 2)
        {
            orbitPoints.clear();
            return;
        }

        if (d.Eccentricity < 1.0)
        {
            GenerateEllipticOrbitPoints(d, orbitPoints, orbitPointsCount, gravitySourceOrigin);
        }
        else
        {
            GenerateHyperbolicOrbitPoints(d, orbitPoints, orbitPointsCount, gravitySourceOrigin, maxDistance);
        }
    }

	inline void UpdateAllOrbits(std::vector<OrbitData>& allOrbits, double scaledDeltaTime)
	{
		for (size_t i = 0; i < allOrbits.size(); ++i)
		{
			// Only update orbits that have a valid attractor
			if (allOrbits[i].AttractorMass > 0.0)
			{
				UpdateOrbitAnomaliesByTime(allOrbits[i], scaledDeltaTime);
			}
		}
	}
}