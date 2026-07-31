#include "Hepch.h"
#include "Module/Render/Font.h"
#include "EngineCore/Core/FileSystem.h"
#include "EngineCore/Core/JobSystem.h"
#include "Module/Render/TextLayout.h"

#include "freetype/freetype.h"
#undef INFINITE
#include "msdf-atlas-gen/msdf-atlas-gen.h"
#include "msdfgen/msdfgen.h"
#include "msdfgen/msdfgen-ext.h"
#include "msdfgen/core/pixel-conversion.hpp"

#include <cstring>
#include <unordered_map>

namespace Himii
{
    Ref<Font> Font::s_DefaultFont = nullptr;

    namespace
    {
        constexpr double EdgeColoringAngleThreshold = 3.0;
        constexpr unsigned long long LinearCongruentialMultiplier = 6364136223846793005ull;
        constexpr int AtlasPaddingPixels = 1;

        msdfgen::FontHandle *CreateFontHandleWithFaceIndex(
                msdfgen::FreetypeHandle *freeTypeHandle, const std::filesystem::path &path,
                int faceIndex)
        {
            if (!freeTypeHandle)
                return nullptr;

            // msdfgen::loadFont 固定 face 0；TTC 通过 FreeType + adopt 支持 FaceIndex。
            FT_Library library = nullptr;
            {
                // 通过临时加载获取底层 FT_Library 不可行（封装私有），直接使用本进程 FreeType。
            }
            (void)freeTypeHandle;

            FT_Library ownedLibrary = nullptr;
            if (FT_Init_FreeType(&ownedLibrary) != 0)
                return nullptr;

            FT_Face face = nullptr;
            const std::string pathString = path.string();
            if (FT_New_Face(ownedLibrary, pathString.c_str(), faceIndex, &face) != 0)
            {
                FT_Done_FreeType(ownedLibrary);
                return nullptr;
            }

            // adopt 后 ownership=false，需自行销毁 Face/Library。
            // 为简化生命周期，改用 msdfgen loadFont 路径：faceIndex==0 时；
            // faceIndex!=0 时仍 adopt，并在调用方负责 Done。
            // 这里将 library 挂到 thread_local 缓存，避免过早释放。
            struct AdoptedFaceOwner
            {
                FT_Library Library = nullptr;
                FT_Face Face = nullptr;
                ~AdoptedFaceOwner()
                {
                    if (Face)
                        FT_Done_Face(Face);
                    if (Library)
                        FT_Done_FreeType(Library);
                }
            };

            // 不能把局部 owner 交给外部。改为：faceIndex==0 用 loadFont；
            // 非 0 则复制为临时策略——使用 FT_New_Face + adopt，并把 library/face
            // 存入静态 map（按 path+face）在进程内常驻。
            struct FaceCacheKey
            {
                std::string Path;
                int FaceIndex = 0;
                bool operator==(const FaceCacheKey &other) const
                {
                    return Path == other.Path && FaceIndex == other.FaceIndex;
                }
            };
            struct FaceCacheKeyHash
            {
                size_t operator()(const FaceCacheKey &key) const
                {
                    return std::hash<std::string>()(key.Path)
                           ^ (static_cast<size_t>(key.FaceIndex) << 1);
                }
            };
            struct CachedFace
            {
                FT_Library Library = nullptr;
                FT_Face Face = nullptr;
            };

            static std::mutex cacheMutex;
            static std::unordered_map<FaceCacheKey, CachedFace, FaceCacheKeyHash> faceCache;

            FaceCacheKey key{pathString, faceIndex};
            {
                std::lock_guard<std::mutex> lock(cacheMutex);
                auto found = faceCache.find(key);
                if (found != faceCache.end())
                {
                    FT_Done_Face(face);
                    FT_Done_FreeType(ownedLibrary);
                    return msdfgen::adoptFreetypeFont(found->second.Face);
                }

                CachedFace cached;
                cached.Library = ownedLibrary;
                cached.Face = face;
                faceCache.emplace(key, cached);
                return msdfgen::adoptFreetypeFont(face);
            }
        }
    }

    void Font::InitDefault(const std::filesystem::path &path, int faceIndex)
    {
        FontSpecification specification;
        specification.FilePath = path;
        specification.FaceIndex = faceIndex;
        s_DefaultFont = CreateRef<Font>(specification);
    }

