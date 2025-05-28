#include <Eigen/Core>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <vector>
#include <assert.h>
#include <math.h>
#include <algorithm>

#include "rasterizer.hpp"
#include "Delaunay.h"
#include "Rectangle.h"
#include "HEMesh.h"
#include "common.h"

using namespace cv;
using namespace Eigen;

// 初始化2D栅格化器
const int row = 600;
const int column = 1000;
const int pointSize = 50;

Vector2f scopeX(0, column - 1);
Vector2f scopeY(0, row - 1);
rst::rasterizer r(column, row);
std::vector<Vector2f> vertices;
std::vector<Vector2f> voronoiVs;
std::vector<std::vector<size_t>> voronoiPolygon;
std::vector<Triangle> delTri;
std::vector<Vector2f> massCenters;
HEMesh delHeMesh;
Delaunay delaunay;

Vector2f Mul(const Vector2f& v1, const Vector2f& v2) {
	return Vector2f(v1.x() * v2.x(), v1.y() * v2.y());
}

Vector2f CalcWindowPoint(Circumcircle& cs, HEHalfEdge* boundaryEdge) {
	Vector2f lineCenter = (boundaryEdge->End()->position + boundaryEdge->Origin()->position) * 0.5f;
	Vector2f rayOri;
	Vector2f rayDir;

	const Vector2f& center = cs.GetCenter();
	if (boundaryEdge->RightOrLeft(center)) {
		rayOri = center;
		rayDir = lineCenter - rayOri;
	}
	else {
		rayOri = center;
		if (!ClampOrNot(center[0], scopeX[0], scopeX[1], 0) ||
			!ClampOrNot(center[1], scopeY[0], scopeY[1], 0)) {
			rayDir = lineCenter - center;
		}
		else {
			rayDir = center - lineCenter;
		}
	}

	float t_x;
	float t_y;
	float invDirX = 1. / std::abs(rayDir.x());
	float invDirY = 1. / std::abs(rayDir.y());

	Vector2f hitP;
	if (rayOri.x() < scopeX[0]) {
		if (rayDir.x() > 0) {
			hitP[0] = scopeX[0];
			t_x = (scopeX[0] - rayOri.x()) * invDirX;
		}
		else {
			t_x = std::numeric_limits<float>::max();
		}
	}
	else if (rayOri.x() == scopeX[0]) {
		t_x = 0;
		hitP[0] = scopeX[0];
	}
	else if (rayOri.x() == scopeX[1]) {
		t_x = 0;
		hitP[0] = scopeX[1];
	}
	else if (rayOri.x() < scopeX[1]) {
		if (rayDir.x() > 0) {
			hitP[0] = scopeX[1];
			t_x = (scopeX[1] - rayOri.x()) * invDirX;
		}
		else if (rayDir.x() < 0) {
			hitP[0] = scopeX[0];
			t_x = (rayOri.x() - scopeX[0]) * invDirX;
		}
		else {
			t_x = std::numeric_limits<float>::max();
		}
	}
	else {
		if (rayDir.x() < 0) {
			hitP[0] = scopeX[1];
			t_x = (rayOri.x() - scopeX[1]) * invDirX;
		}
		else {
			t_x = std::numeric_limits<float>::max();
		}
	}

	if (rayOri.y() < scopeY[0]) {
		if (rayDir.y() > 0) {
			hitP[1] = scopeY[0];
			t_y = (scopeY[0] - rayOri.y()) * invDirY;
		}
		else {
			t_y = std::numeric_limits<float>::max();
		}
	}
	else if (rayOri.y() == scopeY[0]) {
		t_y = 0;
		hitP[1] = scopeY[0];
	}
	else if (rayOri.y() == scopeY[1]) {
		t_y = 0;
		hitP[1] = scopeY[1];
	}
	else if (rayOri.y() < scopeY[1]) {
		if (rayDir.y() > 0) {
			hitP[1] = scopeY[1];
			t_y = (scopeY[1] - rayOri.y()) * invDirY;
		}
		else {
			hitP[1] = scopeY[0];
			t_y = (rayOri.y() - scopeY[0]) * invDirY;
		}
	}
	else {
		if (rayDir.y() < 0) {
			hitP[1] = scopeY[1];
			t_y = (rayOri.y() - scopeY[1]) * invDirY;
		}
		else {
			t_y = std::numeric_limits<float>::max();
		}
	}

	if (t_x < t_y && t_x < std::numeric_limits<float>::max()) {
		hitP[1] = (rayOri + rayDir * t_x).y();
		assert(ClampOrNot(hitP[1], scopeY[0], scopeY[1], 0));
		assert(ClampOrNot(hitP[0], scopeX[0], scopeX[1]));

	}
	else if (t_y < std::numeric_limits<float>::max()) {
		hitP[0] = (rayOri + rayDir * t_y).x();
		assert(ClampOrNot(hitP[0], scopeX[0], scopeX[1], 0));
		assert(ClampOrNot(hitP[1], scopeY[0], scopeY[1]));

	}
	else {
		assert(false);
	}

	//assert(ClampOrNot(hitP.x(), winRect.l().vs->x(), winRect.r().vs->x()));
	//assert(ClampOrNot(hitP.y(), winRect.t().vs->y(), winRect.b().vs->y()));
	return hitP;
}

