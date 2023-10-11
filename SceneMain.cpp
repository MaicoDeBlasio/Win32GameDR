#include "pch.h"
#include "Game.h"   // Forward declared classes must be defined with its header added to the .cpp file
#include "SceneMain.h"

extern void ExitGame() noexcept;

SceneMain::SceneMain(Game* game, bool isRaster) noexcept
//SceneMain::SceneMain(Game* game) noexcept : SceneRaytraced(game), SceneRaytraced::m_isRaster(true)
{
    m_game = game;
    m_isRaster = isRaster;
    m_isFirstFrame = isRaster ? false : true; // If starting app in raster mode, set flag to false.

    m_cubeTransforms3x4 = std::make_unique<XMFLOAT3X4[]>(SceneMain::CubeInstanceCount);
    m_cubeTransforms4x4 = std::make_unique<Matrix[]>(SceneMain::CubeInstanceCount);
    m_prevFrameStructBuffer = std::make_unique<StructuredBuffer<PrevFrameData>>();
    m_blasBuffers[BLASType::Static]  = std::make_unique<AccelerationStructureBuffers[]>(StaticBLAS::staticCount);
    m_blasBuffers[BLASType::Dynamic] = std::make_unique<AccelerationStructureBuffers[]>(DynamicBLAS::dynamicCount);
    m_geometryDesc = std::make_unique<D3D12_RAYTRACING_GEOMETRY_DESC[]>(SceneGeometries::sceneCount);
    m_tlasInstanceDesc = std::make_unique<D3D12_RAYTRACING_INSTANCE_DESC[]>(TLASInstances::tlasCount); // Make shared?

    Initialize();
}

void SceneMain::Initialize()
{
    const auto deviceResources = m_game->GetDeviceResources();
    const auto backBufferCount = deviceResources->GetBackBufferCount();
    const auto commandAlloc    = deviceResources->GetCommandAllocator();
    const auto commandQueue    = deviceResources->GetCommandQueue();
    const auto commandList     = deviceResources->GetCommandList();
    const auto device          = deviceResources->GetD3DDevice();

    // Set the camera's initial position.
    m_camera->LookAt(Vector3(0, 3, 15), Vector3::Zero, Vector3::Up);
    m_camera->UpdateViewMatrix();

    // Set the camera's bounding sphere.
    //m_cameraSphere.Center = m_camera->GetPosition3f();
    //m_cameraSphere.Radius = 1;

    // Set light attributes.
    m_frameConstants->lightDiffuse = Vector4(1.1f, 1.1f, 0.8f, 1);
    m_frameConstants->lightAmbient = Vector4(0.1f, 0.1f, 0.25f, 1);
    m_frameConstants->lightDir = Vector3(0, -1, -1);
    m_frameConstants->lightDir.Normalize();

    // Set scene descriptors for direct descriptor heap indexing in shader.
    m_frameConstants->tlasBufferSrvID = SrvUAVs::TLASBufferSrv;
    m_frameConstants->instBufferSrvID = SrvUAVs::InstanceBufferSrv;

    // Initialize cube transforms.
    // Instance #0
    auto transform = Matrix::CreateTranslation(Vector3(0, 0.15f, 0));// Matrix::Identity;
    //m_cubeTransforms[0] = transform;
    XMStoreFloat3x4(&m_cubeTransforms3x4[0], transform);
    m_cubeTransforms4x4[0] = transform;
    //// Instance #1
    transform = Matrix::CreateTranslation(Vector3(-6, 0.15f, 0));
    //m_cubeTransforms[1] = transform;
    XMStoreFloat3x4(&m_cubeTransforms3x4[1], transform);
    m_cubeTransforms4x4[1] = transform;
    //// Instance #2
    transform = Matrix::CreateTranslation(Vector3(6, 0.15f, 0));
    //m_cubeTransforms[2] = transform;
    XMStoreFloat3x4(&m_cubeTransforms3x4[2], transform);
    m_cubeTransforms4x4[2] = transform;

    // Initial static model world transforms.
    //m_suzanne->SetPosition(Vector3(0, 1,-2));
    //auto& world = Matrix::CreateTranslation(m_suzanne->GetPosition());

    auto sdkMeshModel = m_game->GetSdkMeshModel(SDKMESHModels::Suzanne);
    sdkMeshModel->SetWorld(Vector3(0, 1, -12), Vector3::UnitZ, Vector3::Up);
    //m_SDKMESHModel[SDKMESHModels::Suzanne]->SetWorld(Vector3(0, 1, -12), Vector3::UnitZ, Vector3::Up);
    //m_suzanne->SetWorld(Vector3(0, 1, -12), Vector3::Zero);
    //m_suzanne->SetWorld(world);

    //m_racetrack->SetPosition(Vector3(0,-0.15f, 0));
    //world = Matrix::CreateTranslation(m_racetrack->GetPosition());

    sdkMeshModel = m_game->GetSdkMeshModel(SDKMESHModels::Racetrack);
    sdkMeshModel->SetWorld(Vector3::Zero, Vector3::UnitZ, Vector3::Up);
    //m_SDKMESHModel[SDKMESHModels::Racetrack]->SetWorld(Vector3::Zero, Vector3::UnitZ, Vector3::Up);
    //m_racetrack->SetWorld(Vector3::Zero, Vector3::Zero);
    //m_racetrack->SetWorld(world);
    //m_racetrack_grass->SetWorld(world);
    //m_racetrack_road->SetWorld(world);

    sdkMeshModel = m_game->GetSdkMeshModel(SDKMESHModels::Palmtree);
    sdkMeshModel->SetWorld(Vector3(0, 0, 2), Vector3::UnitZ, Vector3::Up);
    //m_SDKMESHModel[SDKMESHModels::Palmtree]->SetWorld(Vector3(0, 0, 2), Vector3::UnitZ, Vector3::Up);
    //m_palmtree->SetWorld(Vector3(0, 0, 2), Vector3::Zero);
    //m_palmtree->SetWorld(world);

    //m_miniracecar->SetPosition(Vector3(0, -0.15f, 4));
    //world = Matrix::CreateTranslation(m_miniracecar->GetPosition());

    sdkMeshModel = m_game->GetSdkMeshModel(SDKMESHModels::MiniRacecar);
    sdkMeshModel->SetWorld(Vector3(6, 0, 12), -Vector3::UnitZ, Vector3::Up);
    //m_SDKMESHModel[SDKMESHModels::MiniRacecar]->SetWorld(Vector3(6, 0, 12), -Vector3::UnitZ, Vector3::Up);
    //m_miniracecar->SetWorld(Vector3(6, 0, 12), Vector3(0, XM_PI, 0));
    //m_miniracecar->SetWorld(Vector3(6, 0, -1), Vector3::Zero);
    //m_miniracecar->SetWorld(world);

    //m_dove->SetPosition(Vector3(0, 0, 2));
    //world = Matrix::CreateTranslation(m_dove->GetPosition());

    auto fbxModel = m_game->GetFbxModel(FBXModels::Dove);
    fbxModel->SetWorld(Vector3(0, 0.15f, 12), Vector3::Zero);
    //m_FBXModel[FBXModels::Dove]->SetWorld(Vector3(0, 0.15f, 12), Vector3::Zero);
    //m_dove->SetWorld(world);

    // Scaling and rotation is required when not animating the Fbx model.
    //world = Matrix::CreateScale(0.05f)
    //    * Matrix::CreateRotationX(XM_PIDIV2)
    //    * Matrix::CreateTranslation(m_dove->GetPosition());

    CreateInstanceBuffer(device, commandQueue);

    // Create a CPU writeable structured buffer to pass previous frame world transforms to shaders.
    m_prevFrameStructBuffer->Create(device, TLASInstances::tlasCount, backBufferCount, L"WorldPrevStructBuffer");
    //m_prevFrameStructBuffer->Create(device, TLASInstances::tlasCount, Globals::FrameCount, L"WorldPrevStructBuffer");

    // Create SRVs per frame index.
    const auto structBuffer = m_prevFrameStructBuffer->GetResource();
    const auto srvDescHeap  = m_game->GetDescriptorHeap(DescriptorHeaps::SrvUav);
    //CreateBufferShaderResourceView(
    //    device,
    //    structBuffer,
    //    srvDescHeap->GetCpuHandle(SrvUAVs::PrevFrameDataBufferSrv_0),
    //    sizeof(Matrix)
    //);
    CreateBufferShaderResourceViewPerFrameIndex(
        device,
        structBuffer,
        srvDescHeap->GetCpuHandle(SrvUAVs::PrevFrameDataBufferSrv_0),
        TLASInstances::tlasCount, sizeof(PrevFrameData), 0
    );
    CreateBufferShaderResourceViewPerFrameIndex(
        device,
        structBuffer,
        srvDescHeap->GetCpuHandle(SrvUAVs::PrevFrameDataBufferSrv_1),
        TLASInstances::tlasCount, sizeof(PrevFrameData), TLASInstances::tlasCount
    );

    // Build DXR resources.

    BuildBLASGeometryDescs();

    // Reset the command list for the acceleration structure construction.
    commandList->Reset(commandAlloc, nullptr);
    
    BuildStaticBLAS(device, commandList);
    BuildDynamicBLAS(device, commandList, false);   // First dynamic BLAS build, after which only updates are required.

    BuildTLASInstanceDescs();
    BuildTLAS(device, commandList, m_tlasBuffers.get(), TLASInstances::tlasCount, false); // First TLAS build.
    //BuildTLASInstanceDescs(m_tlasInstanceDesc.get());
    //BuildTLAS(device, commandList, m_tlasBuffers.get(), m_tlasInstanceDesc.get(), TLASInstances::tlasCount, false); // First TLAS build.

    deviceResources->ExecuteCommandList();  // Start acceleration structure construction.
    deviceResources->WaitForGpu();          // Wait for GPU to finish (any locally created temp GPU resources will get released once we
                                            // go out of scope.
}

