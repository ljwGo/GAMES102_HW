#pragma once
#include <Eigen/Core>
#include <assert.h>
#include <unordered_map>

#include "UHEMesh/detail/random_set.h"
#include "SmallRootHeap.h"
#include "MyIComparable.h"
#include "UContainer/Pool.h"
#include <spdlog/spdlog.h>

struct Vertex;
struct Edge;
struct HEMeshX;
struct Triangle;
struct HalfEdge;

using namespace Eigen;
using namespace Ubpa;

struct MyPair : virtual MyIComparable<MyPair> {
public:
	std::pair<Vertex*, Vertex*> pair;
	Edge* e;
	Matrix4f Q;
	Vector3f optinumPos;
	float energy;

	MyPair();
	MyPair(Vertex* v1, Vertex* v2);
	MyPair(HalfEdge* he);
	void UpdateQ();

	bool operator==(const MyPair& other) const;
	bool operator<(const MyPair& other) const;
};

class QEM{
public:
	QEM() { heMesh = nullptr; }
	QEM(HEMeshX* heMesh) { this->heMesh = heMesh; }

	void Init();
	void SetHEMesh(HEMeshX* heMesh) { this->heMesh = heMesh; };
	Matrix4f CalcQ(Vertex* v);
	Matrix4f CalcK(Triangle* t);
	Vector4f CalcOptimumPos(Vertex* vs, Vertex* ve);
	Vector4f CalcOptimumPos(Vertex* vs, Vertex* ve, Matrix4f Q);
	void UpdateVertexQ();
	float CalcQuadricError(const Matrix4f& Q, const Vector4f& optimumPos);
	void ContractOnce();
	void Contract(size_t iterCount);
	std::vector<MyPair*> SelectMyPair(Vertex* v);
	std::vector<std::pair<Vertex*, Vertex*>> SelectPair(Vertex* v);
	Pool<MyPair>& GetPool() { return pool; };
	SmallRootHeap<MyPair>& GetHeap() { return heap; };
private:

	HEMeshX* heMesh;
	SmallRootHeap<MyPair> heap;
	Pool<MyPair> pool;
};



