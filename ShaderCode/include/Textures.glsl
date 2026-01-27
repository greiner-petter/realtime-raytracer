
struct Texture {
    int index;
    int width;
    int height;
};
layout(binding = 51, std430) buffer Textures {
    uint textureCount;
    Texture textures[];
};
layout(binding = 52, std430) buffer TextureData {
    vec4 textureData[];
};

vec4 sampleTex(int ID, vec2 uv) {
    if (ID < 0 || ID >= int(textureCount)) {
        return vec4(0.0); // invalid texture ID
    }
    Texture tex = textures[ID];
    // uv = clamp(uv, vec2(0.0), vec2(1.0));
    uv = fract(uv);

    // Convert UV to pixel coordinates, rounded to nearest integer
    int x = int(round(uv.x * float(tex.width - 1)));
    int y = int(round(uv.y * float(tex.height - 1)));

    x = clamp(x, 0, tex.width - 1);
    y = clamp(y, 0, tex.height - 1);

    int idx = tex.index + y * tex.width + x;
    return textureData[idx];
}