std::vector<Vector2f> CalcIntersection(const Vector2f& from, const Vector2f& to) {
	Vector2f rayOri = from;
	Vector2f rayDir = to - from;
	std::vector<Vector2f> result;

	float t_x;
	float t_y;
	float invDirX = 1. / std::abs(rayDir.x());
	float invDirY = 1. / std::abs(rayDir.y());

	Vector2f hitPx;
	Vector2f hitPy;
	if (rayOri.x() < scopeX[0]) {
		if (rayDir.x() > 0) {
			hitPx[0] = scopeX[0];
			t_x = (scopeX[0] - rayOri.x()) * invDirX;
		}
		else {
			t_x = std::numeric_limits<float>::max();
		}
	}
	else if (rayOri.x() == scopeX[0]) {
		t_x = 0;
		hitPx[0] = scopeX[0];
	}
	else if (rayOri.x() == scopeX[1]) {
		t_x = 0;
		hitPx[0] = scopeX[1];
	}
	else if (rayOri.x() < scopeX[1]) {
		if (rayDir.x() > 0) {
			hitPx[0] = scopeX[1];
			t_x = (scopeX[1] - rayOri.x()) * invDirX;
		}
		else if (rayDir.x() < 0) {
			hitPx[0] = scopeX[0];
			t_x = (rayOri.x() - scopeX[0]) * invDirX;
		}
		else {
			t_x = std::numeric_limits<float>::max();
		}
	}
	else {
		if (rayDir.x() < 0) {
			hitPx[0] = scopeX[1];
			t_x = (rayOri.x() - scopeX[1]) * invDirX;
		}
		else {
			t_x = std::numeric_limits<float>::max();
		}
	}

	if (rayOri.y() < scopeY[0]) {
		if (rayDir.y() > 0) {
			hitPy[1] = scopeY[0];
			t_y = (scopeY[0] - rayOri.y()) * invDirY;
		}
		else {
			t_y = std::numeric_limits<float>::max();
		}
	}
	else if (rayOri.y() == scopeY[0]) {
		t_y = 0;
		hitPy[1] = scopeY[0];
	}
	else if (rayOri.y() == scopeY[1]) {
		t_y = 0;
		hitPy[1] = scopeY[1];
	}
	else if (rayOri.y() < scopeY[1]) {
		if (rayDir.y() > 0) {
			hitPy[1] = scopeY[1];
			t_y = (scopeY[1] - rayOri.y()) * invDirY;
		}
		else {
			hitPy[1] = scopeY[0];
			t_y = (rayOri.y() - scopeY[0]) * invDirY;
		}
	}
	else {
		if (rayDir.y() < 0) {
			hitPy[1] = scopeY[1];
			t_y = (rayOri.y() - scopeY[1]) * invDirY;
		}
		else {
			t_y = std::numeric_limits<float>::max();
		}
	}

	if (t_x < t_y) {
		if (t_x > 0 && t_x < 1) {
			hitPx[1] = (rayOri + rayDir * t_x).y();
			assert(ClampOrNot(hitPx.y(), scopeY[0], scopeY[1]));
			assert(ClampOrNot(hitPx.x(), scopeX[0], scopeX[1]));
			result.push_back(hitPx);
		}
		if (t_y > 0 && t_y < 1) {
			hitPy[0] = (rayOri + rayDir * t_y).x();
			assert(ClampOrNot(hitPy.x(), scopeX[0], scopeX[1]));
			assert(ClampOrNot(hitPy.y(), scopeY[0], scopeY[1]));
			result.push_back(hitPy);
		}
	}
	else {
		if (t_y > 0 && t_y < 1) {
			hitPy[0] = (rayOri + rayDir * t_y).x();
			assert(ClampOrNot(hitPy.x(), scopeX[0], scopeX[1]));
			assert(ClampOrNot(hitPy.y(), scopeY[0], scopeY[1]));
			result.push_back(hitPy);
		}
		if (t_x > 0 && t_x < 1) {
			hitPx[1] = (rayOri + rayDir * t_x).y();
			assert(ClampOrNot(hitPx.y(), scopeY[0], scopeY[1]));
			assert(ClampOrNot(hitPx.x(), scopeX[0], scopeX[1]));
			result.push_back(hitPx);
		}
	}

	return result;
}

