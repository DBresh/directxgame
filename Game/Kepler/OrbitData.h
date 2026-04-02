#pragma once
#include <Game/Utility/Vec3d.h>

namespace Simulator
{
    struct OrbitData
    {
        // Masses and Constants
        double AttractorMass = 0.0;
        double BodyMass = 1.0;
        double GravConst = 1.0;
        double MG = 0.0;

        // Primary Orbital Elements
        double Eccentricity = 0.0;
        double SemiMajorAxis = 0.0;
        double SemiMinorAxis = 0.0;
        double FocalParameter = 0.0;

        // Anomalies and Time
        double Period = 0.0;
        double MeanMotion = 0.0;
        double TrueAnomaly = 0.0;
        double MeanAnomaly = 0.0;
        double EccentricAnomaly = 0.0;

        // Spatial Vectors and Bases
        Vec3d CenterPoint;
        Vec3d OrbitNormal;
        Vec3d SemiMinorAxisBasis;
        Vec3d SemiMajorAxisBasis;

        Vec3d Periapsis;
        Vec3d Apoapsis;
        double PeriapsisDistance = 0.0;
        double ApoapsisDistance = 0.0;

        // State Vectors
        Vec3d positionRelativeToAttractor;
        Vec3d velocityRelativeToAttractor;
        double AttractorDistance = 0.0;

        // Additional Parameters
        double OrbitCompressionRatio = 0.0;
        double OrbitNormalDotEclipticNormal = 0.0;
        double SphereOfInfluenceRadius = 0.0;

        // Rendering state
        bool isPathDirty = true; // if path dirty, then visualisator will update orbit points once
    };
}