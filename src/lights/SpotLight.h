#ifndef SPOTLIGHT_H
#define SPOTLIGHT_H

#include "Light.h"
#include "common/Types.h"

struct SpotLight : public TypedLight<LightType::SpotLight> {
    SpotLight(Vec3 const &position, Vec3 const &direction, float alphaMin, float alphaMax, float intensity, Vec3 const &color = Vec3(1, 1, 1))
    : color_intensity(Vec4(color, intensity)), position(Vec4(position, 0)), direction(Vec4(normalize(direction), 0)), alphaMin_Max(Vec4(alphaMin, alphaMax, 0, 0)) {}

    void setColor(Vec3 const &color) { color_intensity.x = color.x; color_intensity.y = color.y; color_intensity.z = color.z;; }
    void setIntensity(float intensity) { color_intensity.w = intensity; }
    void setDirection(Vec3 const &direction) { this->direction = Vec4(normalize(direction), 0); }
    void setPosition(Vec3 const &position) { this->position = Vec4(position, 0); }
    void setAlphaMax(float alphaMax) { alphaMin_Max.y = alphaMax; }
    void setAlphaMin(float alphaMin) { alphaMin_Max.x = alphaMin; }

    virtual void* GetDataLayoutBeginPtr() override { return &color_intensity; }
    virtual size_t GetDataSize() const override { return sizeof(Vec4) * 4; }

    Vec4 color_intensity; // xyz = color, w = intensity
    Vec4 position;
    Vec4 direction;
    Vec4 alphaMin_Max;
};

#endif