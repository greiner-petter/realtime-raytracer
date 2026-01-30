// MERL BRDF Shader - GPU Implementation
// Based on MERL BRDF database format

#define BRDF_SAMPLING_RES_THETA_H 90
#define BRDF_SAMPLING_RES_THETA_D 90
#define BRDF_SAMPLING_RES_PHI_D 360
#define BRDF_DATA_SIZE (BRDF_SAMPLING_RES_THETA_H * BRDF_SAMPLING_RES_THETA_D * BRDF_SAMPLING_RES_PHI_D / 2 * 3)

#define RED_SCALE   (1.0 / 1500.0)
#define GREEN_SCALE (1.15 / 1500.0)
#define BLUE_SCALE  (1.66 / 1500.0)

struct BrdfShader {
    vec4 scaleIndex;  // xyz = color scale, w = data offset into brdfData
};

layout(binding = 29, std430) buffer BrdfShaders {
    uint brdfShaderCount;
    BrdfShader brdfShaders[];
};

// BRDF data buffer (all entries concatenated)
layout(binding = 4, std430) buffer BrdfDataBuffer {
    float brdfData[];
};

Ray shadeIndirectLight(in Ray ray, in vec3 diffuseColor, inout vec3 throughput);
Light getRandomLight();

vec3 rotate_vector(vec3 vector, vec3 axis, float angle) {
    float cos_ang = cos(angle);
    float sin_ang = sin(angle);

    vec3 out_vec = vector * cos_ang;
    float temp = dot(axis, vector) * (1.0 - cos_ang);
    out_vec += axis * temp;
    out_vec += cross(axis, vector) * sin_ang;

    return out_vec;
}

void std_coords_to_half_diff_coords(float theta_in, float phi_in, float theta_out, float phi_out,
                                    out float theta_half, out float phi_half, out float theta_diff,
                                    out float phi_diff) {
    // Compute in vector
    float in_vec_z = cos(theta_in);
    float proj_in_vec = sin(theta_in);
    float in_vec_x = proj_in_vec * cos(phi_in);
    float in_vec_y = proj_in_vec * sin(phi_in);
    vec3 in_vec = normalize(vec3(in_vec_x, in_vec_y, in_vec_z));

    // Compute out vector
    float out_vec_z = cos(theta_out);
    float proj_out_vec = sin(theta_out);
    float out_vec_x = proj_out_vec * cos(phi_out);
    float out_vec_y = proj_out_vec * sin(phi_out);
    vec3 out_vec = normalize(vec3(out_vec_x, out_vec_y, out_vec_z));

    // Compute halfway vector
    float half_x = (in_vec_x + out_vec_x) / 2.0;
    float half_y = (in_vec_y + out_vec_y) / 2.0;
    float half_z = (in_vec_z + out_vec_z) / 2.0;
    vec3 half_vec = normalize(vec3(half_x, half_y, half_z));

    // Compute theta_half, phi_half
    theta_half = acos(half_vec.z);
    phi_half = atan(half_vec.y, half_vec.x);

    // Compute diff vector
    vec3 bi_normal = vec3(0.0, 1.0, 0.0);
    vec3 normal = vec3(0.0, 0.0, 1.0);

    vec3 temp = rotate_vector(in_vec, normal, -phi_half);
    vec3 diff = rotate_vector(temp, bi_normal, -theta_half);

    // Compute theta_diff, phi_diff
    theta_diff = acos(diff.z);
    phi_diff = atan(diff.y, diff.x);
}

int theta_half_index(float theta_half) {
    if (theta_half <= 0.0)
        return 0;
    float theta_half_deg = ((theta_half / (PI / 2.0)) * BRDF_SAMPLING_RES_THETA_H);
    float temp = theta_half_deg * BRDF_SAMPLING_RES_THETA_H;
    temp = sqrt(temp);
    int ret_val = int(temp);
    if (ret_val < 0)
        ret_val = 0;
    if (ret_val >= BRDF_SAMPLING_RES_THETA_H)
        ret_val = BRDF_SAMPLING_RES_THETA_H - 1;

    return ret_val;
}

int theta_diff_index(float theta_diff) {
    int tmp = int(theta_diff / (PI * 0.5) * BRDF_SAMPLING_RES_THETA_D);
    if (tmp < 0)
        return 0;
    else if (tmp < BRDF_SAMPLING_RES_THETA_D - 1)
        return tmp;
    else
        return BRDF_SAMPLING_RES_THETA_D - 1;
}

int phi_diff_index(float phi_diff) {
    // Because of reciprocity, the BRDF is unchanged under
    // phi_diff -> phi_diff + M_PI
    if (phi_diff < 0.0)
        phi_diff += PI;

    // In: phi_diff in [0 .. pi]
    // Out: tmp in [0 .. 179]
    int tmp = int(phi_diff / PI * BRDF_SAMPLING_RES_PHI_D / 2);
    if (tmp < 0)
        return 0;
    else if (tmp < BRDF_SAMPLING_RES_PHI_D / 2 - 1)
        return tmp;
    else
        return BRDF_SAMPLING_RES_PHI_D / 2 - 1;
}

