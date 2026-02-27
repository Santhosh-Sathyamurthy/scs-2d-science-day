#include "../include/demo_application.h"

#include "../include/blob_game_demo.h"
#include "../include/complex_roller_demo.h"
#include "../include/double_pendulum_demo.h"
#include "../include/energy_transfer_demo.h"
#include "../include/motor_demo.h"
#include "../include/rolling_demo.h"
#include "../include/spring_cloth_demo.h"
#include "../include/spring_double_pendulum_demo_Eulerian.h"
#include "../include/spring_double_pendulum_demo_RK6.h"
#include "../include/spring_mass_demo.h"
#include "../include/triple_pendulum_demo.h"


#include <cmath>
#include <sstream>

DemoApplication::DemoApplication() {
    m_cameraTarget = ysMath::Constants::Zero;
    m_cameraPosition = ysMath::LoadVector(0.0f, 0.0f, 5.0f);
    m_cameraUp = ysMath::Constants::YAxis;

    m_assetPath = L"";

    m_geometryVertexBuffer = nullptr;
    m_geometryIndexBuffer = nullptr;

    m_displayHeight = 10.0f;
    m_paused = true;
    m_showingStats = true;
    m_activeDemo = 0;

    // Menu state
    m_showingMenu = false;
    m_menuSelection = 0;

    m_background = ysColor::srgbiToLinear(0xFFFFFF);
    m_foreground = ysColor::srgbiToLinear(0xFFFFFF);
    m_shadow = ysColor::srgbiToLinear(0x0E1012);
    m_highlight1 = ysColor::srgbiToLinear(0xEF4545);
    m_highlight2 = ysColor::srgbiToLinear(0xFFFFFF);

    m_blobBackground = nullptr;
    m_blobForeground = nullptr;
    m_blobFace = nullptr;

    m_uiScale = 1.0f;

    m_recording = false;
    m_screenResolutionIndex = 0;
    for (int i = 0; i < ScreenResolutionHistoryLength; ++i) {
        m_screenResolution[i][0] = m_screenResolution[i][1] = 0;
    }
}

DemoApplication::~DemoApplication() { /* void */
}

void DemoApplication::initialize(void *instance,
                                 ysContextObject::DeviceAPI api) {
    dbasic::Path modulePath = dbasic::GetModulePath();
    dbasic::Path confPath = modulePath.Append(L"delta.conf");

    std::wstring enginePath =
            L"../dependencies/submodules/delta-studio/engines/basic";
    m_assetPath = L"../assets/";
    if (confPath.Exists()) {
        std::wfstream confFile(confPath.ToString(), std::ios::in);

        std::getline(confFile, enginePath);
        std::getline(confFile, m_assetPath);
        enginePath = modulePath.Append(enginePath).ToString();
        m_assetPath = modulePath.Append(m_assetPath).ToString();

        confFile.close();
    }

    m_engine.GetConsole()->SetDefaultFontDirectory(enginePath + L"/fonts/");

    const std::wstring compiledShaderDirectory = m_assetPath + L"/shaders/";
    const std::wstring shaderPath = (m_assetPath + L"../shaders/").c_str();

    dbasic::DeltaEngine::GameEngineSettings settings;
    settings.API = api;
    settings.DepthBuffer = false;
    settings.Instance = instance;
    settings.ShaderDirectory = shaderPath.c_str();
    settings.ShaderCompiledDirectory = compiledShaderDirectory.c_str();
    settings.WindowTitle = L"Simple 2D Constraint Solver Demo | AngeTheGreat";
    settings.WindowPositionX = 0;
    settings.WindowPositionY = 0;
    settings.WindowStyle = ysWindow::WindowStyle::Windowed;
#if _DEBUG
    settings.CompileShaders = true;
#else
    settings.CompileShaders = false;
#endif

    m_engine.CreateGameWindow(settings);

    m_engine.InitializeShaderSet(&m_shaderSet);
    m_engine.InitializeDefaultShaders(&m_shaders, &m_shaderSet);
    m_engine.InitializeConsoleShaders(&m_shaderSet);
    m_engine.SetShaderSet(&m_shaderSet);

    m_assetManager.SetEngine(&m_engine);
#if _DEBUG
    m_assetManager.CompileInterchangeFile(
            (m_assetPath + L"../art/" + L"assets").c_str(), 1.0f, true);
#endif
    m_assetManager.LoadSceneFile((m_assetPath + L"assets").c_str());

    m_shaders.SetCameraMode(dbasic::DefaultShaders::CameraMode::Target);

    m_engine.GetDevice()->CreateIndexBuffer(
            &m_geometryIndexBuffer, sizeof(unsigned short) * 100000, nullptr);
    m_engine.GetDevice()->CreateVertexBuffer(
            &m_geometryVertexBuffer, sizeof(dbasic::Vertex) * 50000, nullptr);

    m_geometryGenerator.initialize(50000, 100000);

    m_logo = m_assetManager.GetModelAsset("ATG_logo");
    m_logoBackground = m_assetManager.GetModelAsset("ATG_logo_background");

    m_blobForeground = m_assetManager.GetModelAsset("Blob_foreground");
    m_blobBackground = m_assetManager.GetModelAsset("Blob_shadow");
    m_blobFace = m_assetManager.GetModelAsset("Blob_face");

    m_activeDemo = 7;
    m_menuSelection = m_activeDemo;

    addDemo(new SpringDoublePendulumDemo_Eulerian);
    addDemo(new SpringDoublePendulumDemo_RK6);
    addDemo(new BlobGameDemo);
    addDemo(new SpringMassDemo);
    addDemo(new SpringClothDemo);
    addDemo(new EnergyTransferDemo);
    addDemo(new RollingDemo);
    addDemo(new DoublePendulumDemo);
    addDemo(new TriplePendulumDemo);
    addDemo(new ComplexRollerDemo);
    addDemo(new MotorDemo);

    m_textRenderer.SetEngine(&m_engine);
    m_textRenderer.SetRenderer(m_engine.GetUiRenderer());
    m_textRenderer.SetFont(m_engine.GetConsole()->GetFont());
}

