// MERL BRDF Shader - GPU Implementation
// Based on MERL BRDF database format

#define BRDF_SAMPLING_RES_THETA_H 90
#define BRDF_SAMPLING_RES_THETA_D 90
#define BRDF_SAMPLING_RES_PHI_D 360

#define RED_SCALE   (1.0 / 1500.0)
#define GREEN_SCALE (1.15 / 1500.0)
#define BLUE_SCALE  (1.66 / 1500.0)

struct BRDFShaderData {
    vec4 dataOffset;  // x = offset into brdfData buffer (in floats)
    vec4 scale;       // xyz = color scale
};

layout(binding = 29, std430) buffer BRDFShaders {
    uint brdfShaderCount;
    uint brdfPadding[3];
    BRDFShaderData brdfShaders[];
};

// BRDF data buffer - contains all BRDF data appended together
layout(binding = 4, std430) buffer BRDFDataBuffer {
    float brdfData[];
};

Ray shadeIndirectLight(in Ray ray, in vec3 diffuseColor, inout vec3 throughput);
Light getRandomLight();

// Rotate vector around axis by angle (Rodrigues' rotation formula)
vec3 rotateVector(vec3 v, vec3 axis, float angle) {
    float cosAng = cos(angle);
    float sinAng = sin(angle);

    vec3 result = v * cosAng;
    float temp = dot(axis, v) * (1.0 - cosAng);
    result += axis * temp;
    result += cross(axis, v) * sinAng;

    return result;
}

// Convert standard coordinates to half-vector/difference-vector coordinates
void stdCoordsToHalfDiffCoords(
    float thetaIn, float phiIn,
    float thetaOut, float phiOut,
    out float thetaHalf, out float phiHalf,
    out float thetaDiff, out float phiDiff
) {
    // Compute in vector
    float inVecZ = cos(thetaIn);
    float projInVec = sin(thetaIn);
    float inVecX = projInVec * cos(phiIn);
    float inVecY = projInVec * sin(phiIn);
    vec3 inVec = normalize(vec3(inVecX, inVecY, inVecZ));

    // Compute out vector
    float outVecZ = cos(thetaOut);
    float projOutVec = sin(thetaOut);
    float outVecX = projOutVec * cos(phiOut);
    float outVecY = projOutVec * sin(phiOut);
    vec3 outVec = normalize(vec3(outVecX, outVecY, outVecZ));

    // Compute halfway vector
    vec3 halfVec = normalize((inVec + outVec) * 0.5);

    // Compute theta_half, phi_half
    thetaHalf = acos(clamp(halfVec.z, -1.0, 1.0));
    phiHalf = atan(halfVec.y, halfVec.x);

    // Compute diff vector by rotating in vector
    vec3 biNormal = vec3(0.0, 1.0, 0.0);
    vec3 normal = vec3(0.0, 0.0, 1.0);

    vec3 temp = rotateVector(inVec, normal, -phiHalf);
    vec3 diff = rotateVector(temp, biNormal, -thetaHalf);

    // Compute theta_diff, phi_diff
    thetaDiff = acos(clamp(diff.z, -1.0, 1.0));
    phiDiff = atan(diff.y, diff.x);
}

// Non-linear theta_half index lookup
// In: [0 .. pi/2], Out: [0 .. 89]
int thetaHalfIndex(float thetaHalf) {
    if (thetaHalf <= 0.0) return 0;

    float thetaHalfDeg = (thetaHalf / (PI * 0.5)) * float(BRDF_SAMPLING_RES_THETA_H);
    float temp = thetaHalfDeg * float(BRDF_SAMPLING_RES_THETA_H);
    temp = sqrt(temp);
    int retVal = int(temp);

    if (retVal < 0) retVal = 0;
    if (retVal >= BRDF_SAMPLING_RES_THETA_H) retVal = BRDF_SAMPLING_RES_THETA_H - 1;

    return retVal;
}

// Linear theta_diff index lookup
// In: [0 .. pi/2], Out: [0 .. 89]
int thetaDiffIndex(float thetaDiff) {
    int tmp = int(thetaDiff / (PI * 0.5) * float(BRDF_SAMPLING_RES_THETA_D));
    if (tmp < 0) return 0;
    if (tmp < BRDF_SAMPLING_RES_THETA_D - 1) return tmp;
    return BRDF_SAMPLING_RES_THETA_D - 1;
}

