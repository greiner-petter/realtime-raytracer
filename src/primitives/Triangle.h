#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "Primitive.h"
#include "common/Types.h"

struct Triangle : public Primitive {
public:
    // Constructor
    Triangle() { type = PrimitiveType::Triangle; }

    Triangle(Vec3 const &a, Vec3 const &b, Vec3 const &c) 
    : vertex{ Vec4(a, 0), Vec4(b, 0), Vec4(c, 0) } { type = PrimitiveType::Triangle; }

    Triangle(Vec3 const &a, Vec3 const &b, Vec3 const &c,
             Vec3 const &na, Vec3 const &nb, Vec3 const &nc)
    : vertex{ Vec4(a, 0), Vec4(b, 0), Vec4(c, 0) },
      normal{ Vec4(na, 0), Vec4(nb, 0), Vec4(nc, 0) } { type = PrimitiveType::Triangle; }

    Triangle(Vec3 const &a, Vec3 const &b, Vec3 const &c,
             Vec3 const &na, Vec3 const &nb, Vec3 const &nc,
             Vec2 const &ta, Vec2 const &tb, Vec2 const &tc)
    : vertex{ Vec4(a, 0), Vec4(b, 0), Vec4(c, 0) },
      normal{ Vec4(na, 0), Vec4(nb, 0), Vec4(nc, 0) },
      surface{ Vec4(ta, 0, 0), Vec4(tb, 0, 0), Vec4(tc, 0, 0) } { type = PrimitiveType::Triangle; }


    Triangle(Vec3 const &a, Vec3 const &b, Vec3 const &c,
             Vec3 const &na, Vec3 const &nb, Vec3 const &nc,
             Vec3 const &tana, Vec3 const &tanb, Vec3 const &tanc,
             Vec3 const &ba, Vec3 const &bb, Vec3 const &bc,
             Vec2 const &ta, Vec2 const &tb, Vec2 const &tc)
    : vertex{ Vec4(a, 0), Vec4(b, 0), Vec4(c, 0) },
      normal{ Vec4(na, 0), Vec4(nb, 0), Vec4(nc, 0) },
      tangent{ Vec4(tana, 0), Vec4(tanb, 0), Vec4(tanc, 0) },
      bitangent{ Vec4(ba, 0), Vec4(bb, 0), Vec4(bc, 0) },
      surface{ Vec4(ta, 0, 0), Vec4(tb, 0, 0), Vec4(tc, 0, 0) } { type = PrimitiveType::Triangle; }

    // Set
    void setVertex(int index, Vec3 const &vertex) { this->vertex[index] = Vec4(vertex, 0); }
    void setNormal(int index, Vec3 const &normal) { this->normal[index] = Vec4(glm::normalize(normal), 0); }
    void setTangent(int index, Vec3 const &tangent) { this->tangent[index] = Vec4(glm::normalize(tangent), 0); }
    void setBitangent(int index, Vec3 const &bitangent) { this->bitangent[index] = Vec4(glm::normalize(bitangent), 0); }
    void setSurface(int index, Vec2 const &surface) { this->surface[index] = Vec4(surface, 0, 0); }

    // Get
    Vec3 getPosition(size_t index) { return this->vertex[index]; }
    Vec3 getNormal(size_t index) { return this->normal[index]; }
    Vec3 getTangent(size_t index) { return this->tangent[index]; }
    Vec3 getBitangent(size_t index) { return this->bitangent[index]; }
    Vec2 getTexCoord(size_t index) { return this->surface[index]; }
    
    float minimumBounds(int dimension) const override;
    float maximumBounds(int dimension) const override;

    virtual void* GetDataLayoutBeginPtr() override { return &vertex[0]; }
    virtual size_t GetDataSize() const override { return sizeof(Vec4) * 3 * 5; }
protected:
    Vec4 vertex[3];     // XYZ + padding
    Vec4 normal[3];     // XYZ + padding
    Vec4 tangent[3];    // XYZ + padding
    Vec4 bitangent[3];  // XYZ + padding
    Vec4 surface[3];    // UV  + padding + padding
};

#endif