void DemoApplication::run() {
    while (true) {
        m_engine.StartFrame();
        if (!m_engine.IsOpen()) break;

        if (m_engine.KeyDownEvent(ysKey::Code::Escape)) {
            if (m_showingMenu) {
                // Close menu first, don't quit
                m_showingMenu = false;
            } else {
                break;
            }
        }

        m_displayHeight = 15.0f;
        m_uiScale = (10.0f / (m_displayHeight + 1.0f));

        updateScreenSizeStability();

        // Intercept toolbar key shortcuts [1]-[4] here so demo.cpp's old
        // 5-button mapping ([3]=FIX,[4]=ROD,[5]=DELETE) doesn't conflict.
        if (!m_showingMenu && !m_demos.empty()) {
            bool keyHandled = false;
            if (m_engine.KeyDownEvent(ysKey::Code::N1)) {
                m_demos[m_activeDemo]->setInteractionMode(InteractionMode::SelectDrag);
                m_paused = false; keyHandled = true;
            } else if (m_engine.KeyDownEvent(ysKey::Code::N2)) {
                m_demos[m_activeDemo]->setInteractionMode(InteractionMode::AddSpring);
                keyHandled = true;
            } else if (m_engine.KeyDownEvent(ysKey::Code::N3)) {
                m_demos[m_activeDemo]->setInteractionMode(InteractionMode::AddRod);
                keyHandled = true;
            } else if (m_engine.KeyDownEvent(ysKey::Code::N4)) {
                m_demos[m_activeDemo]->setInteractionMode(InteractionMode::Delete);
                m_paused = false; keyHandled = true;
            }
            (void)keyHandled;
        }

        // Check if mouse is over toolbar — position only, no event consumption.
        // If the user clicks on a toolbar button, we handle the mode switch and
        // suppress processInput so the demo doesn't also react to the same click.
        bool toolbarConsumedClick = false;
        if (!m_showingMenu) {
            int rawMx = 0, rawMy = 0;
            m_engine.GetMousePos(&rawMx, &rawMy);
            const bool mouseOverToolbar = isMouseOverToolbar((float)rawMx, (float)rawMy);
            if (mouseOverToolbar && m_engine.ProcessMouseButtonDown(ysMouse::Button::Left)) {
                handleToolbarClick((float)rawMx, (float)rawMy);
                toolbarConsumedClick = true;
            }
        }

        // Only process gameplay keys when menu is not open
        if (!m_showingMenu) {
            if (m_engine.KeyDownEvent(ysKey::Code::Space) &&
                m_engine.GetGameWindow()->IsActive()) {
                m_paused = !m_paused;
            }

            if (m_engine.KeyDownEvent(ysKey::Code::F)) {
                m_engine.GetGameWindow()->SetWindowStyle(
                        ysWindow::WindowStyle::Fullscreen);
            }

            if (m_engine.KeyDownEvent(ysKey::Code::R)) {
                m_demos[m_activeDemo]->reset();
                m_demos[m_activeDemo]->initialize();
            }

            if (m_engine.KeyDownEvent(ysKey::Code::V) &&
                m_engine.GetGameWindow()->IsActive()) {
                if (!isRecording() && readyToRecord()) {
                    startRecording();
                } else if (isRecording()) {
                    stopRecording();
                }
            }

            if (isRecording() && !readyToRecord()) { stopRecording(); }

            if (!toolbarConsumedClick) {
                m_demos[m_activeDemo]->processInput();
            }

            // Auto-resume when keyboard shortcut switches back to drag/delete mode
            if (!toolbarConsumedClick) {
                const InteractionMode curMode =
                    m_demos[m_activeDemo]->getInteractionMode();
                if (curMode == InteractionMode::SelectDrag ||
                    curMode == InteractionMode::Delete) {
                    // Only un-pause if we were paused due to adding mode
                    // (don't override explicit user pause via Space)
                    // We can't distinguish, so only auto-resume on key [1] or [5]
                    if (m_engine.KeyDownEvent(ysKey::Code::N1) ||
                        m_engine.KeyDownEvent(ysKey::Code::N4)) {
                        m_paused = false;
                    }
                }
            }

            // Always pause in constraint-adding modes to prevent sparse_matrix
            // dimension assertion. User presses Space to resume after switching
            // back to DRAG mode.
            {
                const InteractionMode curMode =
                    m_demos[m_activeDemo]->getInteractionMode();
                if (curMode == InteractionMode::AddSpring    ||
                    curMode == InteractionMode::AddRod       ||
                    curMode == InteractionMode::AddFixedJoint) {
                    m_paused = true;
                }
            }

            if (!m_paused) {
                m_demos[m_activeDemo]->process(1 / 60.0f);
            } else if (m_engine.KeyDownEvent(ysKey::Code::Right)) {
                m_demos[m_activeDemo]->process(1 / 60.0f);
            }

            if (m_engine.KeyDownEvent(ysKey::Code::S)) {
                m_showingStats = !m_showingStats;
            }
        }

        // Tab toggles the demo selection menu
        if (m_engine.KeyDownEvent(ysKey::Code::Tab)) {
            m_showingMenu = !m_showingMenu;
            if (m_showingMenu) {
                m_menuSelection = m_activeDemo;
            }
        }

        // Menu navigation keys
        if (m_showingMenu) {
            if (m_engine.KeyDownEvent(ysKey::Code::Up)) {
                m_menuSelection =
                    (m_menuSelection - 1 + (int)m_demos.size()) % (int)m_demos.size();
            }

            if (m_engine.KeyDownEvent(ysKey::Code::Down)) {
                m_menuSelection =
                    (m_menuSelection + 1) % (int)m_demos.size();
            }

            if (m_engine.KeyDownEvent(ysKey::Code::Return)) {
                m_activeDemo = m_menuSelection;
                m_demos[m_activeDemo]->reset();
                m_demos[m_activeDemo]->initialize();
                m_showingMenu = false;
                m_paused = true;
            }

            // Mouse support
            // These pixel values must match renderMenu() exactly
            const float screenW        = (float)m_engine.GetScreenWidth();
            const float screenH        = (float)m_engine.GetScreenHeight();
            const float menuWidthPx    = 460.0f;
            const float headerHeightPx = 60.0f;
            const float footerHeightPx = 28.0f;
            const float itemHeightPx   = 34.0f;
            const float paddingPx      = 10.0f;
            const float menuHeightPx   = headerHeightPx + footerHeightPx
                                       + itemHeightPx * (float)m_demos.size()
                                       + paddingPx * 2.0f;

            const float menuPxLeft    = (screenW - menuWidthPx)  * 0.5f;
            const float menuPxRight   = menuPxLeft + menuWidthPx;
            const float menuPxBottom  = (screenH + menuHeightPx) * 0.5f;
            const float listStartPxY  = menuPxBottom - headerHeightPx - paddingPx;

            int rawMouseX = 0, rawMouseY = 0;
            m_engine.GetMousePos(&rawMouseX, &rawMouseY);
            const float mouseX = (float)rawMouseX;
            const float mouseY = screenH - (float)rawMouseY;  // flip: GetMousePos Y is from bottom

            // Hover - mouseY is from bottom (GetMousePos), listStartPxY is from bottom
            if (mouseX >= menuPxLeft && mouseX <= menuPxRight) {
                for (int i = 0; i < (int)m_demos.size(); ++i) {
                    const float itemTop    = listStartPxY - itemHeightPx * (float)(i + 1);
                    const float itemBottom = listStartPxY - itemHeightPx * (float)i;
                    if (mouseY >= itemTop && mouseY <= itemBottom) {
                        m_menuSelection = i;
                        break;
                    }
                }
            }

            // Click
            if (m_engine.ProcessMouseButtonDown(ysMouse::Button::Left) &&
                mouseX >= menuPxLeft && mouseX <= menuPxRight) {
                for (int i = 0; i < (int)m_demos.size(); ++i) {
                    const float itemTop    = listStartPxY - itemHeightPx * (float)(i + 1);
                    const float itemBottom = listStartPxY - itemHeightPx * (float)i;
                    if (mouseY >= itemTop && mouseY <= itemBottom) {
                        m_activeDemo = i;
                        m_menuSelection = i;
                        m_demos[m_activeDemo]->reset();
                        m_demos[m_activeDemo]->initialize();
                        m_showingMenu = false;
                        m_paused = true;
                        break;
                    }
                }
            }
        }

        renderScene();

        m_engine.EndFrame();

        if (isRecording()) { recordFrame(); }
    }

    if (isRecording()) { stopRecording(); }
}

void DemoApplication::destroy() {
    m_shaderSet.Destroy();

    m_engine.GetDevice()->DestroyGPUBuffer(m_geometryVertexBuffer);
    m_engine.GetDevice()->DestroyGPUBuffer(m_geometryIndexBuffer);

    m_assetManager.Destroy();
    m_engine.Destroy();
}

void DemoApplication::drawGenerated(
        const GeometryGenerator::GeometryIndices &indices, int layer) {
    m_engine.DrawGeneric(m_shaders.GetRegularFlags(), m_geometryIndexBuffer,
                         m_geometryVertexBuffer, m_shaders.shaderProgram(),
                         m_shaders.inputLayout(), sizeof(dbasic::Vertex),
                         indices.BaseIndex, indices.BaseVertex,
                         indices.FaceCount, true, layer);
}

void DemoApplication::drawBar(float x, float y, float theta, float length,
                              float width_px) {
    GeometryGenerator::GeometryIndices bar, shadow, bolts;

    const float shadowWidth = 6.0f;

    GeometryGenerator::Line2dParameters lineParams;
    lineParams.x0 = -length / 2;
    lineParams.y0 = 0.0;
    lineParams.x1 = length / 2;
    lineParams.y1 = 0.0;

    GeometryGenerator::Circle2dParameters circleParams;
    circleParams.center_y = 0.0;
    circleParams.maxEdgeLength = pixelsToUnits(2.0f);

    // Bar
    m_geometryGenerator.startShape();

    lineParams.lineWidth = pixelsToUnits(width_px) * m_uiScale;
    m_geometryGenerator.generateLine2d(lineParams);

    circleParams.radius = pixelsToUnits(width_px / 2) * m_uiScale;

    circleParams.center_x = -length / 2;
    m_geometryGenerator.generateCircle2d(circleParams);

    circleParams.center_x = length / 2;
    m_geometryGenerator.generateCircle2d(circleParams);

    m_geometryGenerator.endShape(&bar);

    // Shadows
    m_geometryGenerator.startShape();

    lineParams.lineWidth =
            pixelsToUnits(width_px + shadowWidth * 2) * m_uiScale;
    m_geometryGenerator.generateLine2d(lineParams);
    circleParams.radius = pixelsToUnits(width_px / 2 + shadowWidth) * m_uiScale;

    circleParams.center_x = -length / 2;
    m_geometryGenerator.generateCircle2d(circleParams);

    circleParams.center_x = length / 2;
    m_geometryGenerator.generateCircle2d(circleParams);

    m_geometryGenerator.endShape(&shadow);

    // Bolts
    m_geometryGenerator.startShape();

    circleParams.radius = pixelsToUnits(5) * m_uiScale;
    circleParams.center_x = -length / 2;
    m_geometryGenerator.generateCircle2d(circleParams);

    circleParams.center_x = length / 2;
    m_geometryGenerator.generateCircle2d(circleParams);

    m_geometryGenerator.endShape(&bolts);

    // Draw shapes
    ysMatrix mat = ysMath::RotationTransform(
            ysMath::LoadVector(0.0f, 0.0f, 1.0f), (float) theta);
    mat = ysMath::MatMult(ysMath::TranslationTransform(ysMath::LoadVector(
                                  (float) x, (float) y, 0.0f, 0.0f)),
                          mat);

    m_shaders.ResetBrdfParameters();
    m_shaders.SetColorReplace(true);
    m_shaders.SetLit(false);
    m_shaders.SetFogFar(2001);
    m_shaders.SetFogNear(2000.0);
    m_shaders.SetObjectTransform(mat);

    m_shaders.SetBaseColor(ysColor::srgbiToLinear(0x101213));
    drawGenerated(shadow);

    m_shaders.SetBaseColor(ysMath::LoadVector(1.0f, 1.0f, 1.0f, 1.0f));
    drawGenerated(bar);

    m_shaders.SetBaseColor(ysColor::srgbiToLinear(0x101213));
    drawGenerated(bolts);
}

