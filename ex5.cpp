#include <iostream>
#include <string>

#include "camera/perspectivecamera.h"

#include "scene/fastscene.h"

#include "renderer/kdtreerenderer.h"
#include "renderer/simplerenderer.h"

#include "shader/cooktorranceshader.h"

#include "light/ambientlight.h"
#include "light/pointlight.h"
#include "light/spotlight.h"

#include "primitive/objmodel.h"

int main() {
  FastScene scene;
  scene.setEnvironmentMap(std::make_shared<Texture>("data/space.png"));

  // Set up the camera
  PerspectiveCamera camera;
  camera.setFovAngle(90.0f);
  camera.setPosition(Vector3d(0.0f, -2.5f, 10.0f));
  camera.setForwardDirection(Vector3d(0.0f, 0.6f, -1.0f));
  camera.setUpDirection(Vector3d(0.0f, 1.0f, 0.0f));

  // Add shaders
  auto redCook = std::make_shared<CookTorranceShader>(Color(1.0f, 0.0f, 0.0f), Color(1.0f, 1.0f, 1.0f), 1.0f, 0.3f);
  auto goldCook = std::make_shared<CookTorranceShader>(Color(0.83f, 0.69f, 0.22f), Color(1.0f, 1.0f, 0.0f), 1.2f, 0.2f);

  scene.addObj("data/teapot_stadium.obj", Vector3d(1.0f, 1.0f, 1.0f) * 40.0f, Vector3d(-0.25f, -0.5f, -2.5f), goldCook);
  scene.addObj("data/stadium.obj", Vector3d(1.0f, 1.0f, 1.0f) * 70.0f, Vector3d(-0.5f, -1.0f, -3.0f), redCook);

  // Add lights
  scene.add(std::make_shared<AmbientLight>(0.1f));
  scene.add(std::make_shared<PointLight>(Vector3d(-0.25f, 20.0f, -3.0f), 60.0f));
  scene.add(
      std::make_shared<SpotLight>(Vector3d(-0.25f, 7.0f, -4.0f), Vector3d(0.0f, -1.0f, 0.0f), 10.0f, 30.0f, 25.0f));

  // build the tree
  scene.buildTree();

  // Render the scene
  SimpleRenderer renderer;
  renderer.renderImage(scene, camera, 1920, 1080).save("result.png");

  KDTreeRenderer kd_renderer;
  kd_renderer.renderKDTree(scene, camera, 1920, 1080).save("result_kd.png");

  return 0;
}
