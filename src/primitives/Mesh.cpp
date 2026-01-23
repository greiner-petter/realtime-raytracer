#include "Mesh.h"
#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "Triangle.h"
#include "shaders/Shader.h"
#include "common/Log.h"
#include "scene/ObjLoader.h"

Mesh::Mesh(char const *fileName, std::shared_ptr<Shader> shader, Vec3 const &scale, Vec3 const &translation, bool flipU, bool flipV) : TypedPrimitive(shader) {
  auto prims = ObjLoader::Load(fileName, scale, translation, shader, flipU, flipV);
  
  Vec3 minimumBounds = Vec3(+INFINITY, +INFINITY, +INFINITY);
  Vec3 maximumBounds = Vec3(-INFINITY, -INFINITY, -INFINITY);
  for (auto prim : prims) {
    RT_ASSERT(prim->type == PrimitiveType::Triangle, "Mesh can only store triangles.");
    m_Triangles.push_back(std::static_pointer_cast<Triangle>(prim));
    for (int d = 0; d < 3; ++d) {
      minimumBounds[d] = std::min(minimumBounds[d], prim->minimumBounds(d));
      maximumBounds[d] = std::max(maximumBounds[d], prim->maximumBounds(d));
    }
  }
  minBounds_index = Vec4(minimumBounds, 0);
  maxBounds_count = Vec4(maximumBounds, 0);
}