void DemoApplication::drawRoundedFrame(float x, float y, float width,
                                       float height, float thickness,
                                       float cornerRadius) {
    const float maxCornerRadius = std::fminf(width / 2, height / 2);
    if (cornerRadius > maxCornerRadius) { cornerRadius = maxCornerRadius; }

    GeometryGenerator::Line2dParameters params;
    params.lineWidth = thickness;

    m_geometryGenerator.startShape();

    params.x0 = x - width / 2 + cornerRadius;
    params.y0 = y + height / 2;
    params.x1 = x + width / 2 - cornerRadius;
    params.y1 = y + height / 2;
    m_geometryGenerator.generateLine2d(params);

    params.x0 = x - width / 2 + cornerRadius;
    params.y0 = y - height / 2;
    params.x1 = x + width / 2 - cornerRadius;
    params.y1 = y - height / 2;
    m_geometryGenerator.generateLine2d(params);

    params.x0 = x - width / 2;
    params.y0 = y - height / 2 + cornerRadius;
    params.x1 = x - width / 2;
    params.y1 = y + height / 2 - cornerRadius;
    m_geometryGenerator.generateLine2d(params);

    params.x0 = x + width / 2;
    params.y0 = y - height / 2 + cornerRadius;
    params.x1 = x + width / 2;
    params.y1 = y + height / 2 - cornerRadius;
    m_geometryGenerator.generateLine2d(params);

    GeometryGenerator::Ring2dParameters ringParams;
    ringParams.innerRadius = cornerRadius - thickness / 2;
    ringParams.outerRadius = cornerRadius + thickness / 2;
    ringParams.maxEdgeLength = pixelsToUnits(2.0f);

    const float a0 = 0;
    const float a90 = ysMath::Constants::PI / 2;
    const float a180 = ysMath::Constants::PI;
    const float a270 = 3 * ysMath::Constants::PI / 2;
    const float a360 = ysMath::Constants::TWO_PI;

    ringParams.center_x = x - width / 2 + cornerRadius;
    ringParams.center_y = y - height / 2 + cornerRadius;
    ringParams.startAngle = a180;
    ringParams.endAngle = a270;
    m_geometryGenerator.generateRing2d(ringParams);

    ringParams.center_x = x + width / 2 - cornerRadius;
    ringParams.center_y = y - height / 2 + cornerRadius;
    ringParams.startAngle = a270;
    ringParams.endAngle = a360;
    m_geometryGenerator.generateRing2d(ringParams);

    ringParams.center_x = x - width / 2 + cornerRadius;
    ringParams.center_y = y + height / 2 - cornerRadius;
    ringParams.startAngle = a90;
    ringParams.endAngle = a180;
    m_geometryGenerator.generateRing2d(ringParams);

    ringParams.center_x = x + width / 2 - cornerRadius;
    ringParams.center_y = y + height / 2 - cornerRadius;
    ringParams.startAngle = a0;
    ringParams.endAngle = a90;
    m_geometryGenerator.generateRing2d(ringParams);

    GeometryGenerator::GeometryIndices indices;
    m_geometryGenerator.endShape(&indices);

    drawGenerated(indices, BackgroundLayer);
}

void DemoApplication::drawGrid() {
    float frameWidth, frameHeight;
    getGridFrameSize(&frameWidth, &frameHeight);

    GeometryGenerator::GridParameters gridParams;
    gridParams.x = 0.0f;
    gridParams.y = 0.0f;
    gridParams.width = frameWidth;
    gridParams.height = frameHeight;

    GeometryGenerator::FrameParameters frameParams;
    frameParams.frameHeight = frameHeight;
    frameParams.frameWidth = frameWidth;
    frameParams.lineWidth = pixelsToUnits(2.0f);
    frameParams.x = frameParams.y = 0.0f;

    GeometryGenerator::GeometryIndices main, second, third;

    m_geometryGenerator.startShape();
    gridParams.div_x = gridParams.div_y = 1.0f;
    gridParams.lineWidth = pixelsToUnits(1.0f);
    m_geometryGenerator.generateGrid(gridParams);
    m_geometryGenerator.generateFrame(frameParams);
    m_geometryGenerator.endShape(&main);

    m_geometryGenerator.startShape();
    gridParams.div_x = gridParams.div_y = 0.5f;
    gridParams.lineWidth = pixelsToUnits(1.0f);
    m_geometryGenerator.generateGrid(gridParams);
    m_geometryGenerator.endShape(&second);

    m_geometryGenerator.startShape();
    gridParams.lineWidth = pixelsToUnits(1.0f);
    gridParams.div_x = gridParams.div_y = 0.25f;
    m_geometryGenerator.generateGrid(gridParams);
    m_geometryGenerator.endShape(&third);

    // Draw geometry
    m_shaders.ResetBrdfParameters();
    m_shaders.SetColorReplace(true);
    m_shaders.SetLit(false);
    m_shaders.SetFogFar(2001);
    m_shaders.SetFogNear(2000.0);
    m_shaders.SetObjectTransform(ysMath::LoadIdentity());

    const float c0 = 0.05f, c1 = 0.02f, c2 = 0.01f;

    m_shaders.SetBaseColor(ysMath::LoadVector(c2, c2, c2, 1.0f));
    drawGenerated(third, BackgroundLayer);

    m_shaders.SetBaseColor(ysMath::LoadVector(c1, c1, c1, 1.0f));
    drawGenerated(second, BackgroundLayer);

    m_shaders.SetBaseColor(ysMath::LoadVector(c0, c0, c0, 1.0f));
    drawGenerated(main, BackgroundLayer);

    // Draw logo
    const float logoModelHeight = 2.59f;
    const float logoUiHeight = pixelsToUnits(92) * m_uiScale;

    m_shaders.SetObjectTransform(ysMath::MatMult(
            ysMath::TranslationTransform(ysMath::LoadVector(
                    frameWidth / 2 - 0.5f, -frameHeight / 2 + 0.5f)),
            ysMath::ScaleTransform(
                    ysMath::LoadScalar(logoUiHeight / logoModelHeight))));

    m_shaders.SetBaseColor(m_shadow);
    m_engine.DrawModel(m_shaders.GetRegularFlags(), m_logoBackground,
                       BackgroundLayer);

    m_shaders.SetBaseColor(ysMath::LoadVector(c0, c0, c0, 1.0f));
    m_engine.DrawModel(m_shaders.GetRegularFlags(), m_logo, ForegroundLayer);
}

void DemoApplication::drawFixedPositionConstraint(float x, float y,
                                                  float angle) {
    GeometryGenerator::GeometryIndices indices, pin, shadow;

    const float radius = pixelsToUnits(25.0f * m_uiScale);
    const float boltRadius = pixelsToUnits(5) * m_uiScale;
    const float supportWidth = pixelsToUnits(150.0f) * m_uiScale;
    const float supportThickness = pixelsToUnits(5.0f) * m_uiScale;
    const float shadowThickness = pixelsToUnits(5.0f) * m_uiScale;
    const float maxEdgeLength = pixelsToUnits(5.0f);

    GeometryGenerator::Line2dParameters lineParams;
    lineParams.x0 = -radius;
    lineParams.y0 = lineParams.y1 = -radius / 2;
    lineParams.x1 = radius;
    lineParams.lineWidth = radius;

    m_geometryGenerator.startShape();

    GeometryGenerator::Circle2dParameters circleParams;
    circleParams.center_x = circleParams.center_y = 0;
    circleParams.radius = radius;
    circleParams.maxEdgeLength = maxEdgeLength;
    m_geometryGenerator.generateCircle2d(circleParams);
    m_geometryGenerator.generateLine2d(lineParams);

    lineParams.lineWidth = supportThickness;
    lineParams.x0 = -supportWidth / 2;
    lineParams.x1 = supportWidth / 2;
    lineParams.y0 = lineParams.y1 =
            -radius - shadowThickness - supportThickness / 2;
    m_geometryGenerator.generateLine2d(lineParams);

    m_geometryGenerator.endShape(&indices);

    m_geometryGenerator.startShape();

    lineParams.lineWidth = shadowThickness;
    lineParams.x0 = -radius;
    lineParams.x1 = radius;
    lineParams.y0 = lineParams.y1 = -radius - shadowThickness / 2;
    m_geometryGenerator.generateLine2d(lineParams);

    m_geometryGenerator.endShape(&shadow);

    m_geometryGenerator.startShape();

    circleParams.center_x = circleParams.center_y = 0;
    circleParams.radius = boltRadius;
    m_geometryGenerator.generateCircle2d(circleParams);

    m_geometryGenerator.endShape(&pin);

    // Draw geometry
    ysMatrix mat = ysMath::RotationTransform(
            ysMath::LoadVector(0.0f, 0.0f, 1.0f), (float) angle);
    mat = ysMath::MatMult(
            ysMath::TranslationTransform(ysMath::LoadVector(x, y, 0.0f, 0.0f)),
            mat);

    m_shaders.ResetBrdfParameters();
    m_shaders.SetBaseColor(ysMath::LoadVector(1.0f, 1.0f, 1.0f, 1.0f));
    m_shaders.SetColorReplace(true);
    m_shaders.SetLit(false);
    m_shaders.SetFogFar(2001);
    m_shaders.SetFogNear(2000.0);
    m_shaders.SetObjectTransform(mat);

    m_shaders.SetBaseColor(m_foreground);
    drawGenerated(indices, BackgroundLayer);

    m_shaders.SetBaseColor(m_shadow);
    drawGenerated(shadow, BackgroundLayer);

    m_shaders.SetBaseColor(m_shadow);
    drawGenerated(pin, ForegroundLayer);
}

