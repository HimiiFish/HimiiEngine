#pragma once
// Engine.h: 引擎的核心头文件。
#include "EngineCore/Core/Application.h"
#include "EngineCore/Core/Input.h"
#include "EngineCore/Core/Log.h"
#include "EngineCore/Core/Layer.h"
#include "EngineCore/Core/LayerStack.h"
#include "EngineCore/Core/Window.h"
#include "EngineCore/Core/Core.h"
#include "EngineCore/Core/KeyCodes.h"
#include "EngineCore/Core/MouseCodes.h"
#include "EngineCore/Core/Timestep.h"

#include "EngineCore/ImGui/ImGuiLayer.h"

//---------Renderer相关-----------
#include "Module/Render/Renderer.h"
#include "Module/Render/Renderer2D.h"
#include "Module/Render/Camera.h"
#include "Module/Render/RenderCommand.h"
#include "Module/Render/Buffer.h"
#include "Module/Render/VertexArray.h"
#include "Module/Render/Shader.h"
#include "Module/Render/GraphicsContext.h"
#include "Module/Render/RendererAPI.h"
#include "Module/Render/OrthographicCamera.h"
#include "Module/Render/OrthographicCameraController.h"
#include "Module/Render/Texture.h"
#include "Module/Render/Framebuffer.h"
#include "Module/Render/Font.h"

//---------其他-----------
// ECS
#include "World/Scene/Components.h"
#include "World/Scene/Entity.h"
#include "World/Scene/Scene.h"
#include "World/Scene/SceneSerializer.h"
#include "World/Scene/ScriptableEntity.h"
#include "World/World.h"

#include "EngineCore/Math/Math.h"