#include <iostream>
#include <string>

#include "camera/perspectivecamera.h"

#include "renderer/simplerenderer.h"

#include "scene/simplescene.h"

#include "primitive/box.h"
#include "primitive/infiniteplane.h"
#include "primitive/sphere.h"
#include "primitive/triangle.h"
#ifdef OBJMODEL_FOUND
#include "primitive/objmodel.h"
#endif

#include "shader/simpleshadowshader.h"

#include "light/pointlight.h"
#ifdef AMBIENTLIGHT_FOUND
#include "light/ambientlight.h"
#endif
#ifdef SPOTLIGHT_FOUND
#include "light/spotlight.h"
#endif


int main() {
  // Let's create a simple cornell box scene...
  SimpleScene scene;

  // Add shaders for the walls
  auto red = std::make_shared<SimpleShadowShader>(Color(1.0f, 0.3f, 0.2f));
  auto white = std::make_shared<SimpleShadowShader>(Color(1.0f, 1.0f, 1.0f));
  auto blue = std::make_shared<SimpleShadowShader>(Color(0.2f, 0.3f, 1.0f));
  auto orange = std::make_shared<SimpleShadowShader>(Color(1.0f, 0.5f, 0.0f));

  // Set up the cornell box walls
  scene.add(std::make_shared<InfinitePlane>(Vector3d(0.0f, 0.0f, +5.0f), Vector3d(0.0f, 0.0f, -1.0f), white));
  scene.add(std::make_shared<InfinitePlane>(Vector3d(0.0f, 0.0f, -5.0f), Vector3d(0.0f, 0.0f, +1.0f), white));
  scene.add(std::make_shared<InfinitePlane>(Vector3d(0.0f, +5.0f, 0.0f), Vector3d(0.0f, -1.0f, 0.0f), white));
  scene.add(std::make_shared<InfinitePlane>(Vector3d(0.0f, -5.0f, 0.0f), Vector3d(0.0f, +1.0f, 0.0f), white));
  scene.add(std::make_shared<InfinitePlane>(Vector3d(+5.0f, 0.0f, 0.0f), Vector3d(-1.0f, 0.0f, 0.0f), blue));
  scene.add(std::make_shared<InfinitePlane>(Vector3d(-5.0f, 0.0f, 0.0f), Vector3d(+1.0f, 0.0f, 0.0f), red));

  scene.add(std::make_shared<Box>(Vector3d(2.5f, -3.0f, 1.0f), Vector3d(3.0f, 4.0f, 3.0f), red));
  scene.add(std::make_shared<Box>(Vector3d(-3.0f, -2.0f, 0.0f), Vector3d(1.0f, 6.0f, 1.0f), blue));
  scene.add(std::make_shared<Box>(Vector3d(-0.5f, -4.0f, -2.0f), Vector3d(2.0f, 2.0f, 2.0f), orange));

  // Add teapots (for faster rendering: try commenting two of them out and
  // compile in release mode)
#ifdef OBJMODEL_FOUND
  auto teapot0 = std::make_shared<ObjModel>(orange);
  teapot0->loadObj("data/teapot.obj", Vector3d(2.0f, 2.0f, 2.0f), Vector3d(2.5f, -1.0f, 1.0f));

  auto teapot1 = std::make_shared<ObjModel>(red);
  teapot1->loadObj("data/teapot.obj", Vector3d(2.0f, 2.0f, 2.0f), Vector3d(-3.0f, 1.0f, 0.0f));

  auto teapot2 = std::make_shared<ObjModel>(blue);
  teapot2->loadObj("data/teapot.obj", Vector3d(2.0f, 2.0f, 2.0f), Vector3d(-0.5f, -3.0f, -2.0f));

  scene.add(teapot0);
  scene.add(teapot1);
  scene.add(teapot2);
#else
  scene.addObj("data/teapot.obj", Vector3d(2.0f, 2.0f, 2.0f), Vector3d(2.5f, -1.0f, 1.0f), orange);
  scene.addObj("data/teapot.obj", Vector3d(2.0f, 2.0f, 2.0f), Vector3d(-3.0f, 1.0f, 0.0f), red);
  scene.addObj("data/teapot.obj", Vector3d(2.0f, 2.0f, 2.0f), Vector3d(-0.5f, -3.0f, -2.0f), blue);
#endif

  // Add ambient light
#ifdef AMBIENTLIGHT_FOUND
  scene.add(std::make_shared<AmbientLight>(0.15f));
#endif

  // Lighting situation 1
#if SITUATION == 1
  scene.add(std::make_shared<PointLight>(Vector3d(0.0f, 4.0f, 0.0f), 10.0f));
#endif

  // Lighting situation 2
#if SITUATION == 2
#ifdef SPOTLIGHT_FOUND
  // parameters are: position, direction, inner cone angle, outer cone angle (in
  // degrees), intensity
  scene.add(std::make_shared<SpotLight>(Vector3d(1.0f, 0.0f, -4.0f), Vector3d(0.0f, 0.0f, 1.0f), 50.0f, 70.0f, 20.0f));
#endif
#endif


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
