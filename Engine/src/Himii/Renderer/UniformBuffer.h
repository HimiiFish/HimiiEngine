#pragma once

#include <cstdint>
#include "Himii/Core/Core.h"
#include <cstdint>

namespace Himii
{
    class UniformBuffer {
    public:
        virtual ~UniformBuffer() = default;
        virtual void SetData(const void *data, uint32_t size, uint32_t offset = 0) = 0;

        static Ref<UniformBuffer> Create(uint32_t size, uint32_t binding);
    };
} // namespace Himii
