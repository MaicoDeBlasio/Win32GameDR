#pragma once

class Game;

class Scene
{
public:

    Scene() noexcept;
    //Scene(Game* game) noexcept;
    
    Scene(Scene const&) = delete;
    Scene& operator= (Scene const&) = delete;
    
    Scene(Scene&&) = default;
    Scene& operator= (Scene&&) = default;
    
    virtual ~Scene() = default;

protected:

    std::unique_ptr<Camera> m_camera;                   // Scene camera.

    std::unique_ptr<FrameConstants> m_frameConstants;   // Scene frame constants.

    Game* m_game;

    bool m_isRaster;        // Rasterisation pipeline flag.
    bool m_isFirstFrame;    // First rendering frame flag.

private:

    virtual void Initialize() = 0;

public:

    virtual void Update()     = 0;
    virtual void Render()     = 0;

    const auto   GetRasterFlag()     const noexcept { return m_isRaster; };
    const auto   GetCamera()         const noexcept { return m_camera.get(); };
    const auto   GetFrameConstants() const noexcept { return m_frameConstants.get(); };

    void CalculateFrameStats();
};
