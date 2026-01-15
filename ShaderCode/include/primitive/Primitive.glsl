#include "Sphere.glsl"
#include "Triangle.glsl"
#include "InfinitePlane.glsl"
#include "Box.glsl"

#include "Primitive.h.glsl"

bool intersect(inout Ray ray, in Primitive primitive) {
    switch (primitive.primitiveType) {
        case 1: return intersectSphere(ray, primitive);
        case 2: return intersectTriangle(ray, primitive);
        case 3: return intersectInfinitePlane(ray, primitive);
        case 4: return intersectBox(ray, primitive);
    }
    return false;
}
