#ifndef INFINITE_PLANE_H
#define INFINITE_PLANE_H

#include "Primitive.h"
#include "common/Types.h"

struct InfinitePlane : public Primitive {
public:
    // Constructor
    InfinitePlane() : normal{ Vec4(0, 1, 0, 0) } { type = PrimitiveType::InfinitePlane; }
    InfinitePlane(Vec3 const &origin, Vec3 const &normal)
    : origin{ Vec4(origin, 0) }, normal{ Vec4(normal, 0) } { type = PrimitiveType::InfinitePlane; }

    // Set
    void setOrigin(Vec3 const &origin) { this->origin = Vec4(origin, 0); }
    void setNormal(Vec3 const &normal) { this->normal = Vec4(glm::normalize(normal), 0); }

    // Bounding box
    float minimumBounds(int dimension) const override;
    float maximumBounds(int dimension) const override;

    virtual void* GetDataLayoutBeginPtr() override { return &origin[0]; }
    virtual size_t GetDataSize() const override { return sizeof(Vec4) * 2; }

protected:
  Vec4 origin, normal;
};

#endif