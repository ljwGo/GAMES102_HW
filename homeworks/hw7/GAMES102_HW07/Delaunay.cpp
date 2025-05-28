#include <vector>]
#include <list>
#include <Eigen/Core>
#include <algorithm>

#include "Triangle.hpp"
#include "Circumcircle.h"
#include "Delaunay.h"
#include "Rectangle.h"

#define SPAN 20

Rectangle superRect;

// Bowyer - Watson算法
std::vector<Triangle> Delaunay::Watson(std::vector<Vector2f>& vertices) {
	assert(vertices.size() >= 3 && "Has 3 at lease vertices to use delaunay algorithm");

	std::vector<int> indicates(vertices.size());
	std::list<Triangle> triBuf;
	std::list<Triangle> delaunayTris;
	std::list<Edge> edgeBuf;

	// 构建索引
	for (int i = 0; i < vertices.size(); ++i) {
		indicates[i] = i;
	}

	// 排序
	std::sort(indicates.begin(), indicates.end(), [vertices](int a, int b){
			return vertices[a][0] < vertices[b][0];
		});

	assert(("sort failed" && vertices[indicates[indicates.size()-1]][0] >= vertices[indicates[0]][0]));

	// 确定超矩形
	float xMin = std::numeric_limits<float>::max();
	float xMax = std::numeric_limits<float>::min();
	float yMin = std::numeric_limits<float>::max();
	float yMax = std::numeric_limits<float>::min();

	for (const Vector2f& v : vertices) {
		if (v[0] > xMax) {
			xMax = v[0];
		}
		if (v[0] < xMin) {
			xMin = v[0];
		}

		if (v[1] > yMax) {
			yMax = v[1];
		}
		if (v[1] < yMin){
			yMin = v[1];
		}
	}

	xMin -= SPAN;
	xMax += SPAN;
	yMin -= SPAN;
	yMax += SPAN;

	Vector2f v1(xMin, yMin);
	Vector2f v2(xMin, yMax);
	Vector2f v3(xMax, yMax); 
	Vector2f v4(xMax, yMin);

	//superRect = Rectangle(std::move(v1), std::move(v2), std::move(v3), std::move(v4));
	superRect = Rectangle(v1, v2, v3, v4);

	// 插入第一个点
	for (Edge& e : superRect.GetEdges()) {
		Triangle tri(e, vertices[indicates[0]], indicates[0]);
		tri.GenerateCC();
		triBuf.push_back(std::move(tri));
	}
	
	for (int i = 1; i < indicates.size(); ++i) {
		Vector2f& v = vertices[indicates[i]];
		edgeBuf.clear();
		edgeBuf.clear();

		// 对于list, <没有重载, 使用!=
		for (std::list<Triangle>::iterator iter = triBuf.begin(); iter != triBuf.end(); ) {
			Triangle& tri = *iter;

			// 在外接圆外
			if (!tri.circumcircle.IfVertexInCircle(v)) {
				// 在外接圆右边
				if (v.x() - tri.circumcircle.GetCenter().x() > tri.circumcircle.GetRadius()) {
					delaunayTris.push_back(tri);
					iter = triBuf.erase(iter);
					continue;
				}
			}
			else {
				edgeBuf.insert(edgeBuf.end(), tri.edges.begin(), tri.edges.end());
				iter = triBuf.erase(iter);
				continue;
			}
			iter++;
		}

		// 去除重复边
		for (std::list<Edge>::iterator iter = edgeBuf.begin(); iter != edgeBuf.end(); ) {
			std::list<Edge>::iterator itNext = iter;
			itNext++;
			bool hasSame = false;

			for (; itNext != edgeBuf.end(); ) {
				if (*iter == *itNext) {
					hasSame = true;
					edgeBuf.erase(itNext);
					break;
				}
				itNext++;
			}

			if (hasSame) {
				iter = edgeBuf.erase(iter);
				continue;
			}
			iter++;
		}

		// 添加新三角形
		for (Edge& e : edgeBuf) {
			Triangle tmpTri(e, v, indicates[i]);
			tmpTri.GenerateCC();
			triBuf.emplace_back(tmpTri);
		}
	}

	// 合并
	delaunayTris.insert(delaunayTris.end(), triBuf.begin(), triBuf.end());

	// 去除其中与超矩形顶点有关的所有三角形
	for (std::list<Triangle>::iterator iter = delaunayTris.begin(); iter != delaunayTris.end();) {
		bool flag = true;
		for (const Vector2f* vPtr : iter->v) {
			if (superRect.ifHasVertex(vPtr)) {
				iter = delaunayTris.erase(iter);
				flag = false;
				break;
			}
		}
		if (flag) iter++;
	}

	return std::vector<Triangle>(delaunayTris.begin(), delaunayTris.end());
}
