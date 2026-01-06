#include "AnimationPanel.h"

#include "Himii/Asset/AssetSerializer.h"
#include "Himii/Project/Project.h"
#include "Himii/Utils/PlatformUtils.h" // 假设你有 FileDialog

#include <filesystem>
#include <imgui.h>

namespace Himii
{

    AnimationPanel::AnimationPanel()
    {
        // 启动时创建一个空的动画，避免空指针
        CreateNewAnimation();
    }

    void AnimationPanel::OnImGuiRender(bool &isOpen)
    {
        if (!isOpen)
            return;

        ImGui::Begin("Sprite Animation Editor", &isOpen, ImGuiWindowFlags_MenuBar);

        RenderMenuBar();

        // 使用分栏：左边预览，右边/下边时间轴
        // 这里简化为：上方预览，下方帧列表

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, 300.0f); // 预览区域宽度

        RenderPreview();

        ImGui::NextColumn();

        RenderTimeline();

        ImGui::Columns(1);

        ImGui::End();
    }

    void AnimationPanel::RenderMenuBar()
    {
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New Animation"))
                    CreateNewAnimation();

                if (ImGui::MenuItem("Open Animation..."))
                {
                    std::string filepath = FileDialog::OpenFile("Sprite Animation (*.anim)\0*.anim\0");
                    if (!filepath.empty())
                        SetContext(filepath);
                }

                if (ImGui::MenuItem("Save", "Ctrl+S"))
                    SaveAnimation();

                if (ImGui::MenuItem("Save As..."))
                    SaveAnimationAs();

                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
    }

    void AnimationPanel::RenderPreview()
    {
        ImGui::Text("Preview");
        ImGui::Separator();

        // 播放控制
        if (ImGui::Button(m_IsPlaying ? "Stop" : "Play"))
        {
            m_IsPlaying = !m_IsPlaying;
        }
        ImGui::SameLine();
        ImGui::DragFloat("FPS", &m_FrameRate, 0.5f, 1.0f, 60.0f);

        // 预览逻辑更新
        if (m_CurrentAnimation && m_CurrentAnimation->GetFrameCount() > 0)
        {
            if (m_IsPlaying)
            {
                m_Timer += ImGui::GetIO().DeltaTime;
                if (m_Timer >= (1.0f / m_FrameRate))
                {
                    m_Timer = 0.0f;
                    m_PreviewFrameIndex = (m_PreviewFrameIndex + 1) % m_CurrentAnimation->GetFrameCount();
                }
            }
            else
            {
                // 如果暂停，显示选中的帧，或者第0帧
                if (m_SelectedFrameIndex >= 0 && m_SelectedFrameIndex < m_CurrentAnimation->GetFrameCount())
                    m_PreviewFrameIndex = m_SelectedFrameIndex;
            }

            // 获取当前帧的纹理
            AssetHandle handle = m_CurrentAnimation->GetFrame(m_PreviewFrameIndex);
            Ref<Texture2D> texture = GetTextureFromHandle(handle);

            if (texture)
            {
                // 计算保持比例的大小
                float regionWidth = ImGui::GetContentRegionAvail().x;
                float aspect = (float)texture->GetWidth() / (float)texture->GetHeight();
                float displayHeight = regionWidth / aspect;

                ImGui::Image((void *)(uint64_t)texture->GetRendererID(), {regionWidth, displayHeight}, {0, 1}, {1, 0});
            }
            else
            {
                ImGui::Text("Invalid Texture Handle: %llu", (uint64_t)handle);
            }
        }
        else
        {
            ImGui::Text("No frames in animation.");
        }
    }

  void AnimationPanel::RenderTimeline()
    {
        ImGui::Text("Timeline");
        ImGui::SameLine();
        if (ImGui::Button("Clear"))
        {
            if (m_CurrentAnimation)
            {
                // 创建一个新的空动画，相当于清空
                m_CurrentAnimation = std::make_shared<SpriteAnimation>();
                m_SelectedFrameIndex = -1;
            }
        }

        ImGui::Separator();

        // 开启子窗口，这里是滚动区域
        ImGui::BeginChild("FrameList", ImVec2(0, 0), true); // true 显示边框

        if (m_CurrentAnimation)
        {
            const auto &frames = m_CurrentAnimation->GetFrames();

            // 1. 渲染现有的帧
            for (int i = 0; i < frames.size(); i++)
            {
                AssetHandle handle = frames[i];
                Ref<Texture2D> texture = GetTextureFromHandle(handle);

                ImGui::PushID(i);

                // 缩略图大小
                ImVec2 size = {64, 64};

                // 选中高亮
                if (i == m_SelectedFrameIndex)
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 1, 0, 1)); // 黄色高亮
                }

                bool clicked = false;
                if (texture)
                {
                    // 显示图片按钮
                    if (ImGui::ImageButton("button",(uint64_t)texture->GetRendererID(), size, {0, 1}, {1, 0}))
                        clicked = true;
                }
                else
                {
                    // 如果图片加载失败，显示一个文本块
                    if (ImGui::Button("Null\nTex", size))
                        clicked = true;
                }

                if (i == m_SelectedFrameIndex)
                    ImGui::PopStyleColor();

                if (clicked)
                {
                    m_SelectedFrameIndex = i;
                    m_PreviewFrameIndex = i;
                    m_IsPlaying = false;
                }

                // 在图片下方显示帧索引，横向排列
                ImGui::SameLine();

                ImGui::PopID();
            }
        }

        // 2. 换行，防止最后的 Dummy 跟在最后一个图片后面
        ImGui::NewLine();

        // 3. 关键修复：创建一个填充剩余空间的 Dummy 控件
        // 这样即使列表是空的，或者图片很少，下方大片空白区域也能响应拖拽
        ImVec2 contentSize = ImGui::GetContentRegionAvail();
        if (contentSize.y < 50.0f)
            contentSize.y = 50.0f; // 保证至少有 50px 的高度

        ImGui::Dummy(contentSize); // 不可见的控件，用来接收拖拽

        // 4. 在 Dummy 上附加拖拽目标逻辑
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
            {
                const wchar_t *path = (const wchar_t *)payload->Data;
                std::filesystem::path assetPath = path;

                HIMII_CORE_INFO("Dropped file: {0}", assetPath.string()); // [DEBUG] 打印日志

                std::string ext = assetPath.extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
                if (ext == ".png" || ext == ".jpg" || ext == ".jpeg")
                {
                    auto assetManager = Project::GetAssetManager();
                    if (assetManager)
                    {
                        // 导入资源
                        AssetHandle handle = assetManager->ImportAsset(assetPath);
                        if (handle != 0)
                        {
                            m_CurrentAnimation->AddFrame(handle);
                            HIMII_CORE_INFO("Frame Added! Handle: {0}", (uint64_t)handle); // [DEBUG]
                        }
                        else
                        {
                            HIMII_CORE_ERROR("Failed to import asset, handle is 0");
                        }
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::EndChild();
    }

    void AnimationPanel::CreateNewAnimation()
    {
        m_CurrentAnimation = std::make_shared<SpriteAnimation>();
        m_CurrentFilePath = "";
        m_PreviewFrameIndex = 0;
        m_SelectedFrameIndex = -1;
        m_IsPlaying = false;
    }

    void AnimationPanel::SetContext(const std::filesystem::path &path)
    {
        Ref<SpriteAnimation> anim = SpriteAnimationSerializer::Deserialize(path);
        if (anim)
        {
            m_CurrentAnimation = anim;
            m_CurrentFilePath = path;
            m_PreviewFrameIndex = 0;
            m_SelectedFrameIndex = -1;
        }
    }

    void AnimationPanel::SaveAnimationAs()
    {
        std::string filepath = FileDialog::SaveFile("Sprite Animation (*.anim)\0*.anim\0");
        if (!filepath.empty())
        {
            m_CurrentFilePath = filepath;
            SaveAnimation();
        }
    }

    void AnimationPanel::SaveAnimation()
    {
        if (m_CurrentFilePath.empty())
        {
            SaveAnimationAs();
        }
        else
        {
            if (m_CurrentAnimation)
            {
                SpriteAnimationSerializer::Serialize(m_CurrentFilePath, m_CurrentAnimation);

                // 如果这个文件之前没有被 AssetManager 注册，这里应该触发一次导入
                // 但通常 Serializer 是直接写文件，AssetManager 下次启动或 Refresh 时会发现它
                // 也可以手动调用: Project::GetAssetManager()->ImportAsset(m_CurrentFilePath);
            }
        }
    }

    Ref<Texture2D> AnimationPanel::GetTextureFromHandle(AssetHandle handle)
    {
        if (handle == 0)
            return nullptr;

        auto assetManager = Project::GetAssetManager();
        if (assetManager && assetManager->IsAssetHandleValid(handle))
        {
            return std::static_pointer_cast<Texture2D>(assetManager->GetAsset(handle));
        }
        return nullptr;
    }
} // namespace Himii
