#include "MainScene.h"
#include "imgui.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"


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
// namespace Himii