void DemoApplication::drawCursor(float x, float y) {
    GeometryGenerator::GeometryIndices main, shadow;

    const float radius = pixelsToUnits(20) * m_uiScale;
    const float overallRadius = radius;
    const float shadowThickness = pixelsToUnits(6) * m_uiScale;
    const float maxEdgeLength = pixelsToUnits(10.0f);

    GeometryGenerator::Circle2dParameters params;
    params.maxEdgeLength = maxEdgeLength;

    // Overall
    m_geometryGenerator.startShape();

    params.center_x = params.center_y = 0;
    params.radius = overallRadius;
    m_geometryGenerator.generateCircle2d(params);

    m_geometryGenerator.endShape(&main);

    // Shadows
    m_geometryGenerator.startShape();

    params.center_x = params.center_y = 0;
    params.radius = overallRadius + shadowThickness;
    m_geometryGenerator.generateCircle2d(params);

    m_geometryGenerator.endShape(&shadow);

    // Draw geometry
    ysMatrix mat = ysMath::TranslationTransform(
            ysMath::LoadVector((float) x, (float) y, 0.0f, 0.0f));

    m_shaders.ResetBrdfParameters();
    m_shaders.SetBaseColor(ysMath::LoadVector(1.0f, 1.0f, 1.0f, 1.0f));
    m_shaders.SetColorReplace(true);
    m_shaders.SetLit(false);
    m_shaders.SetFogFar(2001);
    m_shaders.SetFogNear(2000.0);
    m_shaders.SetObjectTransform(mat);

    m_shaders.SetBaseColor(m_shadow);
    drawGenerated(shadow);

    m_shaders.SetBaseColor(m_highlight1);
    drawGenerated(main);
}

void DemoApplication::drawSpring(float x0, float y0, float x1, float y1,
                                 int coils, float radius) {
    const float dx = x1 - x0;
    const float dy = y1 - y0;
    const float length = std::sqrt(dx * dx + dy * dy);
    float theta = (dy < 0) ? ysMath::Constants::TWO_PI - std::acos(dx / length)
                           : std::acos(dx / length);

    GeometryGenerator::Line2dParameters lineParams;
    GeometryGenerator::Rhombus2dParameters rhombParams;
    GeometryGenerator::Circle2dParameters circleParams;
    GeometryGenerator::Trapezoid2dParameters trapParams;

    GeometryGenerator::GeometryIndices rods, rodShadows, retainers,
            retainerShadows, coilBack, coilBackShadow, coilFront,
            coilFrontShadow, bolts;

    const float rodThickness = pixelsToUnits(20) * m_uiScale;
    const float coilThickness = pixelsToUnits(10) * m_uiScale;
    const float shadowThickness = pixelsToUnits(6) * m_uiScale;
    const float rodShadowThickness = shadowThickness * 2 + rodThickness;
    const float rodLength = pixelsToUnits(40) * m_uiScale;
    const float retainerRadius = radius;
    const float coilLength = length + rodThickness / 2 - rodLength * 2;
    const float boltRadius = pixelsToUnits(5) * m_uiScale;

    circleParams.center_y = 0.0;
    circleParams.maxEdgeLength = pixelsToUnits(2.0f);

    // Rods
    m_geometryGenerator.startShape();
    lineParams.y0 = 0;
    lineParams.y1 = 0;
    lineParams.lineWidth = rodThickness;

    lineParams.x0 = -length / 2;
    lineParams.x1 = lineParams.x0 + rodLength;
    m_geometryGenerator.generateLine2d(lineParams);

    lineParams.x0 = length / 2;
    lineParams.x1 = lineParams.x0 - rodLength;
    m_geometryGenerator.generateLine2d(lineParams);

    circleParams.radius = rodThickness / 2;
    circleParams.center_x = -length / 2;
    m_geometryGenerator.generateCircle2d(circleParams);

    circleParams.center_x = length / 2;
    m_geometryGenerator.generateCircle2d(circleParams);

    m_geometryGenerator.endShape(&rods);

    // Rod Shadows
    m_geometryGenerator.startShape();
    lineParams.y0 = 0;
    lineParams.y1 = 0;
    lineParams.lineWidth = rodShadowThickness;

    lineParams.x0 = -length / 2;
    lineParams.x1 = lineParams.x0 + rodLength;
    m_geometryGenerator.generateLine2d(lineParams);

    lineParams.x0 = length / 2;
    lineParams.x1 = lineParams.x0 - rodLength;
    m_geometryGenerator.generateLine2d(lineParams);

    circleParams.radius = rodShadowThickness / 2;
    circleParams.center_x = -length / 2;
    m_geometryGenerator.generateCircle2d(circleParams);

    circleParams.center_x = length / 2;
    m_geometryGenerator.generateCircle2d(circleParams);

    m_geometryGenerator.endShape(&rodShadows);

    // Retainers
    m_geometryGenerator.startShape();
    lineParams.lineWidth = coilThickness;
    lineParams.y0 = -retainerRadius;
    lineParams.y1 = retainerRadius;

    lineParams.x0 = lineParams.x1 = -length / 2 + rodLength;
    m_geometryGenerator.generateLine2d(lineParams);

    lineParams.x0 = lineParams.x1 = length / 2 - rodLength;
    m_geometryGenerator.generateLine2d(lineParams);

    m_geometryGenerator.endShape(&retainers);

    // Retainer Shadows
    m_geometryGenerator.startShape();
    lineParams.lineWidth = coilThickness + shadowThickness * 2;

    lineParams.y0 = -retainerRadius;
    lineParams.y1 = retainerRadius + shadowThickness;
    lineParams.x0 = lineParams.x1 = -length / 2 + rodLength;
    m_geometryGenerator.generateLine2d(lineParams);

    lineParams.y0 = -retainerRadius - shadowThickness;
    lineParams.y1 = retainerRadius;
    lineParams.x0 = lineParams.x1 = length / 2 - rodLength;
    m_geometryGenerator.generateLine2d(lineParams);

    m_geometryGenerator.endShape(&retainerShadows);

    // Coil
    rhombParams.width = coilThickness;
    rhombParams.center_y = 0;

    // Coil back
    m_geometryGenerator.startShape();
    rhombParams.height = 2 * retainerRadius;

    const float rhombSegment = (coilLength - coilThickness) / (2 * coils + 1);
    const float fullRhombusWidth = rhombSegment + coilThickness;

    // full_rhombus_width = width + 2 * shear
    const float rawShear = (fullRhombusWidth - rhombParams.width) / 2;

    rhombParams.shear = rawShear;

    for (int i = 0; i < coils + 1; ++i) {
        rhombParams.center_x = -coilLength / 2.0f + fullRhombusWidth / 2 +
                               rhombSegment * (2 * i);
        m_geometryGenerator.generateRhombus(rhombParams);
    }

    m_geometryGenerator.endShape(&coilBack);

    // Coil front
    m_geometryGenerator.startShape();
    rhombParams.height = 2 * retainerRadius;
    rhombParams.shear = -rhombParams.shear;

    for (int i = 0; i < coils; ++i) {
        rhombParams.center_x = -coilLength / 2.0f + fullRhombusWidth / 2 +
                               rhombSegment * (2 * i + 1);
        m_geometryGenerator.generateRhombus(rhombParams);
    }

    m_geometryGenerator.endShape(&coilFront);

    // Coil front shadow
    m_geometryGenerator.startShape();
    trapParams.height = shadowThickness;
    trapParams.base = coilThickness + shadowThickness * 2;
    trapParams.top =
            trapParams.base - (rawShear / retainerRadius) * shadowThickness * 2;
    rhombParams.width = coilThickness + shadowThickness * 2;
    rhombParams.height = 2 * retainerRadius;
    rhombParams.shear = -rawShear;

    for (int i = 0; i < coils + 1; ++i) {
        if (i < coils) {
            rhombParams.center_x = -coilLength / 2.0f + fullRhombusWidth / 2 +
                                   rhombSegment * (2 * i + 1);
            m_geometryGenerator.generateRhombus(rhombParams);
        }

        trapParams.center_x = -shadowThickness - coilLength / 2.0f +
                              rhombSegment * (2 * i + 1) + trapParams.base / 2;
        trapParams.center_y = retainerRadius + shadowThickness / 2;
        m_geometryGenerator.generateTrapezoid2d(trapParams);
    }

    m_geometryGenerator.endShape(&coilFrontShadow);

    // Coil back shadow
    m_geometryGenerator.startShape();
    rhombParams.shear = -rhombParams.shear;
    trapParams.top = coilThickness + shadowThickness * 2;
    trapParams.base =
            trapParams.top - (rawShear / retainerRadius) * shadowThickness * 2;

    for (int i = 0; i < coils + 1; ++i) {
        rhombParams.center_x = -coilLength / 2.0f + fullRhombusWidth / 2 +
                               rhombSegment * (2 * i);
        m_geometryGenerator.generateRhombus(rhombParams);

        trapParams.center_x = -shadowThickness - coilLength / 2.0f +
                              rhombSegment * (2 * i) + trapParams.top / 2;
        trapParams.center_y = -(retainerRadius + shadowThickness / 2);
        m_geometryGenerator.generateTrapezoid2d(trapParams);
    }

    m_geometryGenerator.endShape(&coilBackShadow);

    // Bolts
    m_geometryGenerator.startShape();

    circleParams.radius = boltRadius;
    circleParams.center_x = -length / 2;
    m_geometryGenerator.generateCircle2d(circleParams);

    circleParams.center_x = length / 2;
    m_geometryGenerator.generateCircle2d(circleParams);

    m_geometryGenerator.endShape(&bolts);

    // Draw geometry
    ysMatrix mat = ysMath::RotationTransform(
            ysMath::LoadVector(0.0f, 0.0f, 1.0f), (float) theta);
    mat = ysMath::MatMult(
            ysMath::TranslationTransform(ysMath::LoadVector(
                    (float) (x0 + x1) / 2, (float) (y0 + y1) / 2, 0.0f, 0.0f)),
            mat);

    m_shaders.ResetBrdfParameters();
    m_shaders.SetBaseColor(ysMath::LoadVector(1.0f, 1.0f, 1.0f, 1.0f));
    m_shaders.SetColorReplace(true);
    m_shaders.SetLit(false);
    m_shaders.SetFogFar(2001);
    m_shaders.SetFogNear(2000.0);
    m_shaders.SetObjectTransform(mat);

    m_shaders.SetBaseColor(m_shadow);
    drawGenerated(coilBackShadow);

    m_shaders.SetBaseColor(m_shadow);
    drawGenerated(rodShadows);

    m_shaders.SetBaseColor(m_foreground);
    drawGenerated(rods);

    m_shaders.SetBaseColor(m_foreground);
    drawGenerated(coilBack);

    m_shaders.SetBaseColor(m_shadow);
    drawGenerated(coilFrontShadow);

    m_shaders.SetBaseColor(m_foreground);
    drawGenerated(coilFront);

    m_shaders.SetBaseColor(m_shadow);
    drawGenerated(retainerShadows);

    m_shaders.SetBaseColor(m_foreground);
    drawGenerated(retainers);

    m_shaders.SetBaseColor(m_shadow);
    drawGenerated(bolts);
}