vec3 lookup_brdf_values(float theta_in, float phi_in, float theta_out, float phi_out, int baseOffset) {
    // Convert to halfangle / difference angle coordinates
    float theta_half, phi_half, theta_diff, phi_diff;
    std_coords_to_half_diff_coords(theta_in, phi_in, theta_out, phi_out, theta_half, phi_half, theta_diff, phi_diff);

    // Find index.
    // Note that phi_half is ignored, since isotropic BRDFs are assumed
    int ind = phi_diff_index(phi_diff) + theta_diff_index(theta_diff) * BRDF_SAMPLING_RES_PHI_D / 2 +
              theta_half_index(theta_half) * BRDF_SAMPLING_RES_PHI_D / 2 * BRDF_SAMPLING_RES_THETA_D;

    vec3 color;
    color.r = brdfData[baseOffset + ind] * RED_SCALE;
    color.g = brdfData[baseOffset + ind + BRDF_SAMPLING_RES_THETA_H * BRDF_SAMPLING_RES_THETA_D * BRDF_SAMPLING_RES_PHI_D / 2] * GREEN_SCALE;
    color.b = brdfData[baseOffset + ind + BRDF_SAMPLING_RES_THETA_H * BRDF_SAMPLING_RES_THETA_D * BRDF_SAMPLING_RES_PHI_D] * BLUE_SCALE;

    return color;
}

vec3 shadeBRDFShaderGI(inout Ray ray, inout vec3 throughput) {
    BrdfShader shader = brdfShaders[ray.primitive.shaderIndex];
    vec3 scale = shader.scaleIndex.xyz;
    int dataOffset = int(shader.scaleIndex.w);

    // Calculate theta_in (angle between view direction and normal)
    float theta_in = acos(dot(-ray.normal, ray.direction));
    float phi_in = 0;

    // Derive local coordinate system
    vec3 x = cross(-ray.direction, ray.normal);
    vec3 y = cross(ray.normal, x);

    vec3 illuminationColor = vec3(0);
    Illumination illum = illuminate(ray, getRandomLight());

    // Diffuse term
    float cosine = dot(-illum.direction, ray.normal);
    if (cosine > 0) {
        vec3 color = vec3(0);

        // Avoid numeric instability
        if (cosine < 1) {
            float theta_out = acos(cosine);

            // Project outgoing vector into local coordinate system
            vec3 c = cross(-illum.direction, ray.normal);
            float phi_out = 2 * atan(dot(c, y), dot(c, x));

            color = lookup_brdf_values(theta_in, phi_in, theta_out, phi_out, dataOffset);
        } else {
            color = lookup_brdf_values(theta_in, phi_in, 0, 0, dataOffset);
        }

        // Calculate colors
        vec3 diffuseColor = scale * color * cosine * lightCount;
        illuminationColor += diffuseColor * illum.color;

        ray = shadeIndirectLight(ray, diffuseColor, throughput);
    } else {
        ray.remainingBounces = 0;
    }

    return illuminationColor;
}

vec3 shadeBRDFShader(inout Ray ray, inout vec3 throughput) {
    BrdfShader shader = brdfShaders[ray.primitive.shaderIndex];
    vec3 scale = shader.scaleIndex.xyz;
    int dataOffset = int(shader.scaleIndex.w);
    ray.remainingBounces = 0;

    // Calculate theta_in (angle between view direction and normal)
    float theta_in = acos(dot(-ray.normal, ray.direction));
    float phi_in = 0;

    // Derive local coordinate system
    vec3 x = cross(-ray.direction, ray.normal);
    vec3 y = cross(ray.normal, x);

    vec3 illuminationColor = vec3(0);
    // Accumulate the light over all light sources
    for (int i = 0; i < lightCount; i++) {
        Illumination illum = illuminate(ray, lights[i]);

        // Diffuse term
        float cosine = dot(-illum.direction, ray.normal);
        if (cosine > 0) {
            vec3 color = vec3(0);

            // Avoid numeric instability
            if (cosine < 1) {
                float theta_out = acos(cosine);

                // Project outgoing vector into local coordinate system
                vec3 c = cross(-illum.direction, ray.normal);
                float phi_out = 2 * atan(dot(c, y), dot(c, x));

                color = lookup_brdf_values(theta_in, phi_in, theta_out, phi_out, dataOffset);
            } else {
                color = lookup_brdf_values(theta_in, phi_in, 0, 0, dataOffset);
            }

            // Calculate colors
            vec3 diffuseColor = scale * color * cosine;
            illuminationColor += diffuseColor * illum.color;
        }
    }

    return illuminationColor;
}
