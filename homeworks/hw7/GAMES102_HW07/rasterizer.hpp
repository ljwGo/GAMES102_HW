#pragma once

#include "Triangle.hpp"
#include <algorithm>
#include "Eigen/Eigen"
#define M_PI 3.1415926

using namespace Eigen;

namespace rst {
class rasterizer
{
  public:
    rasterizer(int w, int h);
    void set_pixel(const Eigen::Vector2f& point, const Eigen::Vector3f& color);
    void clear();
    void drawDelTri(const std::vector<Triangle>& tris, const std::vector<Vector2f>& vs,  const Vector3f& color);
    void drawTriangle(const Triangle& tris);
    void drawSquare(const std::vector<Eigen::Vector2f>& ps, int pixelLen = 5, const Eigen::Vector3f& color = Eigen::Vector3f(255, 255, 255));
    void drawSquare(const Eigen::Vector2f& p, int pixelLen = 5, const Eigen::Vector3f& color = Eigen::Vector3f(255, 255, 255));
    void drawCircleWireframe(const Vector2f& center, float radius, float epsilon);
    std::vector<Eigen::Vector3f>& frame_buffer() { return frame_buf; }
    void draw_line(Eigen::Vector2f begin, Eigen::Vector2f end, Eigen::Vector3f line_color = { 255, 255, 255 });

  private:

  private:
    std::vector<Eigen::Vector3f> frame_buf;
    int width, height;
    int get_index(int x, int y);
};
} // namespace rst
