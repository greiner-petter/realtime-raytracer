#include "primitive/Primitive.h.glsl"

bool intersectPrimitive(inout Ray ray, in Primitive primitive);

bool intersectScene(inout Ray ray) {
    bool didHit = false;
    for (int i = 0; i < primitiveCount; ++i) {
        if (intersectPrimitive(ray, primitives[i]))
            didHit = true;
    }
    return didHit;
}

bool occludeScene(inout Ray ray) {
    for (int i = 0; i < primitiveCount; ++i) {
        if (intersectPrimitive(ray, primitives[i]))
            return true;
    }
    return false;
}