void SceneMain::Update()
{
    //PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");
    const auto timer = m_game->GetTimer();
    const auto deviceResources = m_game->GetDeviceResources();

    const auto elapsedTime = static_cast<float>(timer->GetElapsedSeconds());
    const auto totalTime = static_cast<float>(timer->GetTotalSeconds());

    // TODO: Add your game logic here.
    //elapsedTime;

    // Test for input.

    const auto mouse        = m_game->GetMouse();
    const auto mouseTracker = m_game->GetMouseTracker();
    const auto mouseState   = mouse->GetState();

    mouseTracker->Update(mouseState);
    mouse->SetMode(mouseState.leftButton ? Mouse::MODE_RELATIVE : Mouse::MODE_ABSOLUTE);

    static bool isFlightCam = false; // Toggles camera behaviour for flight simulation.

    //const auto camera = m_scene->GetCamera();

    if (mouseState.positionMode == Mouse::MODE_RELATIVE)
    {
        const auto delta = Vector2(static_cast<float>(mouseState.x), static_cast<float>(mouseState.y)) * Globals::RotationGain;
        m_camera->Pitch(delta.y);   // Pushing mouse forward dips the view, pulling back elevates the view.
        //m_camera->Pitch(-delta.y);

        if (isFlightCam)
        {
            m_camera->Roll(-delta.x);
            m_camera->Walk(-Globals::MovementGain * elapsedTime);
            m_camera->Strafe(100.f * delta.x * Globals::MovementGain * elapsedTime);
        }
        else
            m_camera->RotateY(-delta.x);
    }

    mouse->EndOfInputFrame(); // New DXTK12 method (optional but recommended).

    const auto keyboard = m_game->GetKeyboard();
    const auto keyTracker = m_game->GetKeyTracker();
    const auto keyState = keyboard->GetState();

    keyTracker->Update(keyState);

    if (keyTracker->released.Escape)
    {
        ExitGame();
    }
    if (keyTracker->released.Space)
    {
        // Toggle between raster & DXR pipeline.
        m_isRaster = !m_isRaster;
        m_isFirstFrame = m_isRaster ? false : true; // Set first frame flag if we enter raytracing mode.
    }
    if (keyTracker->released.F)
    {
        // Toggle between first-person and flight camera.
        isFlightCam = !isFlightCam;
    }

    //static bool doPhysics = false;
    //if (m_keyTracker->released.Enter)
    //{
    //    doPhysics = !doPhysics;
    //}

    if (keyState.W) m_camera->Walk(-Globals::MovementGain * elapsedTime);
    if (keyState.S) m_camera->Walk(+Globals::MovementGain * elapsedTime);
    if (keyState.A) m_camera->Strafe(-Globals::MovementGain * elapsedTime);
    if (keyState.D) m_camera->Strafe(+Globals::MovementGain * elapsedTime);

    // Test camera position for collision with the model forming the ground plane.
    BoundingSphere cameraSphere = { m_camera->GetPosition3f() , 0.5f }; // Bounding sphere centre and radius.
    CollisionTriangle groundTriangle = {};

    // Get pointer to current model.
    auto sdkMeshModel = m_game->GetSdkMeshModel(SDKMESHModels::Racetrack);
    //auto sdkMeshModel = m_SDKMESHModel[SDKMESHModels::Racetrack].get();

    m_game->TestGroundCollision(sdkMeshModel, cameraSphere, groundTriangle);
    //TestGroundCollision(m_SDKMESHModel[SDKMESHModels::Racetrack].get(), cameraSphere, groundTriangle);

    if (groundTriangle.collision == ContainmentType::INTERSECTS)
    {
        const auto groundHeight = (groundTriangle.pointa.y + groundTriangle.pointb.y + groundTriangle.pointc.y) / 3.f;
        m_camera->SetPosition(cameraSphere.Center.x, groundHeight + cameraSphere.Radius, cameraSphere.Center.z);
    }

    // We must also update the projection matrix each frame to implement camera jitter.
    // Edit: we will investigate other aa solutions.
    m_camera->UpdateViewMatrix();
    //m_camera->SetLens(1.f, m_aspectRatio, 100.f, 0.1f, m_width, m_height); // Swap near & far plane for reverse-z buffer.
    //m_camera->SetLens(1.f, m_aspectRatio, 0.1f, 100.f, m_width, m_height); // one radian field of view

    // Update frame constants.
    const Matrix view = m_camera->GetView();
    const Matrix proj = m_camera->GetProj();
    const auto viewProj    = view * proj;
    const auto viewProjTex = viewProj * Globals::T;

    const auto currentFrameIndex = deviceResources->GetCurrentFrameIndex();

    //const auto frameConstants = m_scene->GetFrameConstants();

    // First save the previous frame camera constants before they are overwritten.
    m_frameConstants->prevViewProjTex = m_frameConstants->viewProjTex;
    m_frameConstants->viewProj        = viewProj.Transpose();
    m_frameConstants->viewProjTex     = viewProjTex.Transpose();
    m_frameConstants->invProj         = proj.Invert().Transpose();
    m_frameConstants->invViewProj     = viewProj.Invert().Transpose();
    m_frameConstants->cameraPos       = m_camera->GetPosition();
    m_frameConstants->frameCount      = timer->GetFrameCount();
    m_frameConstants->prevFrameBufferSrvID = SrvUAVs::PrevFrameDataBufferSrv_0 + currentFrameIndex;
    m_frameConstants->isFirstFrame    = static_cast<uint32_t>(m_isFirstFrame);
    //m_frameConstants->backbufferSize.width  = m_width;
    //m_frameConstants->backbufferSize.height = m_height;
    //m_frameConstants->projValues.x= proj(2,2); // Projection value A (negated in shader for RH view system).
    //m_frameConstants->projValues.y= proj(3,2); // Projection value B

    m_isFirstFrame = false; // Reset first frame flag.

    // Update the constant buffer required for indirect (GPU-driven) drawing.
    auto constantBufferIndirect     = m_game->GetConstBufferIndirect();
    constantBufferIndirect->staging = *m_frameConstants;
    constantBufferIndirect->CopyStagingToGpu(currentFrameIndex);

    // Save previous frame's world transforms.

    PrevFrameData prevWorldTransforms[TLASInstances::tlasCount] = {};
    //XMFLOAT3X4 prevWorldTransforms[TLASInstances::tlasCount] = {};
    for (uint32_t i = TLASInstances::tlasRedCube; i < SceneMain::CubeInstanceCount; i++)
        prevWorldTransforms[i].world = m_cubeTransforms4x4[i].Transpose(); // I found the bug!
        //prevWorldTransforms[i].world = m_cubeTransforms[i];

    sdkMeshModel = m_game->GetSdkMeshModel(SDKMESHModels::Suzanne);
    prevWorldTransforms[TLASInstances::tlasSuzanne].world = sdkMeshModel->GetWorld().Transpose();
    //XMStoreFloat3x4(&prevWorldTransforms[TLASInstances::tlasSuzanne].world, sdkMeshModel->GetWorld());

    sdkMeshModel = m_game->GetSdkMeshModel(SDKMESHModels::Racetrack);
    prevWorldTransforms[TLASInstances::tlasRacetrack].world = sdkMeshModel->GetWorld().Transpose();
    //XMStoreFloat3x4(&prevWorldTransforms[TLASInstances::tlasRacetrack].world, sdkMeshModel->GetWorld());

    sdkMeshModel = m_game->GetSdkMeshModel(SDKMESHModels::Palmtree);
    prevWorldTransforms[TLASInstances::tlasPalmtree].world = sdkMeshModel->GetWorld().Transpose();
    //XMStoreFloat3x4(&prevWorldTransforms[TLASInstances::tlasPalmtree].world, sdkMeshModel->GetWorld());

    sdkMeshModel = m_game->GetSdkMeshModel(SDKMESHModels::MiniRacecar);
    prevWorldTransforms[TLASInstances::tlasMiniRacecar].world = sdkMeshModel->GetWorld().Transpose();
    //XMStoreFloat3x4(&prevWorldTransforms[TLASInstances::tlasMiniRacecar].world, sdkMeshModel->GetWorld());

    auto fbxModel = m_game->GetFbxModel(FBXModels::Dove);
    prevWorldTransforms[TLASInstances::tlasDove].world = fbxModel->GetWorld().Transpose();
    //XMStoreFloat3x4(&prevWorldTransforms[TLASInstances::tlasDove].world, fbxModel->GetWorld());

    // Update structured buffer with previous frame world transforms.
    auto structBuffer = m_prevFrameStructBuffer.get();
    for (uint32_t i = 0; i < structBuffer->NumElementsPerInstance(); i++)
        (*structBuffer)[i].world = prevWorldTransforms[i].world;    // The dereferencing operator -> is overloaded.
        //structBuffer->operator[](i) = prevWorldTransforms[i];
 
    structBuffer->CopyStagingToGpu(currentFrameIndex);

    // When using persistent map, the application must ensure the CPU finishes writing data into memory before the
    // GPU executes a command list that reads or writes the memory.
    // In common scenarios, the application merely must write to memory before calling ExecuteCommandLists,
    // but using a fence to delay command list execution works as well.
    //deviceResources->WaitForGpu();

    // Update geometries with new world transforms.

    // Cube instance #0
    auto transform = Matrix::CreateRotationX(totalTime) * Matrix::CreateTranslation(Vector3(0, 0.15f, 0));
    //const auto cubeTransforms = m_game->GetCubeTransforms();
    XMStoreFloat3x4(&m_cubeTransforms3x4[0], transform);
    m_cubeTransforms4x4[0] = transform;
    // Cube instance #1
    transform = Matrix::CreateRotationY(totalTime) * Matrix::CreateTranslation(Vector3(-6, 0.15f, 0));
    XMStoreFloat3x4(&m_cubeTransforms3x4[1], transform);
    m_cubeTransforms4x4[1] = transform;
    // Cube instance #2
    transform = Matrix::CreateRotationZ(totalTime) * Matrix::CreateTranslation(Vector3(6, 0.15f, 0));
    XMStoreFloat3x4(&m_cubeTransforms3x4[2], transform);
    m_cubeTransforms4x4[2] = transform;

    //BuildTopLevelAS(m_blasBuffers, true); // update TLAS with new blas instance data
    //UpdateTopLevelAS(m_blasBuffers); // update TLAS with new blas instance data

    sdkMeshModel = m_game->GetSdkMeshModel(SDKMESHModels::MiniRacecar); // Update model pointer.
    //sdkMeshModel = m_SDKMESHModel[SDKMESHModels::MiniRacecar].get(); // Update model pointer.

    // Update mini race car track position.
    static CarState carState =
    {
        0, 0, Vector3::Zero, // Car is stationary at beginning.
    };

    // Apply physical forces to moving scene objects.
    // Apply gravity.
    //if (doPhysics)
    //    carState.velocity = carState.velocity + PhysicsConstants::gravity * elapsedTime;

    const auto speed = carState.velocity.Length();
    bool isPowered = false; // Flag to indicate car is under power from accelerator, brake or reverse gear.
    static bool isAutoNav = false; // Flag to switch car switch car control to auto navigation.

    // Set car's velocity to zero if speed is under threshold.
    //if (speed < 0.005f)
    //{
    //    carState.velocity = Vector3::Zero;
    //    speed = 0;
    //}

    // Apply car control from user input.
    Vector3 worldAccel = {};
    Vector3 worldReverse = {};
    //Vector3 worldBrake = {};
    auto carPos = sdkMeshModel->GetPosition();
    auto carOrient = sdkMeshModel->GetOrientation();
    auto carForward = sdkMeshModel->GetForward();
    //auto carPos = m_miniracecar->GetPosition();
    //auto carOrient = m_miniracecar->GetOrientation();
    //auto carForward = m_miniracecar->GetForward();

    // Start auto navigation.
    if (keyTracker->released.N)
    {
        isAutoNav = true;
    }
    // Acceleration.
    if (keyState.Up)
    {
        if (speed < PhysicsConstants::MaxSpeed * elapsedTime)
            // Transform forward acceleration to current orientation.
            Vector3::TransformNormal(PhysicsConstants::ForwardAccel, sdkMeshModel->GetWorld(), worldAccel);
        //Vector3::TransformNormal(PhysicsConstants::ForwardAccel, m_miniracecar->GetWorld(), worldAccel);
        //carState.velocity = carState.velocity + worldAccel * elapsedTime;
        isPowered = true;
    }
    // Reverse.
    if (keyState.Down)
    {
        if (speed < PhysicsConstants::MaxReverse * elapsedTime)
            // Transform brake force to current orientation.
            Vector3::TransformNormal(PhysicsConstants::ReverseAccel, sdkMeshModel->GetWorld(), worldReverse);
        //Vector3::TransformNormal(PhysicsConstants::ReverseAccel, m_miniracecar->GetWorld(), worldReverse);
        //carState.velocity = carState.velocity + worldBrake * elapsedTime;
        //auto brake = carState.velocity + worldBrake * elapsedTime;
        //carState.velocity.Clamp(Vector3::Zero, brake);
        //carState.velocity = Vector3::Max(Vector3::Zero, brake);
        isPowered = true;
    }
    // Brake.
    if (keyState.B && speed)
    {
        carState.velocity *= 1 - PhysicsConstants::BrakeForce;
        // Transform forward acceleration to current orientation.
        //Vector3::TransformNormal(PhysicsConstants::brakeForce, m_miniracecar->GetWorld(), worldBrake);
        //carState.velocity = carState.velocity + worldAccel * elapsedTime;
    }
    // Turn right.
    if (keyState.Right)
    {
        carOrient.y -= PhysicsConstants::RotationGain * elapsedTime * speed;
    }
    // Turn left.
    if (keyState.Left)
    {
        carOrient.y += PhysicsConstants::RotationGain * elapsedTime * speed;
    }

    // Auto navigation.
    const uint32_t lastCheckPtId = static_cast<uint32_t>(Racetracks::track.size()) - 1;
    const uint32_t nextCheckPtId = carState.nextCheckpoint;
    const uint32_t prevCheckPtId = nextCheckPtId ? nextCheckPtId - 1 : lastCheckPtId; // If cp0 is next, cpLast is previous.
    const auto& nextCheckPt = Racetracks::track[nextCheckPtId];
    const auto& prevCheckPt = Racetracks::track[prevCheckPtId];

    if (isAutoNav)
    {
        // Get vector to next checkpoint target.
        const auto nextTarget = Vector3::Lerp(nextCheckPt.signpost1, nextCheckPt.signpost2, 0.5f); // Target halfway b/n signposts.
        const auto prevTarget = Vector3::Lerp(prevCheckPt.signpost1, prevCheckPt.signpost2, 0.5f); // Target halfway b/n signposts.
        auto targetVector = nextTarget - carPos;

        // Normalize and scale target vector by acceleration factor.
        targetVector.Normalize();
        worldAccel = PhysicsConstants::Acceleration * targetVector;

        // Update car orientation.
        const auto stageDist = Vector3::DistanceSquared(prevTarget, nextTarget);
        const auto distInStage = Vector3::Distance(prevTarget, carPos);
        const auto t = distInStage / stageDist;
        //carOrient.y  = std::lerp(carOrient.y, nextCheckPt.orientation, t);
        carForward = Vector3::Lerp(carForward, nextCheckPt.forward, t);
    }

    //carState.velocity = carState.velocity + worldDrag * elapsedTime;
    //auto drag = carState.velocity + worldDrag * elapsedTime;
    //carState.velocity.Clamp(Vector3::Zero, drag);
    //carState.velocity = Vector3::Max(Vector3::Zero, drag);

    // Update velocity and position.
    // Using v = u + at
    carState.velocity += worldAccel * elapsedTime;
    //carState.velocity += (worldAccel + worldReverse) * elapsedTime;
    const auto speedLimit = nextCheckPt.speedLimit * PhysicsConstants::KphToMps * elapsedTime;
    if (speed > speedLimit)
        carState.velocity *= 1 - PhysicsConstants::BrakeForce;

    // Apply drag only when car is unpowered.
    //if (!isPowered || !isAutoNav)
    //    carState.velocity *= 1 - PhysicsConstants::DragCoeff;
    //    //carState.velocity = Vector3::Max(v, PhysicsConstants::terminalVel);

    carPos += carState.velocity; // position + velocity = new position
    //auto carPos = m_miniracecar->GetPosition() + carState.velocity; // position + velocity = new position

    sdkMeshModel->SetWorld(carPos, carForward, Vector3(0, 1, 0));
    //m_miniracecar->SetWorld(carPos, carForward, Vector3(0,1,0));
    //m_miniracecar->SetWorld(carPos, carOrient);
    //m_miniracecar->SetWorld(carPos, m_miniracecar->GetOrientation());

    // Test for contact with track surface and checkpoints.
    BoundingBox worldBox = {};
    const auto carBox = sdkMeshModel->GetBoundingBox(0);
    //auto carBox = m_miniracecar->GetBoundingBox(0);
    carBox.Transform(worldBox, sdkMeshModel->GetWorld());
    //carBox.Transform(worldBox, m_miniracecar->GetWorld());

    CollisionRay checkPtRay = { nextCheckPt.signpost1, nextCheckPt.signpost2 - nextCheckPt.signpost1 };
    checkPtRay.direction.Normalize();// Collision ray direction must be normalized.
    float fdist = 0;                    // Required by intersection ray-box intersection test but unused. 

    if (worldBox.Intersects(checkPtRay.origin, checkPtRay.direction, fdist))
    {
        if (nextCheckPtId < lastCheckPtId)
            carState.nextCheckpoint++;
        else
            carState.nextCheckpoint = 0;
    }
    /*
    TestGroundCollision(m_racetrack.get(), worldBox, groundTriangle);

    if (groundTriangle.collision == ContainmentType::INTERSECTS)
    {
        //carState.velocity = Vector3::Zero;
        auto groundHeight = (groundTriangle.pointa.y + groundTriangle.pointb.y + groundTriangle.pointc.y) / 3.f;
        auto carPos = m_miniracecar->GetPosition();
        m_miniracecar->SetWorld(Vector3(carPos.x, groundHeight, carPos.z), m_miniracecar->GetOrientation());
    }
    */
    //else
    //{
    //    m_miniracecar->SetWorld(m_miniracecar->GetPosition(), m_miniracecar->GetOrientation());
    //    //m_miniracecar->SetWorld(m_miniracecar->GetPosition(), Vector3(0, -totalTime / 4.f, 0));
    //}

    // Pass game time to FbxLoader to update bone palette.
    //auto fbxModel = m_FBXModel[FBXModels::Dove].get();
    fbxModel->AdvanceTime(totalTime);
    //m_dove->AdvanceTime(totalTime);

    // Rotate dove.
    //auto& world = Matrix::CreateRotationY(elapsedTime) * m_dove->GetWorld();
    fbxModel->SetWorld(fbxModel->GetPosition(), Vector3(0, totalTime, 0));
    //m_dove->SetWorld(m_dove->GetPosition(), Vector3(0, totalTime, 0));
    //m_dove->SetWorld(world);

    //PIXEndEvent();
}

