#ifndef LIGHT_H
#define LIGHT_H

#include "common/Types.h"

enum class LightType : uint32_t {
    None = 0,
    PointLight = 1,
};

struct Light {
    Light(const LightType type) : type(type) {}
    virtual ~Light() = default;

    virtual void* GetDataLayoutBeginPtr() = 0;
    virtual size_t GetDataSize() const = 0;

    LightType type = LightType::None;
    int32_t index = -1;
};

template <LightType Type>
struct TypedLight : public Light {
    explicit TypedLight() : Light(Type) {}
};

#endif
