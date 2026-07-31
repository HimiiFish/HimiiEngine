#include "Hepch.h"
#include "Module/Render/Renderer/TextLayout.h"

namespace Himii
{
    bool TextShaper::IsChineseJapaneseKoreanCodePoint(char32_t codePoint)
    {
        return (codePoint >= 0x3400 && codePoint <= 0x4DBF)
               || (codePoint >= 0x4E00 && codePoint <= 0x9FFF)
               || (codePoint >= 0xF900 && codePoint <= 0xFAFF)
               || (codePoint >= 0x3040 && codePoint <= 0x30FF)
               || (codePoint >= 0xAC00 && codePoint <= 0xD7AF);
    }

    std::vector<char32_t> TextShaper::DecodeUtf8(const std::string &text)
    {
        std::vector<char32_t> codePoints;
        for (size_t byteIndex = 0; byteIndex < text.size();)
        {
            const uint8_t firstByte = static_cast<uint8_t>(text[byteIndex]);
            char32_t codePoint = U'?';
            size_t sequenceLength = 1;
            if (firstByte < 0x80)
                codePoint = firstByte;
            else if ((firstByte & 0xE0) == 0xC0)
            {
                codePoint = firstByte & 0x1F;
                sequenceLength = 2;
            }
            else if ((firstByte & 0xF0) == 0xE0)
            {
                codePoint = firstByte & 0x0F;
                sequenceLength = 3;
            }
            else if ((firstByte & 0xF8) == 0xF0)
            {
                codePoint = firstByte & 0x07;
                sequenceLength = 4;
            }

            bool sequenceIsValid = byteIndex + sequenceLength <= text.size();
            for (size_t continuationIndex = 1;
                 continuationIndex < sequenceLength && sequenceIsValid; ++continuationIndex)
            {
                const uint8_t continuationByte =
                        static_cast<uint8_t>(text[byteIndex + continuationIndex]);
                if ((continuationByte & 0xC0) != 0x80)
                {
                    sequenceIsValid = false;
                    break;
                }
                codePoint = (codePoint << 6) | (continuationByte & 0x3F);
            }
            codePoints.push_back(sequenceIsValid ? codePoint : U'?');
            byteIndex += sequenceIsValid ? sequenceLength : 1;
        }
        return codePoints;
    }

    std::vector<TextLineBreakOpportunity> LineBreakIterator::BuildOpportunities(
            const std::vector<char32_t> &codePoints)
    {
        std::vector<TextLineBreakOpportunity> opportunities;
        for (size_t index = 0; index < codePoints.size(); ++index)
        {
            const char32_t codePoint = codePoints[index];
            TextLineBreakOpportunity opportunity;
            opportunity.CodePointIndex = index;
            opportunity.IsHardBreak = codePoint == U'\n';
            opportunity.IsWhitespace = codePoint == U' ' || codePoint == U'\t' || codePoint == U'\n';
            if (opportunity.IsHardBreak || opportunity.IsWhitespace
                || TextShaper::IsChineseJapaneseKoreanCodePoint(codePoint)
                || index + 1 == codePoints.size())
            {
                opportunities.push_back(opportunity);
            }
        }
        return opportunities;
    }

    TextGlyphRun TextShaper::Shape(
            const Ref<Font> &font, const std::vector<char32_t> &codePoints, float kerning)
    {
        TextGlyphRun glyphRun;
        glyphRun.FontAsset = font;
        if (!font)
            return glyphRun;

        float cursorX = 0.0f;
        for (size_t index = 0; index < codePoints.size(); ++index)
        {
            const char32_t codePoint = codePoints[index];
            const char32_t nextCodePoint =
                    index + 1 < codePoints.size() ? codePoints[index + 1] : U'\0';

            TextGlyphInstance instance;
            instance.SourceCodePoint = codePoint;
            instance.GlyphIndex = static_cast<uint32_t>(codePoint);
            instance.OffsetX = cursorX;
            instance.OffsetY = 0.0f;
            font->TryGetGlyph(codePoint, instance.Quad);
            instance.Advance = font->GetAdvance(codePoint, nextCodePoint);
            if (nextCodePoint != U'\0')
                instance.Advance += kerning;

            glyphRun.Glyphs.push_back(instance);
            cursorX += instance.Advance;
        }
        glyphRun.Width = cursorX;
        return glyphRun;
    }
}
