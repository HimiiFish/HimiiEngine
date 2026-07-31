#include "World/Scene/Scene.h"
#include "World/Scene/Entity.h"
#include "Hepch.h"
#include "World/Scene/Components.h"
#include "World/Scene/SceneInternal.h"
#include "Module/Render/Renderer2D.h"
#include "Module/Render/RenderCommand.h"
#include "Module/Render/Font.h"
#include "Module/Script/ScriptEngine.h"
#include "EngineCore/Core/Log.h"

#include <cmath>
#include <functional>
#include <unordered_map>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>

namespace Himii
{
    void Scene::SetUserInterfacePointerInput(const UserInterfacePointerFrameInput &input)
    {
        m_UserInterfacePointerInput = input;
    }

    void Scene::ClearUserInterfacePointerTransientState()
    {
        auto buttonView = m_Registry.view<UIButtonComponent>();
        for (entt::entity entityHandle : buttonView)
        {
            auto& button = buttonView.get<UIButtonComponent>(entityHandle);
            button.WasClickedThisFrame = false;
        }
    }

    bool Scene::IsPointInsideResolvedRect(
            const ResolvedRectTransform &resolvedRectTransform,
            const glm::mat4 &designToTargetMatrix,
            const glm::vec2 &pointerInTargetPixels) const
    {
        if (!resolvedRectTransform.Valid)
            return false;

        const glm::mat4 drawTransform = designToTargetMatrix
                * resolvedRectTransform.WorldTransform
                * glm::scale(glm::mat4(1.0f), glm::vec3(resolvedRectTransform.Size, 1.0f));
        const glm::mat4 inverseDrawTransform = glm::inverse(drawTransform);
        const glm::vec4 localPoint = inverseDrawTransform
                * glm::vec4(pointerInTargetPixels.x, pointerInTargetPixels.y, 0.0f, 1.0f);
        return std::abs(localPoint.x) <= 0.5f && std::abs(localPoint.y) <= 0.5f;
    }

    Entity Scene::HitTestUserInterfaceButton(
            float targetWidth, float targetHeight, const glm::vec2 &pointerInTargetPixels) const
    {
        Entity canvasEntity = FindCanvasEntity();
        if (!canvasEntity)
            return {};

        const glm::mat4 designToTargetMatrix = GetCanvasToScreenMatrix(targetWidth, targetHeight);
        std::vector<Entity> buttonsInDrawOrder;

        std::function<void(Entity)> collectSubtree;
        collectSubtree = [&](Entity entity)
        {
            if (!entity || !entity.HasComponent<RectTransformComponent>())
                return;

            if (entity.HasComponent<UIButtonComponent>()
                && entity.GetComponent<UIButtonComponent>().Interactable)
            {
                buttonsInDrawOrder.push_back(entity);
            }

            for (UUID childIdentifier : GetEntityChildren(entity))
                collectSubtree(GetEntityByUUID(childIdentifier));
        };

        for (UUID childIdentifier : GetEntityChildren(canvasEntity))
            collectSubtree(GetEntityByUUID(childIdentifier));

        for (auto iterator = buttonsInDrawOrder.rbegin(); iterator != buttonsInDrawOrder.rend();
             ++iterator)
        {
            Entity entity = *iterator;
            const ResolvedRectTransform resolvedRectTransform =
                    ResolveRectTransform(entity, targetWidth, targetHeight);
            if (IsPointInsideResolvedRect(
                        resolvedRectTransform, designToTargetMatrix, pointerInTargetPixels))
                return entity;
        }

        return {};
    }