    void Font::SetDefault(const Ref<Font> &font)
    {
        s_DefaultFont = font;
    }

    Ref<Font> Font::GetDefault()
    {
        return s_DefaultFont;
    }

    Ref<Font> Font::ResolveWithFallback(const Ref<Font> &preferredFont, char32_t codePoint)
    {
        if (preferredFont)
        {
            FontGlyphQuad glyph;
            if (preferredFont->TryGetGlyph(codePoint, glyph) && glyph.Valid && !glyph.IsMissing)
                return preferredFont;
        }
        if (s_DefaultFont && s_DefaultFont != preferredFont)
        {
            FontGlyphQuad glyph;
            if (s_DefaultFont->TryGetGlyph(codePoint, glyph) && glyph.Valid && !glyph.IsMissing)
                return s_DefaultFont;
        }
        return preferredFont ? preferredFont : s_DefaultFont;
    }

    Font::Font(const std::filesystem::path &filepath)
    {
        m_Specification.FilePath = filepath;
        InitializeMetrics();
        CreateNewAtlasPage();
        EnsureInitialAsciiGlyphs();
    }

    Font::Font(const FontSpecification &specification) : m_Specification(specification)
    {
        if (m_Specification.AtlasPageSize < 64)
            m_Specification.AtlasPageSize = 64;
        if (m_Specification.MaximumAtlasPageCount == 0)
            m_Specification.MaximumAtlasPageCount = 1;
        InitializeMetrics();
        CreateNewAtlasPage();
        EnsureInitialAsciiGlyphs();
    }

    Font::~Font() = default;

    Ref<Texture2D> Font::GetAtlasTexture() const
    {
        return m_AtlasPageTextures.empty() ? nullptr : m_AtlasPageTextures.front();
    }

    void Font::InitializeMetrics()
    {
        msdfgen::FreetypeHandle *freeTypeHandle = msdfgen::initializeFreetype();
        if (!freeTypeHandle)
        {
            m_LastError = "Failed to initialize FreeType";
            return;
        }

        const std::filesystem::path resolvedPath =
                FileSystem::MaterializeLooseFile(m_Specification.FilePath.string());
        msdfgen::FontHandle *fontHandle =
                CreateFontHandleWithFaceIndex(freeTypeHandle, resolvedPath, m_Specification.FaceIndex);
        if (!fontHandle)
        {
            m_LastError = "Failed to load font face";
            msdfgen::deinitializeFreetype(freeTypeHandle);
            return;
        }

        msdfgen::FontMetrics metrics = {};
        if (!msdfgen::getFontMetrics(metrics, fontHandle, msdfgen::FONT_SCALING_NONE))
        {
            m_LastError = "Failed to read font metrics";
            msdfgen::destroyFont(fontHandle);
            msdfgen::deinitializeFreetype(freeTypeHandle);
            return;
        }

        // 与 FontGeometry::loadMetrics(fontScale=1) 一致：规范化到 em。
        m_GeometryScale = static_cast<float>(1.0 / (metrics.emSize != 0.0 ? metrics.emSize : 1.0));
        m_Metrics.EmSize = 1.0f;
        m_Metrics.AscenderY = static_cast<float>(metrics.ascenderY * m_GeometryScale);
        m_Metrics.DescenderY = static_cast<float>(metrics.descenderY * m_GeometryScale);
        m_Metrics.LineHeight = static_cast<float>(metrics.lineHeight * m_GeometryScale);

        msdfgen::destroyFont(fontHandle);
        msdfgen::deinitializeFreetype(freeTypeHandle);
    }

    void Font::CreateNewAtlasPage()
    {
        TextureSpecification textureSpecification;
        textureSpecification.Width = m_Specification.AtlasPageSize;
        textureSpecification.Height = m_Specification.AtlasPageSize;
        textureSpecification.Format = ImageFormat::RGB8;
        textureSpecification.ClampToEdge = true;
        textureSpecification.UseLinearFiltering = true;

        AtlasPage page;
        page.Texture = Texture2D::Create(textureSpecification);
        // 清成中性距离 0.5（byte 128），避免未写入区域被当成字形内部。
        const size_t pixelCount = static_cast<size_t>(m_Specification.AtlasPageSize)
                                  * static_cast<size_t>(m_Specification.AtlasPageSize);
        std::vector<uint8_t> clearPixels(pixelCount * 3u, 128);
        page.Texture->SetData(clearPixels.data(), static_cast<uint32_t>(clearPixels.size()));
        page.CursorX = AtlasPaddingPixels;
        page.CursorY = AtlasPaddingPixels;
        page.CurrentRowHeight = 0;

        m_AtlasPages.push_back(page);
        m_AtlasPageTextures.push_back(page.Texture);
    }