void CreateDelHeMesh(const std::vector<Vector2f>& position) {
	std::vector<std::vector<size_t>> polygon;
	polygon.resize(delTri.size());

	// https://www.cnblogs.com/wolbo/p/14383481.html解决, 后一个容器不会自动扩容
	std::transform(delTri.begin(), delTri.end(), polygon.begin(), [](Triangle& tri) {
		return std::vector<size_t>{tri.indicate[0], tri.indicate[1], tri.indicate[2]};
		});

	// 只建立了topology关系
	delHeMesh.Init(polygon);

	// 初始化点
	const std::vector<HEVertex*>& heVertices = delHeMesh.Vertices();
	for (int i = 0; i < heVertices.size(); ++i) {
		heVertices[i]->position = Vector2f(position[i].x(), position[i].y());
	}
}

void CreateVoronoiMesh(HEMesh& delHeMesh, std::vector<Triangle>& delTri, std::vector<Vector2f>& voronoiVs, std::vector<std::vector<size_t>>& voronoiPolygon) {
	// 每一个delaunay顶点都有一个voronoi的cell
	for (HEVertex* v : delHeMesh.Vertices()) {
		std::vector<size_t> polygon;
		HETriangle* tri = nullptr;
		HETriangle* preTri = nullptr;
		HETriangle* oriTri = nullptr;
		size_t ix = -1, preIx = -1, oriIx = -1;
		bool isEnd = false;
		bool isFirst = true;

		// 遍历连接顶点的所有半边(假设顶点是边界点, 那么以边界半边开始, 另一条边界半边结束)
		for (HEHalfEdge* he : v->BoundaryStartOutHe()) {
			if (he->IsOnBoundary() && !he->Pair()->IsOnBoundary()) {
				// 计算外接圆圆心到边界边中点, 再延申到窗口边界的交点
				Vector2f centerP = (he->End()->position + he->Origin()->position) * 0.5f;
				size_t ix = delHeMesh.Index(he->Pair()->Polygon());
				Circumcircle& cs = delTri.at(ix).circumcircle;
				const Vector2f& center = cs.GetCenter();

				// 外接圆圆心在窗口内
				if (ClampOrNot(center[0], scopeX[0], scopeX[1], 0) &&
					ClampOrNot(center[1], scopeY[0], scopeY[1], 0)) {
					Vector2f boundaryP = CalcWindowPoint(cs, he);

					voronoiVs.push_back(boundaryP);
					polygon.push_back(voronoiVs.size() - 1);
				}

				// 判断是否需要添加窗口四个角上的点到cell中
				if (isEnd) {
					Vector2f& boundaryS = voronoiVs[voronoiVs.size() - 1];
					Vector2f& boundaryE = voronoiVs[voronoiVs.size() - 2];
					if (boundaryS.x() != boundaryE.x() && boundaryS.y() != boundaryE.y()) {
						if (boundaryS.x() == scopeX[1] && boundaryE.y() == scopeY[1] ||
							boundaryE.x() == scopeX[1] && boundaryS.y() == scopeY[1]) {
							voronoiVs.push_back(Vector2f(scopeX[1], scopeY[1]));
							polygon.push_back(voronoiVs.size() - 1);
						}
						else if (boundaryS.x() == scopeX[0] && boundaryE.y() == scopeY[1] ||
							boundaryE.x() == scopeX[0] && boundaryS.y() == scopeY[1]) {
							voronoiVs.push_back(Vector2f(scopeX[0], scopeY[1]));
							polygon.push_back(voronoiVs.size() - 1);
						}
						else if (boundaryS.x() == scopeX[1] && boundaryE.y() == scopeY[0] ||
							boundaryE.x() == scopeX[1] && boundaryS.y() == scopeY[0]) {
							voronoiVs.push_back(Vector2f(scopeX[1], scopeY[0]));
							polygon.push_back(voronoiVs.size() - 1);
						}
						else if (boundaryS.x() == scopeX[0] && boundaryE.y() == scopeY[0] ||
							boundaryE.x() == scopeX[0] && boundaryS.y() == scopeY[0]) {
							voronoiVs.push_back(Vector2f(scopeX[0], scopeY[0]));
							polygon.push_back(voronoiVs.size() - 1);
						}
					}

					if (boundaryS.x() == scopeX[0] && boundaryE.x() == scopeX[1] ||
						boundaryE.x() == scopeX[0] && boundaryS.x() == scopeX[1] &&
						boundaryS.y() > scopeY[0] && boundaryS.y() < scopeY[1] &&
						boundaryE.y() > scopeY[0] && boundaryE.y() < scopeY[1]) {

						float z = RightOrLeft(boundaryE, cs.GetCenter(), v->position);
						assert(z != 0);

						// 顺时针
						if (z < 0) {
							if (boundaryS.x() < boundaryE.x()) {
								// 上凸包
								voronoiVs.push_back(Vector2f(scopeX[1], scopeY[1]));
								polygon.push_back(voronoiVs.size() - 1);
								voronoiVs.push_back(Vector2f(scopeX[0], scopeY[1]));
								polygon.push_back(voronoiVs.size() - 1);

							}
							else {
								// 下凸包
								voronoiVs.push_back(Vector2f(scopeX[0], scopeY[0]));
								polygon.push_back(voronoiVs.size() - 1);
								voronoiVs.push_back(Vector2f(scopeX[1], scopeY[0]));
								polygon.push_back(voronoiVs.size() - 1);
							}
						}
						// 逆时针
						else if (z > 0) {
							if (boundaryS.x() < boundaryE.x()) {
								// 下凸包
								voronoiVs.push_back(Vector2f(scopeX[1], scopeY[0]));
								polygon.push_back(voronoiVs.size() - 1);
								voronoiVs.push_back(Vector2f(scopeX[0], scopeY[0]));
								polygon.push_back(voronoiVs.size() - 1);
							}
							else {
								// 上凸包
								voronoiVs.push_back(Vector2f(scopeX[0], scopeY[1]));
								polygon.push_back(voronoiVs.size() - 1);
								voronoiVs.push_back(Vector2f(scopeX[1], scopeY[1]));
								polygon.push_back(voronoiVs.size() - 1);
							}
						}
					}
					else if (boundaryS.y() == scopeY[0] && boundaryE.y() == scopeY[1] ||
						boundaryE.y() == scopeY[0] && boundaryS.y() == scopeY[1] &&
						boundaryS.x() > scopeX[0] && boundaryS.x() < scopeX[1] &&
						boundaryE.x() > scopeX[0] && boundaryE.x() < scopeX[1]) {

						float z = RightOrLeft(boundaryE, cs.GetCenter(), v->position);
						assert(z != 0);

						// 顺时针
						if (z < 0) {
							if (boundaryS.y() < boundaryE.y()) {
								// 右凸包
								voronoiVs.push_back(Vector2f(scopeX[0], scopeY[1]));
								polygon.push_back(voronoiVs.size() - 1);
								voronoiVs.push_back(Vector2f(scopeX[0], scopeY[0]));
								polygon.push_back(voronoiVs.size() - 1);
							}
							else {
								// 左凸包
								voronoiVs.push_back(Vector2f(scopeX[1], scopeY[0]));
								polygon.push_back(voronoiVs.size() - 1);
								voronoiVs.push_back(Vector2f(scopeX[1], scopeY[1]));
								polygon.push_back(voronoiVs.size() - 1);
							}
						}
						// 逆时针
						else if (z > 0) {
							if (boundaryS.y() < boundaryE.y()) {
								// 左凸包
								voronoiVs.push_back(Vector2f(scopeX[1], scopeY[1]));
								polygon.push_back(voronoiVs.size() - 1);
								voronoiVs.push_back(Vector2f(scopeX[1], scopeY[0]));
								polygon.push_back(voronoiVs.size() - 1);
							}
							else {
								// 右凸包
								voronoiVs.push_back(Vector2f(scopeX[0], scopeY[0]));
								polygon.push_back(voronoiVs.size() - 1);
								voronoiVs.push_back(Vector2f(scopeX[0], scopeY[1]));
								polygon.push_back(voronoiVs.size() - 1);
							}
						}
					}
				}
				else {
					// 以边界边开始, 第二条边界边结束. 此时表示一个cell的边基本确定
					isEnd = true;
				}
			}
			else {
				if (he->Polygon() != preTri) {
					tri = he->Polygon();
				}
			}

			if (he->Pair()->IsOnBoundary() && !he->IsOnBoundary()) {
				Vector2f centerP = (he->Pair()->End()->position + he->Pair()->Origin()->position) * 0.5f;
				size_t ix = delHeMesh.Index(he->Polygon());
				Circumcircle& cs = delTri.at(ix).circumcircle;
				const Vector2f& center = cs.GetCenter();

				// 外接圆圆心在窗口内
				if (ClampOrNot(center[0], scopeX[0], scopeX[1], 0) &&
					ClampOrNot(center[1], scopeY[0], scopeY[1], 0)) {
					Vector2f boundaryP = CalcWindowPoint(cs, he->Pair());
					voronoiVs.push_back(std::move(boundaryP));
					polygon.push_back(voronoiVs.size() - 1);
				}

				if (isEnd) {
					Vector2f& boundaryS = voronoiVs[voronoiVs.size() - 2];
					Vector2f& boundaryE = voronoiVs[voronoiVs.size() - 1];
					// 添加可能的四个角之一
					if (boundaryS.x() != boundaryE.x() && boundaryS.y() != boundaryE.y()) {
						if (boundaryS.x() == scopeX[1] && boundaryE.y() == scopeY[1] ||
							boundaryE.x() == scopeX[1] && boundaryS.y() == scopeY[1]) {
							voronoiVs.push_back(Vector2f(scopeX[1], scopeY[1]));
							polygon.push_back(voronoiVs.size() - 1);
						}
						else if (boundaryS.x() == scopeX[0] && boundaryE.y() == scopeY[1] ||
							boundaryE.x() == scopeX[0] && boundaryS.y() == scopeY[1]) {
							voronoiVs.push_back(Vector2f(scopeX[0], scopeY[1]));
							polygon.push_back(voronoiVs.size() - 1);
						}
						else if (boundaryS.x() == scopeX[1] && boundaryE.y() == scopeY[0] ||
							boundaryE.x() == scopeX[1] && boundaryS.y() == scopeY[0]) {
							voronoiVs.push_back(Vector2f(scopeX[1], scopeY[0]));
							polygon.push_back(voronoiVs.size() - 1);
						}
						else if (boundaryS.x() == scopeX[0] && boundaryE.y() == scopeY[0] ||
							boundaryE.x() == scopeX[0] && boundaryS.y() == scopeY[0]) {
							voronoiVs.push_back(Vector2f(scopeX[0], scopeY[0]));
							polygon.push_back(voronoiVs.size() - 1);
						}
					}

					// 添加可能的一条边
					if (boundaryS.x() == scopeX[0] && boundaryE.x() == scopeX[1] ||
						boundaryE.x() == scopeX[0] && boundaryS.x() == scopeX[1] &&
						boundaryS.y() > scopeY[0] && boundaryS.y() < scopeY[1] &&
						boundaryE.y() > scopeY[0] && boundaryE.y() < scopeY[1]) {

						float z = RightOrLeft(boundaryE, cs.GetCenter(), v->position);
						assert(z != 0);

						// 顺时针
						if (z < 0) {
							if (boundaryS.x() < boundaryE.x()) {
								// 上凸包
								voronoiVs.push_back(Vector2f(scopeX[1], scopeY[1]));
								polygon.push_back(voronoiVs.size() - 1);
								voronoiVs.push_back(Vector2f(scopeX[0], scopeY[1]));
								polygon.push_back(voronoiVs.size() - 1);

							}
							else {
								// 下凸包
								voronoiVs.push_back(Vector2f(scopeX[0], scopeY[0]));
								polygon.push_back(voronoiVs.size() - 1);
								voronoiVs.push_back(Vector2f(scopeX[1], scopeY[0]));
								polygon.push_back(voronoiVs.size() - 1);
							}
						}
						// 逆时针
						else if (z > 0) {
							if (boundaryS.x() < boundaryE.x()) {
								// 下凸包
								voronoiVs.push_back(Vector2f(scopeX[1], scopeY[0]));
								polygon.push_back(voronoiVs.size() - 1);
								voronoiVs.push_back(Vector2f(scopeX[0], scopeY[0]));
								polygon.push_back(voronoiVs.size() - 1);
							}
							else {
								// 上凸包
								voronoiVs.push_back(Vector2f(scopeX[0], scopeY[1]));
								polygon.push_back(voronoiVs.size() - 1);
								voronoiVs.push_back(Vector2f(scopeX[1], scopeY[1]));
								polygon.push_back(voronoiVs.size() - 1);
							}
						}
					}
					else if (boundaryS.y() == scopeY[0] && boundaryE.y() == scopeY[1] ||
						boundaryE.y() == scopeY[0] && boundaryS.y() == scopeY[1] &&
						boundaryS.x() > scopeX[0] && boundaryS.x() < scopeX[1] &&
						boundaryE.x() > scopeX[0] && boundaryE.x() < scopeX[1]) {

						float z = RightOrLeft(boundaryE, cs.GetCenter(), v->position);
						assert(z != 0);

						// 顺时针
						if (z < 0) {
							if (boundaryS.y() < boundaryE.y()) {
								// 右凸包
								voronoiVs.push_back(Vector2f(scopeX[0], scopeY[1]));
								polygon.push_back(voronoiVs.size() - 1);
								voronoiVs.push_back(Vector2f(scopeX[0], scopeY[0]));
								polygon.push_back(voronoiVs.size() - 1);
							}
							else {
								// 左凸包
								voronoiVs.push_back(Vector2f(scopeX[1], scopeY[0]));
								polygon.push_back(voronoiVs.size() - 1);
								voronoiVs.push_back(Vector2f(scopeX[1], scopeY[1]));
								polygon.push_back(voronoiVs.size() - 1);
							}
						}
						// 逆时针
						else if (z > 0) {
							if (boundaryS.y() < boundaryE.y()) {
								// 左凸包
								voronoiVs.push_back(Vector2f(scopeX[1], scopeY[1]));
								polygon.push_back(voronoiVs.size() - 1);
								voronoiVs.push_back(Vector2f(scopeX[1], scopeY[0]));
								polygon.push_back(voronoiVs.size() - 1);
							}
							else {
								// 右凸包
								voronoiVs.push_back(Vector2f(scopeX[0], scopeY[0]));
								polygon.push_back(voronoiVs.size() - 1);
								voronoiVs.push_back(Vector2f(scopeX[0], scopeY[1]));
								polygon.push_back(voronoiVs.size() - 1);
							}
						}
					}
				}
				else {
					isEnd = true;
				}
			}
			else {
				if (he->Pair()->Polygon() != preTri) {
					tri = he->Pair()->Polygon();
				}
			}

			// 连接三角形外接圆圆心
			if (tri != nullptr && tri != preTri) {
				ix = delHeMesh.Index(tri);
				const Vector2f& centerP = delTri[ix].circumcircle.GetCenter();
				Vector2f preCenterP;
				bool preOutSide;

				if (preTri != nullptr) {
					preCenterP = delTri[preIx].circumcircle.GetCenter();
					preOutSide = !ClampOrNot(preCenterP[0], scopeX[0], scopeX[1], 0) ||
						!ClampOrNot(preCenterP[1], scopeY[0], scopeY[1], 0);
				}

				if (isFirst) {
					oriTri = tri;
					oriIx = ix;
					isFirst = false;
				}

				// 如果前一个三角形外接圆不在窗口内
				if (preTri != nullptr && preOutSide) {
					// 添加交点		
					std::vector<Vector2f> result = CalcIntersection(preCenterP, centerP);
					for (Vector2f& intersection : result) {
						assert(ClampOrNot(intersection.x(), scopeX[0], scopeX[1]));
						assert(ClampOrNot(intersection.y(), scopeY[0], scopeY[1]));
						voronoiVs.push_back(std::move(intersection));
						polygon.push_back(voronoiVs.size() - 1);
					}
				}

				// 当前三角形外接圆圆心在窗口内
				if (ClampOrNot(centerP[0], scopeX[0], scopeX[1], 0) &&
					ClampOrNot(centerP[1], scopeY[0], scopeY[1], 0)) {
					polygon.push_back(ix);
				}
				// 当前三角形外接圆圆心不在窗口内, 且上一个三角形在窗口内
				else if (preTri != nullptr && !preOutSide) {
					std::vector<Vector2f> result = CalcIntersection(preCenterP, centerP);
					for (Vector2f& intersection : result) {
						assert(ClampOrNot(intersection.x(), scopeX[0], scopeX[1]));
						assert(ClampOrNot(intersection.y(), scopeY[0], scopeY[1]));
						voronoiVs.push_back(std::move(intersection));
						polygon.push_back(voronoiVs.size() - 1);
					}
				}

				preTri = tri;
				preIx = ix;
			}
		}

		// 处理封闭图形
		if (!v->IsOnBoundary() && oriTri != nullptr && tri != nullptr) {
			Circumcircle& oriCs = delTri[oriIx].circumcircle;
			Circumcircle& endCs = delTri[ix].circumcircle;

			std::vector<Vector2f> result = CalcIntersection(endCs.GetCenter(), oriCs.GetCenter());
			for (Vector2f& intersection : result) {
				assert(ClampOrNot(intersection.x(), scopeX[0], scopeX[1]));
				assert(ClampOrNot(intersection.y(), scopeY[0], scopeY[1]));
				voronoiVs.push_back(std::move(intersection));
				polygon.push_back(voronoiVs.size() - 1);
			}
		}

		voronoiPolygon.push_back(std::move(polygon));
	}
}

