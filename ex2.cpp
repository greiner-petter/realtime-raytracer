#include "camera/perspectivecamera.h"

#include "renderer/simplerenderer.h"

#include "scene/simplescene.h"

#include "primitive/box.h"
#include "primitive/infiniteplane.h"
#include "primitive/sphere.h"
#include "primitive/triangle.h"

#include "shader/mirrorshader.h"
#include "shader/refractionshader.h"
#include "shader/simpleshadowshader.h"

#include "light/pointlight.h"

int main() {
  // Let's create a simple cornell box scene...
  SimpleScene scene;
  scene.setEnvironmentMap(std::make_shared<Texture>("data/lion_env.png"));

  auto mirror = std::make_shared<MirrorShader>();
  auto glass = std::make_shared<RefractionShader>(1.31f, 1.0f);

  // add some lights
  scene.add(std::make_shared<PointLight>(Vector3d(0.0f, 10.0f, -10.0f), 250.f, Color(1.0f, 1.0f, 1.0f)));
  scene.add(std::make_shared<PointLight>(Vector3d(0.0f, 10.0f, 10.0f), 250.f, Color(1.0f, 1.0f, 1.0f)));

  // Add shaders for the objects
  auto orange = std::make_shared<SimpleShadowShader>(Color(1.0f, 0.5f, 0.0f));
  auto red = std::make_shared<SimpleShadowShader>(Color(1.0f, 0.3f, 0.2f));

  // Add objects
  scene.add(std::make_shared<Sphere>(Vector3d(-3.0f, 0.0f, 0.0f), 1.5f, glass));
  scene.add(std::make_shared<Box>(Vector3d(3.5f, -1.0f, 0.0f), Vector3d(3.0f, 3.0f, 3.0f), mirror));
  scene.add(std::make_shared<Triangle>(Vector3d(5.0f, -5.0f, 5.0f), Vector3d(-10.0f, -5.0f, 10.0f),
                                       Vector3d(-2.0f, -5.0f, -2.0f), orange));
  scene.add(std::make_shared<Triangle>(Vector3d(-2.0f, -5.0f, -2.0f), Vector3d(10.0f, -5.0f, -10.0f),
                                       Vector3d(5.0f, -5.0f, 5.0f), orange));
  scene.add(std::make_shared<Triangle>(Vector3d(0.0f, -2.0f, 0.0f), Vector3d(2.0f, -2.0f, 0.0f),
                                       Vector3d(0.0f, 0.0f, 0.0f), red));

  // Set up the camera
  PerspectiveCamera camera;
  camera.setFovAngle(70.0f);
  camera.setPosition(Vector3d(-2.5f, 2.5f, -10.0f));
  camera.setForwardDirection(Vector3d(0.4f, -0.33f, 1.0f));
  camera.setUpDirection(Vector3d(0.0f, 1.0f, 0.0f));

  // Render the scene
  SimpleRenderer renderer;
  renderer.renderImage(scene, camera, 1024, 768).save("result.png");

  return 0;
}
