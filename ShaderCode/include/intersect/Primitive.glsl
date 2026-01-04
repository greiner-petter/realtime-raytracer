void intersectPrimitive(Ray ray, Primitive prim, int primIndex, inout Hit hit) {
    if (prim.type == 1) {
        Sphere sphere = spheres[prim.index];
        if (intersectSphere(ray, sphere, hit)) {
            hit.materialID = prim.materialID;
            hit.primitiveIndex = primIndex;
        }
    } else if (prim.type == 2) {
        Triangle triangle = triangles[prim.index];
        if (intersectTriangle(ray, triangle, hit)) {
            hit.materialID = prim.materialID;
            hit.primitiveIndex = primIndex;
        }
    }
}