    void Font::EnsureInitialAsciiGlyphs()
    {
        std::string asciiText;
        for (uint32_t codePoint = 0x0020; codePoint <= 0x007E; ++codePoint)
            asciiText.push_back(static_cast<char>(codePoint));
        PreloadCharacters(asciiText);
    }

    void Font::EnsureInitialLatinGlyphs()
    {
        EnsureInitialAsciiGlyphs();
        std::string latinText;
        for (uint32_t codePoint = 0x00A0; codePoint <= 0x00FF; ++codePoint)
        {
            char buffer[3] = {};
            buffer[0] = static_cast<char>(0xC0 | ((codePoint >> 6) & 0x1F));
            buffer[1] = static_cast<char>(0x80 | (codePoint & 0x3F));
            latinText.append(buffer, 2);
        }
        PreloadCharacters(latinText);
    }

    void Font::EnsureGlyphsForText(const std::string &text)
    {
        ProcessCompletedGenerations();
        for (char32_t codePoint : TextShaper::DecodeUtf8(text))
        {
            if (codePoint == U'\n' || codePoint == U'\r')
                continue;
            EnsureGlyph(codePoint);
        }
        ProcessCompletedGenerations();
    }

    void Font::PreloadCharacters(const std::string &text)
    {
        for (char32_t codePoint : TextShaper::DecodeUtf8(text))
        {
            if (codePoint == U'\n' || codePoint == U'\r')
                continue;
            EnsureGlyph(codePoint);
        }
        ProcessCompletedGenerations();
    }

    std::future<void> Font::PreloadTextAsync(const std::string &text)
    {
        auto sharedPromise = CreateRef<std::promise<void>>();
        std::future<void> future = sharedPromise->get_future();
        // 调用方须保证 Font 在 future 完成前仍然存活。
        Font *fontPointer = this;
        const std::string textCopy = text;

        {
            std::lock_guard<std::mutex> lock(m_GlyphMutex);
            for (char32_t codePoint : TextShaper::DecodeUtf8(textCopy))
            {
                if (codePoint == U'\n' || codePoint == U'\r')
                    continue;
                if (m_CachedGlyphs.find(codePoint) != m_CachedGlyphs.end()
                    || m_UnavailableCodePoints.find(codePoint) != m_UnavailableCodePoints.end()
                    || m_PendingCodePoints.find(codePoint) != m_PendingCodePoints.end())
                    continue;
                m_PendingCodePoints.insert(codePoint);
            }
        }

        JobSystem::Submit(
                [fontPointer, textCopy]()
                {
                    for (char32_t codePoint : TextShaper::DecodeUtf8(textCopy))
                    {
                        if (codePoint == U'\n' || codePoint == U'\r')
                            continue;
                        GeneratedGlyphPayload payload = fontPointer->GenerateGlyphOnWorker(codePoint);
                        {
                            std::lock_guard<std::mutex> lock(fontPointer->m_CompletionMutex);
                            fontPointer->m_CompletedGenerations.push_back(std::move(payload));
                        }
                    }
                },
                [fontPointer, sharedPromise]()
                {
                    fontPointer->ProcessCompletedGenerations();
                    sharedPromise->set_value();
                });

        return future;
    }

    bool Font::EnsureGlyph(char32_t codePoint)
    {
        {
            std::lock_guard<std::mutex> lock(m_GlyphMutex);
            if (m_CachedGlyphs.find(codePoint) != m_CachedGlyphs.end())
                return true;
            if (m_UnavailableCodePoints.find(codePoint) != m_UnavailableCodePoints.end())
                return false;
            if (m_PendingCodePoints.find(codePoint) != m_PendingCodePoints.end())
                return false;
            m_PendingCodePoints.insert(codePoint);
        }

        // 同步生成并上传，保证本帧 EnsureGlyphsForText 后立即可绘。
        GeneratedGlyphPayload payload = GenerateGlyphOnWorker(codePoint);
        return UploadGeneratedGlyph(payload);
    }

