#undef LOG_MODULE
#define LOG_MODULE "CBOR"

#include <cstdint>
#include <optional>
#include <vector>

#include <console.h>

#include "Cbor.h"

#define SIZE_GE(_size)                                  \
    do {                                                \
        if (pos_ + _size > data_.size())                \
            return std::nullopt;                        \
    } while (false)

using namespace std;
using namespace app;

Cbor::Cbor(const std::vector<uint8_t>& data, size_t position)
: data_(data), pos_(position) {}

std::optional<uint8_t> Cbor::parseParameterID()
{
    SIZE_GE(1);
    return data_[pos_++];
}

std::optional<uint32_t> Cbor::parseNumber()
{
    SIZE_GE(1);

    uint8_t type = data_[pos_++];

    switch (type)
    {
        case 0x00:  // 1-byte simple
        case 0x01:  // 1-byte simple
        {
            return type;
        }

        case 0x18:  // 1-byte unsigned
        case 0x38:  // 1-byte signed
        {
            SIZE_GE(1);
            uint32_t value = data_[pos_];
            pos_ += 1;
            return value;
        }

        case 0x19:  // 2-byte integer
        case 0x39:  // 2-byte signed negative
        {
            SIZE_GE(2);
            uint32_t value = (data_[pos_] << 8) | data_[pos_ + 1];
            pos_ += 2;

            return (type == 0x19) ? value : ((-1) - value);
        }

        case 0x1A:  // 4-byte unsigned
        case 0x3A:  // 4-byte signed negative
        {
            SIZE_GE(4);
            uint32_t value = (data_[pos_] << 24) | (data_[pos_ + 1] << 16) |
                             (data_[pos_ + 2] << 8) | data_[pos_ + 3];
            pos_ += 4;

            return (type == 0x1A) ? value : ((-1) - value);
        }

        default:
            LOGE("Unknown entry type: 0x%x", (int)type);
    }

    return std::nullopt;
}

std::optional<size_t> Cbor::parseMap()
{
    SIZE_GE(1);

    uint8_t major = data_[pos_] >> 5;
    uint8_t minor = data_[pos_] & 0x0F;

    if ((major == 5) && (minor < 24))  // the map
    {
        pos_++;
        return minor;  // size
    }

    return std::nullopt;
}

bool Cbor::isEmpty() const
{
    return pos_ >= data_.size();
}

bool Cbor::isType(Cbor::Type type) const
{
    Cbor::Type value = static_cast<Type>(data_[pos_]);

    if ((type == Type::MAP) && (value & 0xE0) == Type::MAP)
    {
        return true;
    }

    if ((type == Type::UINT8 ||
         type == Type::UINT16 ||
         type == Type::UINT32) &&
        (type == value))
    {
        return type;
    }

    return Type::UNKNOWN;
}