void SceneMain::Render()
{
    const auto timer = m_game->GetTimer();
    const auto frameCount = timer->GetFrameCount();

    // Don't try to render anything before the first Update.
    if (frameCount == 0)
    {
        return;
    }

    // Prepare the command list to render a new frame.
    const auto deviceResources = m_game->GetDeviceResources();
    deviceResources->Prepare();
    //Clear();

    const auto device = deviceResources->GetD3DDevice();      // Required to build TLAS
    const auto commandList = deviceResources->GetCommandList();
    //const auto commandQueue = deviceResources->GetCommandQueue();   // Required by PIX instrumentation.
    //PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    // Begin rendering to hdr target.
    //m_hdrRenderTex->BeginScene(commandList); // transition to render target state

    m_game->Clear(commandList); // Handles hdr target transition to 'render target'.

    // TODO: Add your rendering code here.
    // First, save the previous frame's occlusion buffers.
    const auto aoManager = m_game->GetAOManager();
    const auto shadowManager = m_game->GetShadowManager();

    //if (frameCount > 1)
    //{
    //    aoManager->CopyPreviousFrameBuffer(commandList);
    //    shadowManager->CopyPreviousFrameBuffer(commandList);
    //}

    // Reset compute command list and allocator.
    deviceResources->ResetComputeCommandList();
    const auto computeCommandList = deviceResources->GetComputeCommandList();

    // Set the descriptor heap and root signature on the compute command list.
    const auto descHeap = m_game->GetDescriptorHeap(DescriptorHeaps::SrvUav);
    auto rootSig = m_game->GetRootSignature(RootSignatures::Compute);
    ID3D12DescriptorHeap* heap[] = { descHeap->Heap() };
    computeCommandList->SetDescriptorHeaps(1, heap);
    computeCommandList->SetComputeRootSignature(rootSig);
    //commandList->SetComputeRootSignature(m_rootSig[RootSignatures::ComputeSkinning].Get());

    // Perform vertex skinning asnynchronously on the compute queue.
    auto fbxModel = m_game->GetFbxModel(FBXModels::Dove);
    const auto paletteSize = fbxModel->GetBonePaletteSize();
    //auto paletteSize = m_dove->GetBonePaletteSize();
    
    // The debug layer now requires explicit 256 byte alignment to bind the cbv.
    const auto graphicsMemory = m_game->GetGraphicsMemory();
    const auto boneCBMem = graphicsMemory->Allocate(paletteSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    memcpy(boneCBMem.Memory(), fbxModel->GetBonePalette3X4(), paletteSize);
    //memcpy(cb0Memory.Memory(), m_dove->GetBonePalette3X4(), paletteSize);
    computeCommandList->SetComputeRootConstantBufferView(ComputeRootSigParams::BoneCB, boneCBMem.GpuAddress());
    //computeCommandList->SetComputeRootConstantBufferView(ComputeRootSigParams::FrameConstants, cb0Memory.GpuAddress());
    auto pipelineState = m_game->GetPipelineState(PSOs::Skinning);
    computeCommandList->SetPipelineState(pipelineState);
    //commandList->SetComputeRootConstantBufferView(ComputeRootSigParams::FrameConstants, cb0Memory.GpuAddress());
    //commandList->SetPipelineState(m_pipelineState[PSOs::ComputeSkinning].Get());

    // How many groups do we need to dispatch to cover a batch of vertices, where each group covers 256 vertices?
    // (64 is defined in the ComputeShader)
    uint32_t numGroupsX = static_cast<uint32_t>(ceilf(static_cast<float>(fbxModel->GetVertexCount(0)) / 64.f));
    //uint32_t numGroupsX = static_cast<uint32_t>(ceilf(static_cast<float>(m_dove->GetVertexCount(0)) / 256.f));
    computeCommandList->Dispatch(numGroupsX, 1, 1);
    //commandList->Dispatch(numGroupsX, 1, 1);

    // Send the compute command list to the GPU for processing.
    deviceResources->ExecuteAndWaitForGpuCompute();
    graphicsMemory->Commit(deviceResources->GetComputeCommandQueue());

    //D3D12_RESOURCE_BARRIER uavBarrier = {};
    //uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(m_dove->GetSkinnedVertexBuffer(0));
    //commandList->ResourceBarrier(1, &uavBarrier);

    // Update acceleration structures.
    // Due to deformation of the dove's geometry by skinning, we need to rebuild the dynamic BLAS only.
    BuildDynamicBLAS(device, commandList, true);  // Update BLAS with deformed bottom-level geometry.
    //m_game->BuildDynamicBLAS(device, commandList, true);  // Update BLAS with deformed bottom-level geometry.
    //BuildBLAS(device, commandList, false);  // Update BLAS with new bottom-level geometry data.
    BuildTLASInstanceDescs();
    BuildTLAS(device, commandList, m_tlasBuffers.get(), TLASInstances::tlasCount, true);   // Update TLAS with new top-level instance data.
    //BuildTLASInstanceDescs(m_tlasInstanceDesc.get());
    //BuildTLAS(device, commandList, m_tlasBuffers.get(), m_tlasInstanceDesc.get(), TLASInstances::tlasCount, true);   // Update TLAS with new top-level instance data.
    //m_game->BuildTLAS(device, commandList, m_tlasBuffers.get(), GetTLASInstanceDesc(), TLASInstances::tlasCount, true);   // Update TLAS with new top-level instance data.
    //m_game->BuildTLAS(device, commandList, m_tlasBuffers.get(), true);   // Update TLAS with new top-level instance data.
    //m_game->BuildTLAS(device, commandList, true);   // Update TLAS with new top-level instance data.

    // Set the descriptor heap on the direct command list.
    commandList->SetDescriptorHeaps(1, heap);

    // Set the root signature on the direct command list.
    rootSig = m_game->GetRootSignature(RootSignatures::Compute);
    commandList->SetComputeRootSignature(rootSig);

    const auto shaderTableGroup = m_game->GetShaderTableGroup();

    // Ray tracing ambient occlusion pass
    auto stateObject = m_game->GetStateObject(StateObjects::AOPipeline);
    m_game->DoRaytracing(
        commandList,
        stateObject,
        &shaderTableGroup[StateObjects::AOPipeline],
        aoManager->GetBufferWidth(),
        aoManager->GetBufferHeight()
        //m_game->GetBackbufferWidth() / 2,
        //m_game->GetBackbufferHeight() / 2
    );
    // Ray tracing shadow pass
    stateObject = m_game->GetStateObject(StateObjects::ShadowPipeline);
    m_game->DoRaytracing(
        commandList,
        stateObject,
        &shaderTableGroup[StateObjects::ShadowPipeline],
        shadowManager->GetBufferWidth(),
        shadowManager->GetBufferHeight()
    );

    // Blur the ambient access and shadow visibility buffers.
    // Set constants to send to the blur compute shaders.
    const auto blurConstants = m_game->GetBlurConstants();

    auto blurWeights = aoManager->GetBlurWeights();
    m_game->SetBlurConstants(SrvUAVs::AmbientBuffer0Uav, SrvUAVs::AmbientBuffer1Uav, SrvUAVs::NormalDepthSrv, blurWeights, true);
    auto blurCB = graphicsMemory->AllocateConstant(*blurConstants);
    aoManager->Denoise(commandList, blurCB.GpuAddress(), 4); // Denoising no longer sets a different root signature.

    blurWeights = shadowManager->GetBlurWeights();
    m_game->SetBlurConstants(SrvUAVs::ShadowBuffer0Uav, SrvUAVs::ShadowBuffer1Uav, SrvUAVs::NormalDepthSrv, blurWeights, false);
    blurCB = graphicsMemory->AllocateConstant(*blurConstants);
    shadowManager->Denoise(commandList, blurCB.GpuAddress(), 4); // Denoising no longer sets a different root signature.

    // Execute either the rasterization or DXR pipeline based on the toggle value.
    if (m_isRaster)
    {
        // Set texture descriptor heap in prep for postprocessing/tone mapping.
        // Must be called before Set*RootSignature if using HLSL dynamic resources.
        //ID3D12DescriptorHeap* heap[] = { m_descHeap[DescriptorHeaps::SrvUav]->Heap()};
        //commandList->SetDescriptorHeaps(1, heap);

        // Set the root signature applicable to all rasterization pipeline rendering.
        rootSig = m_game->GetRootSignature(RootSignatures::Graphics);
        commandList->SetGraphicsRootSignature(rootSig);

        //const auto frameConstants = m_scene->GetFrameConstants();

        // Set frame constant buffer on GPU.
        // Do this after the call to ExecuteIndirect, or else it will be overwritten by the constant buffer set
        // by the indirect arguments recoreded in the command buffer, and subsequent geometry will not be visible.
        const auto cb0Memory = graphicsMemory->AllocateConstant(*m_frameConstants);
        //commandList->SetGraphicsRootDescriptorTable(RasterRootSigParams::ObjectDescriptorTableSlot,
        // m_srvUavHeap->GetGpuHandle(SrvUavDescriptors::PlaneDiffuseSrv));
        commandList->SetGraphicsRootConstantBufferView(GraphicsRootSigParams::FrameCB, cb0Memory.GpuAddress());

        // Draw cubes.
        pipelineState = m_game->GetPipelineState(PSOs::Cubes);
        commandList->SetPipelineState(pipelineState);

        const auto procGeometry = m_game->GetProcGeometry();
        const auto cube = &procGeometry[ProcGeometries::Cube];
        const uint32_t transformInstanceSize = SceneMain::CubeInstanceCount * sizeof(XMFLOAT3X4);
        cube->transformInstanceBufferView.SizeInBytes = transformInstanceSize;

        // NB: Can't allocate a pointer as a constant, because length of data array pointed to is unknown.
        const auto vb1Memory = graphicsMemory->Allocate(transformInstanceSize);
        //GraphicsResource res = GraphicsMemory::Get().Allocate(geometryInstanceSize);
        //const auto cubeTransforms = m_game->GetCubeTransforms();
        memcpy(vb1Memory.Memory(), m_cubeTransforms3x4.get(), transformInstanceSize);

        cube->transformInstanceBufferView.BufferLocation = vb1Memory.GpuAddress();
        //m_instanceBufferView.BufferLocation = res.GpuAddress();
        //m_instanceBufferView.SizeInBytes = geometryInstanceSize;
        //m_instanceBufferView.StrideInBytes = sizeof(XMFLOAT3X4);

        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &cube->vertexBufferView);
        commandList->IASetVertexBuffers(1, 1, &cube->transformInstanceBufferView);
        commandList->IASetIndexBuffer(&cube->indexBufferView);
        //commandList->IASetVertexBuffers(2, 1, &cubeBuffers.materialInstanceBufferView);
        //commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
        //commandList->IASetVertexBuffers(1, 1, &m_instanceBufferView);
        //commandList->IASetVertexBuffers(2, 1, &m_materialInstanceBufferView);
        //commandList->IASetIndexBuffer(&m_indexBufferView);
        //commandList->DrawIndexedInstanced(36, CUBE_INSTANCE_COUNT, 0, 0, 0);
        commandList->DrawIndexedInstanced(cube->indexCountPerInstance, SceneMain::CubeInstanceCount, 0, 0, 0);
        //commandList->DrawIndexedInstanced(cube->indexCountPerInstance, Globals::CubeInstanceCount, 0, 0, 0);
        //commandList->DrawIndexedInstanced(36, Globals::CubeInstanceCount, 0, 0, 0);
        //commandList->DrawInstanced(3, 1, 0, 0);

        // Draw model meshes.
        pipelineState = m_game->GetPipelineState(PSOs::MeshOpaque);
        commandList->SetPipelineState(pipelineState);

        GraphicsResource cb1Memory;
        MeshConstants meshConstants = {};

        // Suzanne.
        auto sdkMeshModel = m_game->GetSdkMeshModel(SDKMESHModels::Suzanne);

        meshConstants.world = sdkMeshModel->GetWorld().Transpose();
        meshConstants.instanceID = ShaderInstances::instSuzanne;
        //meshConstants.instanceID = MeshGeometries::Suzanne;

        cb1Memory = graphicsMemory->AllocateConstant(meshConstants);
        //auto cb1Memory = m_graphicsMemory->AllocateConstant(sdkMeshModel->GetWorld().Transpose());
        //graphicsRes = m_graphicsMemory->AllocateConstant(Matrix::CreateTranslation(Vector3(0, 0, -2.f)).Transpose());
        commandList->SetGraphicsRootConstantBufferView(GraphicsRootSigParams::MeshCB, cb1Memory.GpuAddress());
        sdkMeshModel->Draw(commandList);
        //m_suzanne->GetRenderingModel()->Draw(commandList);

        // Mini racecar.
        sdkMeshModel = m_game->GetSdkMeshModel(SDKMESHModels::MiniRacecar);

        meshConstants.world = sdkMeshModel->GetWorld().Transpose();
        meshConstants.instanceID = ShaderInstances::instMiniRacecar;
        //meshConstants.instanceID = MeshGeometries::MiniRaceCar;

        cb1Memory = graphicsMemory->AllocateConstant(meshConstants);
        commandList->SetGraphicsRootConstantBufferView(GraphicsRootSigParams::MeshCB, cb1Memory.GpuAddress());
        sdkMeshModel->Draw(commandList);

        // Dove.
        // This is an FBX model that was previously skinned by a vertex shader.
        // Since we now perform skinning with a compute shader, we can draw the skinned mesh directly.
        auto fbxModel = m_game->GetFbxModel(FBXModels::Dove);

        meshConstants.world = fbxModel->GetWorld().Transpose();
        meshConstants.instanceID = ShaderInstances::instDove;
        //meshConstants.instanceID = MeshGeometries::Dove;

        cb1Memory = graphicsMemory->AllocateConstant(meshConstants);
        //cb1Memory = m_graphicsMemory->AllocateConstant(m_dove->GetWorld().Transpose());

        //auto paletteSize = fbxModel->GetBonePaletteSize();
        //auto paletteSize = m_dove->GetBonePaletteSize();
        //auto cb2Memory = m_graphicsMemory->Allocate(paletteSize);
        //memcpy(cb2Memory.Memory(), fbxModel->GetBonePalette3X4(), paletteSize);
        //memcpy(cb2Memory.Memory(), m_dove->GetBonePalette3X4(), paletteSize);

        commandList->SetGraphicsRootConstantBufferView(GraphicsRootSigParams::MeshCB, cb1Memory.GpuAddress());
        //commandList->SetGraphicsRootConstantBufferView(GraphicsRootSigParams::BoneCB, cb2Memory.GpuAddress());
        //commandList->SetPipelineState(m_pipelineState[PSOs::FbxModel].Get());
        fbxModel->DrawSkinned(commandList);
        //m_dove->Draw(commandList);

        // Racetrack map.
        //sdkMeshModel = m_game->GetSdkMeshModel(SDKMESHModels::RacetrackMap);
        //
        //meshConstants.world = sdkMeshModel->GetWorld().Transpose();
        //meshConstants.instanceID = ShaderInstances::instRacetrackReserved2;
        //
        //cb1Memory = graphicsMemory->AllocateConstant(meshConstants);
        //commandList->SetGraphicsRootConstantBufferView(GraphicsRootSigParams::MeshCB, cb1Memory.GpuAddress());
        //sdkMeshModel->Draw(commandList);

        // Racetrack has three meshes, drawn separately.
        sdkMeshModel = m_game->GetSdkMeshModel(SDKMESHModels::Racetrack);

        meshConstants.world = sdkMeshModel->GetWorld().Transpose();

        for (uint32_t meshIndex = 0; meshIndex < 3; meshIndex++)
            //for (uint32_t meshPartIndex = 0; meshPartIndex < 2; meshPartIndex++)
        {
            //auto& modelMesh = m_racetrack->GetRenderingModel()->meshes.at(meshIndex);
            //auto& meshPart = modelMesh->opaqueMeshParts.at(0);
            //auto& modelMesh = m_racetrack->GetRenderingModel()->meshes.at(0);
            //auto& meshPart = modelMesh->opaqueMeshParts.at(meshPartIndex);

            meshConstants.instanceID = ShaderInstances::instRacetrack + meshIndex;
            //switch (meshIndex)
            //    //switch (meshPartIndex)
            //{
            //case 0: // RacetrackRoad
            //    meshConstants.instanceID = MeshGeometries::RacetrackRoad;
            //    //commandList->SetPipelineState(m_pipelineState[PSOs::RacetrackRoad].Get());
            //    break;
            //case 1: // RacetrackGrass
            //    meshConstants.instanceID = MeshGeometries::RacetrackGrass;
            //    //commandList->SetPipelineState(m_pipelineState[PSOs::RacetrackGrass].Get());
            //    break;
            //default:
            //    break;
            //}
            cb1Memory = graphicsMemory->AllocateConstant(meshConstants);
            commandList->SetGraphicsRootConstantBufferView(GraphicsRootSigParams::MeshCB, cb1Memory.GpuAddress());
            sdkMeshModel->Draw(meshIndex, 0, commandList);
            //meshPart->Draw(commandList);
        }

        // Draw geosphere skydome.
        // It's rendered after all other opaque scene geometry (but before transparent scene geometry) to minimise overdraw.
        // The geosphere is no longer being drawn indirectly.
        const auto geoSphere = &procGeometry[ProcGeometries::GeoSphere];
        //const auto& geoSphere = m_procGeometry[ProcGeometries::GeoSphere];

        pipelineState = m_game->GetPipelineState(PSOs::GeoSphere);
        commandList->SetPipelineState(pipelineState);

        // Indicate that the command buffer will be used for indirect drawing.
        //const auto commandBuffer = m_game->GetCommandBuffer();
        //D3D12_RESOURCE_BARRIER commandBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        //    commandBuffer,
        //    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        //    D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
        //commandList->ResourceBarrier(1, &commandBarrier);

        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &geoSphere->vertexBufferView);
        commandList->IASetIndexBuffer(&geoSphere->indexBufferView);

        // Draw the geosphere with ExecuteIndirect.
        //commandList->ExecuteIndirect(m_game->GetCommandSignature(), 1, commandBuffer,
        //    sizeof(IndirectCommand) * deviceResources->GetCurrentFrameIndex(), nullptr, 0);

        commandList->DrawIndexedInstanced(geoSphere->indexCountPerInstance, 1, 0, 0, 0);
        //m_geospherePrimitive->Draw(commandList);

        //commandBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
        //commandBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        //commandList->ResourceBarrier(1, &commandBarrier);

        // Palmtree has two meshes, where the canopy mesh must be drawn with alpha blending.
        sdkMeshModel = m_game->GetSdkMeshModel(SDKMESHModels::Palmtree);

        // Restore opaque mesh pipeline state.
        pipelineState = m_game->GetPipelineState(PSOs::MeshOpaque);
        commandList->SetPipelineState(pipelineState);

        meshConstants.world = sdkMeshModel->GetWorld().Transpose();

        for (uint32_t meshIndex = 0; meshIndex < 2; meshIndex++)
        {
            meshConstants.instanceID = ShaderInstances::instPalmtree + meshIndex;
            if (meshIndex == 1)
            {
                pipelineState = m_game->GetPipelineState(PSOs::MeshAlphaBlend);
                commandList->SetPipelineState(pipelineState);
            }
            //switch (meshIndex)
            //{
            //case 0: // Palm tree trunk
            //    meshConstants.instanceID = ShaderInstances::PalmtreeTrunk;
            //    //meshConstants.instanceID = MeshGeometries::PalmtreeTrunk;
            //    break;
            //case 1: // Palm tree foliage
            //    meshConstants.instanceID = ShaderInstances::PalmtreeFoliage;
            //    //meshConstants.instanceID = MeshGeometries::PalmtreeFoliage;
            //    commandList->SetPipelineState(m_pipelineState[PSOs::MeshAlphaBlend].Get());
            //    break;
            //default:
            //    break;
            //}
            cb1Memory = graphicsMemory->AllocateConstant(meshConstants);
            commandList->SetGraphicsRootConstantBufferView(GraphicsRootSigParams::MeshCB, cb1Memory.GpuAddress());
            //auto& modelMesh = m_palmtree->GetRenderingModel()->meshes.at(meshIndex);
            //auto& meshPart  = modelMesh->opaqueMeshParts.at(0);
            sdkMeshModel->Draw(meshIndex, 0, commandList);
            //meshPart->Draw(commandList);
        }

        /*
        // Draw transparent ground plane last.

        // Set material root constants on GPU.
        //GraphicsResource materialCB = m_graphicsMemory->AllocateConstant(*m_planeConstants);
        //commandList->SetGraphicsRootConstantBufferView(RasterRootSigParams::ObjectConstantSlot, materialCB.GpuAddress());
        //commandList->SetGraphicsRoot32BitConstants(RasterRootSigParams::ObjectConstantSlot, SizeOfInUint32(ObjectConstants),
        // &m_planeConstants.diffuse, 0);

        //commandList->SetPipelineState(m_pipelineState[PSOs::Plane].Get());

        //auto& planeBuffers = m_procGeometry[GeometryInstances::Plane];
        //commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &planeBuffers.vertexBufferView);
        commandList->IASetIndexBuffer(&planeBuffers.indexBufferView);
        //commandList->IASetVertexBuffers(0, 1, &m_planeVertexBufferView);
        //commandList->IASetIndexBuffer(&m_planeIndexBufferView);
        commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
        */

        // Set Hdr texture to required state for postprocessing.
        m_game->PrepareHDRTextureForPostProcessing();
        //m_renderTex[RenderTextures::Hdr]->TransitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
    else
    {
        //BuildTopLevelAS(m_blasBuffers, true); // update TLAS with new blas instance data

        // Bind the heaps and all resources required by the shader, and dispatch rays.
        // Must be called before Set*RootSignature if using HLSL dynamic resources.
        // WARNING! Moving the SetDescriptorHeaps call outside the if else block prevented writes to the normal-depth and rtao buffers,
        // because we must set the descriptor heap downstream of BuildTopLevelAS!
        // This failed silently!
        //ID3D12DescriptorHeap* heap[] = { m_srvUavHeap->Heap() };
        //commandList->SetDescriptorHeaps(1, heap);
        //commandList->SetDescriptorHeaps(1, m_descriptorHeap.GetAddressOf());

        // Color pass
        stateObject = m_game->GetStateObject(StateObjects::MainPipeline);
        m_game->DoRaytracing(
            commandList,
            stateObject,
            &shaderTableGroup[StateObjects::MainPipeline],
            m_game->GetBackbufferWidth(),
            m_game->GetBackbufferHeight()
        );
        //DoRaytracing(commandList);
        m_game->CopyRaytracingOutputToBackbuffer(commandList);
    }

    // Hdr rendering completed.
    //m_hdrRenderTex->EndScene(commandList); // transition to pixel shader resource state
    //PIXEndEvent(commandList);

    // Apply all scene postprocessing steps prior to presentation.
    m_game->DoPostprocessing(commandList);

    // Show the new frame.
    //PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    deviceResources->Present();

    // If using the DirectX Tool Kit for DX12, uncomment this line:
    graphicsMemory->Commit(deviceResources->GetCommandQueue());

    //PIXEndEvent();
}

