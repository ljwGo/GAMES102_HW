#ifndef RASTERIZER_TRIANGLE_H
#define RASTERIZER_TRIANGLE_H

#include "Eigen/Eigen"
#include "Edge.h"
#include "Circumcircle.h"

using namespace Eigen;
class Triangle
{
 public:
    Vector2f* v[3]; /*the original coordinates of the triangle, v0, v1, v2 in
                      counter clockwise order*/
    size_t indicate[3];
    /*Per vertex values*/
    Vector3f color[3];      // color at each vertex;
    Vector2f tex_coords[3]; // texture u,v
    Vector3f normal[3];     // normal vector for each vertex
    Circumcircle circumcircle;

    std::vector<Edge> edges;

    // Texture *tex;
    Triangle();
    Triangle(Edge& e, Vector2f& v, size_t ind);
    Triangle(Vector2f& v1, size_t v1Ind,
        Vector2f& v2, size_t v2Ind,
        Vector2f& v3, size_t v3Ind);

    Eigen::Vector2f v1() const { return *(v[0]); }
    Eigen::Vector2f v2() const { return *(v[1]); }
    Eigen::Vector2f v3() const { return *(v[2]); }

    Edge e1() const { return edges[0]; }
    Edge e2() const { return edges[1]; }
    Edge e3() const { return edges[2]; }

    void setVertex(int ind, Vector2f& ver); /*set i-th vertex coordinates */
    void setNormal(int ind, Vector3f n);   /*set i-th vertex normal vector*/
    void setColor(int ind, float r, float g, float b); /*set i-th vertex color*/
    void setTexCoord(int ind, float s,
                     float t); /*set i-th vertex texture coordinate*/
    // CC refer to circumcircle.
    void GenerateCC();
    bool ifOnInnerSide(const Vector2f& p) const;
    // -1 if not. 0, 1, 2 is the vertex index of this triangle.
    int PublicEdge(const Triangle& other) const;

private:
    void Init(Vector2f& v1, size_t v1Ind, Vector2f& v2, size_t v2Ind, Vector2f& v3, size_t v3Ind);
};

#endif // RASTERIZER_TRIANGLE_H
