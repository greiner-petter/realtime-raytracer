#ifndef INFINITE_PLANE_H
#define INFINITE_PLANE_H

#include "Primitive.h"
#include "common/Types.h"

struct InfinitePlane : public TypedPrimitive<PrimitiveType::InfinitePlane> {
    InfinitePlane(std::shared_ptr<Shader> const &shader) : TypedPrimitive(shader), normal(Vec4(0, 1, 0, 0)) {}

    InfinitePlane(Vec3 const &origin, Vec3 const &normal, std::shared_ptr<Shader> const &shader)
        : TypedPrimitive(shader), origin(Vec4(origin, 0)), normal(Vec4(glm::normalize(normal), 0)) {}

    void setOrigin(Vec3 const &origin) { this->origin = Vec4(origin, 0); }
    void setNormal(Vec3 const &normal) { this->normal = Vec4(glm::normalize(normal), 0); }

    float minimumBounds(int dimension) const { return (this->normal[dimension] == 1.0f) ? this->origin[dimension] - EPSILON : -INFINITY; }
    float maximumBounds(int dimension) const { return (this->normal[dimension] == 1.0f) ? this->origin[dimension] + EPSILON : +INFINITY; }

    virtual void* GetDataLayoutBeginPtr() override { return &origin[0]; }
    virtual size_t GetDataSize() const override { return sizeof(Vec4) * 2; }

    Vec4 origin, normal;
};

#endif