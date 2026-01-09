#ifndef BOX_H
#define BOX_H

#include "Primitive.h"
#include "common/Types.h"

struct Box : public TypedPrimitive<PrimitiveType::Box> {
    Box(std::shared_ptr<Shader> const &shader) : TypedPrimitive(shader), size(Vec4(1, 1, 1, 0)) {}

    Box(Vec3 const &center, Vec3 const &size, std::shared_ptr<Shader> const &shader)
        : TypedPrimitive(shader), center(Vec4(center, 0)), size(Vec4(size, 0)) {}

    void setCenter(Vec3 const &center) { this->center = Vec4(center, 0); }
    void setSize(Vec3 const &size) { this->size = Vec4(size, 0); }

    float minimumBounds(int dimension) const { return this->center[dimension] - this->size[dimension] / 2; }
    float maximumBounds(int dimension) const { return this->center[dimension] + this->size[dimension] / 2; }

    virtual void* GetDataLayoutBeginPtr() override { return &center[0]; }
    virtual size_t GetDataSize() const override { return sizeof(Vec4) * 2; }

    Vec4 center, size;
};

#endif
