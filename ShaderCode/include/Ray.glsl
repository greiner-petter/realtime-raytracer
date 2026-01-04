struct Ray {
    vec3 origin;
    vec3 direction;
};

struct Hit {
    float rayLength;
    vec3 point;
    vec3 normal;
    vec2 surface;
    vec3 tangent;
    vec3 bitangent;
    int materialID;
    int primitiveIndex; // -1 if no hit
};