void CalcMassOfCenter(std::vector<Vector2f>& voronoiVs, std::vector<std::vector<size_t>>& voronoiPolygon) {
	static int iterCount = 0;

	massCenters.resize(voronoiPolygon.size());
	int i = 0;
	int* pi = &i;

	std::transform(voronoiPolygon.begin(), voronoiPolygon.end(), massCenters.begin(), [voronoiVs, pi](auto& polygon) {
		Vector2f massP = CalcMassOfIrregularPolygon(polygon, voronoiVs, vertices.at(*pi));
		//assert(ClampOrNot(massP.x(), scopeX[0], scopeX[1], 0));
		//assert(ClampOrNot(massP.y(), scopeY[0], scopeY[1], 0));
		(*pi)++;
		return massP;
		});
}

// deprecated
void DrawVoronoi(HEMesh& delHeMesh) {
	for (HETriangle* heTri : delHeMesh.Polygons()) {
		std::vector<HETriangle*> adjHeTris = heTri->AdjPolygons();

		size_t ix = delHeMesh.Index(heTri);
		Triangle& tri = delTri[ix];
		Circumcircle& cs = tri.circumcircle;
		r.drawSquare(cs.GetCenter(), 5, Vector3f(255, 0, 0));

		// find boundary edge.
		for (HEHalfEdge* he : heTri->AdjHalfEdges()) {
			if (he->Pair()->IsOnBoundary()) {
				Vector2f center = (he->End()->position + he->Origin()->position) * 0.5f;
				Vector2f coef = solveLine(center, cs.GetCenter());

				// find point in window.
				bool isRight;
				bool isTop;
				if (he->Pair()->RightOrLeft(cs.GetCenter())) {
					isRight = center.x() - cs.GetCenter().x() > 0;
					isTop = center.y() - cs.GetCenter().y() > 0;
				}
				else {
					isRight = cs.GetCenter().x() - center.x() > 0;
					isTop = cs.GetCenter().y() - center.y() > 0;
				}

				int x = -1;
				int y = -1;
				if (isRight && isTop) {
					if (abs(coef[0]) >= 1) {
						y = row;
					}
					else if (abs(coef[0]) < 1) {
						x = column;
					}
				}
				else if (isRight && !isTop) {
					if (abs(coef[0]) >= 1) {
						y = 0;
					}
					else if (abs(coef[0]) < 1) {
						x = column;
					}
				}
				else if (!isRight && isTop) {
					if (abs(coef[0]) >= 1) {
						y = row;
					}
					else if (abs(coef[0]) < 1) {
						x = 0;
					}
				}
				else {
					if (abs(coef[0]) >= 1) {
						y = 0;
					}
					else if (abs(coef[0]) < 1) {
						x = 0;
					}
				}

				if (x != -1) {
					y = x * coef[0] + coef[1];
				}
				else if (y != -1) {
					x = (y - coef[1]) / coef[0];
				}

				r.draw_line(cs.GetCenter(), Vector2f(x, y), Vector3f(200, 200, 200));
			}
		}

		// find neighbor polygon. connect circumcircle center point
		for (HETriangle* adjTri : adjHeTris) {
			if (adjTri == nullptr) continue;

			size_t adjIx = delHeMesh.Index(adjTri);
			Circumcircle& ce = delTri[adjIx].circumcircle;

			r.draw_line(cs.GetCenter(), ce.GetCenter(), Vector3f(200, 200, 0));
		}
	}
}