    Font::GeneratedGlyphPayload Font::GenerateGlyphOnWorker(char32_t codePoint) const
    {
        GeneratedGlyphPayload payload;
        payload.CodePoint = codePoint;

        if (codePoint == U' ' || codePoint == U'\t')
        {
            payload.IsWhitespace = true;
            payload.Advance = codePoint == U'\t' ? 4.0f * 0.25f : 0.25f;
            // 用空格真实 advance 覆盖。
        }

        msdfgen::FreetypeHandle *freeTypeHandle = msdfgen::initializeFreetype();
        if (!freeTypeHandle)
        {
            payload.IsMissing = true;
            payload.ErrorMessage = "FreeType init failed";
            return payload;
        }

        const std::filesystem::path resolvedPath =
                FileSystem::MaterializeLooseFile(m_Specification.FilePath.string());
        msdfgen::FontHandle *fontHandle = CreateFontHandleWithFaceIndex(
                freeTypeHandle, resolvedPath, m_Specification.FaceIndex);
        if (!fontHandle)
        {
            payload.IsMissing = true;
            payload.ErrorMessage = "Font face load failed";
            msdfgen::deinitializeFreetype(freeTypeHandle);
            return payload;
        }

        msdf_atlas::GlyphGeometry glyph;
        if (!glyph.load(fontHandle, m_GeometryScale, codePoint, true))
        {
            payload.IsMissing = true;
            payload.ErrorMessage = "Glyph missing in font";
            msdfgen::destroyFont(fontHandle);
            msdfgen::deinitializeFreetype(freeTypeHandle);
            return payload;
        }

        payload.Advance = static_cast<float>(glyph.getAdvance());
        if (glyph.isWhitespace())
        {
            payload.IsWhitespace = true;
            msdfgen::destroyFont(fontHandle);
            msdfgen::deinitializeFreetype(freeTypeHandle);
            return payload;
        }

        msdf_atlas::GlyphGeometry::GlyphAttributes attributes = {};
        attributes.scale = m_Specification.GenerationScale;
        attributes.range = m_Specification.PixelRange / m_Specification.GenerationScale;
        attributes.miterLimit = m_Specification.MiterLimit;
        glyph.wrapBox(attributes);
        glyph.edgeColoring(&msdfgen::edgeColoringInkTrap, EdgeColoringAngleThreshold, 0);

        int boxLeft = 0;
        int boxBottom = 0;
        int boxWidth = 0;
        int boxHeight = 0;
        glyph.getBoxRect(boxLeft, boxBottom, boxWidth, boxHeight);
        if (boxWidth <= 0 || boxHeight <= 0)
        {
            payload.IsWhitespace = true;
            msdfgen::destroyFont(fontHandle);
            msdfgen::deinitializeFreetype(freeTypeHandle);
            return payload;
        }

        double planeLeft = 0, planeBottom = 0, planeRight = 0, planeTop = 0;
        glyph.getQuadPlaneBounds(planeLeft, planeBottom, planeRight, planeTop);
        payload.PlaneMinimum = {static_cast<float>(planeLeft), static_cast<float>(planeBottom)};
        payload.PlaneMaximum = {static_cast<float>(planeRight), static_cast<float>(planeTop)};
        payload.BitmapWidth = boxWidth;
        payload.BitmapHeight = boxHeight;

        msdfgen::Bitmap<float, 3> floatBitmap(boxWidth, boxHeight);
        msdf_atlas::GeneratorAttributes generatorAttributes;
        generatorAttributes.config.overlapSupport = true;
        generatorAttributes.scanlinePass = true;
        msdf_atlas::msdfGenerator(floatBitmap, glyph, generatorAttributes);

        payload.BitmapPixels.resize(static_cast<size_t>(boxWidth) * static_cast<size_t>(boxHeight) * 3u);
        for (int y = 0; y < boxHeight; ++y)
        {
            for (int x = 0; x < boxWidth; ++x)
            {
                const float *source = floatBitmap(x, y);
                const size_t destinationIndex =
                        (static_cast<size_t>(y) * static_cast<size_t>(boxWidth)
                         + static_cast<size_t>(x))
                        * 3u;
                payload.BitmapPixels[destinationIndex + 0] = msdfgen::pixelFloatToByte(source[0]);
                payload.BitmapPixels[destinationIndex + 1] = msdfgen::pixelFloatToByte(source[1]);
                payload.BitmapPixels[destinationIndex + 2] = msdfgen::pixelFloatToByte(source[2]);
            }
        }

        msdfgen::destroyFont(fontHandle);
        msdfgen::deinitializeFreetype(freeTypeHandle);
        return payload;
    }

