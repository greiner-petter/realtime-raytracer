#ifndef MESH_H
#define MESH_H

#include "primitives/Primitive.h"
#include "common/Types.h"
#include "common/Log.h"

struct Mesh : public TypedPrimitive<PrimitiveType::Mesh> {
  Mesh(char const *fileName, std::shared_ptr<class Shader> shader, Vec3 const &scale, Vec3 const &translation, bool flipU = false, bool flipV = false);

  float minimumBounds(int dimension) const override { return minBounds_index[dimension]; }
  float maximumBounds(int dimension) const override { return maxBounds_count[dimension]; }

  virtual void* GetDataLayoutBeginPtr() override { return &minBounds_index; }
  virtual size_t GetDataSize() const override { return sizeof(Vec4) * 2; }

  Vec4 minBounds_index; // xyz = minBounds, w = index;
  Vec4 maxBounds_count; // xyz = minBounds, w = count;
  std::vector<std::shared_ptr<struct Triangle>> m_Triangles;
};

#endif