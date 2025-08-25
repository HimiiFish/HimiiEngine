#include "EditorLayer.h"
using namespace Himii;
// 需要组件定义
#include "Himii/Scene/Components.h"
// for snprintf
#include <cstdio>

#include "CubeLayer.h" // include complete type for Setting panel access
// 脚本头文件（Inspector 用）
#include "TerrainScript.h"
#include "Move2DScript.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#pragma comment(lib, "Comdlg32.lib")

// 简单的本地文件打开对话框（返回 UTF-8 路径，失败返回空串）
static std::string OpenFileDialog(const wchar_t *filter)
{
    wchar_t fileBuffer[MAX_PATH] = L"";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = fileBuffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = filter; // 形如 L"Images\0*.png;*.jpg\0All\0*.*\0\0"
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameW(&ofn) == TRUE)
    {
        // 转 UTF-8
        int needed = WideCharToMultiByte(CP_UTF8, 0, fileBuffer, -1, nullptr, 0, nullptr, nullptr);
        if (needed > 0)
        {
            std::string utf8(needed - 1, '\0');
            WideCharToMultiByte(CP_UTF8, 0, fileBuffer, -1, utf8.data(), needed, nullptr, nullptr);
            return utf8;
        }
    }
    return {};
}
#endif

void EditorLayer::OnImGuiRender()
{
    static bool dockspaceOpen = true;
    static bool opt_fullscreen = true;
    static bool opt_padding = false;
    static bool s_ShowSettingWindow = true; // 菜单控制的独立窗口显隐

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    if (!opt_padding)
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("Himii Editor", &dockspaceOpen, window_flags);

    if (!opt_padding)
        ImGui::PopStyleVar();
    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    // Dockspace
    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("HIMII_DockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));
    }

    // Menu bar
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            ImGui::MenuItem("New");
            ImGui::MenuItem("Open...");
            ImGui::MenuItem("Save");
            ImGui::Separator();
            if (ImGui::MenuItem("Exit"))
                Himii::Application::Get().Close();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window"))
        {
            ImGui::MenuItem("Inspector", nullptr, true);
            ImGui::MenuItem("Hierarchy", nullptr, true);
            ImGui::MenuItem("Console", nullptr, true);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Setting"))
        {
            ImGui::MenuItem("Show Settings", nullptr, &s_ShowSettingWindow);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // Setting 独立窗口
    if (s_ShowSettingWindow)
    {
        if (ImGui::Begin("Setting", &s_ShowSettingWindow))
        {
            // 找到运行层以读取/设置参数
            CubeLayer *cube = nullptr;
            for (auto *layer : Himii::Application::Get().GetLayerStack())
            {
                cube = dynamic_cast<CubeLayer *>(layer);
                if (cube) break;
            }
            if (cube)
            {
                if (ImGui::CollapsingHeader("Editor Camera", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::DragFloat("Move Speed", &cube->m_MoveSpeed, 0.1f, 0.1f, 100.0f);
                    ImGui::DragFloat("Mouse Sensitivity", &cube->m_MouseSensitivity, 0.01f, 0.01f, 2.0f);
                    ImGui::DragFloat("FovY (deg)", &cube->m_FovYDeg, 0.1f, 10.0f, 120.0f);
                    ImGui::DragFloat("NearZ", &cube->m_NearZ, 0.01f, 0.01f, 10.0f);
                    ImGui::DragFloat("FarZ", &cube->m_FarZ, 1.0f, 0.1f, 5000.0f);
                    ImGui::DragFloat3("Position", &cube->m_CamPos.x, 0.05f);
                }
                if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::ColorEdit3("Ambient Color", &cube->m_AmbientColor.x);
                    ImGui::DragFloat("Ambient Intensity", &cube->m_AmbientIntensity, 0.01f, 0.0f, 5.0f);
                    ImGui::DragFloat3("Light Direction", &cube->m_LightDir.x, 0.01f);
                    ImGui::DragFloat("Light Intensity", &cube->m_LightIntensity, 0.01f, 0.0f, 10.0f);
                }
            }
            else
            {
                ImGui::TextDisabled("CubeLayer not found.");
            }
        }
        ImGui::End();
    }

    // Scene 视口（显示 FBO 颜色纹理）
    ImGui::Begin("Scene");
    {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        m_LastSceneAvail = avail;
        m_SceneHovered = ImGui::IsWindowHovered();
        m_SceneFocused = ImGui::IsWindowFocused();
        if (m_SceneTexture)
        {
            ImVec2 imagePos = ImGui::GetCursorScreenPos();
            m_SceneImageMin = imagePos;
            m_SceneImageSize = avail;
            ImGui::Image((ImTextureID)(intptr_t)m_SceneTexture, avail, ImVec2(0, 1), ImVec2(1, 0));
        }
        else
        {
            ImGui::TextDisabled("No Scene texture");
        }
    }
    ImGui::End();

    // Game 视口
    ImGui::Begin("Game");
    {
        ImVec2 gAvail = ImGui::GetContentRegionAvail();
        m_LastGameAvail = gAvail;
        m_GameHovered = ImGui::IsWindowHovered();
        m_GameFocused = ImGui::IsWindowFocused();
        if (m_GameTexture)
        {
            ImGui::Image((ImTextureID)(intptr_t)m_GameTexture, gAvail, ImVec2(0, 1), ImVec2(1, 0));
        }
        else
        {
            ImGui::TextDisabled("No Game texture");
        }
    }
    ImGui::End();

    // 根据 Scene/Game 的悬停状态，决定是否让 ImGui 层阻断底层输入
    // 目标：当鼠标在 Scene 上滚动时，让滚轮事件传到底层相机控制器
    auto &app = Himii::Application::Get();
    if (auto *imgui = app.GetImGuiLayer())
    {
        const bool wantBlock = !(m_SceneHovered || m_SceneFocused || m_GameHovered || m_GameFocused);
        imgui->BlockEvents(wantBlock);
    }

    // Hierarchy 面板：列出场景中的实体
    ImGui::Begin("Hierarchy");
    if (m_ActiveScene)
    {
        auto &reg = m_ActiveScene->Registry();
        auto view = reg.view<Transform>();
        for (auto e : view)
        {
            std::string label;
            if (auto *t = reg.try_get<Tag>(e))
                label = t->name.empty() ? std::string("Entity") : t->name;
            else
            {
                if (auto *id = reg.try_get<ID>(e))
                {
                    char tmp[64];
                    std::snprintf(tmp, sizeof(tmp), "Entity %llu", (unsigned long long)(uint64_t)id->id);
                    label = tmp;
                }
                else
                {
                    char tmp[64];
                    std::snprintf(tmp, sizeof(tmp), "Entity %u", (unsigned)e);
                    label = tmp;
                }
            }
            bool selected = (e == m_SelectedEntity);
            if (ImGui::Selectable(label.c_str(), selected))
                m_SelectedEntity = e;
        }
        if (ImGui::Button("Create Entity"))
        {
            auto e = m_ActiveScene->CreateEntity("Entity");
            e.AddComponent<SpriteRenderer>(glm::vec4{0.8f, 0.8f, 0.8f, 1.0f});
        }
        if (m_SelectedEntity != entt::null && !reg.valid(m_SelectedEntity))
            m_SelectedEntity = entt::null;
    }
    else
    {
        ImGui::TextUnformatted("No active Scene.");
    }
    ImGui::End();

    // Inspector 面板
    ImGui::Begin("Inspector");
    if (m_ActiveScene && m_SelectedEntity != entt::null && m_ActiveScene->Registry().valid(m_SelectedEntity))
    {
        auto &reg = m_ActiveScene->Registry();
        if (reg.any_of<Tag>(m_SelectedEntity))
        {
            auto &tag = reg.get<Tag>(m_SelectedEntity);
            char nameBuf[128];
            std::snprintf(nameBuf, sizeof(nameBuf), "%s", tag.name.c_str());
            if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
                tag.name = nameBuf;
        }
        if (reg.any_of<Transform>(m_SelectedEntity))
        {
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto &tr = reg.get<Transform>(m_SelectedEntity);
                ImGui::DragFloat3("Position", &tr.Position.x, 0.01f);
                ImGui::DragFloat3("Rotation", &tr.Rotation.x, 0.5f);
                ImGui::DragFloat3("Scale", &tr.Scale.x, 0.01f, 0.01f, 100.0f);
            }
        }
        if (reg.any_of<CameraComponent>(m_SelectedEntity))
        {
            if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto &cc = reg.get<CameraComponent>(m_SelectedEntity);
                ImGui::Checkbox("Primary", &cc.primary);
                int proj = (cc.projection == ProjectionType::Perspective) ? 0 : 1;
                if (ImGui::Combo("Projection", &proj, "Perspective\0Orthographic\0"))
                    cc.projection = proj == 0 ? ProjectionType::Perspective : ProjectionType::Orthographic;
                if (cc.projection == ProjectionType::Perspective)
                {
                    ImGui::DragFloat("FovY (deg)", &cc.fovYDeg, 0.1f, 10.0f, 120.0f);
                }
                ImGui::DragFloat("NearZ", &cc.nearZ, 0.01f, 0.01f, 10.0f);
                ImGui::DragFloat("FarZ", &cc.farZ, 1.0f, 0.1f, 5000.0f);
                ImGui::Checkbox("Use LookAt", &cc.useLookAt);
                ImGui::DragFloat3("LookAt Target", &cc.lookAtTarget.x, 0.05f);
                ImGui::DragFloat3("Up", &cc.up.x, 0.05f);
            }
        }
        if (reg.any_of<SpriteRenderer>(m_SelectedEntity))
        {
            if (ImGui::CollapsingHeader("SpriteRenderer", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto &sr = reg.get<SpriteRenderer>(m_SelectedEntity);
                ImGui::ColorEdit4("Color", &sr.color.x);
                ImGui::DragFloat("Tiling", &sr.tiling, 0.01f, 0.1f, 100.0f);

                ImGui::Separator();
                ImGui::TextUnformatted("Texture");
                static std::string s_LastTexturePath;
#ifdef _WIN32
                if (ImGui::Button("Select Texture..."))
                {
                    const wchar_t *filter =
                            L"Image Files\0*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.gif;*.ktx;*.dds\0All Files\0*.*\0\0";
                    std::string path = OpenFileDialog(filter);
                    if (!path.empty())
                    {
                        auto tex = Himii::Texture2D::Create(path);
                        if (tex)
                        {
                            sr.texture = tex;
                            s_LastTexturePath = path;
                        }
                    }
                }
#else
                ImGui::TextDisabled("(当前平台未实现文件对话框)");
#endif
                ImGui::SameLine();
                if (ImGui::Button("Clear"))
                {
                    sr.texture = {};
                }
                if (sr.texture)
                {
                    ImGui::Text("Size: %u x %u", sr.texture->GetWidth(), sr.texture->GetHeight());
                    if (!s_LastTexturePath.empty())
                        ImGui::TextWrapped("%s", s_LastTexturePath.c_str());
                }

                ImGui::Separator();
                ImGui::TextUnformatted("UV");
                if (ImGui::Button("Reset UV (0..1)"))
                {
                    sr.uvs[0] = {0.0f, 0.0f};
                    sr.uvs[1] = {1.0f, 0.0f};
                    sr.uvs[2] = {1.0f, 1.0f};
                    sr.uvs[3] = {0.0f, 1.0f};
                }

                ImGui::TextUnformatted("Grid UV (cols/rows/col/row)");
                static int cols = 1, rows = 1, col = 0, row = 0;
                static float padNorm = 0.0f;
                ImGui::InputInt("Cols", &cols);
                ImGui::SameLine();
                ImGui::InputInt("Rows", &rows);
                ImGui::InputInt("Col", &col);
                ImGui::SameLine();
                ImGui::InputInt("Row", &row);
                ImGui::DragFloat("Padding (norm)", &padNorm, 0.001f, 0.0f, 0.25f);
                bool canGrid = sr.texture && cols > 0 && rows > 0 && col >= 0 && row >= 0;
                if (ImGui::Button("Apply Grid UV") && canGrid)
                {
                    sr.uvs = sr.texture->GetUVFromGrid(col, row, cols, rows, padNorm);
                }
                if (!sr.texture)
                    ImGui::TextDisabled("(需要先加载纹理)");

                ImGui::Separator();
                ImGui::TextUnformatted("Pixels UV (min/max/padding px)");
                static float minX = 0, minY = 0, maxX = 0, maxY = 0, padX = 0, padY = 0;
                ImGui::InputFloat("MinX", &minX);
                ImGui::SameLine();
                ImGui::InputFloat("MinY", &minY);
                ImGui::InputFloat("MaxX", &maxX);
                ImGui::SameLine();
                ImGui::InputFloat("MaxY", &maxY);
                ImGui::InputFloat("PadX", &padX);
                ImGui::SameLine();
                ImGui::InputFloat("PadY", &padY);
                bool canPx = sr.texture && maxX > minX && maxY > minY;
                if (ImGui::Button("Apply Pixels UV") && canPx)
                {
                    sr.uvs = sr.texture->GetUVFromPixels({minX, minY}, {maxX, maxY}, {padX, padY});
                }

                ImGui::Separator();
                ImGui::TextUnformatted("Manual UV (0..1)");
                float uv0[2] = {sr.uvs[0].x, sr.uvs[0].y};
                float uv1[2] = {sr.uvs[1].x, sr.uvs[1].y};
                float uv2[2] = {sr.uvs[2].x, sr.uvs[2].y};
                float uv3[2] = {sr.uvs[3].x, sr.uvs[3].y};
                if (ImGui::InputFloat2("UV0", uv0)) sr.uvs[0] = {uv0[0], uv0[1]};
                if (ImGui::InputFloat2("UV1", uv1)) sr.uvs[1] = {uv1[0], uv1[1]};
                if (ImGui::InputFloat2("UV2", uv2)) sr.uvs[2] = {uv2[0], uv2[1]};
                if (ImGui::InputFloat2("UV3", uv3)) sr.uvs[3] = {uv3[0], uv3[1]};
            }
        }
        if (reg.any_of<MeshRenderer>(m_SelectedEntity))
        {
            if (ImGui::CollapsingHeader("MeshRenderer", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto &mr = reg.get<MeshRenderer>(m_SelectedEntity);
                ImGui::Text("VAO: %p", (void *)mr.vertexArray.get());
                ImGui::Text("Shader: %p", (void *)mr.shader.get());
                ImGui::Text("Texture: %p", mr.texture ? (void *)mr.texture.get() : (void *)nullptr);
                if (mr.texture)
                    ImGui::Text("Tex Size: %u x %u", mr.texture->GetWidth(), mr.texture->GetHeight());
                ImGui::TextDisabled("(Mesh content is built in CubeLayer and bound here)");
            }
        }
        if (reg.any_of<SkyboxTag>(m_SelectedEntity))
        {
            ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "[Skybox]");
        }

        // 脚本（NativeScriptComponent）检查与参数编辑
        if (reg.any_of<Himii::NativeScriptComponent>(m_SelectedEntity))
        {
            auto &nsc = reg.get<Himii::NativeScriptComponent>(m_SelectedEntity);
            if (nsc.Instance)
            {
                if (auto *ts = dynamic_cast<TerrainScript *>(nsc.Instance))
                {
                    if (ImGui::CollapsingHeader("TerrainScript", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        bool sizeChanged = false;
                        sizeChanged |= ImGui::InputInt("Width", &ts->Width);
                        sizeChanged |= ImGui::InputInt("Depth", &ts->Depth);
                        sizeChanged |= ImGui::InputInt("Height", &ts->Height);
                        if (sizeChanged) ts->Dirty = true;

                        ImGui::Separator();
                        ImGui::TextDisabled("Noise Settings");
                        bool nChanged = false;
                        nChanged |= ImGui::InputScalar("Seed", ImGuiDataType_U32, &ts->Noise.seed);
                        nChanged |= ImGui::DragFloat("Biome Scale", &ts->Noise.biomeScale, 0.001f, 0.0001f, 1.0f);
                        nChanged |= ImGui::DragFloat("Continent Scale", &ts->Noise.continentScale, 0.001f, 0.0001f, 1.0f);
                        nChanged |= ImGui::DragFloat("Continent Strength", &ts->Noise.continentStrength, 0.001f, 0.0f, 2.0f);
                        nChanged |= ImGui::DragFloat("Plains Scale", &ts->Noise.plainsScale, 0.001f, 0.0001f, 2.0f);
                        nChanged |= ImGui::InputInt("Plains Octaves", &ts->Noise.plainsOctaves);
                        nChanged |= ImGui::DragFloat("Plains Lacunarity", &ts->Noise.plainsLacunarity, 0.01f, 1.0f, 4.0f);
                        nChanged |= ImGui::DragFloat("Plains Gain", &ts->Noise.plainsGain, 0.01f, 0.0f, 1.0f);
                        nChanged |= ImGui::DragFloat("Mountain Scale", &ts->Noise.mountainScale, 0.001f, 0.0001f, 2.0f);
                        nChanged |= ImGui::InputInt("Mountain Octaves", &ts->Noise.mountainOctaves);
                        nChanged |= ImGui::DragFloat("Mountain Lacunarity", &ts->Noise.mountainLacunarity, 0.01f, 1.0f, 4.0f);
                        nChanged |= ImGui::DragFloat("Mountain Gain", &ts->Noise.mountainGain, 0.01f, 0.0f, 1.0f);
                        nChanged |= ImGui::DragFloat("Ridge Sharpness", &ts->Noise.ridgeSharpness, 0.01f, 0.1f, 4.0f);
                        nChanged |= ImGui::DragFloat("Warp Scale", &ts->Noise.warpScale, 0.001f, 0.0f, 2.0f);
                        nChanged |= ImGui::DragFloat("Warp Amp", &ts->Noise.warpAmp, 0.01f, 0.0f, 10.0f);
                        nChanged |= ImGui::DragFloat("Detail Scale", &ts->Noise.detailScale, 0.001f, 0.0f, 2.0f);
                        nChanged |= ImGui::DragFloat("Detail Amp", &ts->Noise.detailAmp, 0.001f, 0.0f, 1.0f);
                        nChanged |= ImGui::DragFloat("Height Mul", &ts->Noise.heightMul, 0.01f, 0.0f, 4.0f);
                        nChanged |= ImGui::DragFloat("Plateau", &ts->Noise.plateau, 0.01f, 0.0f, 1.0f);
                        nChanged |= ImGui::InputInt("Step Levels", &ts->Noise.stepLevels);
                        nChanged |= ImGui::DragFloat("Curve Exponent", &ts->Noise.curveExponent, 0.01f, 0.1f, 5.0f);
                        nChanged |= ImGui::DragFloat("Valley Depth", &ts->Noise.valleyDepth, 0.001f, 0.0f, 1.0f);
                        nChanged |= ImGui::DragFloat("Sea Level", &ts->Noise.seaLevel, 0.001f, 0.0f, 1.0f);
                        nChanged |= ImGui::DragFloat("Mountain Weight", &ts->Noise.mountainWeight, 0.001f, 0.0f, 1.0f);
                        if (nChanged) ts->Dirty = true;

                        if (ImGui::Button("Mark Rebuild")) ts->Dirty = true;
                        ImGui::SameLine();
                        ImGui::TextDisabled(ts->Dirty ? "Dirty" : "Clean");
                    }
                }
                if (auto *mv = dynamic_cast<Move2DScript *>(nsc.Instance))
                {
                    if (ImGui::CollapsingHeader("Move2DScript", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::DragFloat("Speed", &mv->Speed, 0.01f, 0.0f, 10.0f);
                    }
                }
            }
            else
            {
                ImGui::TextDisabled("NativeScriptComponent present, but no Instance (not yet instantiated).\nIt will instantiate on Scene update.");
            }
        }

        if (ImGui::Button("Delete Entity"))
        {
            m_ActiveScene->DestroyEntity(m_SelectedEntity);
            m_SelectedEntity = entt::null;
        }
    }
    else
    {
        ImGui::TextUnformatted("Select an entity in Hierarchy.");
    }
    ImGui::End();

    ImGui::Begin("Console");
    ImGui::TextUnformatted("Logs appear here.");
    ImGui::End();

    ImGui::End(); // 结束主编辑器窗口
}
// 可选：供外部查询 Scene 面板的期望尺寸
// 我们用 width/height 字段让运行层设置与读取
bool EditorLayer::GetSceneMousePixel(uint32_t texW, uint32_t texH, uint32_t &outX, uint32_t &outY) const
{
    if (!m_SceneTexture || m_SceneImageSize.x <= 0 || m_SceneImageSize.y <= 0)
        return false;
    ImVec2 mouse = ImGui::GetMousePos();
    float localX = mouse.x - m_SceneImageMin.x;
    float localY = mouse.y - m_SceneImageMin.y;
    if (localX < 0 || localY < 0 || localX >= m_SceneImageSize.x || localY >= m_SceneImageSize.y)
        return false;
    // ImGui image flipped vertically; map Y accordingly
    float u = localX / m_SceneImageSize.x;
    float v = 1.0f - (localY / m_SceneImageSize.y);
    outX = (uint32_t)glm::clamp(u * texW, 0.0f, (float)texW - 1.0f);
    outY = (uint32_t)glm::clamp(v * texH, 0.0f, (float)texH - 1.0f);
    return true;
}