    void Scene::ProcessUserInterfacePointer()
    {
        m_UserInterfacePointerHandled = false;
        ClearUserInterfacePointerTransientState();

        const float targetWidth = static_cast<float>(m_ViewportWidth);
        const float targetHeight = static_cast<float>(m_ViewportHeight);

        if (!m_UserInterfacePointerInput.Enabled || targetWidth <= 0.0f || targetHeight <= 0.0f)
        {
            if (m_UserInterfaceHoverEntityIdentifier != 0)
            {
                Entity previousHover = GetEntityByUUID(m_UserInterfaceHoverEntityIdentifier);
                if (previousHover && previousHover.HasComponent<UIButtonComponent>())
                {
                    previousHover.GetComponent<UIButtonComponent>().IsPointerInside = false;
                    ScriptEngine::OnPointerEvent(previousHover, ScriptEngine::ScriptPointerEventType::Exit);
                }
                m_UserInterfaceHoverEntityIdentifier = 0;
            }
            if (m_UserInterfacePressedEntityIdentifier != 0)
            {
                Entity previousPressed = GetEntityByUUID(m_UserInterfacePressedEntityIdentifier);
                if (previousPressed && previousPressed.HasComponent<UIButtonComponent>())
                {
                    previousPressed.GetComponent<UIButtonComponent>().IsPressed = false;
                    ScriptEngine::OnPointerEvent(previousPressed, ScriptEngine::ScriptPointerEventType::Up);
                }
                m_UserInterfacePressedEntityIdentifier = 0;
            }
            return;
        }

        Entity hitEntity{};
        if (m_UserInterfacePointerInput.HasPosition)
        {
            hitEntity = HitTestUserInterfaceButton(
                    targetWidth, targetHeight, m_UserInterfacePointerInput.PositionInTargetPixels);
        }

        const UUID hitIdentifier = hitEntity ? hitEntity.GetUUID() : UUID(0);
        if (hitEntity)
            m_UserInterfacePointerHandled = true;

        // Hover enter/exit
        if (hitIdentifier != m_UserInterfaceHoverEntityIdentifier)
        {
            if (m_UserInterfaceHoverEntityIdentifier != 0)
            {
                Entity previousHover = GetEntityByUUID(m_UserInterfaceHoverEntityIdentifier);
                if (previousHover && previousHover.HasComponent<UIButtonComponent>())
                {
                    previousHover.GetComponent<UIButtonComponent>().IsPointerInside = false;
                    ScriptEngine::OnPointerEvent(previousHover, ScriptEngine::ScriptPointerEventType::Exit);
                }
            }
            m_UserInterfaceHoverEntityIdentifier = hitIdentifier;
            if (hitEntity && hitEntity.HasComponent<UIButtonComponent>())
            {
                hitEntity.GetComponent<UIButtonComponent>().IsPointerInside = true;
                ScriptEngine::OnPointerEvent(hitEntity, ScriptEngine::ScriptPointerEventType::Enter);
            }
        }
        else if (hitEntity && hitEntity.HasComponent<UIButtonComponent>())
        {
            hitEntity.GetComponent<UIButtonComponent>().IsPointerInside = true;
        }

        // Pressed visual: keep pressed on original target even if pointer leaves.
        if (m_UserInterfacePressedEntityIdentifier != 0)
        {
            Entity pressedEntity = GetEntityByUUID(m_UserInterfacePressedEntityIdentifier);
            if (pressedEntity && pressedEntity.HasComponent<UIButtonComponent>())
                pressedEntity.GetComponent<UIButtonComponent>().IsPressed = true;
        }

        if (m_UserInterfacePointerInput.PrimaryButtonPressedThisFrame && hitEntity
            && hitEntity.HasComponent<UIButtonComponent>())
        {
            m_UserInterfacePressedEntityIdentifier = hitIdentifier;
            hitEntity.GetComponent<UIButtonComponent>().IsPressed = true;
            ScriptEngine::OnPointerEvent(hitEntity, ScriptEngine::ScriptPointerEventType::Down);
        }

        if (m_UserInterfacePointerInput.PrimaryButtonReleasedThisFrame)
        {
            Entity pressedEntity = GetEntityByUUID(m_UserInterfacePressedEntityIdentifier);
            if (pressedEntity && pressedEntity.HasComponent<UIButtonComponent>())
            {
                auto& pressedButton = pressedEntity.GetComponent<UIButtonComponent>();
                pressedButton.IsPressed = false;
                ScriptEngine::OnPointerEvent(pressedEntity, ScriptEngine::ScriptPointerEventType::Up);
                if (hitIdentifier == m_UserInterfacePressedEntityIdentifier)
                {
                    pressedButton.WasClickedThisFrame = true;
                    ScriptEngine::OnPointerEvent(pressedEntity, ScriptEngine::ScriptPointerEventType::Click);
                }
            }
            m_UserInterfacePressedEntityIdentifier = 0;
        }
    }

