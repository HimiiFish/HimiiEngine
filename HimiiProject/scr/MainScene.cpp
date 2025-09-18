#include "MainScene.h"
#include "imgui.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace Himii
{
    MainScene::MainScene() :
        Layer("Example2D"), m_CameraController(1280.0f / 720.0f), m_SquareColor({0.2f, 0.38f, 0.64f, 1.0f})
    {
    }

    void MainScene::OnAttach()
    {
        HIMII_PROFILE_FUNCTION();

        // ��������֡���壬�ߴ����ô��ڴ�С�������� MainScene �����������
        auto &app = Application::Get();

        // ��С ECS ����������������ɫ����ʵ�壨ʹ�� Entity ��װ��
        {
            auto e1 = m_Scene.CreateEntity("Red Quad");
            auto &tr1 = e1.GetComponent<Transform>();
            tr1.Position = {-0.5f, -0.5f, 0.0f};
            e1.AddComponent<SpriteRenderer>(glm::vec4{0.9f, 0.2f, 0.3f, 1.0f});

            auto e2 = m_Scene.CreateEntity("Green Quad");
            auto &tr2 = e2.GetComponent<Transform>();
            tr2.Position = {0.3f, 0.1f, 0.0f};
            e2.AddComponent<SpriteRenderer>(glm::vec4{0.2f, 0.8f, 0.4f, 1.0f});

            auto e3 = m_Scene.CreateEntity("Blue Quad");
            auto &tr3 = e3.GetComponent<Himii::Transform>();
            tr3.Position = {-0.2f, 0.6f, 0.0f};
            e3.AddComponent<SpriteRenderer>(glm::vec4{0.3f, 0.5f, 1.0f, 1.0f});

            auto e4 = m_Scene.CreateEntity("My Quad");
            // Ĭ�Ϲ��� SpriteRenderer����ɫ����������ɫ
            e4.AddComponent<SpriteRenderer>(glm::vec4{1.0f, 1.0f, 1.0f, 1.0f});
            // �󶨶�ά�ƶ��ű�
            {
                auto &nsc = e4.AddComponent<NativeScriptComponent>();
            }
        }
    }
    void MainScene::OnDetach()
    {
        HIMII_PROFILE_FUNCTION();
    }

    void MainScene::OnUpdate(Timestep ts)
    {
        HIMII_PROFILE_FUNCTION();

        m_CameraController.OnUpdate(ts);

        RenderCommand::SetClearColor({0.1f, 0.12f, 0.16f, 1.0f});
        RenderCommand::Clear();

        HIMII_PROFILE_SCOPE("Renderer Draw");
        Renderer2D::BeginScene(m_CameraController.GetCamera());
        m_Scene.OnUpdate(ts);
        Renderer2D::EndScene();
    }
    void MainScene::OnImGuiRender()
    {
        HIMII_PROFILE_FUNCTION();

    }

    void MainScene::OnEvent(Himii::Event &event)
    {
    }
} // namespace Himii