void DemoApplication::drawDisk(float x, float y, float theta, float radius) {
    GeometryGenerator::GeometryIndices main, shadow, details;

    const float minRadius = pixelsToUnits(40) * m_uiScale;
    const float overallRadius = std::fmaxf(minRadius, radius);
    const float centerRadius = pixelsToUnits(20) * m_uiScale;
    const float boltRadius = pixelsToUnits(5) * m_uiScale;
    const float shadowThickness = pixelsToUnits(6) * m_uiScale;
    const float maxEdgeLength = pixelsToUnits(10.0f);

    GeometryGenerator::Circle2dParameters params;
    params.maxEdgeLength = maxEdgeLength;

    // Overall
    m_geometryGenerator.startShape();

    params.center_x = params.center_y = 0;
    params.radius = overallRadius;
    m_geometryGenerator.generateCircle2d(params);

    m_geometryGenerator.endShape(&main);

    // Shadows
    m_geometryGenerator.startShape();

    params.center_x = params.center_y = 0;
    params.radius = overallRadius + shadowThickness;
    m_geometryGenerator.generateCircle2d(params);

    m_geometryGenerator.endShape(&shadow);

    // Details
    m_geometryGenerator.startShape();

    params.center_x = params.center_y = 0;
    params.radius = centerRadius;
    m_geometryGenerator.generateCircle2d(params);

    params.center_x = 0;
    params.center_y = -(overallRadius + centerRadius) / 2;
    params.radius = boltRadius;
    m_geometryGenerator.generateCircle2d(params);

    m_geometryGenerator.endShape(&details);

    // Draw geometry
    ysMatrix mat = ysMath::RotationTransform(
            ysMath::LoadVector(0.0f, 0.0f, 1.0f), (float) theta);
    mat = ysMath::MatMult(ysMath::TranslationTransform(ysMath::LoadVector(
                                  (float) x, (float) y, 0.0f, 0.0f)),
                          mat);

    m_shaders.ResetBrdfParameters();
    m_shaders.SetBaseColor(ysMath::LoadVector(1.0f, 1.0f, 1.0f, 1.0f));
    m_shaders.SetColorReplace(true);
    m_shaders.SetLit(false);
    m_shaders.SetFogFar(2001);
    m_shaders.SetFogNear(2000.0);
    m_shaders.SetObjectTransform(mat);

    m_shaders.SetBaseColor(m_shadow);
    drawGenerated(shadow);

    m_shaders.SetBaseColor(m_foreground);
    drawGenerated(main);

    m_shaders.SetBaseColor(m_shadow);
    drawGenerated(details);
}

void DemoApplication::drawLineConstraint(float x, float y, float dx, float dy,
                                         float roller_x, float roller_y,
                                         float length, float sliderLength,
                                         bool drawTrack) {
    const float width = length;
    const float height = pixelsToUnits(40.0f) * m_uiScale;
    const float thickness = pixelsToUnits(10.0f) * m_uiScale;
    const float sliderThickness = pixelsToUnits(10.0f) * m_uiScale;
    const float boltRadius = pixelsToUnits(5) * m_uiScale;

    const float theta = (dy > 0) ? std::acosf(dx) : -std::acosf(dx);

    const float local_x = roller_x - x;
    const float local_y = roller_y - y;

    GeometryGenerator::GeometryIndices slider, pin;
    GeometryGenerator::Line2dParameters params;
    GeometryGenerator::Circle2dParameters circleParams;
    circleParams.maxEdgeLength = pixelsToUnits(2.0f);

    m_geometryGenerator.startShape();

    params.lineWidth = sliderThickness;
    params.x0 = -sliderLength / 2 + local_x;
    params.x1 = sliderLength / 2 + local_x;
    params.y0 = params.y1 = local_y;
    m_geometryGenerator.generateLine2d(params);

    circleParams.center_x = params.x0;
    circleParams.center_y = local_y;
    circleParams.radius = sliderThickness / 2;
    m_geometryGenerator.generateCircle2d(circleParams);

    circleParams.center_x = params.x1;
    circleParams.center_y = local_y;
    circleParams.radius = sliderThickness / 2;
    m_geometryGenerator.generateCircle2d(circleParams);

    m_geometryGenerator.endShape(&slider);

    m_geometryGenerator.startShape();

    circleParams.center_x = local_x;
    circleParams.center_y = local_y;
    circleParams.radius = boltRadius;
    m_geometryGenerator.generateCircle2d(circleParams);

    m_geometryGenerator.endShape(&pin);

    // Draw geometry
    ysMatrix mat = ysMath::RotationTransform(
            ysMath::LoadVector(0.0f, 0.0f, 1.0f), (float) theta);
    mat = ysMath::MatMult(ysMath::TranslationTransform(ysMath::LoadVector(
                                  (float) x, (float) y, 0.0f, 0.0f)),
                          mat);

    m_shaders.ResetBrdfParameters();
    m_shaders.SetColorReplace(true);
    m_shaders.SetLit(false);
    m_shaders.SetFogFar(2001);
    m_shaders.SetFogNear(2000.0);
    m_shaders.SetObjectTransform(mat);

    m_shaders.SetBaseColor(m_foreground);
    if (drawTrack)
        drawRoundedFrame(0.0f, 0.0f, width, height, thickness, height / 2.0f);
    drawGenerated(slider, BackgroundLayer);

    m_shaders.SetBaseColor(m_shadow);
    drawGenerated(pin, ForegroundLayer);
}

void DemoApplication::drawMotor(float x, float y, float theta, float radius,
                                bool positive) {
    const float width = pixelsToUnits(10.0f) * m_uiScale;

    GeometryGenerator::GeometryIndices arrow;
    GeometryGenerator::Ring2dParameters params;

    m_geometryGenerator.startShape();

    params.arrowLength = ysMath::Constants::TWO_PI * 0.25;
    params.arrowOnEnd = !positive;
    params.center_x = 0;
    params.center_y = 0;
    params.drawArrow = true;
    params.innerRadius = radius - width / 2;
    params.outerRadius = radius + width / 2;
    params.maxEdgeLength = pixelsToUnits(2.0f);
    params.startAngle = 1.0f;
    params.endAngle = ysMath::Constants::TWO_PI - 0.25f;
    m_geometryGenerator.generateRing2d(params);

    m_geometryGenerator.endShape(&arrow);

    // Draw geometry
    ysMatrix mat = ysMath::RotationTransform(
            ysMath::LoadVector(0.0f, 0.0f, 1.0f), (float) theta);
    mat = ysMath::MatMult(ysMath::TranslationTransform(ysMath::LoadVector(
                                  (float) x, (float) y, 0.0f, 0.0f)),
                          mat);

    m_shaders.ResetBrdfParameters();
    m_shaders.SetColorReplace(true);
    m_shaders.SetLit(false);
    m_shaders.SetFogFar(2001);
    m_shaders.SetFogNear(2000.0);
    m_shaders.SetObjectTransform(mat);

    m_shaders.SetBaseColor(m_highlight1);
    drawGenerated(arrow, BackgroundLayer);
}

