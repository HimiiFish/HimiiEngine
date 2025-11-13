#pragma once

#include <cstdint>     // for uint64_t
#include <cstddef>     // for std::size_t
#include <functional>

namespace Himii
{
    class UUID
    {
    private:
        uint64_t m_UUID = 0; // 64-bit UUID
    public:
        UUID(/* args */);
        UUID(uint64_t uuid);
        UUID(const UUID&)=default;

        operator uint64_t() const { return m_UUID; }
    };
} // namespace Himii

namespace std
{
    template <typename T>struct hash;

    template<>
    struct hash<Himii::UUID>
    {
        std::size_t operator()(const Himii::UUID& uuid) const
        {
            return (uint64_t)uuid;
        }
    };
    
    
}