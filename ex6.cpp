#include <iostream>
#include <string>

#include "camera/perspectivecamera.h"

#include "scene/fastscene.h"

#include "primitive/sphere.h"

#include "renderer/simplerenderer.h"
#ifdef SUPERRENDERER_FOUND
#include "renderer/superrenderer.h"
#endif

#include "shader/materialshader.h"

#include "light/ambientlight.h"
#include "light/pointlight.h"
#include "light/spotlight.h"

int main() {
  FastScene scene;
  scene.setEnvironmentMap(std::make_shared<Texture>("data/TychoSkymapII.t5_04096x02048.png"));

  // Set up the camera
  PerspectiveCamera camera;
  camera.setPosition(Vector3d(0.0f, -0.2f, 0.0f));
  camera.setForwardDirection(normalized(Vector3d(-1.0f, -0.15f, 0.6f)));
  camera.setUpDirection(normalized(Vector3d(0.0f, -1.0f, 0.0f)));
  camera.setFovAngle(90.0f);

  // Add the earth
  auto earthDiffuse = std::make_shared<Texture>("data/earth_diffuse.ppm");
  auto earthSpecular = std::make_shared<Texture>("data/earth_specular.ppm");
  auto earthNormal = std::make_shared<Texture>("data/earth_normal.ppm");
  auto earthClouds = std::make_shared<Texture>("data/earth_clouds.ppm");
  auto earthCloudShader = std::make_shared<MaterialShader>();
  earthCloudShader->setDiffuseMap(earthClouds);
  earthCloudShader->setAlphaMap(earthClouds);
  earthCloudShader->setDiffuseCoefficient(0.9f);
  earthCloudShader->setSpecularCoefficient(0.1f);
  auto earthShader = std::make_shared<MaterialShader>();
  earthShader->setDiffuseMap(earthDiffuse);
  earthShader->setDiffuseCoefficient(0.5f);
  earthShader->setSpecularMap(earthSpecular);
  earthShader->setSpecularCoefficient(0.5f);
  earthShader->setShininessExponent(15.0f);
  earthShader->setNormalMap(earthNormal);
  scene.add(std::make_shared<Sphere>(Vector3d(-50.0f, 0.0f, 60.0f), 20.0f, earthShader));
  scene.add(std::make_shared<Sphere>(Vector3d(-50.0f, 0.0f, 60.0f), 20.05f, earthCloudShader));

  // Add the spaceship
  auto spaceshipDiffuse = std::make_shared<Texture>("data/space_frigate_6_diffuse.ppm");
  auto spaceshipSpecular = std::make_shared<Texture>("data/space_frigate_6_specular.ppm");
  auto spaceshipNormal = std::make_shared<Texture>("data/space_frigate_6_normal.ppm");
  auto spaceshipReflection = std::make_shared<Texture>("data/space_frigate_6_specular.ppm");
  auto spaceshipShader = std::make_shared<MaterialShader>();
  spaceshipShader->setDiffuseMap(spaceshipDiffuse);
  spaceshipShader->setDiffuseCoefficient(0.75f);
  spaceshipShader->setSpecularMap(spaceshipSpecular);
  spaceshipShader->setNormalMap(spaceshipNormal);
  spaceshipShader->setNormalCoefficient(0.8f);
  spaceshipShader->setSpecularCoefficient(0.25f);
  spaceshipShader->setShininessExponent(30.0f);
  spaceshipShader->setReflectionMap(spaceshipReflection);
  spaceshipShader->setReflectance(0.75f);

  scene.addObj("data/space_frigate_6.obj", Vector3d(-1.0f, 1.0f, 1.0f) / 20.0f, Vector3d(-1.3f, -0.3f, 0.7f),
               spaceshipShader);

  // Add lights
  scene.add(std::make_shared<PointLight>(Vector3d(0.0f, 0.0f, 30.0f), 1000.0f));
  scene.add(std::make_shared<AmbientLight>(0.3f));

  // build the tree
  scene.buildTree();

  // Render the scene
  SimpleRenderer renderer;
  renderer.renderImage(scene, camera, 1024, 768).save("result.png");

#ifdef SUPERRENDERER_FOUND
  SuperRenderer sr;
  sr.setSuperSamplingFactor(4);
  sr.renderImage(scene, camera, 1024, 768).save("result_super.png");
#endif

  return 0;
}