void DemoApplication::renderTitle() {
    if (!m_showingStats) return;

    GeometryGenerator::FrameParameters frameParams;
    GeometryGenerator::Line2dParameters lineParams;
    GeometryGenerator::GeometryIndices frame, fill;

    float gridWidth, gridHeight;
    getGridFrameSize(&gridWidth, &gridHeight);

    const float leftMargin = pixelsToUnits(50.0f);
    const float topMargin = pixelsToUnits(50.0f);
    const float titleBoxHeight = pixelsToUnits(180.0f);
    const float titleBoxWidth = pixelsToUnits(600.0f);
    const float lineWidth = pixelsToUnits(1.0f);

    m_geometryGenerator.startShape();

    frameParams.frameHeight = titleBoxHeight;
    frameParams.frameWidth = titleBoxWidth;
    frameParams.x = frameParams.y = 0;
    frameParams.lineWidth = lineWidth;
    m_geometryGenerator.generateFrame(frameParams);

    m_geometryGenerator.endShape(&frame);

    m_geometryGenerator.startShape();

    lineParams.lineWidth = titleBoxHeight;
    lineParams.x0 = -titleBoxWidth / 2;
    lineParams.x1 = titleBoxWidth / 2;
    lineParams.y0 = lineParams.y1 = 0;
    m_geometryGenerator.generateLine2d(lineParams);

    m_geometryGenerator.endShape(&fill);

    // Draw geometry
    ysMatrix mat = ysMath::TranslationTransform(ysMath::LoadVector(
            (float) -gridWidth / 2 + leftMargin + titleBoxWidth / 2,
            (float) gridHeight / 2 - topMargin - titleBoxHeight / 2, 0.0f,
            0.0f));

    m_shaders.ResetBrdfParameters();
    m_shaders.SetColorReplace(true);
    m_shaders.SetLit(false);
    m_shaders.SetFogFar(2001);
    m_shaders.SetFogNear(2000.0);
    m_shaders.SetObjectTransform(mat);

    m_shaders.SetBaseColor(m_shadow);
    drawGenerated(fill, ForegroundLayer);

    m_shaders.SetBaseColor(m_foreground);
    drawGenerated(frame, ForegroundLayer);

    float p_y = unitsToPixels(gridHeight / 2 - topMargin) - 36;
    m_textRenderer.RenderText(m_demos[m_activeDemo]->getName(),
                              unitsToPixels(-gridWidth / 2 + leftMargin) + 10,
                              p_y, 32);

    p_y -= 32;
    m_textRenderer.RenderText(
            "INFO", unitsToPixels(-gridWidth / 2 + leftMargin) + 10, p_y, 26);

    m_textRenderer.RenderText(
            "PROFILING",
            unitsToPixels(-gridWidth / 2 + leftMargin + titleBoxWidth / 2) + 10,
            p_y, 26);

    p_y -= 20;
    std::stringstream ss;
    const long freq = std::lroundf(m_demos[m_activeDemo]->getSteps() /
                                   m_demos[m_activeDemo]->getTimestep());

    ss << "RT FR = " << std::lroundf(m_engine.GetAverageFramerate())
       << " FPS       \n";
    ss << "SR = " << freq << " HZ      \n";
    ss << "STEPS = " << m_demos[m_activeDemo]->getSteps() << "     \n";
    ss << "ENERGY = " << m_demos[m_activeDemo]->energy() << "      \n";
    m_textRenderer.RenderText(
            ss.str(), unitsToPixels(-gridWidth / 2 + leftMargin) + 10, p_y, 16);

    ss = std::stringstream();

    ss << "F-EVAL = " << m_demos[m_activeDemo]->getForceEvalMicroseconds()
       << " us       \n";
    ss << "C-EVAL = " << m_demos[m_activeDemo]->getConstraintEvalMicroseconds()
       << " us  \n";
    ss << "C-SOLVE = "
       << m_demos[m_activeDemo]->getConstraintSolveMicroseconds() << " us\n";
    ss << "ODE = " << m_demos[m_activeDemo]->getOdeSolveMicroseconds()
       << " us        \n";
    m_textRenderer.RenderText(
            ss.str(),
            unitsToPixels(-gridWidth / 2 + leftMargin + titleBoxWidth / 2) + 10,
            p_y, 16);

    p_y -= 5 * 16;

    m_textRenderer.RenderText(
            (m_paused) ? "PAUSED // OKAY     " : "RUNNING // OKAY     ",
            unitsToPixels(-gridWidth / 2 + leftMargin) + 10, p_y, 26);

    // Show hint that Tab opens the menu
    m_textRenderer.RenderText(
            "[TAB] SELECT DEMO",
            unitsToPixels(-gridWidth / 2 + leftMargin) + 10, p_y - 30, 14);
}

void DemoApplication::renderMenu() {
    if (!m_showingMenu) return;

    // ---------------------------------------------------------------
    // All sizes defined in pixels first, then converted to world units
    // for geometry, or used directly as pixels for text.
    //
    // Screen pixel convention: x left->right, y top->down, origin top-left.
    // World unit convention:   x left->right, y bottom->up, origin screen center.
    //
    // Conversions:
    //   world_x  =  pixelsToUnits(px_x - screenW/2)
    //   world_y  =  pixelsToUnits(screenH/2 - px_y)
    //   px_x     =  unitsToPixels(world_x) + screenW/2
    //   px_y     =  screenH/2 - unitsToPixels(world_y)
    // ---------------------------------------------------------------

    const float screenW = (float)m_engine.GetScreenWidth();
    const float screenH = (float)m_engine.GetScreenHeight();

    // Menu dimensions in pixels
    const float menuWidthPx    = 460.0f;
    const float headerHeightPx = 60.0f;
    const float footerHeightPx = 28.0f;
    const float itemHeightPx   = 34.0f;
    const float paddingPx      = 10.0f;
    const float menuHeightPx   = headerHeightPx + footerHeightPx
                               + itemHeightPx * (float)m_demos.size()
                               + paddingPx * 2.0f;

    // Menu top-left corner in screen pixels (centered)
    const float menuPxLeft  = (screenW - menuWidthPx)  * 0.5f;
    const float menuPxTop   = (screenH - menuHeightPx) * 0.5f;
    const float menuPxRight = menuPxLeft + menuWidthPx;

    // Convert to world units for geometry (menu is centered so world origin = screen center)
    const float menuWidth    = pixelsToUnits(menuWidthPx);
    const float menuHeight   = pixelsToUnits(menuHeightPx);
    const float headerHeight = pixelsToUnits(headerHeightPx);
    const float itemHeight   = pixelsToUnits(itemHeightPx);
    const float padding      = pixelsToUnits(paddingPx);

    // In world units, menu is centered at (0,0)
    // Top of menu  = +menuHeight/2
    // Divider is headerHeight below the top
    const float dividerWorldY = menuHeight / 2.0f - headerHeight;

    // Center Y of first list item (world units, Y-up)
    const float firstItemWorldY = dividerWorldY - padding - itemHeight * 0.5f;

    // Highlight bar world Y for selected item
    const float highlightWorldY = firstItemWorldY - itemHeight * (float)m_menuSelection;

    // ---- Build geometry ----
    GeometryGenerator::GeometryIndices fill, frame, highlightBar, divider;
    GeometryGenerator::Line2dParameters lineParams;
    GeometryGenerator::FrameParameters frameParams;

    // Background fill
    m_geometryGenerator.startShape();
    lineParams.lineWidth = menuHeight;
    lineParams.x0 = -menuWidth / 2.0f;
    lineParams.x1 =  menuWidth / 2.0f;
    lineParams.y0 = lineParams.y1 = 0.0f;
    m_geometryGenerator.generateLine2d(lineParams);
    m_geometryGenerator.endShape(&fill);

    // Border frame
    m_geometryGenerator.startShape();
    frameParams.frameWidth  = menuWidth;
    frameParams.frameHeight = menuHeight;
    frameParams.x = 0.0f;
    frameParams.y = 0.0f;
    frameParams.lineWidth = pixelsToUnits(1.0f);
    m_geometryGenerator.generateFrame(frameParams);
    m_geometryGenerator.endShape(&frame);

    // Divider line under header
    m_geometryGenerator.startShape();
    lineParams.lineWidth = pixelsToUnits(1.0f);
    lineParams.x0 = -menuWidth / 2.0f;
    lineParams.x1 =  menuWidth / 2.0f;
    lineParams.y0 = lineParams.y1 = dividerWorldY;
    m_geometryGenerator.generateLine2d(lineParams);
    m_geometryGenerator.endShape(&divider);

    // Highlight bar
    m_geometryGenerator.startShape();
    lineParams.lineWidth = itemHeight - pixelsToUnits(3.0f);
    lineParams.x0 = -menuWidth / 2.0f + pixelsToUnits(3.0f);
    lineParams.x1 =  menuWidth / 2.0f - pixelsToUnits(3.0f);
    lineParams.y0 = lineParams.y1 = highlightWorldY;
    m_geometryGenerator.generateLine2d(lineParams);
    m_geometryGenerator.endShape(&highlightBar);

    // ---- Draw geometry ----
    m_shaders.ResetBrdfParameters();
    m_shaders.SetColorReplace(true);
    m_shaders.SetLit(false);
    m_shaders.SetFogFar(2001);
    m_shaders.SetFogNear(2000.0);
    m_shaders.SetObjectTransform(ysMath::TranslationTransform(
            ysMath::LoadVector(0.0f, 0.0f, 0.0f, 0.0f)));

    m_shaders.SetBaseColor(m_shadow);
    drawGenerated(fill, ForegroundLayer);

    m_shaders.SetBaseColor(m_foreground);
    drawGenerated(frame, ForegroundLayer);

    m_shaders.SetBaseColor(m_foreground);
    drawGenerated(divider, ForegroundLayer);

    m_shaders.SetBaseColor(ysMath::LoadVector(0.28f, 0.28f, 0.28f, 1.0f));
    drawGenerated(highlightBar, ForegroundLayer);

    // ---- Text ----
    // The text renderer uses the same coords as renderTitle():
    // x = unitsToPixels(world_x),  y = unitsToPixels(world_y)
    // where world origin is screen center, Y increases upward.
    // So to go DOWN the screen we SUBTRACT from y.

    const float textX = unitsToPixels(-menuWidth / 2.0f) + 14.0f;

    // Start near the top of the menu
    float textY = unitsToPixels(menuHeight / 2.0f) - 30.0f;
    m_textRenderer.RenderText("SELECT DEMO", textX, textY, 28.0f);

    textY -= 26.0f;
    m_textRenderer.RenderText(
            "[UP][DOWN] NAVIGATE   [ENTER] LOAD   [TAB/ESC] CLOSE",
            textX, textY, 11.0f);

    // List items: start just below the divider
    // dividerWorldY in world units, convert to text pixel y then step downward
    float itemTextY = unitsToPixels(dividerWorldY) - paddingPx - 14.0f;

    for (int i = 0; i < (int)m_demos.size(); ++i) {
        const bool isActive   = (i == m_activeDemo);
        const bool isSelected = (i == m_menuSelection);

        std::string label = (isActive ? "> " : "  ") + m_demos[i]->getName();

        if (isSelected) {
            m_textRenderer.RenderText(label, textX,     itemTextY, 18.0f);
            m_textRenderer.RenderText(label, textX + 1, itemTextY, 18.0f);
        } else {
            m_textRenderer.RenderText(label, textX, itemTextY, 15.0f);
        }

        itemTextY -= itemHeightPx;  // step downward in text coords
    }
}

