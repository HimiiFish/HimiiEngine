#pragma once

#include "box2d/box2d.h"

#include <cstdint>

namespace Himii
{
    /// Scene 多翻译单元共享的内部辅助（非公共 API）。
    namespace SceneInternal
    {
        inline void *BodyIdToPointer(b2BodyId bodyIdentifier)
        {
            return reinterpret_cast<void *>(*reinterpret_cast<uintptr_t *>(&bodyIdentifier));
        }

        inline b2BodyId PointerToBodyId(void *pointer)
        {
            uintptr_t value = reinterpret_cast<uintptr_t>(pointer);
            return *reinterpret_cast<b2BodyId *>(&value);
        }
    }
}