    bool Font::AllocateShelfRectangle(
            int width, int height, uint32_t &pageIndex, int &pixelX, int &pixelY)
    {
        const int pageSize = static_cast<int>(m_Specification.AtlasPageSize);
        if (width + AtlasPaddingPixels * 2 > pageSize || height + AtlasPaddingPixels * 2 > pageSize)
            return false;

        for (uint32_t index = 0; index < m_AtlasPages.size(); ++index)
        {
            AtlasPage &page = m_AtlasPages[index];
            if (page.CursorX + width + AtlasPaddingPixels > pageSize)
            {
                page.CursorX = AtlasPaddingPixels;
                page.CursorY += page.CurrentRowHeight + AtlasPaddingPixels;
                page.CurrentRowHeight = 0;
            }
            if (page.CursorY + height + AtlasPaddingPixels > pageSize)
                continue;

            pageIndex = index;
            pixelX = page.CursorX;
            pixelY = page.CursorY;
            page.CursorX += width + AtlasPaddingPixels;
            page.CurrentRowHeight = std::max(page.CurrentRowHeight, height);
            return true;
        }

        if (m_AtlasPages.size() >= m_Specification.MaximumAtlasPageCount)
            return false;

        CreateNewAtlasPage();
        AtlasPage &page = m_AtlasPages.back();
        pageIndex = static_cast<uint32_t>(m_AtlasPages.size() - 1);
        pixelX = page.CursorX;
        pixelY = page.CursorY;
        page.CursorX += width + AtlasPaddingPixels;
        page.CurrentRowHeight = std::max(page.CurrentRowHeight, height);
        return true;
    }

    void Font::MarkCapacityExceeded(char32_t codePoint)
    {
        if (!m_CapacityExceededWarned)
        {
            HIMII_CORE_WARNING(
                    "Font atlas page capacity exceeded for '{0}' (face {1}). Missing glyphs will be used.",
                    m_Specification.FilePath.string(), m_Specification.FaceIndex);
            m_CapacityExceededWarned = true;
        }
        m_UnavailableCodePoints.insert(codePoint);
        m_LastError = "Atlas page capacity exceeded";
    }

    bool Font::UploadGeneratedGlyph(const GeneratedGlyphPayload &payload)
    {
        std::lock_guard<std::mutex> lock(m_GlyphMutex);
        m_PendingCodePoints.erase(payload.CodePoint);

        if (!payload.ErrorMessage.empty())
            m_LastError = payload.ErrorMessage;

        if (payload.IsMissing)
        {
            m_UnavailableCodePoints.insert(payload.CodePoint);
            CachedGlyph missing;
            missing.IsMissing = true;
            missing.Advance = m_Metrics.EmSize * 0.5f;
            m_CachedGlyphs[payload.CodePoint] = missing;
            return false;
        }

        if (payload.IsWhitespace || payload.BitmapPixels.empty())
        {
            CachedGlyph whitespace;
            whitespace.IsWhitespace = true;
            whitespace.Advance = payload.Advance > 0.0f ? payload.Advance : m_Metrics.EmSize * 0.25f;
            m_CachedGlyphs[payload.CodePoint] = whitespace;
            return true;
        }

        uint32_t pageIndex = 0;
        int pixelX = 0;
        int pixelY = 0;
        if (!AllocateShelfRectangle(payload.BitmapWidth, payload.BitmapHeight, pageIndex, pixelX, pixelY))
        {
            MarkCapacityExceeded(payload.CodePoint);
            CachedGlyph missing;
            missing.IsMissing = true;
            missing.Advance = payload.Advance;
            m_CachedGlyphs[payload.CodePoint] = missing;
            return false;
        }

        m_AtlasPages[pageIndex].Texture->SetDataRegion(
                static_cast<uint32_t>(pixelX), static_cast<uint32_t>(pixelY),
                static_cast<uint32_t>(payload.BitmapWidth),
                static_cast<uint32_t>(payload.BitmapHeight),
                const_cast<uint8_t *>(payload.BitmapPixels.data()),
                static_cast<uint32_t>(payload.BitmapPixels.size()));

        CachedGlyph cached;
        cached.PageIndex = pageIndex;
        cached.PixelX = pixelX;
        cached.PixelY = pixelY;
        cached.PixelWidth = payload.BitmapWidth;
        cached.PixelHeight = payload.BitmapHeight;
        cached.PlaneMinimum = payload.PlaneMinimum;
        cached.PlaneMaximum = payload.PlaneMaximum;
        cached.Advance = payload.Advance;
        m_CachedGlyphs[payload.CodePoint] = cached;
        return true;
    }

