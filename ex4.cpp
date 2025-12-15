#include <iostream>
#include <string>

#include "camera/perspectivecamera.h"
#include "renderer/simplerenderer.h"
#include "scene/simplescene.h"

#include "primitive/box.h"
#include "primitive/infiniteplane.h"
#include "primitive/objmodel.h"
#include "primitive/sphere.h"

#include "shader/brdfshader.h"

#include "shader/lambertshader.h"
#include "shader/mirrorshader.h"
#include "shader/phongshader.h"
#include "shader/cooktorranceshader.h"

#include "light/ambientlight.h"
#include "light/pointlight.h"
#include "light/spotlight.h"

int main() {
  // Let's create a simple scene...
  SimpleScene scene;

  // Add shaders
  auto mirror = std::make_shared<MirrorShader>();
  auto white = std::make_shared<LambertShader>(Color(0.9f, 0.9f, 0.9f));
  auto red = std::make_shared<LambertShader>(Color(1.0f, 0.3f, 0.2f));
  auto blue = std::make_shared<LambertShader>(Color(0.2f, 0.3f, 1.0f));
  auto orange = std::make_shared<PhongShader>(Color(1.0f, 0.64f, 0.0f), 1.0f, Color(1.0f, 1.0f, 1.0f), 1.0f, 25.0f);
  auto gold= std::make_shared<CookTorranceShader>(Color(0.83f, 0.69f, 0.22f), Color(1.0f, 1.0f, 0.0f), 1.2f, 0.2f);
  auto blueMetallic = std::make_shared<BrdfShader>("data/blue-metallic-paint.binary", Color(7.0f, 7.0f, 7.0f));
  auto darkRed = std::make_shared<BrdfShader>("data/dark-red-paint.binary", Color(7.0f, 7.0f, 7.0f));

  // Set up the walls
  // ---------------------------------------------------------------------------
  scene.add(std::make_shared<InfinitePlane>(Vector3d(0.0f, 0.0f, +5.0f), Vector3d(0.0f, 0.0f, -1.0f), mirror));

  scene.add(std::make_shared<InfinitePlane>(Vector3d(0.0f, 0.0f, -5.0f), Vector3d(0.0f, 0.0f, +1.0f), mirror));
  scene.add(std::make_shared<InfinitePlane>(Vector3d(0.0f, +5.0f, 0.0f), Vector3d(0.0f, -1.0f, 0.0f), white));
  scene.add(std::make_shared<InfinitePlane>(Vector3d(0.0f, -5.0f, 0.0f), Vector3d(0.0f, +1.0f, 0.0f), white));
  scene.add(std::make_shared<InfinitePlane>(Vector3d(+5.0f, 0.0f, 0.0f), Vector3d(-1.0f, 0.0f, 0.0f), blue));
  scene.add(std::make_shared<InfinitePlane>(Vector3d(-5.0f, 0.0f, 0.0f), Vector3d(+1.0f, 0.0f, 0.0f), red));

  scene.add(std::make_shared<Sphere>(Vector3d(-3.0f, 0.0f, 0.0f), 1.0f, blueMetallic));
  scene.add(std::make_shared<Sphere>(Vector3d(0.0f, 2.0f, 0.0f), 1.0f, orange));
  scene.add(std::make_shared<Sphere>(Vector3d(3.0f, 0.0f, 0.0f), 1.0f, darkRed));

  // Add the teapot
  auto teapot = std::make_shared<ObjModel>(gold);
  teapot->loadObj("data/teapot.obj", Vector3d(3.0f, 3.0f, 3.0f), Vector3d(0.0f, -5.0f, 0.0f));
  scene.add(teapot);

  // Add ambient light
  scene.add(std::make_shared<AmbientLight>(0.15f));
  scene.add(std::make_shared<PointLight>(Vector3d(0.0f, 4.0f, -4.0f), 15.0f));
  scene.add(std::make_shared<PointLight>(Vector3d(0.0f, 4.0f, 4.0f), 15.0f));

  // Set up the camera
  PerspectiveCamera camera;
  camera.setFovAngle(90.0f);
  camera.setPosition(Vector3d(0.0f, 0.0f, -10.0f));
  camera.setForwardDirection(Vector3d(0.0f, 0.0f, 1.0f));
  camera.setUpDirection(Vector3d(0.0f, 1.0f, 0.0f));

  // Render the scene
  SimpleRenderer renderer;
  renderer.renderImage(scene, camera, 512, 512).save("result.png");

  return 0;
}
