#pragma once
#include <string>
#include <array>
#include "glm/vec2.hpp"
#include "Himii/Core/Core.h"

namespace Himii
{
    class Texture 
    {
    public:
        virtual ~Texture() = default;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;

        virtual void SetData(void *data, uint32_t size) = 0;

        virtual void Bind(uint32_t slot=0) const = 0;

        virtual bool operator==(const Texture &other) const=0;
    private:
    };

    class Texture2D :public Texture
    {
    public:
        static Ref<Texture2D> Create(uint32_t width,uint32_t height);
        static Ref<Texture2D> Create(const std::string &path);

    // 精灵图集 UV 计算接口：
    // - Grid 版本：按 (cols,rows) 切分图集，col/row 从左下开始；padding 为归一（0..1）
    // - Pixels 版本：直接以像素矩形计算，padding 为像素
    virtual std::array<glm::vec2,4> GetUVFromGrid(int col, int row, int cols, int rows, float paddingNorm = 0.0f) const = 0;
    virtual std::array<glm::vec2,4> GetUVFromPixels(const glm::vec2& pixelMin,
                            const glm::vec2& pixelMax,
                            const glm::vec2& paddingPx = {0.0f, 0.0f}) const = 0;
    };
} // namespace Himii