void DemoApplication::drawLines(ysVector2 *p0, ysVector2 *p1, int n0, int n1) {
    if (n0 <= 0) return;

    GeometryGenerator::GeometryIndices lines;
    GeometryGenerator::PathParameters params;

    params.p0 = p0;
    params.p1 = p1;
    params.n0 = n0;
    params.n1 = n1;

    m_geometryGenerator.startShape();

    params.i = 0;
    params.width = pixelsToUnits(0.5f);
    if (!m_geometryGenerator.startPath(params)) { return; }

    ysVector2 prev = p0[0];
    for (int i = 1; i < n0 + n1; ++i) {
        ysVector2 *p = (i >= n0) ? p1 : p0;
        const int index = (i >= n0) ? i - n0 : i;
        const float s = (float) (i) / (n0 + n1);
        const ysVector2 p_i = p[index];
        params.i = i;
        params.width = std::fmaxf(pixelsToUnits(2.0f) * s, pixelsToUnits(0.5f));

        if (s > 0.95f) {
            params.width += pixelsToUnits(((s - 0.95f) / 0.05f) * 6);
        }

        m_geometryGenerator.generatePathSegment(params);

        prev = p_i;
    }

    m_geometryGenerator.endShape(&lines);

    m_shaders.ResetBrdfParameters();
    m_shaders.SetColorReplace(true);
    m_shaders.SetLit(false);
    m_shaders.SetFogFar(2001);
    m_shaders.SetFogNear(2000.0);

    m_shaders.SetBaseColor(m_highlight1);
    drawGenerated(lines, ForegroundLayer);
}

void DemoApplication::drawBlob(float x, float y) {
    const float blobModelHeight = 2.94f;
    const float blobHeight = 1.0f;

    m_shaders.SetObjectTransform(ysMath::MatMult(
            ysMath::TranslationTransform(ysMath::LoadVector(x, y)),
            ysMath::ScaleTransform(
                    ysMath::LoadScalar(blobHeight / blobModelHeight))));

    m_shaders.SetBaseColor(m_shadow);
    m_engine.DrawModel(m_shaders.GetRegularFlags(), m_blobBackground,
                       ForegroundLayer);

    m_shaders.SetBaseColor(m_foreground);
    m_engine.DrawModel(m_shaders.GetRegularFlags(), m_blobForeground,
                       ForegroundLayer);

    m_shaders.SetBaseColor(m_shadow);
    m_engine.DrawModel(m_shaders.GetRegularFlags(), m_blobFace,
                       ForegroundLayer);
}

float DemoApplication::pixelsToUnits(float pixels) const {
    const float f = m_displayHeight / m_engine.GetGameWindow()->GetGameHeight();
    return pixels * f;
}

float DemoApplication::unitsToPixels(float units) const {
    const float f = m_engine.GetGameWindow()->GetGameHeight() / m_displayHeight;
    return units * f;
}

void DemoApplication::getGridFrameSize(float *w, float *h) const {
    *w = pixelsToUnits((float) m_engine.GetScreenWidth() - 100.0f);
    *h = pixelsToUnits((float) m_engine.GetScreenHeight() - 100.0f);
}

void DemoApplication::renderScene() {
    m_shaders.SetClearColor(ysColor::linearToSrgb(m_shadow));

    const int screenWidth = m_engine.GetGameWindow()->GetGameWidth();
    const int screenHeight = m_engine.GetGameWindow()->GetGameHeight();

    m_shaders.SetScreenDimensions((float) screenWidth, (float) screenHeight);

    m_shaders.SetCameraPosition(m_cameraPosition);
    m_shaders.SetCameraTarget(m_cameraTarget);
    m_shaders.SetCameraUp(m_cameraUp);
    m_shaders.CalculateUiCamera();

    if (screenWidth > 0 && screenHeight > 0) {
        const float aspectRatio = screenWidth / (float) screenHeight;
        m_shaders.SetProjection(ysMath::Transpose(
                ysMath::OrthographicProjection(aspectRatio * m_displayHeight,
                                               m_displayHeight, 0.001f,
                                               500.0f)));
    }

    m_geometryGenerator.reset();

    m_demos[m_activeDemo]->render();
    renderTitle();
    renderMenu();
    renderToolbar();   // Draw menu overlay on top of everything

    m_engine.GetDevice()->EditBufferDataRange(
            m_geometryVertexBuffer,
            (char *) m_geometryGenerator.getVertexData(),
            sizeof(dbasic::Vertex) *
                    m_geometryGenerator.getCurrentVertexCount(),
            0);

    m_engine.GetDevice()->EditBufferDataRange(
            m_geometryIndexBuffer, (char *) m_geometryGenerator.getIndexData(),
            sizeof(unsigned short) * m_geometryGenerator.getCurrentIndexCount(),
            0);
}

void DemoApplication::startRecording() {
    m_recording = true;

#ifdef ATG_SCS_DEMO_ENABLE_VIDEO_CAPTURE
    atg_dtv::Encoder::VideoSettings settings{};

    settings.fname = "scs_demo_video_capture.mp4";
    settings.inputWidth = m_engine.GetScreenWidth();
    settings.inputHeight = m_engine.GetScreenHeight();
    settings.width = settings.inputWidth;
    settings.height = settings.inputHeight;
    settings.hardwareEncoding = true;
    settings.inputAlpha = true;
    settings.bitRate = 40000000;

    m_encoder.run(settings, 2);
#endif /* ATG_SCS_DEMO_ENABLE_VIDEO_CAPTURE */
}

void DemoApplication::updateScreenSizeStability() {
    m_screenResolution[m_screenResolutionIndex][0] = m_engine.GetScreenWidth();
    m_screenResolution[m_screenResolutionIndex][1] = m_engine.GetScreenHeight();

    m_screenResolutionIndex =
            (m_screenResolutionIndex + 1) % ScreenResolutionHistoryLength;
}

bool DemoApplication::readyToRecord() {
    const int w = m_screenResolution[0][0];
    const int h = m_screenResolution[0][1];

    if (w <= 0 && h <= 0) return false;
    if ((w % 2) != 0 || (h % 2) != 0) return false;

    for (int i = 1; i < ScreenResolutionHistoryLength; ++i) {
        if (m_screenResolution[i][0] != w) return false;
        if (m_screenResolution[i][1] != h) return false;
    }

    return true;
}

void DemoApplication::stopRecording() {
    m_recording = false;

#ifdef ATG_SCS_DEMO_ENABLE_VIDEO_CAPTURE
    m_encoder.commit();
    m_encoder.stop();
#endif /* ATG_SCS_DEMO_ENABLE_VIDEO_CAPTURE */
}

void DemoApplication::recordFrame() {
#ifdef ATG_SCS_DEMO_ENABLE_VIDEO_CAPTURE
    const int width = m_engine.GetScreenWidth();
    const int height = m_engine.GetScreenHeight();

    atg_dtv::Frame *frame = m_encoder.newFrame(true);
    if (frame != nullptr &&
        m_encoder.getError() == atg_dtv::Encoder::Error::None) {
        m_engine.GetDevice()->ReadRenderTarget(m_engine.GetScreenRenderTarget(),
                                               frame->m_rgb);
    }

    m_encoder.submitFrame();
#endif /* ATG_SCS_DEMO_ENABLE_VIDEO_CAPTURE */
}

void DemoApplication::addDemo(Demo *demo) {
    m_demos.push_back(demo);

    demo->setApplication(this);
    demo->initialize();
}