void Flush() {
	if (vertices.size() >= 3) {
		// Delaunay三角剖分
		delTri = delaunay.Watson(vertices);
		CreateDelHeMesh(vertices);

		// 构建voronoi顶点
		voronoiVs.resize(delTri.size());
		std::transform(delTri.begin(), delTri.end(), voronoiVs.begin(), [](Triangle& tri) {
			// 有时候外接圆的圆心会落在窗口外(即x<0, x>column-1; y<0或y>row-1)
			return tri.circumcircle.GetCenter();
			});

		//TestMassOfTriangle();
		//DrawVoronoi(delHeMesh);

		voronoiPolygon.clear();
		CreateVoronoiMesh(delHeMesh, delTri, voronoiVs, voronoiPolygon);

		// Calc mass center
		CalcMassOfCenter(voronoiVs, voronoiPolygon);
	}
}

void Draw() {
	r.clear();

	// draw delaunay
	r.drawDelTri(delTri, vertices, Vector3f(100, 200, 100));

	// draw voronoi
	for (const std::vector<size_t>& voronoiCell : voronoiPolygon) {
		for (int i = 0; i < voronoiCell.size(); ++i) {
			int iNext = (i + 1) % voronoiCell.size();
			assert(ClampOrNot(voronoiVs[voronoiCell[i]].x(), scopeX[0], scopeX[1]));
			assert(ClampOrNot(voronoiVs[voronoiCell[i]].y(), scopeY[0], scopeY[1]));
			r.draw_line(voronoiVs[voronoiCell[i]], voronoiVs[voronoiCell[iNext]]);
		}
	}

	// draw mass of center
	for (const Vector2f& mass : massCenters) {
		r.drawSquare(mass, 5, Vector3f(0, 0, 255));
	}

	// draw vertices
	for (const Vector2f& p : vertices) {
		r.drawSquare(p);
	}
}

