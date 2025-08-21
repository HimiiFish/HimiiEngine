#pragma once
// Engine.h: 引擎的核心头文件。
#include "Himii/Core/Application.h"
#include "Himii/Core/Input.h"
#include "Himii/Core/Log.h"
#include "Himii/Core/Layer.h"
#include "Himii/Core/LayerStack.h"
#include "Himii/Core/Window.h"
#include "Himii/ImGui/ImGuiLayer.h"
#include "Himii/Core/Core.h"
#include "Himii/Core/KeyCodes.h"
#include "Himii/Core/MouseCodes.h"
#include "Himii/Core/Timestep.h"

//---------Renderer相关-----------
#include "Himii/Renderer/Renderer.h"
#include "Himii/Renderer/Renderer2D.h"
#include "Himii/Renderer/RenderCommand.h"
#include "Himii/Renderer/Buffer.h"
#include "Himii/Renderer/VertexArray.h"
#include "Himii/Renderer/Shader.h"
#include "Himii/Renderer/GraphicsContext.h"
#include "Himii/Renderer/RendererAPI.h"
#include "Himii/Renderer/OrthographicCamera.h"
#include "Himii/Renderer/OrthographicCameraController.h"
#include "Himii/Renderer/Texture.h"
#include "Himii/Renderer/Framebuffer.h"

//---------其他-----------