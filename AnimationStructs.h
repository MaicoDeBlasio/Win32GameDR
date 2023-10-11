//
// Animation and AI structs for opponents and non-player game objects.
//

#pragma once

// RaytracingHlslCompat.h declares 'using' DirectX namespaces, so it must be in the #include list first.

namespace PhysicsConstants
{
    //using namespace DirectX::SimpleMath;
    const auto  Gravity		= Vector3(0,-9.81f, 0);// in m/s/s
    const auto  ForwardAccel= Vector3(0, 0, 0.1f); // in  m/s/s
    const auto  ReverseAccel= Vector3(0, 0,-0.1f); // in  m/s/s
    //const auto brakeForce   = DirectX::SimpleMath::Vector3(0, 0,-1); // in  m/s/s
    //const auto dragCoeff    = DirectX::SimpleMath::Vector3(0, 0,-0.1f); // in  m/s/s
    const float Acceleration= 0.075f;					// in m/s/s
    const float BrakeForce  = 0.2f;				// in m/s/s
    const float DragCoeff   = 0.01f;				// in m/s/s
    const float MaxSpeed	= 40;					// in km/h
    const float MaxReverse  = 30;					// in km/h
    const float RotationGain= DirectX::XM_PI * 10;  // radians
    const float MpsToKph	= 3.6f;					// m/s to km/h conversion factor
    const float KphToMps	= 1.f / 3.6f;			// km/h to m/s conversion factor
}

struct Checkpoint
{
    uint32_t id;
    float	 speedLimit;	// in km/h
    Vector3  forward;		// forward vector
    //float						 orientation;	// in radians
    Vector3  signpost1;		// left signpost
    Vector3  signpost2;		// right signpost
};

struct CarState
{
    uint32_t id;
    uint32_t nextCheckpoint;
    Vector3  velocity;		// in m/s
};

namespace Racetracks
{
    //using namespace DirectX::SimpleMath;
    using namespace PhysicsConstants;
    const std::vector<Checkpoint> track =
    {
        {
            0,
            MaxSpeed,
            Vector3(0,0,-1),
            Vector3(5,0,0),
            Vector3(7,0,0),
        },
        {
            1,
            MaxSpeed,
            Vector3(-1,0,0),
            Vector3(0,0,-10),
            Vector3(0,0,-12),
        },
        {
            2,
            MaxSpeed,
            Vector3(0,0,1),
            Vector3(-5,0,0),
            Vector3(-7,0,0),
        },
        {
            3,
            MaxSpeed,
            Vector3(1,0,0),
            Vector3(0,0,10),
            Vector3(0,0,12),
        },
    };
}