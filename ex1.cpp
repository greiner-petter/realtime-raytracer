#include "camera/perspectivecamera.h"

#include "renderer/simplerenderer.h"

#include "scene/simplescene.h"

#include "primitive/infiniteplane.h"
#include "primitive/sphere.h"
#include "primitive/triangle.h"

#include "shader/flatshader.h"

int main() {
  // Let's create a simple cornell box scene...
  SimpleScene scene;

  // Add shaders for the walls
  auto red = std::make_shared<FlatShader>(Color(1.0f, 0.3f, 0.2f));
  auto white = std::make_shared<FlatShader>(Color(1.0f, 1.0f, 1.0f));
  auto blue = std::make_shared<FlatShader>(Color(0.2f, 0.3f, 1.0f));

  // Add shaders for the objects
  auto green = std::make_shared<FlatShader>(Color(0.0f, 1.0f, 0.0f));
  auto purple = std::make_shared<FlatShader>(Color(0.5f, 0.2f, 1.0f));
  auto orange = std::make_shared<FlatShader>(Color(1.0f, 0.5f, 0.0f));

  // Set up the cornell box walls
  scene.add(std::make_shared<InfinitePlane>(Vector3d(0.0f, 0.0f, +5.0f), Vector3d(0.0f, 0.0f, -1.0f), purple));
  scene.add(std::make_shared<InfinitePlane>(Vector3d(0.0f, 0.0f, -5.0f), Vector3d(0.0f, 0.0f, +1.0f), purple));
  scene.add(std::make_shared<InfinitePlane>(Vector3d(0.0f, +5.0f, 0.0f), Vector3d(0.0f, -1.0f, 0.0f), white));
  scene.add(std::make_shared<InfinitePlane>(Vector3d(0.0f, -5.0f, 0.0f), Vector3d(0.0f, +1.0f, 0.0f), white));
  scene.add(std::make_shared<InfinitePlane>(Vector3d(+5.0f, 0.0f, 0.0f), Vector3d(-1.0f, 0.0f, 0.0f), blue));
  scene.add(std::make_shared<InfinitePlane>(Vector3d(-5.0f, 0.0f, 0.0f), Vector3d(+1.0f, 0.0f, 0.0f), red));

  // Add a sphere
  scene.add(std::make_shared<Sphere>(Vector3d(-3.0f, 0.0f, 0.0f), 1.5f, green));
  scene.add(std::make_shared<Triangle>(Vector3d(0.0f, -5.0f, -4.0f), Vector3d(0.0f, -3.0f, 0.0f),
                                       Vector3d(5.0f, -2.0f, -3.0f), orange));

  // Set up the camera
  PerspectiveCamera camera;
  camera.setFovAngle(70);
  camera.setPosition(Vector3d(-2.5f, 2.5f, -10.0f));
  camera.setForwardDirection(Vector3d(0.25f, -0.33f, 1.0f));
  camera.setUpDirection(Vector3d(0.2f, 1.0f, 0.0f));

  // Render the scene
  SimpleRenderer renderer;
  renderer.renderImage(scene, camera, 512, 512).save("result.png");

  return 0;
}
