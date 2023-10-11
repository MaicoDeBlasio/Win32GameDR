#include "pch.h"
#include "Game.h"
#include "Scene.h"

Scene::Scene() noexcept
//Scene::Scene(Game* game) noexcept
{
    //m_game = game;

    m_camera          = std::make_unique<Camera>();
    m_frameConstants  = std::make_unique<FrameConstants>();

}

// Frank Luna frame stats for window title bar
void Scene::CalculateFrameStats()
//void Scene::CalculateFrameStats(DX::StepTimer const& timer)
{
    // Code computes the average frames per second, and also the average time it takes to render one frame.
    // These stats are appended to the window caption bar.
    const auto deviceResources = m_game->GetDeviceResources();
    const auto timer = m_game->GetTimer();

    static uint32_t frameCnt = 0;
    static float timeElapsed = 0;

    frameCnt++;

    // Compute averages over one second period.
    if (static_cast<float>(timer->GetTotalSeconds()) - timeElapsed >= 1.f)
    {
        //auto fps = static_cast<float>(frameCnt); // fps = frameCnt / 1
        //float fps = mTimer.GetFramesPerSecond();
        const auto mspf = 1000.f / static_cast<float>(frameCnt);
        //auto mspf = 1000.f / fps;

        const auto fpsStr = std::to_wstring(frameCnt);
        //std::wstring fpsStr = std::to_wstring(fps);
        const auto mspfStr = std::to_wstring(mspf);

        std::wstring windowText = L"Frame rate:" //mMainWndCaption +
            L"    fps: " + fpsStr +
            L"   mspf: " + mspfStr;

        SetWindowText(deviceResources->GetWindow(), windowText.c_str());

        // Reset for next average.
        frameCnt = 0;
        timeElapsed += 1.f;
    }
}
