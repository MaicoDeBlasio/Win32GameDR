//
// Collision structures.
//

#pragma once

// RaytracingHlslCompat.h declares 'using' DirectX namespaces, so it must be in the #include list first.

struct CollisionSphere
{
    BoundingSphere sphere;
    ContainmentType collision;
};

struct CollisionBox
{
    BoundingOrientedBox obox;
    ContainmentType collision;
};

struct CollisionAABox
{
    BoundingBox aabox;
    ContainmentType collision;
};

struct CollisionFrustum
{
    BoundingFrustum frustum;
    ContainmentType collision;
};

struct CollisionTriangle
{
    Vector3 pointa;
    Vector3 pointb;
    Vector3 pointc;
    ContainmentType collision;
};

struct CollisionRay
{
    Vector3 origin;
    Vector3 direction;
};

// added GameSphere and GameBox

struct GameSphere
{
    BoundingSphere local;
    ContainmentType inFrustum;
    CollisionSphere world;
};

struct GameBox
{
    BoundingBox local;
    ContainmentType inFrustum;
    CollisionAABox world;
};
