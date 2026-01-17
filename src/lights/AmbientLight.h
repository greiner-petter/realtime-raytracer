#ifndef AMBIENTLIGHT_H
#define AMBIENTLIGHT_H

#include "Light.h"
#include "common/Types.h"

struct AmbientLight : public TypedLight<LightType::AmbientLight> {
    AmbientLight(float intensity, Vec3 const &color = Vec3(1, 1, 1)) : color_intensity(Vec4(color, intensity)) {}

    void setColor(Vec3 const &color) { color_intensity.x = color.x; color_intensity.y = color.y; color_intensity.z = color.z;; }
    void setIntensity(float intensity) { color_intensity.w = intensity; }

    virtual void* GetDataLayoutBeginPtr() override { return &color_intensity; }
    virtual size_t GetDataSize() const override { return sizeof(color_intensity); }

    Vec4 color_intensity; // xyz = color, w = intensity
};

#endif
