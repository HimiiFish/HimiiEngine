#pragma once
#include "string"
#include "Himii/Core/Core.h"

namespace Himii
{
    class Texture 
    {
    public:
        virtual ~Texture() = default;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;

        virtual void Bind(uint32_t slot=0) const = 0;
    private:
    };

    class Texture2D :public Texture
    {
    public:
        static Ref<Texture2D> Create(const std::string &path);

    };
} // namespace Himii