    void Scene::RenderUI(float viewportWidth, float viewportHeight)
    {
        if (viewportWidth == 0.0f || viewportHeight == 0.0f)
            return;

        Entity canvasEntity = FindCanvasEntity();
        if (!canvasEntity)
            return;

        const glm::mat4 projection = glm::ortho(0.0f, viewportWidth, 0.0f, viewportHeight, -1.0f, 1.0f);
        const glm::mat4 view = glm::mat4(1.0f);
        const glm::mat4 canvasToScreen = GetCanvasToScreenMatrix(viewportWidth, viewportHeight);

        PrepareUserInterfaceFonts();
        RenderCommand::SetDepthTest(false);
        Renderer2D::BeginScene(projection, view);
        RenderUIElements(canvasToScreen, viewportWidth, viewportHeight);
        Renderer2D::EndScene();
        RenderCommand::SetDepthTest(true);
    }

    void Scene::PrepareUserInterfaceFonts()
    {
        struct FontTextCollection
        {
            Ref<Font> FontAsset;
            std::string CombinedText;
        };

        std::unordered_map<Font*, FontTextCollection> textByFont;
        auto textView = m_Registry.view<RectTransformComponent, UITextComponent>();
        for (entt::entity entityHandle : textView)
        {
            Entity entity{entityHandle, this};
            if (!IsEntityUnderCanvas(entity))
                continue;
            const auto& text = entity.GetComponent<UITextComponent>();
            Ref<Font> fontAsset = text.FontAsset ? text.FontAsset : Font::GetDefault();
            if (!fontAsset)
                continue;

            FontTextCollection& collection = textByFont[fontAsset.get()];
            collection.FontAsset = fontAsset;
            collection.CombinedText += text.TextString;
        }

        for (auto& [fontPointer, collection] : textByFont)
        {
            (void)fontPointer;
            collection.FontAsset->EnsureGlyphsForText(collection.CombinedText);
        }
    }

    void Scene::RenderUIElements(
            const glm::mat4& designToTargetMatrix, float targetWidth, float targetHeight)
    {
        Entity canvasEntity = FindCanvasEntity();
        if (!canvasEntity)
            return;

        enum class UserInterfacePrimitiveType
        {
            None = 0,
            Image,
            Text
        };
        UserInterfacePrimitiveType lastPrimitiveType = UserInterfacePrimitiveType::None;
        const auto flushIfPrimitiveTypeChanged =
                [&](UserInterfacePrimitiveType nextPrimitiveType)
        {
            if (lastPrimitiveType != UserInterfacePrimitiveType::None
                && lastPrimitiveType != nextPrimitiveType)
            {
                // 有序命令流：相邻不同类型批次强制分隔，保持 SiblingIndex 顺序。
                Renderer2D::FlushCurrentBatch();
            }
            lastPrimitiveType = nextPrimitiveType;
        };

        std::function<void(Entity)> drawUserInterfaceSubtree;
        drawUserInterfaceSubtree = [&](Entity entity)
        {
            if (!entity || !entity.HasComponent<RectTransformComponent>())
                return;

            const ResolvedRectTransform resolvedRectTransform =
                    ResolveRectTransform(entity, targetWidth, targetHeight);
            if (!resolvedRectTransform.Valid)
                return;

            if (entity.HasComponent<UIImageComponent>())
            {
                flushIfPrimitiveTypeChanged(UserInterfacePrimitiveType::Image);
                auto& image = entity.GetComponent<UIImageComponent>();
                const glm::mat4 drawTransform = designToTargetMatrix
                        * resolvedRectTransform.WorldTransform
                        * glm::scale(glm::mat4(1.0f),
                                     glm::vec3(resolvedRectTransform.Size, 1.0f));

                glm::vec4 drawColor = image.Color;
                if (entity.HasComponent<UIButtonComponent>())
                    drawColor = entity.GetComponent<UIButtonComponent>().EvaluateEffectiveColor(
                            image.Color);

                if (image.Texture)
                    Renderer2D::DrawQuad(
                            drawTransform, image.Texture, 1.0f, drawColor, (int)(entt::entity)entity);
                else
                    Renderer2D::DrawQuad(drawTransform, drawColor, (int)(entt::entity)entity);
            }

            if (entity.HasComponent<UITextComponent>())
            {
                flushIfPrimitiveTypeChanged(UserInterfacePrimitiveType::Text);
                auto& text = entity.GetComponent<UITextComponent>();
                Ref<Font> fontAsset = text.FontAsset ? text.FontAsset : Font::GetDefault();
                if (fontAsset)
                {
                    fontAsset->ProcessCompletedGenerations();
                    const glm::mat4 drawTransform = designToTargetMatrix
                            * resolvedRectTransform.WorldTransform;
                    TextLayoutSettings layoutSettings;
                    layoutSettings.RectangleSize = resolvedRectTransform.Size;
                    layoutSettings.FontSize = std::max(text.FontSize, 1.0f);
                    layoutSettings.Kerning = text.Kerning;
                    layoutSettings.LineSpacing = text.LineSpacing;
                    layoutSettings.HorizontalAlignment = text.HorizontalAlignment;
                    layoutSettings.VerticalAlignment = text.VerticalAlignment;
                    Renderer2D::DrawStringInRectangle(
                            text.TextString, fontAsset, drawTransform,
                            layoutSettings, text.Color,
                            (int)(entt::entity)entity);
                }
            }

            for (UUID childIdentifier : GetEntityChildren(entity))
                drawUserInterfaceSubtree(GetEntityByUUID(childIdentifier));
        };

        for (UUID childIdentifier : GetEntityChildren(canvasEntity))
            drawUserInterfaceSubtree(GetEntityByUUID(childIdentifier));
    }