// ─── Toolbar ──────────────────────────────────────────────────────────────────
// 4 tools on the right edge, vertically centered.
// Pixel-art icons, active = white fill/dark icon, inactive = dark fill/white icon.
// ──────────────────────────────────────────────────────────────────────────────

static const InteractionMode kToolbarModes[] = {
    InteractionMode::SelectDrag,
    InteractionMode::AddSpring,
    InteractionMode::AddRod,
    InteractionMode::Delete,
};
static const char* kToolbarKeys[]   = { "[1]", "[2]", "[3]", "[4]" };
static const char* kToolbarLabels[] = { "DRAG", "SPRING", "ROD", "DELETE" };
static constexpr int kToolbarCount  = 4;

void DemoApplication::getToolbarButtonRect(int i, float *x, float *y,
                                           float *w, float *h) const {
    const float screenW = (float)m_engine.GetScreenWidth();
    const float screenH = (float)m_engine.GetScreenHeight();
    *w = ToolbarBtnSize;
    *h = ToolbarBtnSize;
    *x = screenW - ToolbarBtnSize - ToolbarPadding;
    const float totalH = (float)kToolbarCount * ToolbarBtnSize
                       + (float)(kToolbarCount - 1) * ToolbarPadding;
    const float startY = (screenH - totalH) * 0.5f;
    *y = startY + (float)i * (ToolbarBtnSize + ToolbarPadding);
}

bool DemoApplication::isMouseOverToolbar(float mouseX, float mouseY) const {
    if (m_demos.empty()) return false;
    for (int i = 0; i < kToolbarCount; ++i) {
        float bx, by, bw, bh;
        getToolbarButtonRect(i, &bx, &by, &bw, &bh);
        if (mouseX >= bx && mouseX <= bx + bw &&
            mouseY >= by && mouseY <= by + bh)
            return true;
    }
    return false;
}

void DemoApplication::handleToolbarClick(float mouseX, float mouseY) {
    if (m_demos.empty()) return;
    for (int i = 0; i < kToolbarCount; ++i) {
        float bx, by, bw, bh;
        getToolbarButtonRect(i, &bx, &by, &bw, &bh);
        if (mouseX >= bx && mouseX <= bx + bw &&
            mouseY >= by && mouseY <= by + bh) {
            m_demos[m_activeDemo]->setInteractionMode(kToolbarModes[i]);
            if (kToolbarModes[i] == InteractionMode::SelectDrag ||
                kToolbarModes[i] == InteractionMode::Delete)
                m_paused = false;
            break;
        }
    }
}

void DemoApplication::renderToolbar() {
    if (m_demos.empty()) return;

    const InteractionMode activeMode = m_demos[m_activeDemo]->getInteractionMode();
    const float screenW = (float)m_engine.GetScreenWidth();
    const float screenH = (float)m_engine.GetScreenHeight();

    // Background panel behind all buttons
    {
        float bx0, by0, bw, bh;
        getToolbarButtonRect(0, &bx0, &by0, &bw, &bh);
        float bxL, byL, bwL, bhL;
        getToolbarButtonRect(kToolbarCount - 1, &bxL, &byL, &bwL, &bhL);

        const float pad   = ToolbarPadding;
        const float pxX   = bx0 - pad;
        const float pxY   = by0 - pad;
        const float pxW   = bw  + pad * 2.0f;
        const float pxH   = (byL + bhL) - by0 + pad * 2.0f;
        const float pcxPx = pxX + pxW * 0.5f;
        const float pcyPx = pxY + pxH * 0.5f;
        const float pwW   = pixelsToUnits(pxW);
        const float pwH   = pixelsToUnits(pxH);
        const float pwX   = pixelsToUnits(pcxPx - screenW * 0.5f);
        const float pwY   = pixelsToUnits(screenH * 0.5f - pcyPx);

        GeometryGenerator::GeometryIndices panelFill, panelFrame;
        GeometryGenerator::Line2dParameters lp;
        GeometryGenerator::FrameParameters  fp;

        m_geometryGenerator.startShape();
        lp.lineWidth = pwH; lp.x0 = -pwW*0.5f; lp.x1 = pwW*0.5f; lp.y0 = lp.y1 = 0;
        m_geometryGenerator.generateLine2d(lp);
        m_geometryGenerator.endShape(&panelFill);

        m_geometryGenerator.startShape();
        fp.frameWidth = pwW; fp.frameHeight = pwH; fp.x = fp.y = 0;
        fp.lineWidth = pixelsToUnits(1.0f);
        m_geometryGenerator.generateFrame(fp);
        m_geometryGenerator.endShape(&panelFrame);

        m_shaders.ResetBrdfParameters();
        m_shaders.SetColorReplace(true);
        m_shaders.SetLit(false);
        m_shaders.SetFogFar(2001); m_shaders.SetFogNear(2000.0);
        m_shaders.SetObjectTransform(ysMath::TranslationTransform(
            ysMath::LoadVector(pwX, pwY, 0.0f, 0.0f)));
        m_shaders.SetBaseColor(m_shadow);
        drawGenerated(panelFill, ForegroundLayer);
        m_shaders.SetBaseColor(ysMath::LoadVector(0.20f, 0.20f, 0.20f, 1.0f));
        drawGenerated(panelFrame, ForegroundLayer);
    }

    // Each button
    for (int i = 0; i < kToolbarCount; ++i) {
        float bx, by, bw, bh;
        getToolbarButtonRect(i, &bx, &by, &bw, &bh);

        const bool isActive = (kToolbarModes[i] == activeMode);
        const float cxPx = bx + bw * 0.5f;
        const float cyPx = by + bh * 0.5f;
        const float wX   = pixelsToUnits(cxPx - screenW * 0.5f);
        const float wY   = pixelsToUnits(screenH * 0.5f - cyPx);
        const float wW   = pixelsToUnits(bw);
        const float wH   = pixelsToUnits(bh);

        // Button fill
        GeometryGenerator::GeometryIndices btnFill, btnFrame;
        GeometryGenerator::Line2dParameters lp;
        GeometryGenerator::FrameParameters  fp;

        m_geometryGenerator.startShape();
        lp.lineWidth = wH - pixelsToUnits(4.0f);
        lp.x0 = -wW*0.5f + pixelsToUnits(2.0f);
        lp.x1 =  wW*0.5f - pixelsToUnits(2.0f);
        lp.y0 = lp.y1 = 0;
        m_geometryGenerator.generateLine2d(lp);
        m_geometryGenerator.endShape(&btnFill);

        // Button border — thicker when active
        m_geometryGenerator.startShape();
        fp.frameWidth  = wW - pixelsToUnits(4.0f);
        fp.frameHeight = wH - pixelsToUnits(4.0f);
        fp.x = fp.y    = 0;
        fp.lineWidth   = pixelsToUnits(isActive ? 2.5f : 1.0f);
        m_geometryGenerator.generateFrame(fp);
        m_geometryGenerator.endShape(&btnFrame);

        m_shaders.ResetBrdfParameters();
        m_shaders.SetColorReplace(true);
        m_shaders.SetLit(false);
        m_shaders.SetFogFar(2001); m_shaders.SetFogNear(2000.0);
        m_shaders.SetObjectTransform(ysMath::TranslationTransform(
            ysMath::LoadVector(wX, wY, 0.0f, 0.0f)));

        const ysVector fillColor  = isActive
            ? m_foreground
            : ysMath::LoadVector(0.06f, 0.06f, 0.06f, 1.0f);
        const ysVector iconColor  = isActive ? m_shadow : m_foreground;
        const ysVector frameColor = isActive
            ? m_foreground
            : ysMath::LoadVector(0.30f, 0.30f, 0.30f, 1.0f);

        m_shaders.SetBaseColor(fillColor);
        drawGenerated(btnFill, ForegroundLayer);
        m_shaders.SetBaseColor(frameColor);
        drawGenerated(btnFrame, ForegroundLayer);


        // Text centered in button: key number on top, tool name below
        const float tX    = unitsToPixels(wX - wW * 0.5f) + 6.0f;
        const float topWY = wY + wH * 0.5f;
        const float keyY  = unitsToPixels(topWY) - 18.0f;
        const float lblY  = unitsToPixels(topWY) - 36.0f;

        m_textRenderer.RenderText(kToolbarKeys[i],   tX, keyY, 13.0f);
        m_textRenderer.RenderText(kToolbarLabels[i], tX, lblY, 11.0f);
    }

    // Hint below toolbar when awaiting second click
    const InteractionMode mode = m_demos[m_activeDemo]->getInteractionMode();
    if (mode == InteractionMode::AddSpring || mode == InteractionMode::AddRod) {
        float bx, by, bw, bh;
        getToolbarButtonRect(kToolbarCount - 1, &bx, &by, &bw, &bh);
        const float hintPxY = by + bh + ToolbarPadding * 2.0f;
        const float hWY = pixelsToUnits(screenH * 0.5f - (hintPxY + 10.0f));
        const float hWX = pixelsToUnits(bx + bw * 0.5f - screenW * 0.5f);
        const float tX  = unitsToPixels(hWX - pixelsToUnits(bw * 0.5f)) + 2.0f;
        const float tY  = unitsToPixels(hWY);
        m_textRenderer.RenderText("PAUSED",  tX, tY,          10.0f);
        m_textRenderer.RenderText("CLICK",   tX, tY - 12.0f,  10.0f);
        m_textRenderer.RenderText("2ND PT",  tX, tY - 24.0f,  10.0f);
    }
}