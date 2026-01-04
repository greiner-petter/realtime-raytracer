#ifndef MESH_H
#define MESH_H

#include "common/Types.h"
#include "scene/Primitive.h"

class Mesh {
public:


  static std::vector<std::shared_ptr<Primitive>> LoadObj(char const *fileName, Vec3 const &scale, Vec3 const &translation, bool flipU, bool flipV);

};

#endif