    Scene::OrthographicViewBounds Scene::GetPrimaryOrthographicViewBounds() const
    {
        OrthographicViewBounds bounds;
        Entity cameraEntity = const_cast<Scene*>(this)->GetPrimaryCameraEntity();
        if (!cameraEntity || !cameraEntity.HasComponent<CameraComponent>()
            || !cameraEntity.HasComponent<TransformComponent>())
            return bounds;

        auto& cameraComponent = cameraEntity.GetComponent<CameraComponent>();
        if (cameraComponent.Camera.GetProjectionType() != SceneCamera::ProjectionType::Orthographic)
            return bounds;

        const float orthographicSize = cameraComponent.Camera.GetOrthographicSize();
        const float aspectRatio = (m_ViewportHeight > 0)
                                         ? static_cast<float>(m_ViewportWidth) / static_cast<float>(m_ViewportHeight)
                                         : 1.0f;

        bounds.Valid = true;
        bounds.CameraWorldMatrix = GetEntityWorldTransformMatrix(cameraEntity);
        bounds.Left = -orthographicSize * aspectRatio * 0.5f;
        bounds.Right = orthographicSize * aspectRatio * 0.5f;
        bounds.Bottom = -orthographicSize * 0.5f;
        bounds.Top = orthographicSize * 0.5f;
        return bounds;
    }

    Scene::DesignFrameWorldBounds Scene::GetDesignFrameWorldBounds() const
    {
        DesignFrameWorldBounds designBounds;
        Entity canvasEntity = FindCanvasEntity();
        if (!canvasEntity || !canvasEntity.HasComponent<CanvasComponent>())
            return designBounds;

        const float targetWidth = std::max(static_cast<float>(m_ViewportWidth), 1.0f);
        const float targetHeight = std::max(static_cast<float>(m_ViewportHeight), 1.0f);
        const CanvasLayoutContext layoutContext =
                GetCanvasLayoutContext(targetWidth, targetHeight);
        if (!layoutContext.Valid)
            return designBounds;

        float cameraWidth = targetWidth * 0.01f;
        float cameraHeight = targetHeight * 0.01f;
        OrthographicViewBounds cameraBounds = GetPrimaryOrthographicViewBounds();
        if (cameraBounds.Valid)
        {
            cameraWidth = cameraBounds.Right - cameraBounds.Left;
            cameraHeight = cameraBounds.Top - cameraBounds.Bottom;
        }

        const float targetAspectRatio = targetWidth / targetHeight;
        float frameWidth = cameraWidth;
        float frameHeight = frameWidth / targetAspectRatio;
        if (frameHeight > cameraHeight)
        {
            frameHeight = cameraHeight;
            frameWidth = frameHeight * targetAspectRatio;
        }
        const float designToWorldScale =
                frameWidth / std::max(layoutContext.LogicalSize.x, 1.0e-5f);

        designBounds.Valid = true;
        designBounds.DesignToWorldScale = designToWorldScale;
        designBounds.HalfWidth = frameWidth * 0.5f;
        designBounds.HalfHeight = frameHeight * 0.5f;
        return designBounds;
    }