// Build geometry descs for bottom-level AS.
void SceneMain::BuildBLASGeometryDescs()
//void Game::BuildBLASGeometryDescs(D3D12_RAYTRACING_GEOMETRY_DESC* geometryDescs)
//void Game::BuildGeometryDescsForBottomLevelAS(std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& geometryDescs)
//void Game::BuildGeometryDescsForBottomLevelAS(std::array<std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>, 2>& geometryDescs)
//void Game::BuildGeometryDescsForBottomLevelAS(std::array<std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>, GeometryType::Count>& geometryDescs)
{
    // Mark the geometry as opaque. 
    // PERFORMANCE TIP: mark geometry as opaque whenever applicable as it can enable important ray processing optimizations.
    // Note: When rays encounter opaque geometry an any hit shader will not be executed whether it is present or not.
    //D3D12_RAYTRACING_GEOMETRY_FLAGS geometryFlags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    //geometryDescs[GeometryType::Triangle].resize(2);

    // Scaling down x100 and 90 deg x-rotation are required if not animating the Fbx model.
    //auto& doveRTL = Matrix::CreateScale(0.01f) * Matrix::CreateRotationX(XM_PIDIV2);
    //XMFLOAT3X4 doveRTL3x4 = {};
    //XMStoreFloat3x4(&doveRTL3x4, doveRTL);
    //auto doveRTL3x4Mem = m_graphicsMemory->AllocateConstant(doveRTL3x4);

    // Test updating geometry per frame by updating the 'Transform3x4' field.
    //auto totalTime = static_cast<float>(m_timer->GetTotalSeconds());
    //auto& transformRTL = Matrix::CreateTranslation(cos(totalTime), 0, 0);
    //XMFLOAT3X4 transformRTL3x4 = {};
    //XMStoreFloat3x4(&transformRTL3x4, transformRTL);

    //auto grTransform = m_graphicsMemory->AllocateConstant(transformRTL3x4);
    //auto grTransform = GraphicsMemory::Get().Allocate(sizeof(XMFLOAT3X4));
    //memcpy(grTransform.Memory(), &transformRTL3x4, sizeof(XMFLOAT3X4));

    //auto& roadRTL = Matrix::CreateTranslation(Vector3(0, 0.1f, 0)); // This is a transform offset from the origin.
    //XMFLOAT3X4 roadRTL3x4 = {};
    //XMStoreFloat3x4(&roadRTL3x4, roadRTL);
    //auto roadRTL3x4Mem = m_graphicsMemory->AllocateConstant(roadRTL3x4);

    // Get buffers for cube meshes built inside this application.
    const auto& procGeometry = m_game->GetProcGeometry();
    const auto& cubeBuffers = procGeometry[ProcGeometries::Cube];
    //const auto& cubeBuffers = m_procGeometry[ProcGeometries::Cube];
    // Model pointers will be updated for each mesh geometry.
    auto sdkMeshModel = m_game->GetSdkMeshModel(SDKMESHModels::Suzanne);
    auto fbxModel = m_game->GetFbxModel(FBXModels::Dove);
    //auto sdkMeshModel = m_SDKMESHModel[SDKMESHModels::Suzanne].get();
    //auto fbxModel = m_FBXModel[FBXModels::Dove].get();
    //auto& planeBuffers = m_procGeometry[GeometryInstances::Plane];

    //const auto sceneGeometryDesc = m_mainScene->GetGeometryDesc();

    for (uint32_t geometryIndex = 0; geometryIndex < SceneGeometries::sceneCount; geometryIndex++)
        //for (uint32_t geometryIndex = 0; geometryIndex < MeshGeometries::Count; geometryIndex++)
        //for (uint32_t geometryIndex = 0; geometryIndex < TriangleMeshes::Count; geometryIndex++)
    {   // Cube geometry.
        //geometryDescs[0].resize(1);

        // These geometryDesc fields are common to all geometry instances.
        //auto& geometryDesc = sceneGeometryDesc[geometryIndex];// [0] ;
        auto& geometryDesc = m_geometryDesc[geometryIndex];// [0] ;
        //auto& geometryDesc = geometryDescs[geometryIndex];// [0] ;
        //auto& geometryDesc = geometryDescs[GeometryType::Triangle][0];
        geometryDesc = {};

        // Assign the right geometry data to each geometryDesc.
        switch (geometryIndex)
        {
        case SceneGeometries::sceneCube:
            //case TriangleMeshes::Cube:
            geometryDesc.Triangles.IndexBuffer = cubeBuffers.indexBuffer->GetGPUVirtualAddress();
            geometryDesc.Triangles.IndexCount = static_cast<uint32_t>(cubeBuffers.indexBuffer->GetDesc().Width) / sizeof(uint32_t); //sizeof(uint16_t);
            //geometryDesc.Triangles.IndexBuffer = m_indexBuffer->GetGPUVirtualAddress();
            //geometryDesc.Triangles.IndexCount = static_cast<uint32_t>(m_indexBuffer->GetDesc().Width) / sizeof(uint16_t);
            geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT; //DXGI_FORMAT_R16_UINT;
            //geometryDesc.Triangles.IndexFormat  = DXGI_FORMAT_R16_UINT;
            geometryDesc.Triangles.VertexCount = static_cast<uint32_t>(cubeBuffers.vertexBuffer->GetDesc().Width) / sizeof(VertexPositionNormalTexture);
            geometryDesc.Triangles.VertexBuffer.StartAddress = cubeBuffers.vertexBuffer->GetGPUVirtualAddress();
            //geometryDesc.Triangles.VertexCount = static_cast<uint32_t>(m_vertexBuffer->GetDesc().Width) / sizeof(VertexPositionNormal);
            //geometryDesc.Triangles.VertexBuffer.StartAddress = m_vertexBuffer->GetGPUVirtualAddress();
            geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(VertexPositionNormalTexture);
            geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
            geometryDesc.Triangles.Transform3x4 = 0;
            //geometryDesc.Triangles.Transform3x4 = grTransform.GpuAddress();
            geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;// geometryFlags;
            geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            break;
        case SceneGeometries::sceneSuzanne:
            //case TriangleMeshes::SDKMESH:
            geometryDesc.Triangles.IndexBuffer = sdkMeshModel->GetIndexBuffer(0,0)->GetGPUVirtualAddress();
            geometryDesc.Triangles.IndexCount = sdkMeshModel->GetIndexCount(0,0);
            geometryDesc.Triangles.IndexFormat = sdkMeshModel->GetIndexFormat(0,0);
            //geometryDesc.Triangles.IndexFormat  = DXGI_FORMAT_R16_UINT;
            geometryDesc.Triangles.VertexCount = sdkMeshModel->GetVertexCount(0,0);
            geometryDesc.Triangles.VertexBuffer.StartAddress = sdkMeshModel->GetVertexBuffer(0,0)->GetGPUVirtualAddress();
            geometryDesc.Triangles.VertexBuffer.StrideInBytes = sdkMeshModel->GetVertexStride(0,0);
            geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
            geometryDesc.Triangles.Transform3x4 = 0;
            //geometryDesc.Triangles.Transform3x4 = grTransform.GpuAddress();
            geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
            geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            break;
        case SceneGeometries::sceneRacetrackRoad:
            sdkMeshModel = m_game->GetSdkMeshModel(SDKMESHModels::Racetrack);

            //sdkMeshModel = m_SDKMESHModel[SDKMESHModels::Racetrack].get();
            //case TriangleMeshes::SDKMESH:
            geometryDesc.Triangles.IndexBuffer = sdkMeshModel->GetIndexBuffer(0,0)->GetGPUVirtualAddress();
            // Must manually calculate index count for subsequent submeshes for correct interpretation by DXR.
            //geometryDesc.Triangles.IndexBuffer = m_racetrack_road->GetIndexBuffer(0,0)->GetGPUVirtualAddress();
            //geometryDesc.Triangles.IndexCount = m_racetrack->GetIndexBufferSize(0,1) / static_cast<uint32_t>(sizeof(uint16_t));
            //geometryDesc.Triangles.IndexCount  = static_cast<uint32_t>(m_racetrack->GetIndexBuffer(0,1)->GetDesc().Width / sizeof(uint16_t));
            geometryDesc.Triangles.IndexCount = sdkMeshModel->GetIndexCount(0,0);
            geometryDesc.Triangles.IndexFormat = sdkMeshModel->GetIndexFormat(0,0);
            //geometryDesc.Triangles.IndexFormat  = DXGI_FORMAT_R16_UINT;
            geometryDesc.Triangles.VertexCount = sdkMeshModel->GetVertexCount(0,0);
            geometryDesc.Triangles.VertexBuffer.StartAddress = sdkMeshModel->GetVertexBuffer(0,0)->GetGPUVirtualAddress();
            geometryDesc.Triangles.VertexBuffer.StrideInBytes = sdkMeshModel->GetVertexStride(0,0);
            geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
            geometryDesc.Triangles.Transform3x4 = 0;
            //geometryDesc.Triangles.Transform3x4 = roadRTL3x4Mem.GpuAddress();
            //geometryDesc.Triangles.Transform3x4 = grTransform.GpuAddress();
            //geometryDesc.Triangles.VertexCount = m_racetrack_road->GetVertexCount(0,0);
            //geometryDesc.Triangles.VertexBuffer.StartAddress  = m_racetrack_road->GetVertexBuffer(0,0)->GetGPUVirtualAddress();
            //geometryDesc.Triangles.VertexBuffer.StrideInBytes = m_racetrack_road->GetVertexStride(0,0);
            geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
            geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            break;
        case SceneGeometries::sceneRacetrackSkirt:
            //case TriangleMeshes::SDKMESH:
            geometryDesc.Triangles.IndexBuffer = sdkMeshModel->GetIndexBuffer(1,0)->GetGPUVirtualAddress();
            geometryDesc.Triangles.IndexCount = sdkMeshModel->GetIndexCount(1,0);
            geometryDesc.Triangles.IndexFormat = sdkMeshModel->GetIndexFormat(1,0);
            //geometryDesc.Triangles.IndexBuffer = m_racetrack_grass->GetIndexBuffer(0,0)->GetGPUVirtualAddress();
            //geometryDesc.Triangles.IndexCount  = m_racetrack_grass->GetIndexCount(0,0);
            //geometryDesc.Triangles.IndexFormat  = DXGI_FORMAT_R16_UINT;
            geometryDesc.Triangles.VertexCount = sdkMeshModel->GetVertexCount(1,0);
            geometryDesc.Triangles.VertexBuffer.StartAddress = sdkMeshModel->GetVertexBuffer(1,0)->GetGPUVirtualAddress();
            geometryDesc.Triangles.VertexBuffer.StrideInBytes = sdkMeshModel->GetVertexStride(1,0);
            geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
            geometryDesc.Triangles.Transform3x4 = 0;
            //geometryDesc.Triangles.Transform3x4 = grTransform.GpuAddress();
            //geometryDesc.Triangles.VertexCount = m_racetrack_grass->GetVertexCount(0,0);
            //geometryDesc.Triangles.VertexBuffer.StartAddress  = m_racetrack_grass->GetVertexBuffer(0,0)->GetGPUVirtualAddress();
            //geometryDesc.Triangles.VertexBuffer.StrideInBytes = m_racetrack_grass->GetVertexStride(0,0);
            geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
            geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            break;
        case SceneGeometries::sceneRacetrackMap:
            geometryDesc.Triangles.IndexBuffer = sdkMeshModel->GetIndexBuffer(2,0)->GetGPUVirtualAddress();
            geometryDesc.Triangles.IndexCount = sdkMeshModel->GetIndexCount(2,0);
            geometryDesc.Triangles.IndexFormat = sdkMeshModel->GetIndexFormat(2,0);
            geometryDesc.Triangles.VertexCount = sdkMeshModel->GetVertexCount(2,0);
            geometryDesc.Triangles.VertexBuffer.StartAddress = sdkMeshModel->GetVertexBuffer(2,0)->GetGPUVirtualAddress();
            geometryDesc.Triangles.VertexBuffer.StrideInBytes = sdkMeshModel->GetVertexStride(2,0);
            geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
            geometryDesc.Triangles.Transform3x4 = 0;
            geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
            geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            break;
        case SceneGeometries::scenePalmtreeTrunk:
            sdkMeshModel = m_game->GetSdkMeshModel(SDKMESHModels::Palmtree);

            //sdkMeshModel = m_SDKMESHModel[SDKMESHModels::Palmtree].get();
            geometryDesc.Triangles.IndexBuffer = sdkMeshModel->GetIndexBuffer(0,0)->GetGPUVirtualAddress();
            geometryDesc.Triangles.IndexCount = sdkMeshModel->GetIndexCount(0,0);
            geometryDesc.Triangles.IndexFormat = sdkMeshModel->GetIndexFormat(0,0);
            //geometryDesc.Triangles.IndexFormat  = DXGI_FORMAT_R16_UINT;
            geometryDesc.Triangles.VertexCount = sdkMeshModel->GetVertexCount(0,0);
            geometryDesc.Triangles.VertexBuffer.StartAddress = sdkMeshModel->GetVertexBuffer(0,0)->GetGPUVirtualAddress();
            geometryDesc.Triangles.VertexBuffer.StrideInBytes = sdkMeshModel->GetVertexStride(0,0);
            geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
            geometryDesc.Triangles.Transform3x4 = 0;
            //geometryDesc.Triangles.Transform3x4 = grTransform.GpuAddress();
            //geometryDesc.Triangles.Transform3x4 = roadRTL3x4Mem.GpuAddress();
            geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
            geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            break;
        case SceneGeometries::scenePalmtreeCanopy:
            geometryDesc.Triangles.IndexBuffer = sdkMeshModel->GetIndexBuffer(1,0)->GetGPUVirtualAddress();
            geometryDesc.Triangles.IndexCount = sdkMeshModel->GetIndexCount(1,0);
            geometryDesc.Triangles.IndexFormat = sdkMeshModel->GetIndexFormat(1,0);
            //geometryDesc.Triangles.IndexFormat  = DXGI_FORMAT_R16_UINT;
            geometryDesc.Triangles.VertexCount = sdkMeshModel->GetVertexCount(1,0);
            geometryDesc.Triangles.VertexBuffer.StartAddress = sdkMeshModel->GetVertexBuffer(1,0)->GetGPUVirtualAddress();
            geometryDesc.Triangles.VertexBuffer.StrideInBytes = sdkMeshModel->GetVertexStride(1,0);
            geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
            geometryDesc.Triangles.Transform3x4 = 0;
            //geometryDesc.Triangles.Transform3x4 = grTransform.GpuAddress();
            //geometryDesc.Triangles.Transform3x4 = roadRTL3x4Mem.GpuAddress();
            geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE; // Transparent geometry requires anyhit shader invocation.
            geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            break;
        case SceneGeometries::sceneMiniRacecar:
            sdkMeshModel = m_game->GetSdkMeshModel(SDKMESHModels::MiniRacecar);
            //sdkMeshModel = m_SDKMESHModel[SDKMESHModels::MiniRacecar].get();

            geometryDesc.Triangles.IndexBuffer = sdkMeshModel->GetIndexBuffer(0,0)->GetGPUVirtualAddress();
            geometryDesc.Triangles.IndexCount = sdkMeshModel->GetIndexCount(0,0);
            geometryDesc.Triangles.IndexFormat = sdkMeshModel->GetIndexFormat(0,0);
            //geometryDesc.Triangles.IndexFormat  = DXGI_FORMAT_R16_UINT;
            geometryDesc.Triangles.VertexCount = sdkMeshModel->GetVertexCount(0,0);
            geometryDesc.Triangles.VertexBuffer.StartAddress = sdkMeshModel->GetVertexBuffer(0,0)->GetGPUVirtualAddress();
            geometryDesc.Triangles.VertexBuffer.StrideInBytes = sdkMeshModel->GetVertexStride(0,0);
            geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
            geometryDesc.Triangles.Transform3x4 = 0;
            //geometryDesc.Triangles.Transform3x4 = grTransform.GpuAddress();
            geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
            geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            break;
        case SceneGeometries::sceneDove:
            //case TriangleMeshes::Dove:
            geometryDesc.Triangles.IndexBuffer = fbxModel->GetIndexBuffer(0)->GetGPUVirtualAddress();
            geometryDesc.Triangles.IndexCount = fbxModel->GetIndexCount(0);
            geometryDesc.Triangles.IndexFormat = fbxModel->GetIndexFormat(0);// DXGI_FORMAT_R16_UINT; // Fbx index size is only _r16.
            //geometryDesc.Triangles.IndexFormat  = DXGI_FORMAT_R16_UINT;
            geometryDesc.Triangles.VertexCount = fbxModel->GetVertexCount(0);
            geometryDesc.Triangles.VertexBuffer.StartAddress = fbxModel->GetSkinnedVertexBuffer(0)->GetGPUVirtualAddress();
            geometryDesc.Triangles.VertexBuffer.StrideInBytes = fbxModel->GetSkinnedVertexStride();
            geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
            geometryDesc.Triangles.Transform3x4 = 0;
            //geometryDesc.Triangles.Transform3x4 = grTransform.GpuAddress();
            //geometryDesc.Triangles.Transform3x4 = doveRTL3x4Mem.GpuAddress();
            //geometryDesc.Triangles.VertexBuffer.StartAddress = m_dove->GetVertexBuffer(0)->GetGPUVirtualAddress();
            //geometryDesc.Triangles.VertexBuffer.StrideInBytes = m_dove->GetVertexStride();
            geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
            geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            break;
        default:
            break;
        }
    }
}

