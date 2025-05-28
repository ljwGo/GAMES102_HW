#include <algorithm>
#include "rasterizer.hpp"
#include <opencv2/opencv.hpp>
#include <math.h>
#include <stdexcept>

// Bresenham's line drawing algorithm
// Code taken from a stack overflow answer: https://stackoverflow.com/a/16405254
void rst::rasterizer::draw_line(Eigen::Vector2f begin, Eigen::Vector2f end, Eigen::Vector3f line_color)
{
    auto x1 = begin.x();
    auto y1 = begin.y();
    auto x2 = end.x();
    auto y2 = end.y();

    int x,y,dx,dy,dx1,dy1,px,py,xe,ye,i;

    dx=x2-x1;
    dy=y2-y1;
    dx1=fabs(dx);
    dy1=fabs(dy);
    px=2*dy1-dx1;
    py=2*dx1-dy1;

    if(dy1<=dx1)
    {
        if(dx>=0)
        {
            x=x1;
            y=y1;
            xe=x2;
        }
        else
        {
            x=x2;
            y=y2;
            xe=x1;
        }
        Eigen::Vector2f point = Eigen::Vector2f(x, y);
        set_pixel(point,line_color);
        for(i=0;x<xe;i++)
        {
            x=x+1;
            if(px<0)
            {
                px=px+2*dy1;
            }
            else
            {
                if((dx<0 && dy<0) || (dx>0 && dy>0))
                {
                    y=y+1;
                }
                else
                {
                    y=y-1;
                }
                px=px+2*(dy1-dx1);
            }
//            delay(0);
            Eigen::Vector2f point = Eigen::Vector2f(x, y);
            set_pixel(point,line_color);
        }
    }
    else
    {
        if(dy>=0)
        {
            x=x1;
            y=y1;
            ye=y2;
        }
        else
        {
            x=x2;
            y=y2;
            ye=y1;
        }
        Eigen::Vector2f point = Eigen::Vector2f(x, y);
        set_pixel(point,line_color);
        for(i=0;y<ye;i++)
        {
            y=y+1;
            if(py<=0)
            {
                py=py+2*dx1;
            }
            else
            {
                if((dx<0 && dy<0) || (dx>0 && dy>0))
                {
                    x=x+1;
                }
                else
                {
                    x=x-1;
                }
                py=py+2*(dx1-dy1);
            }
//            delay(0);
            Eigen::Vector2f point = Eigen::Vector2f(x, y);
            set_pixel(point,line_color);
        }
    }
}

void rst::rasterizer::drawSquare(const std::vector<Eigen::Vector2f>& ps, int pixelLen, const Eigen::Vector3f& color)
{
    for (const Vector2f& p : ps) {
        drawSquare(p, pixelLen, color);
    }
}

void rst::rasterizer::drawSquare(const Eigen::Vector2f& p, int pixelLen, const Eigen::Vector3f& color)
{
    int top = p.y() - pixelLen < 0 ? 0 : p.y() - pixelLen;
    int bottom = p.y() + pixelLen > height - 1 ? height - 1 : p.y() + pixelLen;
    int left = p.x() - pixelLen < 0 ? 0 : p.x() - pixelLen;
    int right = p.x() + pixelLen > width - 1 ? width - 1 : p.x() + pixelLen;

    for (int i = left; i <= right; ++i) {
        for (int j = top; j <= bottom; ++j) {
            set_pixel(Vector2f(i, j), color);
        }
    }
}

void rst::rasterizer::drawCircleWireframe(const Vector2f& center, float radius, float epsilon = 0.05)
{
    const Eigen::Vector3f color(0, 200, 0);
    float theta = -2 * M_PI;
    while (theta <= M_PI) {
        int x = std::round(center.x() + radius * std::cos(theta));
        int y = std::round(center.y() + radius * std::sin(theta));
        set_pixel(Vector2f(x, y), color);
        set_pixel(Vector2f(x-1, y), color);
        set_pixel(Vector2f(x+1, y), color);
        set_pixel(Vector2f(x, y-1), color);
        set_pixel(Vector2f(x, y+1), color);
        set_pixel(Vector2f(x-1, y-1), color);
        set_pixel(Vector2f(x-1, y+1), color);
        set_pixel(Vector2f(x+1, y-1), color);
        set_pixel(Vector2f(x+1, y+1), color);
        theta += epsilon;
    }
}

void rst::rasterizer::clear()
{
	std::fill(frame_buf.begin(), frame_buf.end(), Eigen::Vector3f{ 0, 0, 0 });
}

rst::rasterizer::rasterizer(int w, int h) : width(w), height(h)
{
    frame_buf.resize(w * h);
}

int rst::rasterizer::get_index(int x, int y)
{
    return y + x * width;
}

void rst::rasterizer::set_pixel(const Eigen::Vector2f& point, const Eigen::Vector3f& color)
{
    if (point.x() < 0 || point.x() >= width ||
        point.y() < 0 || point.y() >= height) return;
    auto ind = point.x() + point.y() * width;
    frame_buf[ind] = color;
}

