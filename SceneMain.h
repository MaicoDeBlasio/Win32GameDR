#pragma once
#include "SceneRaytraced.h"

class SceneMain final : public SceneRaytraced
{

public:

    enum SceneGeometries
    {
        sceneCube, //Plane,
        sceneSuzanne,
        sceneRacetrackRoad, sceneRacetrackSkirt, sceneRacetrackMap,
        scenePalmtreeTrunk, scenePalmtreeCanopy,
        sceneMiniRacecar,
        sceneDove, // FBX model.
        sceneCount
    };

    enum StaticBLAS
    {
        staticCube, //Plane,
        staticSuzanne,
        staticRacetrack,  // Contains racetrack grass, road & map geometries.
        staticPalmtree,   // Contains palmtree trunk & canopy geometries.
        staticMiniRacecar,
        //PalmtreeTrunk, PalmtreeFoliage, MiniRacecar,
        //Dove, // FBX model.
        staticCount
    };

    enum DynamicBLAS
    {
        dynamicDove,
        dynamicCount
    };

    enum TLASInstances
    {   // Cube instances (total of cube instances is defined in CUBE_INSTANCE_COUNT).
        tlasRedCube, tlasGreenCube, tlasBlueCube,
        //CubeInstances = CUBE_INSTANCE_COUNT,
        //Plane,
        tlasSuzanne, tlasRacetrack, tlasPalmtree, tlasMiniRacecar,
        //Suzanne, Racetrack, PalmtreeTrunk, PalmtreeFoliage, MiniRaceCar,
        //Suzanne, RacetrackGrass, RacetrackRoad, PalmtreeTrunk, PalmtreeFoliage, MiniRaceCar,
        tlasDove, // FBX model.
        tlasCount
    };

    // This namespace unrolls the BLAS and TLAS instance indexes, with padding for correct indexing into the
    // InstanceData structured buffer in shaders with the user provided DXR intrinsic InstanceID().
    enum ShaderInstances
    {   // Cube instances (total of cube instances is defined in CUBE_INSTANCE_COUNT).
        instRedCube, instGreenCube, instBlueCube,
        //CubeInstances = CUBE_INSTANCE_COUNT,
        //Plane,
        instSuzanne,
        instRacetrack, instRacetrackReserved, instRacetrackReserved2,
        instPalmtree, instPalmtreeReserved,
        instMiniRacecar,
        //Suzanne, RacetrackRoad, RacetrackGrass, PalmtreeTrunk, PalmtreeFoliage, MiniRaceCar,
        instDove, // FBX model.
        instCount
    };

    SceneMain(Game* game, bool isRaster) noexcept;

    SceneMain(SceneMain const&) = delete;
    SceneMain& operator= (SceneMain const&) = delete;

    SceneMain(SceneMain&&) = default;
    SceneMain& operator= (SceneMain&&) = default;

    ~SceneMain() = default;

    // Implement virtual base class methods.
    void Update();
    void Render();

private:

    void Initialize();  // Implement abstract base class method.

    // Implement abstract SceneRaytraced class methods.
    void CreateInstanceBuffer(ID3D12Device* device, ID3D12CommandQueue* commandQueue);
    void BuildBLASGeometryDescs();
    void BuildStaticBLAS(ID3D12Device10* device, ID3D12GraphicsCommandList7* commandList);
    void BuildDynamicBLAS(ID3D12Device10* device, ID3D12GraphicsCommandList7* commandList, bool isUpdate);
    void BuildTLASInstanceDescs();

    std::unique_ptr<XMFLOAT3X4[]> m_cubeTransforms3x4;
    std::unique_ptr<Matrix[]>     m_cubeTransforms4x4;

    std::unique_ptr<StructuredBuffer<PrevFrameData>> m_prevFrameStructBuffer; // CPU writeable structured buffer.

public:

    static constexpr uint32_t    CubeInstanceCount = 3; // number of raytraced cube instances

    //const auto GetCubeTransforms() const noexcept { return m_cubeTransforms.get(); }; // temp
    //const auto GetTLASBuffers() const noexcept { return m_tlasBuffers.get(); }; // temp
    //const auto GetBLASBuffers(uint32_t i) const noexcept { return m_blasBuffers[i].get(); }; // temp
    //const auto GetGeometryDesc() const noexcept { return m_geometryDesc.get(); }; // temp
    //const auto GetTLASInstanceDesc() const noexcept { return m_tlasInstanceDesc.get(); }; // temp
};