void SceneMain::BuildStaticBLAS(ID3D12Device10* device, ID3D12GraphicsCommandList7* commandList)
    //bool isFirstBuild)
//AccelerationStructureBuffers Game::BuildBottomLevelAS(const D3D12_RAYTRACING_GEOMETRY_DESC& geometryDesc)
//AccelerationStructureBuffers Game::BuildBottomLevelAS(const std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& geometryDescs,
// D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags)
{
    //auto device = m_deviceResources->GetD3DDevice();
    //auto commandList = m_deviceResources->GetCommandList();
    //ComPtr<ID3D12Resource> scratch;
    //ComPtr<ID3D12Resource> bottomLevelAS;

    // Build bottom-level AS.
    //AccelerationStructureBuffers bottomLevelAS[TriangleMeshes::Count];// [GeometryType::Count] ; // only triangle geometry type exists in this example
    //D3D12_RAYTRACING_GEOMETRY_DESC geometryDescs[TriangleMeshes::Count] = {};
    //std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs(TriangleMeshes::Count);
    //std::array<std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>, 2> geometryDescs;
    //std::array<std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>, GeometryType::Count> geometryDescs;

    //if (isFirstBuild)
    //{
    //    BuildBLASGeometryDescs();
    //    //BuildBLASGeometryDescs(geometryDescs);
    //    //m_blasBuffers.resize(static_cast<UINT>(TriangleMeshes::Count));
    //}

    //D3D12_RESOURCE_BARRIER uavBarriers[StaticBLAS::staticCount] = {};
    //D3D12_RESOURCE_BARRIER resourceBarriers[MeshGeometries::Count] = {};
    //D3D12_RESOURCE_BARRIER resourceBarriers[TriangleMeshes::Count] = {};
    //D3D12_RESOURCE_BARRIER resourceBarriers[GeometryType::Count]{};

    //const auto sceneGeometryDesc = m_mainScene->GetGeometryDesc();

    for (uint32_t blasIndex = 0; blasIndex < StaticBLAS::staticCount; blasIndex++)
        //for (uint32_t meshIndex = 0; meshIndex < MeshGeometries::Count; meshIndex++)
        //for (uint32_t meshIndex = 0; meshIndex < TriangleMeshes::Count; meshIndex++)
    {
        // Get the size requirements for the scratch and AS buffers.
        //D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = {};// = bottomLevelBuildDesc.Inputs;
        bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
        bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        bottomLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE
            | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
        //bottomLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

        switch (blasIndex)
        {
        case StaticBLAS::staticCube:
            //bottomLevelInputs.pGeometryDescs = &sceneGeometryDesc[SceneMain::SceneGeometries::Cube];//.data();
            bottomLevelInputs.pGeometryDescs = &m_geometryDesc[SceneGeometries::sceneCube];//.data();
            //bottomLevelInputs.pGeometryDescs = &geometryDescs[meshIndex];//.data();
            //bottomLevelInputs.pGeometryDescs = &geometryDesc;//.data();
            bottomLevelInputs.NumDescs = 1;// static_cast<UINT>(geometryDescs.size());
            break;
        case StaticBLAS::staticSuzanne:
            //bottomLevelInputs.pGeometryDescs = &sceneGeometryDesc[SceneMain::SceneGeometries::Suzanne];
            bottomLevelInputs.pGeometryDescs = &m_geometryDesc[SceneGeometries::sceneSuzanne];
            bottomLevelInputs.NumDescs = 1;
            break;
        case StaticBLAS::staticRacetrack:
            // GeometryID 0: Racetrack road; GeometryID 1: Racetrack grass.
            //bottomLevelInputs.pGeometryDescs = &sceneGeometryDesc[SceneMain::SceneGeometries::RacetrackRoad];
            bottomLevelInputs.pGeometryDescs = &m_geometryDesc[SceneGeometries::sceneRacetrackRoad];
            bottomLevelInputs.NumDescs = 3;
            //bottomLevelInputs.NumDescs = 2;
            break;
        case StaticBLAS::staticPalmtree:
            // GeometryID 0: Palmtree trunk; GeometryID 1: Palmtree canopy.
            //bottomLevelInputs.pGeometryDescs = &sceneGeometryDesc[SceneMain::SceneGeometries::PalmtreeTrunk];
            bottomLevelInputs.pGeometryDescs = &m_geometryDesc[SceneGeometries::scenePalmtreeTrunk];
            bottomLevelInputs.NumDescs = 2;
            break;
            //case BLAS::PalmtreeFoliage:
            //    bottomLevelInputs.pGeometryDescs = &m_geometryDesc[MeshGeometries::PalmtreeFoliage];
            //    bottomLevelInputs.NumDescs = 1;
            //    break;
        case StaticBLAS::staticMiniRacecar:
            //bottomLevelInputs.pGeometryDescs = &sceneGeometryDesc[SceneMain::SceneGeometries::MiniRacecar];
            bottomLevelInputs.pGeometryDescs = &m_geometryDesc[SceneGeometries::sceneMiniRacecar];
            bottomLevelInputs.NumDescs = 1;
            break;
            //case BLAS::Dove:
            //    bottomLevelInputs.pGeometryDescs = &m_geometryDesc[MeshGeometries::Dove];
            //    bottomLevelInputs.NumDescs = 1;
            //    break;
        default:
            break;
        }

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
        device->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
        DX::ThrowIfFalse(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

        // Create a scratch buffer.
        //const auto blasStatic = m_mainScene->GetBLASBuffers(BLASType::Static);
        //auto& blas = blasStatic[blasIndex];
        auto& blas = m_blasBuffers[BLASType::Static][blasIndex];
        AllocateUAVBufferWithLayout(device, bottomLevelPrebuildInfo.ScratchDataSizeInBytes, &blas.scratch, L"ScratchResource");
        /*
        AllocateUAVBuffer(
            device,
            bottomLevelPrebuildInfo.ScratchDataSizeInBytes,
            &blas.scratch,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            L"ScratchResource"
        );
        */
        //AllocateUAVBuffer(device, bottomLevelPrebuildInfo.ScratchDataSizeInBytes, &scratch,
        // D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"ScratchResource");

        // Allocate resources for acceleration structures.
        // Acceleration structures can only be placed in resources that are created in the default heap (or custom heap equivalent). 
        // Default heap is OK since the application doesn’t need CPU read/write access to them. 
        // The resources that will contain acceleration structures must be created in the state D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, 
        // and must have resource flag D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS. The ALLOW_UNORDERED_ACCESS requirement simply acknowledges both: 
        //  - the system will be doing this type of access in its implementation of acceleration structure builds behind the scenes.
        //  - from the app point of view, synchronization of writes/reads to acceleration structures is accomplished using UAV barriers.
        //D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

        AllocateUAVBufferWithLayout(device, bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, &blas.accelerationStructure,
            L"StaticBLAS", true);
        /*
        AllocateUAVBuffer(
            device,
            bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes,
            &blas.accelerationStructure,
            D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
            L"BottomLevelAccelerationStructure"
        );
        */
        //AllocateUAVBuffer(device, bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, &bottomLevelAS,
        // D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, L"BottomLevelAccelerationStructure");
    //}

    // Bottom-level AS desc.
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
        bottomLevelBuildDesc.Inputs = bottomLevelInputs;
        bottomLevelBuildDesc.ScratchAccelerationStructureData = blas.scratch->GetGPUVirtualAddress();
        bottomLevelBuildDesc.DestAccelerationStructureData = blas.accelerationStructure->GetGPUVirtualAddress();
        //bottomLevelBuildDesc.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress();
        //bottomLevelBuildDesc.DestAccelerationStructureData = bottomLevelAS->GetGPUVirtualAddress();

        // Build the acceleration structure.
        commandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);

        //AccelerationStructureBuffers bottomLevelASBuffers;
        //bottomLevelASBuffers.accelerationStructure = bottomLevelAS;
        //bottomLevelASBuffers.scratch = scratch;
        //bottomLevelASBuffers.instanceDesc = nullptr; // not required by BLAS
        ////bottomLevelASBuffers.ResultDataMaxSizeInBytes = bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes;
        //return bottomLevelASBuffers;
    }

    // Batch all barriers for BLAS builds.
    //commandList->ResourceBarrier(StaticBLAS::staticCount, uavBarriers);
    //commandList->ResourceBarrier(MeshGeometries::Count, resourceBarriers);
    //commandList->ResourceBarrier(TriangleMeshes::Count, resourceBarriers);
    //commandList->ResourceBarrier(GeometryType::Count, resourceBarriers);
}

