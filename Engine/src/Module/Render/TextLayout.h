#pragma once

#include <string>
#include <vector>
#include <functional>

#include "glm/glm.hpp"
#include "Module/Render/Font.h"

namespace Himii
{
    // 为后续 HarfBuzz 预留：布局以 glyph run 为核心，而不是直接绑定 code point。
    struct TextGlyphInstance
    {
        uint32_t GlyphIndex = 0;
        char32_t SourceCodePoint = 0;
        float Advance = 0.0f;
        float OffsetX = 0.0f;
        float OffsetY = 0.0f;
        FontGlyphQuad Quad;
    };

    struct TextGlyphRun
    {
        Ref<Font> FontAsset;
        std::vector<TextGlyphInstance> Glyphs;
        float Width = 0.0f;
    };

    struct TextLineBreakOpportunity
    {
        size_t CodePointIndex = 0;
        bool IsHardBreak = false;
        bool IsWhitespace = false;
    };

    class LineBreakIterator
    {
    public:
        // 架构预留：当前使用英文单词 + CJK 单字符的简单规则，后续可替换为 UAX #14。
        static std::vector<TextLineBreakOpportunity> BuildOpportunities(
                const std::vector<char32_t> &codePoints);
    };

    class TextShaper
    {
    public:
        static std::vector<char32_t> DecodeUtf8(const std::string &text);
        static bool IsChineseJapaneseKoreanCodePoint(char32_t codePoint);

        // 当前实现：一对一 code point → glyph；接口返回 glyph run，便于接入 HarfBuzz。
        static TextGlyphRun Shape(
                const Ref<Font> &font, const std::vector<char32_t> &codePoints, float kerning);
    };
}
