#include "scene/scene.h"
#include "shader/refractionshader.h"

RefractionShader::RefractionShader(float indexInside, float indexOutside) : indexInside(indexInside), indexOutside(indexOutside) {}

Color RefractionShader::shade(Scene const &scene, Ray const &ray) const {
  // Circumvent getting environment map color into the mix
  if (ray.getRemainingBounces() > 0) {
    // Get the normal of the primitive which was hit
    Vector3d normalVector = ray.normal;

    // Calculate the index of refraction
    float refractiveIndex = indexOutside / indexInside;
    // What if we are already inside the object?
    // Note: This assumes normals pointing outwards, this is not the case for e.g. the Box primitive
    if (dotProduct(normalVector, ray.direction) > 0) {
      normalVector = -normalVector;
      refractiveIndex = indexInside / indexOutside;
    }

    // Using the notation from the lecture
    float cosineTheta = dotProduct(normalVector, -ray.direction);
    float cosinePhi = std::sqrt(1 + refractiveIndex * refractiveIndex * (cosineTheta * cosineTheta - 1)); // NaN if radicant is < 0
    // Calculate t, the new ray direction
    Vector3d t = refractiveIndex * ray.direction + (refractiveIndex * cosineTheta - cosinePhi) * normalVector;

    // Create the refraction ray
    Ray refractionRay = ray;
    // Reset the ray
    refractionRay.length = INFINITY;
    refractionRay.primitive = nullptr;

    // Check whether it is a refraction. NaN enters the else branch.
    if (dotProduct(t, normalVector) <= 0.0) {
      refractionRay.origin = ray.origin + (ray.length + REFR_EPS) * ray.direction;
      refractionRay.direction = normalized(t);
    } else { // Otherwise, it is a total reflection.
      refractionRay.origin = ray.origin + (ray.length - REFR_EPS) * ray.direction;
      // Next we get the reflection vector
      Vector3d const reflectionVector = ray.direction - 2.0f * dotProduct(normalVector, ray.direction) * normalVector;

      // Change the ray direction and origin
      refractionRay.direction = normalized(reflectionVector);
    }

    // Send out a new refracted ray into the scene
    return scene.traceRay(refractionRay);
  }
  return Color(0.0f, 0.0f, 0.0f);
}

bool RefractionShader::isTransparent() const { return true; }