void SceneMain::BuildDynamicBLAS(ID3D12Device10* device, ID3D12GraphicsCommandList7* commandList, bool isUpdate)
{
    //D3D12_RESOURCE_BARRIER uavBarriers[DynamicBLAS::dynamicCount] = {};
    //const auto sceneGeometryDesc = m_mainScene->GetGeometryDesc();

    for (uint32_t blasIndex = 0; blasIndex < DynamicBLAS::dynamicCount; blasIndex++)
    {
        // Get the size requirements for the scratch and AS buffers.
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = {};
        bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
        bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        bottomLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
        //bottomLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

        switch (blasIndex)
        {
        case DynamicBLAS::dynamicDove:
            //bottomLevelInputs.pGeometryDescs = &sceneGeometryDesc[SceneMain::SceneGeometries::sceneDove];
            bottomLevelInputs.pGeometryDescs = &m_geometryDesc[SceneGeometries::sceneDove];
            bottomLevelInputs.NumDescs = 1;
            break;
        default:
            break;
        }

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
        device->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
        DX::ThrowIfFalse(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

        // Create a scratch buffer.
        //const auto blasDynamic = m_mainScene->GetBLASBuffers(BLASType::Dynamic);
        //auto& blas = blasDynamic[blasIndex];
        auto& blas = m_blasBuffers[BLASType::Dynamic][blasIndex];

        if (!isUpdate)
        {
            AllocateUAVBufferWithLayout(device, bottomLevelPrebuildInfo.ScratchDataSizeInBytes, &blas.scratch, L"ScratchResource");
            /*
            AllocateUAVBuffer(
                device,
                bottomLevelPrebuildInfo.ScratchDataSizeInBytes,
                &blas.scratch,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                L"ScratchResource"
            );
            */
            // Allocate resources for acceleration structures.
            // Acceleration structures can only be placed in resources that are created in the default heap (or custom heap equivalent). 
            // Default heap is OK since the application doesn’t need CPU read/write access to them. 
            // The resources that will contain acceleration structures must be created in the state D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, 
            // and must have resource flag D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS. The ALLOW_UNORDERED_ACCESS requirement simply acknowledges both: 
            //  - the system will be doing this type of access in its implementation of acceleration structure builds behind the scenes.
            //  - from the app point of view, synchronization of writes/reads to acceleration structures is accomplished using UAV barriers.

            AllocateUAVBufferWithLayout(device, bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, &blas.accelerationStructure,
                L"DynamicBLAS", true);
            /*
            AllocateUAVBuffer(
                device,
                bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes,
                &blas.accelerationStructure,
                D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
                L"BottomLevelAccelerationStructure"
            );
            */
        }

        // Bottom-level AS desc.
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
        bottomLevelBuildDesc.Inputs = bottomLevelInputs;
        bottomLevelBuildDesc.ScratchAccelerationStructureData = blas.scratch->GetGPUVirtualAddress();
        bottomLevelBuildDesc.DestAccelerationStructureData = blas.accelerationStructure->GetGPUVirtualAddress();

        // Build the acceleration structure.
        commandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);

        //uavBarriers[blasIndex] = CD3DX12_RESOURCE_BARRIER::UAV(blas.accelerationStructure.Get());
    }

    // Batch all resource barriers for BLAS builds.
    //commandList->ResourceBarrier(DynamicBLAS::dynamicCount, uavBarriers);
}

