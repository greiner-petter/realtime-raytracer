#ifndef PRIMITIVE_H
#define PRIMITIVE_H

struct Primitive {
    uint primitiveType;
    int primitiveIndex;
    uint shaderType;
    int shaderIndex;
};

const Primitive NULLPRIMITIVE = Primitive(0u, -1, 0u, -1);

layout(binding = 10, std430) buffer Primitives {
    uint primitiveCount;
    Primitive primitives[];
};

#endif