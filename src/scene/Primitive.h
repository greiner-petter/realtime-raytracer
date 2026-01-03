#ifndef PRIMITIVE_H
#define PRIMITIVE_H

#include "common/Types.h"

enum class PrimitiveType : uint32_t {
    None = 0,
    Sphere = 1,
    Triangle = 2,
};

struct Primitive {

    // Bounding box
    virtual float minimumBounds(int dimension) const = 0;
    virtual float maximumBounds(int dimension) const = 0;

    Vec3 minimumBounds() const { return Vec3(minimumBounds(0), minimumBounds(1), minimumBounds(2)); }
    Vec3 maximumBounds() const { return Vec3(maximumBounds(0), maximumBounds(1), maximumBounds(2)); }

    int32_t materialID = 1;
};

#endif