//template <class InstanceDescType, class BLASPtrType>
void SceneMain::BuildTLASInstanceDescs()
//void SceneMain::BuildTLASInstanceDescs(D3D12_RAYTRACING_INSTANCE_DESC* tlasInstanceDesc)
//void Game::BuildBottomLevelASInstanceDescs(BLASPtrType* bottomLevelASaddresses, ComPtr<ID3D12Resource>* instanceDescsResource)
//void Game::BuildBottomLevelASInstanceDescs(std::vector<D3D12_GPU_VIRTUAL_ADDRESS>& bottomLevelASaddresses)
//void Game::BuildBottomLevelASInstanceDescs(std::vector<D3D12_GPU_VIRTUAL_ADDRESS>& bottomLevelASaddresses, ID3D12Resource** ppInstanceDescsResource)
//void Game::BuildBottomLevelASInstanceDescs(std::vector<D3D12_GPU_VIRTUAL_ADDRESS>& bottomLevelASaddresses, ComPtr<ID3D12Resource>* instanceDescsResource)
//void Game::BuildBottomLevelASInstanceDescs(D3D12_GPU_VIRTUAL_ADDRESS* bottomLevelASaddresses, ComPtr<ID3D12Resource>* instanceDescsResource)
{
    //auto device = m_deviceResources->GetD3DDevice();
    //auto commandQueue = m_deviceResources->GetCommandQueue();
    //ComPtr<ID3D12Resource> temp;
    //D3D12_RAYTRACING_INSTANCE_DESC* instanceDescs{};
    //std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
    //std::vector<InstanceDescType> instanceDescs;

    //ZeroMemory(instanceDescs, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * 4u);
    //instanceDescs.resize(4); // 3 triangle instances + 1 ground plane
    //instanceDescs.resize((UINT64)m_geometryInstanceCount + 1); // 3 triangle instances + 1 ground plane
    //instanceDescs.resize(GeometryType::Count);
    //instanceDescs.resize(NUM_BLAS);

    // Width of a bottom-level AS geometry.
    // Make the plane a little larger than the actual number of primitives in each dimension.
    //const XMUINT3 NUM_AABB = XMUINT3(700, 1, 700);
    //const XMFLOAT3 fWidth = XMFLOAT3(
    //    NUM_AABB.x * c_aabbWidth + (NUM_AABB.x - 1) * c_aabbDistance,
    //    NUM_AABB.y * c_aabbWidth + (NUM_AABB.y - 1) * c_aabbDistance,
    //    NUM_AABB.z * c_aabbWidth + (NUM_AABB.z - 1) * c_aabbDistance);
    //const XMVECTOR vWidth = XMLoadFloat3(&fWidth);

    // Bottom-level AS with instanced triangle polygon.
    //for (UINT i = 0; i < CUBE_INSTANCE_COUNT; i++)
    //const auto cubeTransforms = m_mainScene->GetCubeTransforms(); // temp
    //const auto blasStatic = m_mainScene->GetBLASBuffers(BLASType::Static);
    //const auto blasDynamic = m_mainScene->GetBLASBuffers(BLASType::Dynamic);
    //const auto tlasInstanceDesc = GetTLASInstanceDesc();
    //const auto tlasInstanceDesc = m_mainScene->GetTLASInstanceDesc();

    auto sdkMeshModel = m_game->GetSdkMeshModel(SDKMESHModels::Suzanne); // SdkMesh model pointer will be updated where necessary.
    auto fbxModel = m_game->GetFbxModel(FBXModels::Dove);

    for (uint32_t instanceIndex = 0; instanceIndex < TLASInstances::tlasCount; instanceIndex++)
    {
        auto& instanceDesc = m_tlasInstanceDesc[instanceIndex];
        //auto& instanceDesc = m_instanceDescMappedData[instanceIndex];
        instanceDesc = {};
        // Instance flags, including backface culling, winding, etc - TODO: should be accessible from outside.
        // Assign the right instance lookup indexes, acceleration structures and transforms to each instanceDesc.
        switch (instanceIndex)
        {
        case TLASInstances::tlasRedCube:
            instanceDesc.InstanceID = ShaderInstances::instRedCube; // InstanceID is visible in the shader as InstanceID()
            instanceDesc.InstanceMask = 1;//0xFF
            instanceDesc.InstanceContributionToHitGroupIndex = MeshType::Cube * RayType::Count; // Index offset of the hit group invoked upon intersection.
            //instanceDesc.AccelerationStructure = blasStatic[SceneMain::StaticBLAS::staticCube].accelerationStructure->GetGPUVirtualAddress();
            instanceDesc.AccelerationStructure = m_blasBuffers[BLASType::Static][StaticBLAS::staticCube].accelerationStructure->GetGPUVirtualAddress();
            //memcpy(instanceDesc.Transform, &cubeTransforms[ShaderInstances::RedCube], sizeof(instanceDesc.Transform)); // This works!
            memcpy(instanceDesc.Transform, &m_cubeTransforms3x4[ShaderInstances::instRedCube], sizeof(instanceDesc.Transform)); // This works!
            instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE; // Converts raytracing world coords to RHS.
            break;
        case TLASInstances::tlasGreenCube:
            instanceDesc.InstanceID = ShaderInstances::instGreenCube; // InstanceID is visible in the shader as InstanceID()
            instanceDesc.InstanceMask = 1;//0xFF
            instanceDesc.InstanceContributionToHitGroupIndex = MeshType::Cube * RayType::Count; // Index offset of the hit group invoked upon intersection.
            //instanceDesc.AccelerationStructure = blasStatic[SceneMain::StaticBLAS::staticCube].accelerationStructure->GetGPUVirtualAddress();
            instanceDesc.AccelerationStructure = m_blasBuffers[BLASType::Static][StaticBLAS::staticCube].accelerationStructure->GetGPUVirtualAddress();
            //memcpy(instanceDesc.Transform, &cubeTransforms[ShaderInstances::GreenCube], sizeof(instanceDesc.Transform)); // This works!
            memcpy(instanceDesc.Transform, &m_cubeTransforms3x4[ShaderInstances::instGreenCube], sizeof(instanceDesc.Transform)); // This works!
            instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE; // Converts raytracing world coords to RHS.
            break;
        case TLASInstances::tlasBlueCube:
            instanceDesc.InstanceID = ShaderInstances::instBlueCube; // InstanceID is visible in the shader as InstanceID()
            instanceDesc.InstanceMask = 1;//0xFF
            instanceDesc.InstanceContributionToHitGroupIndex = MeshType::Cube * RayType::Count; // Index offset of the hit group invoked upon intersection.
            //instanceDesc.AccelerationStructure = blasStatic[SceneMain::StaticBLAS::staticCube].accelerationStructure->GetGPUVirtualAddress();
            instanceDesc.AccelerationStructure = m_blasBuffers[BLASType::Static][StaticBLAS::staticCube].accelerationStructure->GetGPUVirtualAddress();
            //memcpy(instanceDesc.Transform, &cubeTransforms[ShaderInstances::BlueCube], sizeof(instanceDesc.Transform)); // This works!
            memcpy(instanceDesc.Transform, &m_cubeTransforms3x4[ShaderInstances::instBlueCube], sizeof(instanceDesc.Transform)); // This works!
            instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE; // Converts raytracing world coords to RHS.
            break;
        case TLASInstances::tlasSuzanne:
            instanceDesc.InstanceID = ShaderInstances::instSuzanne; // InstanceID is visible in the shader as InstanceID()
            instanceDesc.InstanceMask = 1;//0xFF
            instanceDesc.InstanceContributionToHitGroupIndex = MeshType::Opaque * RayType::Count; // Index offset of the hit group invoked upon intersection.
            //instanceDesc.AccelerationStructure = blasStatic[SceneMain::StaticBLAS::staticSuzanne].accelerationStructure->GetGPUVirtualAddress();
            instanceDesc.AccelerationStructure = m_blasBuffers[BLASType::Static][StaticBLAS::staticSuzanne].accelerationStructure->GetGPUVirtualAddress();
            XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(instanceDesc.Transform), sdkMeshModel->GetWorld());
            //XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(instanceDesc.Transform), m_SDKMESHModel[SDKMESHModels::Suzanne]->GetWorld());
            instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE; // Converts raytracing world coords to RHS.
            break;
        case TLASInstances::tlasRacetrack:
            sdkMeshModel = m_game->GetSdkMeshModel(SDKMESHModels::Racetrack);
            instanceDesc.InstanceID = ShaderInstances::instRacetrack; // InstanceID is visible in the shader as InstanceID()
            instanceDesc.InstanceMask = 1;//0xFF
            instanceDesc.InstanceContributionToHitGroupIndex = MeshType::Opaque * RayType::Count; // Index offset of the hit group invoked upon intersection.
            //instanceDesc.AccelerationStructure = blasStatic[SceneMain::StaticBLAS::staticRacetrack].accelerationStructure->GetGPUVirtualAddress();
            instanceDesc.AccelerationStructure = m_blasBuffers[BLASType::Static][StaticBLAS::staticRacetrack].accelerationStructure->GetGPUVirtualAddress();
            XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(instanceDesc.Transform), sdkMeshModel->GetWorld());
            //XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(instanceDesc.Transform), m_SDKMESHModel[SDKMESHModels::Racetrack]->GetWorld());
            instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE; // Converts raytracing world coords to RHS.
            break;
            //case MeshInstances::RacetrackGrass:
            //    //instanceDesc.InstanceContributionToHitGroupIndex = MeshType::Opaque; // Index of the hit group invoked upon intersection.
            //    instanceDesc.InstanceContributionToHitGroupIndex = MeshType::Opaque * RayType::Count; // Index of the hit group invoked upon intersection.
            //    instanceDesc.AccelerationStructure = m_blasBuffers[MeshGeometries::RacetrackGrass].accelerationStructure->GetGPUVirtualAddress();
            //    //XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(instanceDesc.Transform), m_racetrack_grass->GetWorld());
            //    XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(instanceDesc.Transform), m_SDKMESHModel[SDKMESHModels::Racetrack]->GetWorld());
            //    break;
        case TLASInstances::tlasPalmtree:
            sdkMeshModel = m_game->GetSdkMeshModel(SDKMESHModels::Palmtree);
            instanceDesc.InstanceID = ShaderInstances::instPalmtree; // InstanceID is visible in the shader as InstanceID()
            instanceDesc.InstanceMask = 1;//0xFF
            instanceDesc.InstanceContributionToHitGroupIndex = MeshType::Transparent * RayType::Count; // Index offset of the hit group invoked upon intersection.
            //instanceDesc.AccelerationStructure = blasStatic[SceneMain::StaticBLAS::staticPalmtree].accelerationStructure->GetGPUVirtualAddress();
            instanceDesc.AccelerationStructure = m_blasBuffers[BLASType::Static][StaticBLAS::staticPalmtree].accelerationStructure->GetGPUVirtualAddress();
            XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(instanceDesc.Transform), sdkMeshModel->GetWorld());
            //XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(instanceDesc.Transform), m_SDKMESHModel[SDKMESHModels::Palmtree]->GetWorld());
            instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE; // Converts raytracing world coords to RHS.
            break;
            //case TLASInstances::PalmtreeFoliage:
            //    instanceDesc.InstanceID = ShaderInstances::PalmtreeFoliage; // InstanceID is visible in the shader as InstanceID()
            //    instanceDesc.InstanceMask = 1;//0xFF
            //    instanceDesc.InstanceContributionToHitGroupIndex = MeshType::Transparent * RayType::Count; // Index offset of the hit group invoked upon intersection.
            //    instanceDesc.AccelerationStructure = m_blasBuffers[BLAS::PalmtreeFoliage].accelerationStructure->GetGPUVirtualAddress();
            //    XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(instanceDesc.Transform), m_SDKMESHModel[SDKMESHModels::Palmtree]->GetWorld());
            //    instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE; // Converts raytracing world coords to RHS.
            //    break;
        case TLASInstances::tlasMiniRacecar:
            sdkMeshModel = m_game->GetSdkMeshModel(SDKMESHModels::MiniRacecar);
            instanceDesc.InstanceID = ShaderInstances::instMiniRacecar; // InstanceID is visible in the shader as InstanceID()
            instanceDesc.InstanceMask = 1;//0xFF
            instanceDesc.InstanceContributionToHitGroupIndex = MeshType::Opaque * RayType::Count; // Index offset of the hit group invoked upon intersection.
            //instanceDesc.AccelerationStructure = blasStatic[SceneMain::StaticBLAS::staticMiniRacecar].accelerationStructure->GetGPUVirtualAddress();
            instanceDesc.AccelerationStructure = m_blasBuffers[BLASType::Static][StaticBLAS::staticMiniRacecar].accelerationStructure->GetGPUVirtualAddress();
            XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(instanceDesc.Transform), sdkMeshModel->GetWorld());
            //XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(instanceDesc.Transform), m_SDKMESHModel[SDKMESHModels::MiniRacecar]->GetWorld());
            instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE; // Converts raytracing world coords to RHS.
            break;
        case TLASInstances::tlasDove:
            instanceDesc.InstanceID = ShaderInstances::instDove; // InstanceID is visible in the shader as InstanceID()
            instanceDesc.InstanceMask = 1;//0xFF
            instanceDesc.InstanceContributionToHitGroupIndex = MeshType::Opaque * RayType::Count; // Index offset of the hit group invoked upon intersection.
            //instanceDesc.AccelerationStructure = blasDynamic[SceneMain::DynamicBLAS::dynamicDove].accelerationStructure->GetGPUVirtualAddress();
            instanceDesc.AccelerationStructure = m_blasBuffers[BLASType::Dynamic][DynamicBLAS::dynamicDove].accelerationStructure->GetGPUVirtualAddress();
            XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(instanceDesc.Transform), fbxModel->GetWorld());
            //XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(instanceDesc.Transform), m_FBXModel[FBXModels::Dove]->GetWorld());
            instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE; // Converts raytracing world coords to RHS.
            break;
        default:
            break;
        }
    }
    //for (uint32_t i = 0; i < Globals::Cube_Instance_Count; i++)
    //{
    //    auto& instanceDesc = m_instanceDescs[i];
    //    //auto& instanceDesc = instanceDescs[GeometryType::Triangle];
    //    instanceDesc = {};
    //    // Instance flags, including backface culling, winding, etc - TODO: should
    //    // be accessible from outside.
    //    instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE;
    //    // Instance ID visible in the shader in InstanceID()
    //    instanceDesc.InstanceID = i;
    //    instanceDesc.InstanceMask = 1;//0xFF
    //    // Index of the hit group invoked upon intersection
    //    instanceDesc.InstanceContributionToHitGroupIndex = i * RayType::Count;
    //    //instanceDesc.InstanceContributionToHitGroupIndex = 0;
    //    instanceDesc.AccelerationStructure = m_blasBuffers[TriangleMeshes::Cube].accelerationStructure->GetGPUVirtualAddress();
    //    //instanceDesc.AccelerationStructure = bottomLevelASaddresses[GeometryType::Triangle];
    //    memcpy(instanceDesc.Transform, &m_geometryTransforms[i], sizeof(instanceDesc.Transform)); // this works!
    //}
    //
    //// Bottom-level AS with a single plane.
    //{
    //    auto& instanceDesc = m_instanceDescs[MeshInstances::Plane];
    //    //auto& instanceDesc = m_instanceDescs[3];
    //    //auto& instanceDesc = instanceDescs[BottomLevelASType::Triangle];
    //    instanceDesc = {};
    //    instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE;
    //    // Instance ID visible in the shader in InstanceID()
    //    instanceDesc.InstanceID = 1;
    //    instanceDesc.InstanceMask = 1;
    //    // Set hit group offset to beyond the shader records for the triangle.
    //    instanceDesc.InstanceContributionToHitGroupIndex = MeshInstances::Plane * RayType::Count;
    //    //instanceDesc.InstanceContributionToHitGroupIndex = 3 * RayType::Count; // plane is associated with the next hit group!
    //    //instanceDesc.InstanceContributionToHitGroupIndex = 1; // plane is associated with the next hit group!
    //    instanceDesc.AccelerationStructure = m_blasBuffers[TriangleMeshes::Plane].accelerationStructure->GetGPUVirtualAddress();
    //    //instanceDesc.AccelerationStructure = bottomLevelASaddresses[GeometryType::Triangle];
    //
    //    // Calculate transformation matrix.
    //    //const XMVECTOR vBasePosition = vWidth * XMLoadFloat3(&XMFLOAT3(-0.35f, 0.0f, -0.35f));
    //
    //    // Scale in XZ dimensions.
    //    //XMMATRIX mScale = XMMatrixScaling(fWidth.x, fWidth.y, fWidth.z);
    //    //XMMATRIX mTranslation = XMMatrixTranslationFromVector(vBasePosition);
    //    //XMMATRIX mTransform = mScale * mTranslation;
    //    auto transform = Matrix::Identity;
    //    XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(instanceDesc.Transform), transform);
    //}

    // Create instanced bottom-level AS with procedural geometry AABBs.
    // Instances share all the data, except for a transform.
    //{
    //    auto& instanceDesc = instanceDescs[BottomLevelASType::AABB];
    //    instanceDesc = {};
    //    instanceDesc.InstanceMask = 1;
    //
    //    // Set hit group offset to beyond the shader records for the triangle AABB.
    //    instanceDesc.InstanceContributionToHitGroupIndex = BottomLevelASType::AABB * RayType::Count;
    //    instanceDesc.AccelerationStructure = bottomLevelASaddresses[BottomLevelASType::AABB];
    //
    //    // Move all AABBS above the ground plane.
    //    XMMATRIX mTranslation = XMMatrixTranslationFromVector(XMLoadFloat3(&XMFLOAT3(0, c_aabbWidth / 2, 0)));
    //    XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(instanceDesc.Transform), mTranslation);
    //}

    //m_tlasBuffers.instanceDesc->Unmap(0, nullptr);
    //else
    //{
    //    UINT64 bufferSize = static_cast<UINT64>(4u * sizeof(instanceDescs[0]));
    //    AllocateUploadBuffer(device, instanceDescs, bufferSize, &m_tlasBuffers.instanceDesc, L"InstanceDescs");
    //    //UINT64 bufferSize = static_cast<UINT64>(instanceDescs.size() * sizeof(instanceDescs[0]));
    //    //AllocateUploadBuffer(device, instanceDescs.data(), bufferSize, &m_tlasBuffers.instanceDesc, L"InstanceDescs");
    //}

    //AllocateUploadBuffer(device, instanceDescs.data(), bufferSize, &temp, L"InstanceDescs");
    //AllocateUploadBuffer(device, instanceDescs.data(), bufferSize, ppInstanceDescsResource, L"InstanceDescs");
    //AllocateUploadBuffer(device, instanceDescs.data(), bufferSize, &(*ppInstanceDescsResource), L"InstanceDescs");
    //m_tlasBuffers.instanceDesc = temp;
    //ResourceUploadBatch resourceUpload(device);
    //resourceUpload.Begin();
    //
    //ThrowIfFailed(CreateStaticBuffer(device, resourceUpload, instanceDescs, D3D12_RESOURCE_STATE_GENERIC_READ,
    //    ppInstanceDescsResource)); // must be a pointer to pointer
    ////(*instanceDescsResource).ReleaseAndGetAddressOf())); // must be a pointer to pointer
    //
    //auto uploadResourcesFinished(resourceUpload.End(commandQueue));
    //uploadResourcesFinished.wait();
};

