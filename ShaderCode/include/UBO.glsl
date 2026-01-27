layout(std140, binding = 0) uniform UBO {
    vec2 u_resolution;
    float u_aspectRatio;
    float u_FocusDistance;
    uint u_SampleIndex;
    float u_Seed;
    uint u_RayBounces;
    uint u_EnableGI;
    uint u_EnvMapTexture;
    vec3 u_CameraPosition;
    vec3 u_CameraForward;
    vec3 u_CameraRight;
    vec3 u_CameraUp;
};