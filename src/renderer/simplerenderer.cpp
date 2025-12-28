#include "camera/camera.h"
#include "renderer/simplerenderer.h"
#include "scene/scene.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>

void SimpleRenderer::renderThread(const Scene *scene, Camera const *camera, Texture *image, int width, int widthStep, int widthOffset, int height, int heightStep, int heightOffset, std::atomic<int> *k, int const stepSize) {
  float const aspectRatio = static_cast<float>(height) / width;
  for (int y = heightOffset; y < height; y += heightStep) {
    for (int x = widthOffset; x < width; x += widthStep) {
      Ray ray = camera->createRay((static_cast<float>(x) / width * 2 - 1), -(static_cast<float>(y) / height * 2 - 1) * aspectRatio);
      image->setPixelAt(x, y, clamped(scene->traceRay(ray)));

      // Super hacky progress bar!
      if (++*k % stepSize == 0) {
        std::cout << "=" << std::flush;
      }
    }
  }
}

Texture SimpleRenderer::renderImage(Scene const &scene, Camera const &camera, int width, int height) {
  Texture image(width, height);

  // Setup timer
  std::chrono::steady_clock::time_point start, stop;

  // Reset Ray counting
  Ray::resetRayCount();

  // Super-hacky progress bar!
  std::cout << "(SimpleRenderer): Begin rendering..." << std::endl;
  std::cout << "| 0%";
  int const barSize = 50;
  int const stepSize = (width * height) / barSize;
  for (int i = 0; i < barSize - 3 - 5; ++i)
    std::cout << " ";
  std::cout << "100% |" << std::endl << "|";
  std::atomic<int> k(0);

  // Start timer
  start = std::chrono::steady_clock::now();

  // Spawn a thread for every logical processor -1, calling the renderThread function
  int const nThreads = std::thread::hardware_concurrency();
  std::vector<std::thread> threads;
  for (int t = 0; t < nThreads - 1; ++t) {
    threads.emplace_back(renderThread, &scene, &camera, &image, width, nThreads, t, height, 1, 0, &k, stepSize);
  }

  // Call the renderThread function yourself
  renderThread(&scene, &camera, &image, width, nThreads, nThreads - 1, height, 1, 0, &k, stepSize);

  // Rejoin the threads
  for (int t = 0; t < nThreads - 1; ++t) {
    threads[t].join();
  }

  // Stop timer
  stop = std::chrono::steady_clock::now();

  std::cout << "| Done!" << std::endl;

  // Calculate the Time taken in seconds
  double seconds = std::chrono::duration_cast<std::chrono::duration<double>>(stop - start).count();

  std::cout << "Time: " << seconds << "s" << std::endl;

  // Get the number of seconds per ray
  int rays = Ray::getRayCount();

  std::cout << "Paths: " << rays << std::endl;
  std::cout << "Paths per second: " << std::fixed << std::setprecision(0) << rays / seconds << std::endl;


  return image;
}