// Phi_diff index lookup
// In: [-pi .. pi], Out: [0 .. 179]
int phiDiffIndex(float phiDiff) {
    // Because of reciprocity, the BRDF is unchanged under phi_diff -> phi_diff + pi
    if (phiDiff < 0.0) phiDiff += PI;

    int tmp = int(phiDiff / PI * float(BRDF_SAMPLING_RES_PHI_D / 2));
    if (tmp < 0) return 0;
    if (tmp < BRDF_SAMPLING_RES_PHI_D / 2 - 1) return tmp;
    return BRDF_SAMPLING_RES_PHI_D / 2 - 1;
}

// Lookup BRDF values given incoming/outgoing angles and data offset
vec3 lookupBRDF(float thetaIn, float phiIn, float thetaOut, float phiOut, int baseOffset) {
    // Convert to half-angle / difference-angle coordinates
    float thetaHalf, phiHalf, thetaDiff, phiDiff;
    stdCoordsToHalfDiffCoords(thetaIn, phiIn, thetaOut, phiOut,
                              thetaHalf, phiHalf, thetaDiff, phiDiff);

    // Find index (phi_half is ignored since isotropic BRDFs are assumed)
    int ind = phiDiffIndex(phiDiff)
            + thetaDiffIndex(thetaDiff) * (BRDF_SAMPLING_RES_PHI_D / 2)
            + thetaHalfIndex(thetaHalf) * (BRDF_SAMPLING_RES_PHI_D / 2) * BRDF_SAMPLING_RES_THETA_D;

    // Channel offsets in the data array
    int channelOffset = BRDF_SAMPLING_RES_THETA_H * BRDF_SAMPLING_RES_THETA_D * (BRDF_SAMPLING_RES_PHI_D / 2);

    vec3 color;
    color.r = brdfData[baseOffset + ind] * float(RED_SCALE);
    color.g = brdfData[baseOffset + ind + channelOffset] * float(GREEN_SCALE);
    color.b = brdfData[baseOffset + ind + 2 * channelOffset] * float(BLUE_SCALE);

    return max(color, vec3(0.0));
}

vec3 shadeBRDFShaderGI(inout Ray ray, inout vec3 throughput) {
    const BRDFShaderData shader = brdfShaders[ray.primitive.shaderIndex];
    int baseOffset = int(shader.dataOffset.x);
    vec3 scale = shader.scale.xyz;

    // Calculate theta_in (angle between view direction and normal)
    float thetaIn = acos(dot(-ray.normal, ray.direction));
    float phiIn = 0;

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
            float thetaOut = acos(cosine);

            // Project outgoing vector into local coordinate system
            vec3 c = cross(-illum.direction, ray.normal);
            float phiOut = 2 * atan(dot(c, y), dot(c, x));

            color = lookupBRDF(thetaIn, phiIn, thetaOut, phiOut, baseOffset);
        } else {
            color = lookupBRDF(thetaIn, phiIn, 0, 0, baseOffset);
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
    const BRDFShaderData shader = brdfShaders[ray.primitive.shaderIndex];
    int baseOffset = int(shader.dataOffset.x);
    vec3 scale = shader.scale.xyz;
    ray.remainingBounces = 0;

    // Calculate theta_in (angle between view direction and normal)
    float thetaIn = acos(dot(-ray.normal, ray.direction));
    float phiIn = 0;

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
                float thetaOut = acos(cosine);

                // Project outgoing vector into local coordinate system
                vec3 c = cross(-illum.direction, ray.normal);
                float phiOut = 2 * atan(dot(c, y), dot(c, x));

                color = lookupBRDF(thetaIn, phiIn, thetaOut, phiOut, baseOffset);
            } else {
                color = lookupBRDF(thetaIn, phiIn, 0, 0, baseOffset);
            }

            // Calculate colors
            vec3 diffuseColor = scale * color * cosine;
            illuminationColor += diffuseColor * illum.color;
        }
    }

    return illuminationColor;
}
