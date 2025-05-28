//
// Created by LEI XU on 4/11/19.
//

#include "Triangle.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>

Triangle::Triangle()
{
    v[0] = nullptr;
    v[1] = nullptr;
    v[2] = nullptr;

    color[0] << 0.0, 0.0, 0.0;
    color[1] << 0.0, 0.0, 0.0;
    color[2] << 0.0, 0.0, 0.0;

    tex_coords[0] << 0.0, 0.0;
    tex_coords[1] << 0.0, 0.0;
    tex_coords[2] << 0.0, 0.0;

}
Triangle::Triangle(Edge& e, Vector2f& v, size_t ind)
{
    Init(*(e.vs), e.vsInd, *(e.ve), e.veInd, v, ind);
}
Triangle::Triangle(Vector2f& v1, size_t v1Ind,
    Vector2f& v2, size_t v2Ind,
    Vector2f& v3, size_t v3Ind)
{
    Init(v1, v1Ind, v2, v2Ind, v3, v3Ind);
}

void Triangle::setVertex(int ind, Eigen::Vector2f& ver) { v[ind] = &ver; }
void Triangle::setNormal(int ind, Vector3f n) { normal[ind] = n; }
void Triangle::setColor(int ind, float r, float g, float b)
{
    if ((r < 0.0) || (r > 255.) || (g < 0.0) || (g > 255.) || (b < 0.0) ||
        (b > 255.)) {
        throw std::runtime_error("Invalid color values");
    }

    color[ind] = Vector3f((float)r / 255., (float)g / 255., (float)b / 255.);
    return;
}
void Triangle::setTexCoord(int ind, float s, float t)
{
    tex_coords[ind] = Vector2f(s, t);
}

bool Triangle::ifOnInnerSide(const Vector2f& p) const
{
    Vector3f e1 = Vector3f(v[1]->x() - v[0]->x(), v[1]->y() - v[0]->y(), 0);
    Vector3f p1 = Vector3f(p.x() - v[0]->x(), p.y() - v[0]->y(), 0);
    Vector3f e2 = Vector3f(v[2]->x() - v[1]->x(), v[2]->y() - v[1]->y(), 0);
    Vector3f p2 = Vector3f(p.x() - v[1]->x(), p.y() - v[1]->y(), 0);
    Vector3f e3 = Vector3f(v[0]->x() - v[2]->x(), v[0]->y() - v[2]->y(), 0);
    Vector3f p3 = Vector3f(p.x() - v[2]->x(), p.y() - v[2]->y(), 0);

    assert(e1[2] == 0. && p1[2] == 0.);

    // anticlockwise(ÄæÊ±ÕëË³Ðò)
    float a = e1.cross(p1).z();
    float b = e2.cross(p2).z();
    float c = e3.cross(p3).z();
    
    if (e1.cross(p1).z() < 0 && e2.cross(p2).z() < 0 && e3.cross(p3).z() < 0) {
        return true;
    }

    return false;
}

void Triangle::GenerateCC()
{
    circumcircle = Circumcircle(this);
}

int Triangle::PublicEdge(const Triangle& other) const
{
    int index = 0;
    for (const Edge& e : edges) {
        for (const Edge& otherE : other.edges) {
            if (e == otherE) {
                return index;
            }
        }
        index++;
    }
    return -1;
}

void Triangle::Init(Vector2f& v1, size_t v1Ind, Vector2f& v2, size_t v2Ind, Vector2f& v3, size_t v3Ind)
{
    const Vector2f& e1 = v2 - v1;
    const Vector2f& e2 = v3 - v2;
    const Vector2f& e3 = v1 - v3;
    float e1Len = e1.norm();
    float e2Len = e2.norm();
    float e3Len = e3.norm();

    float maxLen = e1Len;
    if (e2Len > maxLen) maxLen = e2Len;
    if (e3Len > maxLen) maxLen = e3Len;
    float perimeter = e1Len + e2Len + e3Len;

    assert(("invalid triangle", perimeter - maxLen > maxLen));

    v[0] = &v1;
    v[1] = &v2;
    v[2] = &v3;

    indicate[0] = v1Ind;
    indicate[1] = v2Ind;
    indicate[2] = v3Ind;

    edges.push_back(Edge(v1, v1Ind, v2, v2Ind));
    edges.push_back(Edge(v2, v2Ind, v3, v3Ind));
    edges.push_back(Edge(v3, v3Ind, v1, v1Ind));
}
