#ifndef POINTLIGHT_H
#define POINTLIGHT_H

#include "Light.h"
#include "common/Types.h"

struct PointLight : public TypedLight<LightType::PointLight> {
    PointLight(Vec3 const &position, float intensity, float radius = 0, Vec3 const &color = Vec3(1, 1, 1)) : position_radus(Vec4(position, radius)), color_intensity(Vec4(color, intensity)) {}

    void setColor(Vec3 const &color) { color_intensity.x = color.x; color_intensity.y = color.y; color_intensity.z = color.z;; }
    void setIntensity(float intensity) { color_intensity.w = intensity; }
    void setPosition(Vec3 const &position) { position_radus.x = position.x; position_radus.y = position.y; position_radus.z = position.z; }
    void setRadius(float radius) { position_radus.w = radius; }

    virtual void* GetDataLayoutBeginPtr() override { return &color_intensity; }
    virtual size_t GetDataSize() const override { return sizeof(Vec4) * 2; }

    Vec4 color_intensity; // xyz = color, w = intensity
    Vec4 position_radus; // xyz = pos, w = radius
};

#endif
