
struct Primitive {
    uint type;
    int index;
    int materialID;
};

struct Sphere {
    vec4 center_radius; // xyz = center, w = radius
};

struct Triangle {
    vec4 vertex[3];
    vec4 normal[3];
    vec4 tangent[3];
    vec4 bitangent[3];
    vec4 surface[3];
};