    glm::mat4 Scene::GetDesignToWorldMatrix() const
    {
        DesignFrameWorldBounds designBounds = GetDesignFrameWorldBounds();
        if (!designBounds.Valid)
            return glm::mat4(1.0f);

        // 设计中心原点 (0,0) → 世界原点；XY 平面 z=0。
        return glm::scale(glm::mat4(1.0f),
                          glm::vec3(designBounds.DesignToWorldScale, designBounds.DesignToWorldScale, 1.0f));
    }

    void Scene::DrawPrimaryCameraBounds(const glm::vec4& color) const
    {
        OrthographicViewBounds bounds = GetPrimaryOrthographicViewBounds();
        if (!bounds.Valid)
            return;

        // 画在 z=0 玩法平面，避免主相机在 z=10 时 Edit 拉近后框落到相机背后消失。
        const glm::vec3 cameraTranslation = glm::vec3(bounds.CameraWorldMatrix[3]);
        const glm::vec3 bottomLeft{cameraTranslation.x + bounds.Left, cameraTranslation.y + bounds.Bottom, 0.0f};
        const glm::vec3 bottomRight{cameraTranslation.x + bounds.Right, cameraTranslation.y + bounds.Bottom, 0.0f};
        const glm::vec3 topRight{cameraTranslation.x + bounds.Right, cameraTranslation.y + bounds.Top, 0.0f};
        const glm::vec3 topLeft{cameraTranslation.x + bounds.Left, cameraTranslation.y + bounds.Top, 0.0f};

        Renderer2D::DrawLine(bottomLeft, bottomRight, color);
        Renderer2D::DrawLine(bottomRight, topRight, color);
        Renderer2D::DrawLine(topRight, topLeft, color);
        Renderer2D::DrawLine(topLeft, bottomLeft, color);
    }

    void Scene::DrawCanvasDesignBounds(const glm::vec4& color) const
    {
        DesignFrameWorldBounds designBounds = GetDesignFrameWorldBounds();
        if (!designBounds.Valid)
            return;

        const glm::vec3 bottomLeft{-designBounds.HalfWidth, -designBounds.HalfHeight, 0.0f};
        const glm::vec3 bottomRight{designBounds.HalfWidth, -designBounds.HalfHeight, 0.0f};
        const glm::vec3 topRight{designBounds.HalfWidth, designBounds.HalfHeight, 0.0f};
        const glm::vec3 topLeft{-designBounds.HalfWidth, designBounds.HalfHeight, 0.0f};

        Renderer2D::DrawLine(bottomLeft, bottomRight, color);
        Renderer2D::DrawLine(bottomRight, topRight, color);
        Renderer2D::DrawLine(topRight, topLeft, color);
        Renderer2D::DrawLine(topLeft, bottomLeft, color);
    }

    void Scene::RenderUIInEditor(EditorCamera& editorCamera, bool drawUserInterfaceContent)
    {
        if (drawUserInterfaceContent)
            PrepareUserInterfaceFonts();
        RenderCommand::SetDepthTest(false);
        Renderer2D::BeginScene(editorCamera);

        DrawPrimaryCameraBounds(glm::vec4(0.95f, 0.85f, 0.2f, 0.9f));
        DrawCanvasDesignBounds(glm::vec4(0.35f, 0.85f, 0.95f, 0.9f));

        if (drawUserInterfaceContent && FindCanvasEntity())
            RenderUIElements(
                    GetDesignToWorldMatrix(),
                    static_cast<float>(m_ViewportWidth),
                    static_cast<float>(m_ViewportHeight));

        Renderer2D::EndScene();
        RenderCommand::SetDepthTest(true);
    }