void SceneMain::CreateInstanceBuffer(ID3D12Device* device, ID3D12CommandQueue* commandQueue)
{
    // Structured buffer data which we will use to replace cbuffers in raytracing shaders.
    const std::vector<MeshGeometryData> geometryData =
    {
        {   // 0: Cube
            SrvUAVs::CubeVertexBufferSrv,             SrvUAVs::CubeIndexBufferSrv, false,
        },
        {   // 1: Suzanne
            SrvUAVs::SuzanneVertexBufferSrv,          SrvUAVs::SuzanneIndexBufferSrv, true,
            //m_SDKMESHModel[SDKMESHModels::Suzanne]->GetIndexFormat(0,0) == DXGI_FORMAT_R16_UINT ? true : false,
        },
        {  // 2: Racetrack road
            SrvUAVs::RacetrackRoadVertexBufferSrv,    SrvUAVs::RacetrackRoadIndexBufferSrv, true,
            //m_SDKMESHModel[SDKMESHModels::Racetrack]->GetIndexFormat(0,0) == DXGI_FORMAT_R16_UINT ? true : false,
        },
        {  // 3: Racetrack skirt
            SrvUAVs::RacetrackSkirtVertexBufferSrv,   SrvUAVs::RacetrackSkirtIndexBufferSrv, true,
            //m_SDKMESHModel[SDKMESHModels::Racetrack]->GetIndexFormat(1,0) == DXGI_FORMAT_R16_UINT ? true : false,
        },
        {  // 4: Racetrack map
            SrvUAVs::RacetrackMapVertexBufferSrv,     SrvUAVs::RacetrackMapIndexBufferSrv, true,
            //m_SDKMESHModel[SDKMESHModels::Racetrack]->GetIndexFormat(1,0) == DXGI_FORMAT_R16_UINT ? true : false,
        },
        {   // 5: Palmtree trunk
            SrvUAVs::PalmtreeTrunkVertexBufferSrv,    SrvUAVs::PalmtreeTrunkIndexBufferSrv, true, 
            //m_SDKMESHModel[SDKMESHModels::Palmtree]->GetIndexFormat(0,0) == DXGI_FORMAT_R16_UINT ? true : false,
        },
        {   // 6: Palmtree canopy
            SrvUAVs::PalmtreeCanopyVertexBufferSrv,  SrvUAVs::PalmtreeCanopyIndexBufferSrv, true,
            //m_SDKMESHModel[SDKMESHModels::Palmtree]->GetIndexFormat(1,0) == DXGI_FORMAT_R16_UINT ? true : false,
        },
        {   // 7: Mini racecar
            SrvUAVs::MiniRacecarVertexBufferSrv,      SrvUAVs::MiniRacecarIndexBufferSrv, true,
            //m_SDKMESHModel[SDKMESHModels::MiniRacecar]->GetIndexFormat(0,0) == DXGI_FORMAT_R16_UINT ? true : false,
        },
        {   // 8: Dove
            SrvUAVs::DoveSkinnedVertexBufferUav,      SrvUAVs::DoveIndexBufferSrv, true,
            //m_FBXModel[FBXModels::Dove]->GetIndexFormat(0) == DXGI_FORMAT_R16_UINT ? true : false,
        },
        //{ SrvUAVs::DoveVertexBufferSrv,    SrvUAVs::DoveIndexBufferSrv},    // Dove
    };

    // Structured buffer data which we will use to replace cbuffers in color raytracing
    const std::vector<InstanceData> instanceData =
    {
        {   // 0: Red cube
            geometryData[MeshGeometries::Cube],// SrvUAVs::CubeVertexBufferSrv, SrvUAVs::CubeIndexBufferSrv, true,
            // Material
            Vector4(1, 0, 0, 1), Vector4(0, 0, 0, 1), Vector4(1, 0, 0, 0),
            SrvUAVs::CubeAlbedoSrv, SrvUAVs::CubeNormalSrv, SrvUAVs::CubeEmissiveSrv, SrvUAVs::CubeRMASrv, 1.f,
            //SrvUAVs::DiffuseIBLSrv, SrvUAVs::SpecularIBLSrv,
            //Vector3(1, 0, 0), 0, 0, 1.f, 0.4f, 50.f
        },
        {   // 1: Green cube
            geometryData[MeshGeometries::Cube],// SrvUAVs::CubeVertexBufferSrv, SrvUAVs::CubeIndexBufferSrv, true,
            // Material
            Vector4(0, 1, 0, 1), Vector4(0, 0, 0, 1), Vector4(1, 0, 0, 0),
            SrvUAVs::CubeAlbedoSrv, SrvUAVs::CubeNormalSrv, SrvUAVs::CubeEmissiveSrv, SrvUAVs::CubeRMASrv, 1.f,
            //SrvUAVs::DiffuseIBLSrv, SrvUAVs::SpecularIBLSrv,
            //Vector3(0, 1, 0), 0, 0, 1.f, 0.4f, 50.f
        },
        {   // 2: Blue cube
            geometryData[MeshGeometries::Cube],// SrvUAVs::CubeVertexBufferSrv, SrvUAVs::CubeIndexBufferSrv, true,
            // Material
            Vector4(0, 0, 1, 1), Vector4(0, 0, 0, 1), Vector4(1, 0, 0, 0),
            SrvUAVs::CubeAlbedoSrv, SrvUAVs::CubeNormalSrv, SrvUAVs::CubeEmissiveSrv, SrvUAVs::CubeRMASrv, 1.f,
            //SrvUAVs::DiffuseIBLSrv, SrvUAVs::SpecularIBLSrv,
            //Vector3(0, 0, 1), 0, 0, 1.f, 0.4f, 50.f
        },
        {   // 3: Suzanne
            geometryData[MeshGeometries::Suzanne],// SrvUAVs::SuzanneVertexBufferSrv, SrvUAVs::SuzanneIndexBufferSrv, m_suzanne->IndexSize16(0,0),
            // Material
            Vector4(1, 0, 1, 1), Vector4(0, 0, 0, 1), Vector4(1, 0, 0, 0),
            SrvUAVs::SuzanneAlbedoSrv, SrvUAVs::CubeNormalSrv, SrvUAVs::CubeEmissiveSrv, SrvUAVs::CubeRMASrv, 1.f,
            //SrvUAVs::DiffuseIBLSrv, SrvUAVs::SpecularIBLSrv,
            //Vector3::One, 0, 0.25f, 1.f, 0.4f, 50.f
        },
        {   // 4: Racetrack road
            geometryData[MeshGeometries::RacetrackRoad],// SrvUAVs::RacetrackRoadVertexBufferSrv, SrvUAVs::RacetrackRoadIndexBufferSrv, m_racetrack->IndexSize16(0,0),
            //SrvUAVs::RacetrackRoadVertexBufferSrv, SrvUAVs::RacetrackRoadIndexBufferSrv, m_racetrack_road->IndexSize16(0,0),
            //SrvUAVs::RacetrackRoadVertexBufferSrv, SrvUAVs::RacetrackRoadIndexBufferSrv, m_racetrack->IndexSize16(0,1),
            // Material
            Vector4(1, 1, 0, 1), Vector4(0, 0, 0, 1), Vector4(1, 0, 0, 0),
            SrvUAVs::RacetrackRoadAlbedoSrv, SrvUAVs::RacetrackRoadNormalSrv, 0, SrvUAVs::RacetrackRoadRMASrv, 1.f,
            //SrvUAVs::DiffuseIBLSrv, SrvUAVs::SpecularIBLSrv,
            //Vector3::One, 0, 0.25f, 1.f, 0.4f, 50.f
        },
        {   // 5: Racetrack skirt
            geometryData[MeshGeometries::RacetrackSkirt],// SrvUAVs::RacetrackGrassVertexBufferSrv, SrvUAVs::RacetrackGrassIndexBufferSrv, m_racetrack->IndexSize16(1,0),
            //SrvUAVs::RacetrackGrassVertexBufferSrv, SrvUAVs::RacetrackGrassIndexBufferSrv, m_racetrack_grass->IndexSize16(0,0),
            //SrvUAVs::RacetrackGrassVertexBufferSrv, SrvUAVs::RacetrackGrassIndexBufferSrv, m_racetrack->IndexSize16(0,0),
            // Material
            Vector4(0, 1, 1, 1), Vector4(0, 0, 0, 1), Vector4(1, 0, 0, 0),
            SrvUAVs::RacetrackSkirtAlbedoSrv, SrvUAVs::RacetrackSkirtNormalSrv, 0, SrvUAVs::RacetrackSkirtRMASrv, 10.f,
            //SrvUAVs::DiffuseIBLSrv, SrvUAVs::SpecularIBLSrv,
            //Vector3::One, 0, 0.25f, 1.f, 0.4f, 50.f
        },
        {   // 6: Racetrack map
            geometryData[MeshGeometries::RacetrackMap],// SrvUAVs::RacetrackGrassVertexBufferSrv, SrvUAVs::RacetrackGrassIndexBufferSrv, m_racetrack->IndexSize16(1,0),
            //SrvUAVs::RacetrackGrassVertexBufferSrv, SrvUAVs::RacetrackGrassIndexBufferSrv, m_racetrack_grass->IndexSize16(0,0),
            //SrvUAVs::RacetrackGrassVertexBufferSrv, SrvUAVs::RacetrackGrassIndexBufferSrv, m_racetrack->IndexSize16(0,0),
            // Material
            Vector4(0, 1, 1, 1), Vector4(0, 0, 0, 1), Vector4(1, 0, 0, 0),
            SrvUAVs::RacetrackMapAlbedoSrv, SrvUAVs::DefaultNormalSrv, 0, SrvUAVs::DefaultRMASrv, 1.f,
            //SrvUAVs::DiffuseIBLSrv, SrvUAVs::SpecularIBLSrv,
            //Vector3::One, 0, 0.25f, 1.f, 0.4f, 50.f
        },
        {   // 7: Palmtree trunk
            geometryData[MeshGeometries::PalmtreeTrunk],
            // Material
            Vector4(0.25f, 0.25f, 0.25f, 1), Vector4(0, 0, 0, 1), Vector4(1, 0, 0, 0),
            SrvUAVs::PalmtreeAlbedoSrv, SrvUAVs::PalmtreeNormalSrv, 0, SrvUAVs::PalmtreeRMASrv, 1.f,
            //SrvUAVs::DiffuseIBLSrv, SrvUAVs::SpecularIBLSrv,
        },
        {   // 8: Palmtree canopy
            geometryData[MeshGeometries::PalmtreeCanopy],
            // Material
            Vector4(0, 0, 1, 1), Vector4(0, 0, 0, 1), Vector4(1, 0, 0, 0),
            SrvUAVs::PalmtreeAlbedoSrv, SrvUAVs::PalmtreeNormalSrv, 0, SrvUAVs::PalmtreeRMASrv, 1.f,
            //SrvUAVs::DiffuseIBLSrv, SrvUAVs::SpecularIBLSrv,
        },
        {   // 9: Mini racecar
            geometryData[MeshGeometries::MiniRacecar],
            // Material
            Vector4(1, 0, 0, 1), Vector4(0, 0, 0, 1), Vector4(1, 0, 0, 0),
            SrvUAVs::MiniRacecarAlbedoSrv, SrvUAVs::DefaultNormalSrv, 0, SrvUAVs::MiniRacecarRMASrv, 1.f,
            //SrvUAVs::DiffuseIBLSrv, SrvUAVs::SpecularIBLSrv,
        },
        {   // 10: Dove
            geometryData[MeshGeometries::Dove],// SrvUAVs::DoveSkinnedVertexBufferUav,    SrvUAVs::DoveIndexBufferSrv, true, // Fbx index format is only r16.
            //SrvUAVs::DoveVertexBufferSrv,    SrvUAVs::DoveIndexBufferSrv,
            // Material
            Vector4(0, 1, 0, 1), Vector4(0, 0, 0, 1), Vector4(1, 0, 0, 0),
            SrvUAVs::DoveAlbedoSrv, SrvUAVs::DoveNormalSrv, SrvUAVs::CubeEmissiveSrv, SrvUAVs::CubeRMASrv, 1.f,
            //SrvUAVs::DiffuseIBLSrv, SrvUAVs::SpecularIBLSrv,
            //Vector3::One, 0, 0, 1.f, 0, 50.f
        },
    };

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    DX::ThrowIfFailed(CreateStaticBuffer(
        device,
        resourceUpload,
        instanceData,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        &m_instanceStructBuffer
        //&m_structBuffer[StructBuffers::InstanceData]
    ));

    auto uploadResourcesFinished = resourceUpload.End(commandQueue);
    uploadResourcesFinished.wait();

    auto descHeap = m_game->GetDescriptorHeap(DescriptorHeaps::SrvUav);
    CreateBufferShaderResourceView(
        device,
        m_instanceStructBuffer.Get(),
        //m_structBuffer[StructBuffers::InstanceData].Get(),
        descHeap->GetCpuHandle(SrvUAVs::InstanceBufferSrv),
        //m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::InstanceBufferSrv),
        sizeof(InstanceData)
    );
}
