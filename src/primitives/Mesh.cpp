// #include "Mesh.h"
// #include <array>
// #include <cassert>
// #include <fstream>
// #include <iostream>
// #include <sstream>
// #include <string>
// #include "Triangle.h"

// Vec3 componentQuotient(const Vec3 &left, const Vec3 &right) { return Vec3(left.x / right.x, left.y / right.y, left.z / right.z); }

// Vec3 componentProduct(const Vec3 &left, const Vec3 &right) { return Vec3(left.x * right.x, left.y * right.y, left.z * right.z); }

// const std::string WHITESPACE = " \n\r\t\f\v";
// std::string ltrim(const std::string &s) {
//   size_t start = s.find_first_not_of(WHITESPACE);
//   return (start == std::string::npos) ? "" : s.substr(start);
// }

// std::string rtrim(const std::string &s) {
//   size_t end = s.find_last_not_of(WHITESPACE);
//   return (end == std::string::npos) ? "" : s.substr(0, end + 1);
// }

// std::string trim(const std::string &s) { return rtrim(ltrim(s)); }

// std::vector<std::shared_ptr<Primitive>> Mesh::LoadObj(char const *fileName, Vec3 const &scale, Vec3 const &translation, bool flipU, bool flipV) {
//   std::vector<std::shared_ptr<Primitive>> faces;
//   std::vector<std::array<int, 3>> indices;

//   // Open file from disk
//   std::ifstream file;
//   file.open(fileName);
//   if (!file.is_open()) {
//     std::cout << "(Scene): Could not open .obj file: " << fileName << std::endl;
//     return std::vector<std::shared_ptr<Primitive>>();
//   }

//   // Print the file name
//   std::cout << "(Scene): Loading \"" << fileName << "\"" << std::endl;

//   // Actual model data
//   std::vector<Vec3> vData;
//   std::vector<Vec3> tangentData;
//   std::vector<Vec3> bitangentData;
//   std::vector<Vec3> normalData;
//   std::vector<Vec3> vnData;
//   std::vector<Vec2> vtData;

//   // Read vertices, normals, textures, and faces from the file
//   std::string line;
//   while (getline(file, line)) {
//     std::stringstream lineStream(trim(line));
//     std::string type;
//     lineStream >> type;

//     // Vertices
//     if (type == "v") {
//       float x, y, z;
//       lineStream >> x >> y >> z;
//       vData.emplace_back(componentProduct(Vec3(x, y, z), scale) + translation);
//       tangentData.emplace_back();
//       bitangentData.emplace_back();
//       normalData.emplace_back();
//     }

//     // Texture coordinates
//     if (type == "vt") {
//       float u, v;
//       lineStream >> u >> v;
//       vtData.emplace_back(flipU ? 1.0f - u : u, flipV ? 1.0f - v : v);
//     }

//     // Normals
//     if (type == "vn") {
//       float a, b, c;
//       lineStream >> a >> b >> c;
//       vnData.emplace_back(glm::normalize(componentQuotient(
//           Vec3(a, b, c),
//           scale))); // Division needed for preventing stretched normals, normals' = (transform^-1)^T * normals
//     }

//     // Faces
//     if (type == "f") {
//       std::string vertex[3];
//       std::array<int, 3> vertInd = {-1, -1, -1};
//       std::array<int, 3> texInd = {-1, -1, -1};
//       std::array<int, 3> normInd = {-1, -1, -1};
//       lineStream >> vertex[0] >> vertex[1] >> vertex[2];

//       // triangulate polygons, like quads (which must be given in triangle fan notation)
//       while (!vertex[2].empty()) {
//         auto triangle = std::make_shared<Triangle>();

//         for (int i = 0; i < 3; ++i) {
//           std::stringstream vertexSteam(vertex[i]);
//           std::string reference;

