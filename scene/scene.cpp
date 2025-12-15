#include "light/light.h"
#include "primitive/triangle.h"
#include "scene/scene.h"
#include "shader/shader.h"
#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

const std::string WHITESPACE = " \n\r\t\f\v";
std::string ltrim(const std::string &s) {
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

std::string rtrim(const std::string &s) {
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

std::string trim(const std::string &s) { return rtrim(ltrim(s)); }

void Scene::add(const std::shared_ptr<Light> &light) { this->lights_.push_back(light); }

void Scene::add(const std::shared_ptr<Primitive> &primitive) {
  assert(primitive->shader() != nullptr);
  this->primitives_.push_back(primitive);
}

void Scene::addObj(char const *fileName, Vector3d const &scale, Vector3d const &translation,
                   const std::shared_ptr<Shader> &shader, bool flipU, bool flipV) {
  std::vector<std::shared_ptr<Primitive>> triangles = loadObj(fileName, scale, translation, shader, flipU, flipV);
  this->primitives_.insert(this->primitives_.end(), std::make_move_iterator(triangles.begin()),
                           std::make_move_iterator(triangles.end()));
}

std::vector<std::shared_ptr<Primitive>> Scene::loadObj(char const *fileName, Vector3d const &scale,
                                                       Vector3d const &translation,
                                                       const std::shared_ptr<Shader> &shader, bool flipU, bool flipV) {
  std::vector<std::shared_ptr<Primitive>> faces;
  std::vector<std::array<int, 3>> indices;

  // Open file from disk
  std::ifstream file;
  file.open(fileName);
  if (!file.is_open()) {
    std::cout << "(Scene): Could not open .obj file: " << fileName << std::endl;
    return std::vector<std::shared_ptr<Primitive>>();
  }

  // Print the file name
  std::cout << "(Scene): Loading \"" << fileName << "\"" << std::endl;

  // Actual model data
  std::vector<Vector3d> vData;
  std::vector<Vector3d> tangentData;
  std::vector<Vector3d> bitangentData;
  std::vector<Vector3d> normalData;
  std::vector<Vector3d> vnData;
  std::vector<Vector2d> vtData;

  // Read vertices, normals, textures, and faces from the file
  std::string line;
  while (getline(file, line)) {
    std::stringstream lineStream(trim(line));
    std::string type;
    lineStream >> type;

    // Vertices
    if (type == "v") {
      float x, y, z;
      lineStream >> x >> y >> z;
      vData.emplace_back(componentProduct(Vector3d(x, y, z), scale) + translation);
      tangentData.emplace_back();
      bitangentData.emplace_back();
      normalData.emplace_back();
    }

    // Texture coordinates
    if (type == "vt") {
      float u, v;
      lineStream >> u >> v;
      vtData.emplace_back(flipU ? 1.0f - u : u, flipV ? 1.0f - v : v);
    }

    // Normals
    if (type == "vn") {
      float a, b, c;
      lineStream >> a >> b >> c;
      vnData.emplace_back(normalized(componentQuotient(
          Vector3d(a, b, c),
          scale))); // Division needed for preventing stretched normals, normals' = (transform^-1)^T * normals
    }

    // Faces
    if (type == "f") {
      std::string vertex[3];
      std::array<int, 3> vertInd = {-1, -1, -1};
      std::array<int, 3> texInd = {-1, -1, -1};
      std::array<int, 3> normInd = {-1, -1, -1};
      lineStream >> vertex[0] >> vertex[1] >> vertex[2];

      // triangulate polygons, like quads (which must be given in triangle fan notation)
      while (!vertex[2].empty()) {
        auto triangle = std::make_shared<Triangle>(shader);

        for (int i = 0; i < 3; ++i) {
          std::stringstream vertexSteam(vertex[i]);
          std::string reference;

          // vertex index
          getline(vertexSteam, reference, '/');
          try {
            vertInd[i] = stoi(reference) - 1;
            triangle->setVertex(i, vData.at(vertInd[i]));
          } catch (...) {
            std::cout << "Error: vertex index invalid on line \"" << line << "\"" << std::endl;
          }

          // texture index
          if (getline(vertexSteam, reference, '/')) {
            if (!reference.empty()) {
              try {
                texInd[i] = stoi(reference) - 1;
                triangle->setSurface(i, vtData.at(texInd[i]));
              } catch (...) {
                std::cout << "Error: texture coordinate index invalid on line \"" << line << "\"" << std::endl;
              }
            }

            // normal index
            if (getline(vertexSteam, reference, '/')) {
              try {
                normInd[i] = stoi(reference) - 1;
                triangle->setNormal(i, vnData.at(normInd[i]));
              } catch (...) {
                std::cout << "Error: normal index invalid on line \"" << line << "\"" << std::endl;
              }
            }
          }
        }

        // calculate and accumulate tangent and bitangent vectors
        if (std::all_of(vertInd.begin(), vertInd.end(), [](int i) { return i > -1; }) &&
            std::all_of(texInd.begin(), texInd.end(), [](int i) { return i > -1; })) {
          for (int i = 0; i < 3; i++) {
            const Vector3d deltaPos1 = vData.at(vertInd[(i + 1) % 3]) - vData.at(vertInd[i]);
            const Vector3d deltaPos2 = vData.at(vertInd[(i + 2) % 3]) - vData.at(vertInd[i]);

            const Vector2d deltaUV1 = vtData.at(texInd[(i + 1) % 3]) - vtData.at(texInd[i]);
            const Vector2d deltaUV2 = vtData.at(texInd[(i + 2) % 3]) - vtData.at(texInd[i]);

            const float r = 1.0f / (deltaUV1.u * deltaUV2.v - deltaUV1.v * deltaUV2.u);
            tangentData[vertInd[i]] += (deltaPos1 * deltaUV2.v - deltaPos2 * deltaUV1.v) * r;
            bitangentData[vertInd[i]] += (deltaPos2 * deltaUV1.u - deltaPos1 * deltaUV2.u) * r;

            normalData[vertInd[i]] += crossProduct(tangentData[vertInd[i]], bitangentData[vertInd[i]]);
          }
        }

        faces.push_back(triangle);
        indices.push_back(vertInd);

        // get the next triangle
        if (lineStream.eof())
          break;

        vertex[1] = vertex[2];
        lineStream >> vertex[2];
      }
    }
  }

  // Close the file
  file.close();

  // set the normalized tangents and bitangents
  for (int i = 0; i < faces.size(); i++) {
    for (int j = 0; j < 3; j++) {
      Vector3d tangent = normalized(tangentData[indices[i][j]]);
      const Vector3d bitangent = normalized(bitangentData[indices[i][j]]);
      // try to use the normal from the obj file, if it doesn't exist, use the computed normal
      Vector3d normal = normalized(normalData[indices[i][j]]);
      if (vnData.size() > 0)
        normal = dynamic_cast<Triangle *>(faces[i].get())->getNormal(j);

      // gram-schmidt orthogonalization
      tangent = normalized(tangent - normal * dotProduct(normal, tangent));
      // check handedness of coordinate system
      if (dotProduct(crossProduct(normal, tangent), bitangent) < 0.0f)
        tangent *= -1.0f;

      dynamic_cast<Triangle *>(faces[i].get())->setTangent(j, tangent);
      dynamic_cast<Triangle *>(faces[i].get())->setBitangent(j, bitangent);
      dynamic_cast<Triangle *>(faces[i].get())->setNormal(j, normal);
    }
  }

  // Debug output
  std::cout << " -> " << vData.size() << " vertices parsed" << std::endl;
  std::cout << " -> " << vnData.size() << " normals parsed" << std::endl;
  std::cout << " -> " << vtData.size() << " uv-positions parsed" << std::endl;
  std::cout << " -> " << faces.size() << " primitives parsed" << std::endl;

  return faces;
}

Color Scene::traceRay(Ray &ray) const {
  if (this->findIntersection(ray) && ray.remainingBounces-- > 0) {
    // If the ray has hit an object, call the shader ...
    return ray.primitive->shader()->shade(*this, ray);
  } else if (this->environmentMap) {
    // ... otherwise look up the environment map ...
    float const phi = std::acos(ray.direction.y);
    float const rho = std::atan2(ray.direction.z, ray.direction.x) + float(PI);
    return this->environmentMap->color(rho / (2.0f * float(PI)), phi / float(PI));
  } else {
    // ... if all else fails, just return the background color
    return this->backgroundColor;
  }
}
