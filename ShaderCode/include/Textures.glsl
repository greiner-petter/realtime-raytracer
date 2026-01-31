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

vec4 getTexPixelAt(Texture tex, int x, int y) {
    x = x % tex.width;
    if (x < 0) x += tex.width;
    y = y % tex.height;
    if (y < 0) y += tex.height;
    int idx = tex.index + y * tex.width + x;
    return textureData[idx];
}

vec4 sampleTex(int ID, vec2 uv) {
    if (ID < 0 || ID >= int(textureCount)) {
        return vec4(0.0); // invalid texture ID
    }
    Texture tex = textures[ID];
    uv = fract(uv);

    // bilinear interpolation
    // adjacent pixel coordinates
    float px = uv.x * float(tex.width);
    float py = uv.y * float(tex.height);

    int left = int(floor(px));
    int right = int(ceil(px));
    int top = int(floor(py));
    int bottom = int(ceil(py));

    // weights
    float w0 = float(right) - px;
    float w1 = 1.0 - w0;
    float w2 = float(bottom) - py;
    float w3 = 1.0 - w2;

    // get color values and interpolate
    vec4 val0 = getTexPixelAt(tex, left, top);
    vec4 val1 = getTexPixelAt(tex, right, top);
    vec4 val2 = getTexPixelAt(tex, left, bottom);
    vec4 val3 = getTexPixelAt(tex, right, bottom);

    return w2 * w0 * val0 + w2 * w1 * val1 + w3 * w0 * val2 + w3 * w1 * val3;
}