    bool Scene::IsWorldPositionInsideDesignFrame(const glm::vec2& worldPosition) const
    {
        DesignFrameWorldBounds designBounds = GetDesignFrameWorldBounds();
        if (!designBounds.Valid)
            return false;

        return worldPosition.x >= -designBounds.HalfWidth && worldPosition.x <= designBounds.HalfWidth
               && worldPosition.y >= -designBounds.HalfHeight && worldPosition.y <= designBounds.HalfHeight;
    }

    Entity Scene::FindCanvasEntity() const
    {
        auto canvasView = m_Registry.view<CanvasComponent>();
        for (auto entityHandle : canvasView)
            return Entity{entityHandle, const_cast<Scene*>(this)};
        return {};
    }

    bool Scene::IsEntityUnderCanvas(Entity entity) const
    {
        Entity canvasEntity = FindCanvasEntity();
        if (!entity || !canvasEntity)
            return false;
        if (entity == canvasEntity)
            return true;
        return IsEntityDescendantOf(entity, canvasEntity);
    }

    Scene::CanvasLayoutContext Scene::GetCanvasLayoutContext(
            float targetWidth, float targetHeight) const
    {
        CanvasLayoutContext context;
        Entity canvasEntity = FindCanvasEntity();
        if (!canvasEntity || targetWidth <= 0.0f || targetHeight <= 0.0f)
            return context;

        const auto& canvas = canvasEntity.GetComponent<CanvasComponent>();
        context.Valid = true;
        context.TargetSize = {targetWidth, targetHeight};
        context.ScaleFactor = canvas.ScaleMode == CanvasScaleMode::ConstantPixelSize
                                      ? 1.0f
                                      : ComputeCanvasScaleFactor(targetWidth, targetHeight);
        context.LogicalSize = context.TargetSize / std::max(context.ScaleFactor, 1.0e-5f);
        return context;
    }

    Scene::ResolvedRectTransform Scene::ResolveRectTransform(
            Entity entity, float targetWidth, float targetHeight) const
    {
        ResolvedRectTransform result;
        if (!entity || !entity.HasComponent<RectTransformComponent>() || !IsEntityUnderCanvas(entity))
            return result;

        const CanvasLayoutContext context = GetCanvasLayoutContext(targetWidth, targetHeight);
        if (!context.Valid)
            return result;

        Entity canvasEntity = FindCanvasEntity();
        if (entity == canvasEntity)
        {
            result.Valid = true;
            result.Size = context.LogicalSize;
            result.WorldTransform = glm::mat4(1.0f);
            auto& rectTransform = entity.GetComponent<RectTransformComponent>();
            rectTransform.ResolvedSize = result.Size;
            rectTransform.CachedWorldTransform = result.WorldTransform;
            rectTransform.WorldTransformDirty = false;
            return result;
        }

        Entity parentEntity = GetParentEntity(entity);
        if (!parentEntity || !parentEntity.HasComponent<RectTransformComponent>())
            return result;

        const ResolvedRectTransform parentResult =
                ResolveRectTransform(parentEntity, targetWidth, targetHeight);
        if (!parentResult.Valid)
            return result;

        auto& rectTransform = entity.GetComponent<RectTransformComponent>();
        const glm::vec2 anchorMinimum = glm::clamp(
                rectTransform.AnchorMinimum, glm::vec2(0.0f), glm::vec2(1.0f));
        const glm::vec2 anchorMaximum = glm::clamp(
                rectTransform.AnchorMaximum, anchorMinimum, glm::vec2(1.0f));
        const glm::vec2 pivot = glm::clamp(rectTransform.Pivot, glm::vec2(0.0f), glm::vec2(1.0f));

        const glm::vec2 anchorRange = anchorMaximum - anchorMinimum;
        const glm::vec2 resolvedSize =
                glm::max(parentResult.Size * anchorRange + rectTransform.SizeDelta, glm::vec2(0.01f));
        const glm::vec2 anchorReference =
                -parentResult.Size * 0.5f
                + parentResult.Size
                          * (anchorMinimum + anchorRange * pivot);
        const glm::vec2 pivotPosition = anchorReference + rectTransform.AnchoredPosition;
        const glm::vec2 rectCenter = pivotPosition + (glm::vec2(0.5f) - pivot) * resolvedSize;

        const glm::mat4 localTransform =
                glm::translate(glm::mat4(1.0f), glm::vec3(rectCenter, 0.0f))
                * glm::rotate(
                        glm::mat4(1.0f), rectTransform.RotationRadians,
                        glm::vec3(0.0f, 0.0f, 1.0f));

        result.Valid = true;
        result.Size = resolvedSize;
        result.WorldTransform = parentResult.WorldTransform * localTransform;
        rectTransform.ResolvedSize = resolvedSize;
        rectTransform.CachedWorldTransform = result.WorldTransform;
        rectTransform.WorldTransformDirty = false;
        return result;
    }