//           // vertex index
//           getline(vertexSteam, reference, '/');
//           try {
//             vertInd[i] = stoi(reference) - 1;
//             triangle->setVertex(i, vData.at(vertInd[i]));
//           } catch (...) {
//             std::cout << "Error: vertex index invalid on line \"" << line << "\"" << std::endl;
//           }

//           // texture index
//           if (getline(vertexSteam, reference, '/')) {
//             if (!reference.empty()) {
//               try {
//                 texInd[i] = stoi(reference) - 1;
//                 triangle->setSurface(i, vtData.at(texInd[i]));
//               } catch (...) {
//                 std::cout << "Error: texture coordinate index invalid on line \"" << line << "\"" << std::endl;
//               }
//             }

//             // normal index
//             if (getline(vertexSteam, reference, '/')) {
//               try {
//                 normInd[i] = stoi(reference) - 1;
//                 triangle->setNormal(i, vnData.at(normInd[i]));
//               } catch (...) {
//                 std::cout << "Error: normal index invalid on line \"" << line << "\"" << std::endl;
//               }
//             }
//           }
//         }

//         // calculate and accumulate tangent and bitangent vectors
//         if (std::all_of(vertInd.begin(), vertInd.end(), [](int i) { return i > -1; }) &&
//             std::all_of(texInd.begin(), texInd.end(), [](int i) { return i > -1; })) {
//           for (int i = 0; i < 3; i++) {
//             const Vec3 deltaPos1 = vData.at(vertInd[(i + 1) % 3]) - vData.at(vertInd[i]);
//             const Vec3 deltaPos2 = vData.at(vertInd[(i + 2) % 3]) - vData.at(vertInd[i]);

//             const Vec2 deltaUV1 = vtData.at(texInd[(i + 1) % 3]) - vtData.at(texInd[i]);
//             const Vec2 deltaUV2 = vtData.at(texInd[(i + 2) % 3]) - vtData.at(texInd[i]);

//             const float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
//             tangentData[vertInd[i]] += (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
//             bitangentData[vertInd[i]] += (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;

//             normalData[vertInd[i]] += glm::cross(tangentData[vertInd[i]], bitangentData[vertInd[i]]);
//           }
//         }

//         faces.push_back(triangle);
//         indices.push_back(vertInd);

//         // get the next triangle
//         if (lineStream.eof())
//           break;

//         vertex[1] = vertex[2];
//         lineStream >> vertex[2];
//       }
//     }
//   }

//   // Close the file
//   file.close();

//   // set the normalized tangents and bitangents
//   for (int i = 0; i < faces.size(); i++) {
//     for (int j = 0; j < 3; j++) {
//       Vec3 tangent = glm::normalize(tangentData[indices[i][j]]);
//       const Vec3 bitangent = glm::normalize(bitangentData[indices[i][j]]);
//       // try to use the normal from the obj file, if it doesn't exist, use the computed normal
//       Vec3 normal = glm::normalize(normalData[indices[i][j]]);
//       if (vnData.size() > 0)
//         normal = dynamic_cast<Triangle *>(faces[i].get())->getNormal(j);

//       // gram-schmidt orthogonalization
//       tangent = glm::normalize(tangent - normal * glm::dot(normal, tangent));
//       // check handedness of coordinate system
//       if (glm::dot(glm::cross(normal, tangent), bitangent) < 0.0f)
//         tangent *= -1.0f;

//       dynamic_cast<Triangle *>(faces[i].get())->setTangent(j, tangent);
//       dynamic_cast<Triangle *>(faces[i].get())->setBitangent(j, bitangent);
//       dynamic_cast<Triangle *>(faces[i].get())->setNormal(j, normal);
//     }
//   }

//   // Debug output
//   std::cout << " -> " << vData.size() << " vertices parsed" << std::endl;
//   std::cout << " -> " << vnData.size() << " normals parsed" << std::endl;
//   std::cout << " -> " << vtData.size() << " uv-positions parsed" << std::endl;
//   std::cout << " -> " << faces.size() << " primitives parsed" << std::endl;

//   return faces;
// }