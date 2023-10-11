//
// Game_Collision.cpp
//

// Collision methods for Game class:

#include "pch.h"
#include "Game.h"

void Game::TestGroundCollision(SDKMESHModel* groundModel, const BoundingSphere& sphere, CollisionTriangle& triangle)
{
    // If the ground plane model is not transformed from its model space origin, then we should also be able to 
    // perform the collision test between the ground triangle and the sphere in world space.

    //auto toLocal(mFarm->GetWorld().Invert());

    //BoundingSphere localSphere{};// (mCameraSphere);
    //mCameraSphere.Transform(localSphere, toLocal);

    //uint32_t indices[3] = {};
    //CollisionTriangle triangle = {};

    const auto triCount  = groundModel->GetCollisionTriangleCount();
    //auto triangles(mFarm->GetTriangleCount());
    const auto vertices  = groundModel->GetCollisionVertices();
    const auto indices16 = groundModel->GetCollisionIndices16();
    const auto indices32 = groundModel->GetCollisionIndices32();
    const auto isIndex16 = groundModel->GetCollisionIndexFormat() == DXGI_FORMAT_R16_UINT ? true : false;
    //auto vertices(mFarm->GetVertices());
    //auto indices(mFarm->GetIndices());

    // Find the nearest triangle/sphere intersection.
    for (uint32_t i = 0; i < triCount; ++i)
        //for (uint32_t i = 0; i < mFarm->mTriangles; ++i)
    {
        //testIndices[0] = indices[i * 3 + 0];
        //testIndices[1] = indices[i * 3 + 1];
        //testIndices[2] = indices[i * 3 + 2];

        // Vertices for this triangle.
        if (isIndex16)
        {
            triangle.pointa = vertices[indices16[i * 3 + 0]].Position;
            triangle.pointb = vertices[indices16[i * 3 + 1]].Position;
            triangle.pointc = vertices[indices16[i * 3 + 2]].Position;
        }
        else
        {
            triangle.pointa = vertices[indices32[i * 3 + 0]].Position;
            triangle.pointb = vertices[indices32[i * 3 + 1]].Position;
            triangle.pointc = vertices[indices32[i * 3 + 2]].Position;
        }

        //collisionTri.pointa = vertices[testIndices[0]].Position;
        //collisionTri.pointb = vertices[testIndices[1]].Position;
        //collisionTri.pointc = vertices[testIndices[2]].Position;

        triangle.collision = ContainmentType::DISJOINT;

        // We have to iterate over all the triangles in order to find the nearest intersection.

        //if (collSphere.sphere.Intersects(ct.pointa, ct.pointb, ct.pointc))
        //if (temp.Intersects(ct.pointa, ct.pointb, ct.pointc))

        // Compute the surface normal for the collision triangle.
        // From DirectXCollision.inl
        const auto N = XMVector3Normalize(XMVector3Cross(
            XMVectorSubtract(triangle.pointb, triangle.pointa),
            XMVectorSubtract(triangle.pointc, triangle.pointa)));
        //XMVECTOR N = XMVector3Normalize(XMVector3Cross(XMVectorSubtract(v[1], v[0]), XMVectorSubtract(v[2], v[0])));

        // Assert that the triangle is not degenerate.
        if (!XMVector3Equal(N, XMVectorZero()))
        {    // Will not work if degenerate triangles are present!
            if (sphere.Intersects(triangle.pointa, triangle.pointb, triangle.pointc))
                //if (localSphere.Intersects(t.pointa, t.pointb, t.pointc))
                //if (localSphere.Intersects(v[0], v[1], v[2]))
            {
                triangle.collision = ContainmentType::INTERSECTS;
                //collisionFlags |= t.collision;
                //mCollisionFlag |= 0x1u;
                // Return to caller after the first collision is detected.
                return;
            }
        }
    }
}

void Game::TestGroundCollision(SDKMESHModel* groundModel, const BoundingBox& box, CollisionTriangle& triangle)
{
    // If the ground plane model is not transformed from its model space origin, then we should also be able to 
    // perform the collision test between the ground triangle and the bounding box in world space.

    const auto triCount  = groundModel->GetCollisionTriangleCount();
    const auto vertices  = groundModel->GetCollisionVertices();
    const auto indices16 = groundModel->GetCollisionIndices16();
    const auto indices32 = groundModel->GetCollisionIndices32();
    const auto isIndex16 = groundModel->GetCollisionIndexFormat() == DXGI_FORMAT_R16_UINT ? true : false;

    // Find the nearest triangle/box intersection.
    for (uint32_t i = 0; i < triCount; ++i)
    {
        // Vertices for this triangle.
        if (isIndex16)
        {
            triangle.pointa = vertices[indices16[i * 3 + 0]].Position;
            triangle.pointb = vertices[indices16[i * 3 + 1]].Position;
            triangle.pointc = vertices[indices16[i * 3 + 2]].Position;
        }
        else
        {
            triangle.pointa = vertices[indices32[i * 3 + 0]].Position;
            triangle.pointb = vertices[indices32[i * 3 + 1]].Position;
            triangle.pointc = vertices[indices32[i * 3 + 2]].Position;
        }

        triangle.collision = ContainmentType::DISJOINT;

        // We have to iterate over all the triangles in order to find the nearest intersection.
        // Compute the surface normal for the collision triangle.
        // From DirectXCollision.inl
        const auto N = XMVector3Normalize(XMVector3Cross(
            XMVectorSubtract(triangle.pointb, triangle.pointa),
            XMVectorSubtract(triangle.pointc, triangle.pointa)));

        // Assert that the triangle is not degenerate.
        if (!XMVector3Equal(N, XMVectorZero()))
        {    // Will not work if degenerate triangles are present!
            if (box.Intersects(triangle.pointa, triangle.pointb, triangle.pointc))
            {
                triangle.collision = ContainmentType::INTERSECTS;
                // Return to caller after the first collision is detected.
                return;
            }
        }
    }
}
