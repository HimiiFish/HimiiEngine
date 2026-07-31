#pragma once

#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <atomic>
#include <future>
#include <string>

#include "EngineCore/Core/Core.h"
#include "Resource/Asset.h"
#include "Module/Render/Texture.h"
#include "glm/glm.hpp"

namespace Himii
{
    enum class TextHorizontalAlignment
    {
        Left = 0,
        Center,
        Right
    };

    enum class TextVerticalAlignment
    {
        Top = 0,
        Middle,
        Bottom
    };

    struct TextLayoutSettings
    {
        glm::vec2 RectangleSize{300.0f, 100.0f};
        float FontSize = 48.0f; // em 字号：1em = FontSize 设计像素
        float Kerning = 0.0f;
        float LineSpacing = 0.0f;
        TextHorizontalAlignment HorizontalAlignment = TextHorizontalAlignment::Left;
        TextVerticalAlignment VerticalAlignment = TextVerticalAlignment::Top;
    };

    struct FontSpecification
    {
        std::filesystem::path FilePath;
        int FaceIndex = 0;
        uint32_t AtlasPageSize = 1024;
        uint32_t MaximumAtlasPageCount = 8;
        double GenerationScale = 40.0;
        double PixelRange = 2.0;
        double MiterLimit = 1.0;
        std::vector<AssetHandle> FallbackFontHandles;
    };

    struct FontMetrics
    {
        float EmSize = 1.0f;
        float AscenderY = 0.0f;
        float DescenderY = 0.0f;
        float LineHeight = 0.0f;
    };

    struct FontGlyphQuad
    {
        bool Valid = false;
        bool IsWhitespace = false;
        bool IsMissing = false;
        uint32_t AtlasPageIndex = 0;
        Ref<Texture2D> AtlasPageTexture;
        glm::vec2 AtlasUVMinimum{0.0f};
        glm::vec2 AtlasUVMaximum{0.0f};
        glm::vec2 PlaneMinimum{0.0f};
        glm::vec2 PlaneMaximum{0.0f};
        float Advance = 0.0f;
    };

    struct FontDiagnosticsSnapshot
    {
        uint32_t GlyphCount = 0;
        uint32_t PageCount = 0;
        uint32_t PendingGenerationCount = 0;
        uint64_t ApproximateVideoMemoryBytes = 0;
        std::vector<char32_t> MissingCodePoints;
        std::string LastError;
        std::vector<Ref<Texture2D>> AtlasPages;
    };

    class Font : public Asset {
    public:
        Font(const FontSpecification &specification);
        explicit Font(const std::filesystem::path &filepath);
        ~Font() override;

        AssetType GetType() const override
        {
            return AssetType::Font;
        }

        const FontSpecification &GetSpecification() const
        {
            return m_Specification;
        }
        const FontMetrics &GetMetrics() const
        {
            return m_Metrics;
        }
        const std::filesystem::path &GetFilePath() const
        {
            return m_Specification.FilePath;
        }
        int GetFaceIndex() const
        {
            return m_Specification.FaceIndex;
        }

        // 兼容旧调用：返回第一页图集。
        Ref<Texture2D> GetAtlasTexture() const;
        const std::vector<Ref<Texture2D>> &GetAtlasPages() const
        {
            return m_AtlasPageTextures;
        }

        void EnsureGlyphsForText(const std::string &text);
        void PreloadCharacters(const std::string &text);
        std::future<void> PreloadTextAsync(const std::string &text);
        void ProcessCompletedGenerations();

        bool TryGetGlyph(char32_t codePoint, FontGlyphQuad &output) const;
        float GetAdvance(char32_t codePoint, char32_t nextCodePoint) const;
        FontDiagnosticsSnapshot GetDiagnosticsSnapshot() const;

        static Ref<Font> GetDefault();
        static void InitDefault(const std::filesystem::path &path, int faceIndex = 0);
        static void SetDefault(const Ref<Font> &font);
        static Ref<Font> ResolveWithFallback(const Ref<Font> &preferredFont, char32_t codePoint);

    private:
        struct CachedGlyph
        {
            bool IsWhitespace = false;
            bool IsMissing = false;
            uint32_t PageIndex = 0;
            int PixelX = 0;
            int PixelY = 0;
            int PixelWidth = 0;
            int PixelHeight = 0;
            glm::vec2 PlaneMinimum{0.0f};
            glm::vec2 PlaneMaximum{0.0f};
            float Advance = 0.0f;
        };

        struct AtlasPage
        {
            Ref<Texture2D> Texture;
            int CursorX = 1;
            int CursorY = 1;
            int CurrentRowHeight = 0;
        };

        struct GeneratedGlyphPayload
        {
            char32_t CodePoint = 0;
            bool IsWhitespace = false;
            bool IsMissing = false;
            float Advance = 0.0f;
            glm::vec2 PlaneMinimum{0.0f};
            glm::vec2 PlaneMaximum{0.0f};
            int BitmapWidth = 0;
            int BitmapHeight = 0;
            std::vector<uint8_t> BitmapPixels;
            std::string ErrorMessage;
        };

        void InitializeMetrics();
        void EnsureInitialLatinGlyphs();
        // 仅预生成基础 ASCII，CJK 按需加载，避免启动卡顿。
        void EnsureInitialAsciiGlyphs();
        bool EnsureGlyph(char32_t codePoint);
        GeneratedGlyphPayload GenerateGlyphOnWorker(char32_t codePoint) const;
        bool UploadGeneratedGlyph(const GeneratedGlyphPayload &payload);
        bool AllocateShelfRectangle(int width, int height, uint32_t &pageIndex, int &pixelX, int &pixelY);
        void CreateNewAtlasPage();
        void MarkCapacityExceeded(char32_t codePoint);

        FontSpecification m_Specification;
        FontMetrics m_Metrics;
        float m_GeometryScale = 1.0f;

        mutable std::mutex m_GlyphMutex;
        std::unordered_map<char32_t, CachedGlyph> m_CachedGlyphs;
        std::unordered_set<char32_t> m_UnavailableCodePoints;
        std::unordered_set<char32_t> m_PendingCodePoints;
        std::vector<AtlasPage> m_AtlasPages;
        std::vector<Ref<Texture2D>> m_AtlasPageTextures;
        bool m_CapacityExceededWarned = false;
        std::string m_LastError;

        mutable std::mutex m_CompletionMutex;
        std::vector<GeneratedGlyphPayload> m_CompletedGenerations;

        static Ref<Font> s_DefaultFont;
    };
} // namespace Himii
