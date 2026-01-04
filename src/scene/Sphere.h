#ifndef SPHERE_H
#define SPHERE_H

#include "Primitive.h"
#include "common/Types.h"

struct Sphere : public Primitive {
    Sphere() { type = PrimitiveType::Sphere; }
    Sphere(const Vec3& center, float radius) {
        center_radius = Vec4(center, radius);
        type = PrimitiveType::Sphere;
    }
    Sphere(const Vec4& centerRadius) : center_radius(centerRadius) {
        type = PrimitiveType::Sphere;
    }

    Vec3 GetCenter() const { return Vec3(center_radius.x, center_radius.y, center_radius.z); }
    float GetRadius() const { return center_radius.w; }

    void SetCenter(const Vec3& center) { center_radius.x = center.x; center_radius.y = center.y; center_radius.z = center.z; }
    void SetRadius(float radius) { center_radius.w = radius; }

    float minimumBounds(int dimension) const override;
    float maximumBounds(int dimension) const override;

    virtual void* GetDataLayoutBeginPtr() override { return &center_radius; }
    virtual size_t GetDataSize() const override { return sizeof(center_radius); }

    Vec4 center_radius; // xyz = center, w = radius
};

#endif