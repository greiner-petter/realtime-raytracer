#ifndef MESH_H
#define MESH_H

#include "common/Types.h"
#include "primitives/Primitive.h"

class Mesh {
public:


  static std::vector<std::shared_ptr<Primitive>> LoadObj(char const *fileName, std::shared_ptr<class Shader> shader, Vec3 const &scale, Vec3 const &translation, bool flipU, bool flipV);

};

#endif