    float Scene::ComputeCanvasScaleFactor(float viewportWidth, float viewportHeight) const
    {
        Entity canvasEntity = FindCanvasEntity();
        if (!canvasEntity)
            return 1.0f;

        const auto& canvas = canvasEntity.GetComponent<CanvasComponent>();
        const float referenceWidth = std::max(canvas.ReferenceResolution.x, 1.0f);
        const float referenceHeight = std::max(canvas.ReferenceResolution.y, 1.0f);
        const float scaleWidth = viewportWidth / referenceWidth;
        const float scaleHeight = viewportHeight / referenceHeight;
        const float match = glm::clamp(canvas.MatchWidthOrHeight, 0.0f, 1.0f);

        // Unity Scale With Screen Size（对数加权）
        const float logWidth = std::log2(std::max(scaleWidth, 1.0e-5f));
        const float logHeight = std::log2(std::max(scaleHeight, 1.0e-5f));
        const float logWeighted = glm::mix(logWidth, logHeight, match);
        return std::pow(2.0f, logWeighted);
    }

    glm::mat4 Scene::GetCanvasToScreenMatrix(float viewportWidth, float viewportHeight) const
    {
        if (!FindCanvasEntity())
            return glm::mat4(1.0f);

        const CanvasLayoutContext context = GetCanvasLayoutContext(viewportWidth, viewportHeight);
        if (!context.Valid)
            return glm::mat4(1.0f);
        // 设计中心原点 → 屏幕中心；letterbox 由 scaleFactor 保证等比。
        return glm::translate(glm::mat4(1.0f), glm::vec3(viewportWidth * 0.5f, viewportHeight * 0.5f, 0.0f))
               * glm::scale(
                       glm::mat4(1.0f),
                       glm::vec3(context.ScaleFactor, context.ScaleFactor, 1.0f));
    }

    void Scene::SyncCanvasReferenceResolutionToTransform(Entity canvasEntity)
    {
        if (!canvasEntity || !canvasEntity.HasComponent<CanvasComponent>()
            || !canvasEntity.HasComponent<RectTransformComponent>())
            return;

        const auto& canvas = canvasEntity.GetComponent<CanvasComponent>();
        auto& userInterfaceTransform = canvasEntity.GetComponent<RectTransformComponent>();
        // Canvas 根节点锁定在设计空间原点；Size 仅镜像参考分辨率，不参与矩阵。
        userInterfaceTransform.AnchoredPosition = glm::vec2(0.0f);
        userInterfaceTransform.RotationRadians = 0.0f;
        userInterfaceTransform.AnchorMinimum = glm::vec2(0.0f);
        userInterfaceTransform.AnchorMaximum = glm::vec2(1.0f);
        userInterfaceTransform.Pivot = glm::vec2(0.5f);
        userInterfaceTransform.SizeDelta = glm::vec2(0.0f);
        userInterfaceTransform.ResolvedSize = canvas.ReferenceResolution;
        MarkEntityTransformDirty(canvasEntity);
    }

}