    void Font::ProcessCompletedGenerations()
    {
        std::vector<GeneratedGlyphPayload> completed;
        {
            std::lock_guard<std::mutex> lock(m_CompletionMutex);
            completed.swap(m_CompletedGenerations);
        }
        for (const GeneratedGlyphPayload &payload : completed)
            UploadGeneratedGlyph(payload);
    }

    bool Font::TryGetGlyph(char32_t codePoint, FontGlyphQuad &output) const
    {
        output = {};
        std::lock_guard<std::mutex> lock(m_GlyphMutex);
        auto found = m_CachedGlyphs.find(codePoint);
        if (found == m_CachedGlyphs.end())
        {
            // 缺失时尝试 '?'
            found = m_CachedGlyphs.find(U'?');
            if (found == m_CachedGlyphs.end())
                return false;
            output.IsMissing = true;
        }

        const CachedGlyph &glyph = found->second;
        output.IsWhitespace = glyph.IsWhitespace;
        output.IsMissing = output.IsMissing || glyph.IsMissing;
        output.Advance = glyph.Advance;
        if (glyph.IsWhitespace || glyph.IsMissing)
        {
            output.Valid = true;
            return true;
        }

        if (glyph.PageIndex >= m_AtlasPageTextures.size())
            return false;

        const float pageSize = static_cast<float>(m_Specification.AtlasPageSize);
        output.Valid = true;
        output.AtlasPageIndex = glyph.PageIndex;
        output.AtlasPageTexture = m_AtlasPageTextures[glyph.PageIndex];
        output.AtlasUVMinimum = {
                static_cast<float>(glyph.PixelX) / pageSize,
                static_cast<float>(glyph.PixelY) / pageSize};
        output.AtlasUVMaximum = {
                static_cast<float>(glyph.PixelX + glyph.PixelWidth) / pageSize,
                static_cast<float>(glyph.PixelY + glyph.PixelHeight) / pageSize};
        output.PlaneMinimum = glyph.PlaneMinimum;
        output.PlaneMaximum = glyph.PlaneMaximum;
        return true;
    }

    float Font::GetAdvance(char32_t codePoint, char32_t nextCodePoint) const
    {
        FontGlyphQuad glyph;
        if (!TryGetGlyph(codePoint, glyph))
            return m_Metrics.EmSize * 0.5f;
        (void)nextCodePoint; // kerning 可在后续通过 FreeType 查询补充
        if (codePoint == U'\t')
            return glyph.Advance * 4.0f;
        return glyph.Advance;
    }

    FontDiagnosticsSnapshot Font::GetDiagnosticsSnapshot() const
    {
        std::lock_guard<std::mutex> lock(m_GlyphMutex);
        FontDiagnosticsSnapshot snapshot;
        snapshot.GlyphCount = static_cast<uint32_t>(m_CachedGlyphs.size());
        snapshot.PageCount = static_cast<uint32_t>(m_AtlasPageTextures.size());
        snapshot.PendingGenerationCount = static_cast<uint32_t>(m_PendingCodePoints.size());
        snapshot.ApproximateVideoMemoryBytes =
                static_cast<uint64_t>(snapshot.PageCount) * m_Specification.AtlasPageSize
                * m_Specification.AtlasPageSize * 3ull;
        snapshot.MissingCodePoints.assign(
                m_UnavailableCodePoints.begin(), m_UnavailableCodePoints.end());
        snapshot.LastError = m_LastError;
        snapshot.AtlasPages = m_AtlasPageTextures;
        return snapshot;
    }
}