void onMouseCallBack(int e, int x, int y, int flag, void* userdata) {
	if (e == EVENT_LBUTTONUP) {
		vertices.push_back(Vector2f(x, y));

		Flush();
		Draw();
	}
	else if (e == EVENT_RBUTTONUP) {
		Vector2f hitPoint(x, y);
		for (const Triangle& tri : delTri) {
			if (tri.ifOnInnerSide(hitPoint)) {
				r.drawCircleWireframe(tri.circumcircle.GetCenter(), tri.circumcircle.GetRadius(), 0.05);
				break;
			}
		}
	}
}

int main(void) {
	namedWindow("voronoi");
	resizeWindow("voronoi", Size(row, column));
	setMouseCallback("voronoi", onMouseCallBack);

	int key = 0;
	static int iterCount = 0;

	std::cout << std::numeric_limits<float>::min() << std::endl;

	while (key != 27) {
		// 特别注意32是Mat中每个元素的内存空间, F是类型, C3是3通道的意思
		// 所以有时一个像素(rgba, 32位), 需要使用Mat的四个位置来存储(选择8U, 即8bit)
		switch (key)
		{
		case 'k':
			vertices = std::move(massCenters);
			std::cout << "Current iter count : " << iterCount++ << std::endl;
			Flush();
			Draw();
			break;
		case 'j':
			vertices = GeneratePs(pointSize, Vector2i(1, column - 2), Vector2i(1, row - 2));
			Flush();
			Draw();
			break;
		default:
			break;
		}

		Mat image(row, column, CV_32FC3, r.frame_buffer().data());
		image.convertTo(image, CV_8UC3, 1.0f);
		cv::imshow("voronoi", image);

		key = waitKey(10);
	}
	return 0;
}

