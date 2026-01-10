#ifndef POINTLIGHT_H
#define POINTLIGHT_H

#include "Light.h"
#include "common/Types.h"

struct PointLight : public TypedLight<LightType::PointLight> {
    PointLight(Vec3 const &position, float intensity, Vec3 const &color = Vec3(1, 1, 1)) : position(Vec4(position, 0)), color_intensity(Vec4(color, intensity)) {}

    void setPosition(Vec3 const &position) { this->position = Vec4(position, 0); }

    void setColor(Vec3 const &color) { color_intensity.x = color.x; color_intensity.y = color.y; color_intensity.z = color.z;; }
    void setIntensity(float intensity) { color_intensity.w = intensity; }

    virtual void* GetDataLayoutBeginPtr() override { return &position; }
    virtual size_t GetDataSize() const override { return sizeof(Vec4) * 2; }

    Vec4 position;
    Vec4 color_intensity; // xyz = color, w = intensity
};

#endif
