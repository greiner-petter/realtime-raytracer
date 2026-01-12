#ifndef PRIMITIVE_H
#define PRIMITIVE_H

#include "common/Types.h"
#include "shaders/Shader.h"

enum class PrimitiveType : uint32_t {
    None = 0,
    Sphere = 1,
    Triangle = 2,
    InfinitePlane = 3,
    Box = 4,
};

struct Primitive {
    Primitive(const std::shared_ptr<Shader> &shader, const PrimitiveType type) : shader(shader), type(type) {}
    virtual ~Primitive() = default;

    // Bounding box
    virtual float minimumBounds(int dimension) const = 0;
    virtual float maximumBounds(int dimension) const = 0;

    // Bounding box
    Vec3 minimumBounds() const { return Vec3(minimumBounds(0), minimumBounds(1), minimumBounds(2)); }
    Vec3 maximumBounds() const { return Vec3(maximumBounds(0), maximumBounds(1), maximumBounds(2)); }

    virtual void* GetDataLayoutBeginPtr() = 0;
    virtual size_t GetDataSize() const = 0;

    PrimitiveType type = PrimitiveType::None;
    int32_t index = -1; // Index in the specific primitive array (spheres, triangles, etc.)
    std::shared_ptr<Shader> shader;
};

template <PrimitiveType Type>
struct TypedPrimitive : public Primitive {
    explicit TypedPrimitive(const std::shared_ptr<Shader>& shader) : Primitive(shader, Type) {}
};

#endif