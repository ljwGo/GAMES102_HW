#pragma once

#include "Eigen/Sparse"
#include <UHEMesh/HEMesh.h>

#include <vector>
#include <UGM/UGM.h>
#include <spdlog/spdlog.h>

#define EPSILON 0.001
#define MPI 3.1415926

// Traits特性
typedef Eigen::SparseMatrix<double> SpMat;
typedef Eigen::SparseVector<double> SpVec;
typedef Eigen::Triplet<double> Tri;
typedef Eigen::SparseQR<SpMat, Eigen::COLAMDOrdering<int>> SQR;
typedef Eigen::LeastSquaresConjugateGradient<SpMat> LSCG;

struct Vertex;
struct Edge;
struct Triangle;
struct HalfEdge;

enum BoundaryWeightCalcMode {
	OneSide,
	DropOneSide
};

// 这样做, 可以让各子类(如Vertex和HalfEdge彼此访问, Traits相当于中介者身份)
using HEMeshXTraits = Ubpa::HEMeshTraits<Vertex, Edge, Triangle, HalfEdge>;

struct HalfEdge : Ubpa::THalfEdge<HEMeshXTraits> {
	// you can add any attributes and mothods to HalfEdge

};

struct Vertex : Ubpa::TVertex<HEMeshXTraits> {
	// you can add any attributes and mothods to Vertex
	Ubpa::pointf3 position{ 0.f };
	Ubpa::pointf3 newP{ 0.f };

	std::vector<V*>* ListNeighbourhoodPointOne() {
		if (IsIsolated()) return nullptr;

		std::vector<V*>* neighbourhoodOnePoints = new std::vector<V*>();
		H* originHe = HalfEdge();
		V* newV = originHe->End();

		neighbourhoodOnePoints->push_back(newV);
		H* he = originHe->RotateNext();

		while (he != originHe) {
			newV = he->End();

			neighbourhoodOnePoints->push_back(newV);
			he = he->RotateNext();
		}

		return neighbourhoodOnePoints;
	}

	Ubpa::vecf3 const operator-(const V& other) const {
		return this->position - other.position;
	}

	bool operator==(const V& other) const {
		return this->position == other.position;
	}

	float GetAmixed(Vertex* v) {
		float A_mixed = 0.f;
		if (v->IsOnBoundary()) return A_mixed;

		for (auto* adjV : v->AdjVertices()) {
			Vertex* np;
			auto* adjVE = adjV->HalfEdge();
			while (true) {
				if (v == adjVE->Next()->End()) {
					np = adjVE->End();
					break;
				}
				else
					adjVE = adjVE->Pair()->Next();
			}

			// 锐角三角形
			if ((adjV->position - v->position).dot(np->position - v->position) >= 0.f && (v->position - adjV->position).dot(np->position - adjV->position) >= 0.f && (v->position - np->position).dot(adjV->position - np->position) >= 0.f) {
				//T is non-obtuse
				if (GetTriangleArea(*v, *adjV, *np) > EPSILON) {
					// 不知道这里怎么来的?
					A_mixed += (
						v->position.distance2(adjV->position)
						* (v->position - np->position)
						.cot_theta(adjV->position - np->position) +
						v->position.distance2(np->position)
						* (v->position - adjV->position)
						.cot_theta(np->position - adjV->position)
						) / 8.f;
				}
			}
			else {
				// 有钝角
				if ((adjV->position - v->position)
					.dot(np->position - v->position) < 0.f) {
					//T is obtuse && the angle of T at v is obtuse
					A_mixed += GetTriangleArea(*v, *adjV, *np) / 2.f;
				}
				else {
					//T is obtuse && the angle of T at v is non-obtuse
					A_mixed += GetTriangleArea(*v, *adjV, *np) / 4.f;
				}
			}
		}

		return A_mixed;
	}

	float GetAmixedAll(Vertex* v) {
		float A_mixed = 0.f;

		for (auto* adjV : v->AdjVertices()) {
			Vertex* np = nullptr;
			auto* origin = adjV->HalfEdge();
			auto* adjVE = origin;

			if (v == adjVE->Next()->End()) {
				np = adjVE->End();
			}
			else {
				adjVE = adjVE->Pair()->Next();
			}
			while (adjVE != origin) {
				if (v == adjVE->Next()->End()) {
					np = adjVE->End();
					break;
				}
				else
					adjVE = adjVE->Pair()->Next();
			}

			if (np == nullptr) {
				continue;
			}

			if ((adjV->position - v->position).dot(np->position - v->position) >= 0.f && (v->position - adjV->position).dot(np->position - adjV->position) >= 0.f && (v->position - np->position).dot(adjV->position - np->position) >= 0.f) {
				//T is non-obtuse
				if (GetTriangleArea(*v, *adjV, *np) > EPSILON) {
					A_mixed += (
						v->position.distance2(adjV->position)
						* (v->position - np->position)
						.cot_theta(adjV->position - np->position) +
						v->position.distance2(np->position)
						* (v->position - adjV->position)
						.cot_theta(np->position - adjV->position)
						) / 8.f;
				}
			}
			else {
				if ((adjV->position - v->position)
					.dot(np->position - v->position) < 0.f) {
					//T is obtuse && the angle of T at v is obtuse
					A_mixed += GetTriangleArea(*v, *adjV, *np) / 2.f;
				}
				else {
					//T is obtuse && the angle of T at v is non-obtuse
					A_mixed += GetTriangleArea(*v, *adjV, *np) / 4.f;
				}
			}
		}

		return A_mixed;
	}

	// use localMinCurvature method.
	// K_mean dot normal = sum(cos(a)+cos(b))(v_i - v) / 2A;
	void CalcNewPosOnce(float lambda) {
		if (IsOnBoundary()) return;

		Ubpa::vecf3 direct = GetMeanCurvatureOperator();
		this->newP = this->position - direct * lambda * 0.5f;
	}

	Ubpa::vecf3 GetMeanCurvatureOperator() {
		float Amixed = GetAmixed(this);
		if (Amixed < EPSILON) return Ubpa::vecf3(0.f);

		Ubpa::vecf3 k_mean = GetMeanCurvatureVecter();
		return k_mean * .5f / Amixed;
	}

	Ubpa::vecf3 GetMeanCurvatureVecter() {
		Ubpa::vecf3 k_mean{ 0.f };
		
		if (IsOnBoundary()) return k_mean;

		for (auto* adjV : this->AdjVertices()) {
			V* pPre, * pNext;
			auto* adjVE = adjV->HalfEdge();

			while (true) {
				if (adjVE->Next()->End() == this) {
					pNext = adjVE->End();
					break;
				}
				else {
					adjVE = adjVE->RotateNext();
				}
			}

			while (true) {
				if (adjVE->End() == this) {
					pPre = adjVE->Next()->End();
					break;
				}
				else {
					adjVE = adjVE->RotateNext();
				}
			}

			if (GetTriangleArea(*this, *adjV, *pNext) > EPSILON && GetTriangleArea(*this, *adjV, *pPre) > EPSILON) {
				// 注意不是cos而是cot余切
				float cota = (adjV->position - pPre->position).cot_theta(this->position - pPre->position);
				float cotb = (adjV->position - pNext->position).cot_theta(this->position - pNext->position);
				k_mean += (cota + cotb) * (this->position - adjV->position);
			}
		}

		return k_mean;
	}

	float GetWeightDropSide(Vertex* from, Vertex* to) {
		Vertex* pPre = nullptr, * pNext = nullptr;
		struct HalfEdge* originHe = to->HalfEdge();
		struct HalfEdge* he = originHe;

		if (he->End() == from) {
			pNext = he->Next()->End();
		}
		else {
			he = he->RotateNext();
		}
		while (he != originHe)
		{
			if (he->End() == from) {
				pNext = he->Next()->End();
				break;
			}
			he = he->RotateNext();
		}

		if (he->Next()->End() == from) {
			pPre = he->End();
		}
		else {
			he = he->RotateNext();
		}
		while (he != originHe) {
			if (he->Next()->End() == from) {
				pPre = he->End();
				break;
			}
			he = he->RotateNext();
		}

		// 取边界点一边的cot权重
		float cot_vp = 0, cot_vn = 0;
		if (pPre != nullptr && pNext != nullptr && GetTriangleArea(*pPre, *from, *to) > EPSILON && GetTriangleArea(*pNext, *from, *to) > EPSILON) {
			Ubpa::vecf3 preV2V = from->position - pPre->position;
			Ubpa::vecf3 preV2AdjV = to->position - pPre->position;
			Ubpa::vecf3 nextV2V = from->position - pNext->position;
			Ubpa::vecf3 nextV2AdjV = to->position - pNext->position;
			// 注意避免点重合引发cot值问题(同tan定义域)
			return preV2V.cot_theta(preV2AdjV) + nextV2V.cot_theta(nextV2AdjV);
		}

		return 0;
	}

	float GetWeightOneSide(Vertex* from, Vertex* to) {
		Vertex *pPre = nullptr, * pNext = nullptr;
		struct HalfEdge* originHe = to->HalfEdge();
		struct HalfEdge* he = originHe;

		if (he->End() == from) {
			pNext = he->Next()->End();
		}
		else {
			he = he->RotateNext();
		}
		while (he != originHe)
		{
			if (he->End() == from) {
				pNext = he->Next()->End();
				break;
			}
			he = he->RotateNext();
		}

		if (he->Next()->End() == from) {
			pPre = he->End();
		}
		else {
			he = he->RotateNext();
		}
		while (he != originHe) {
			if (he->Next()->End() == from) {
				pPre = he->End();
				break;
			}
			he = he->RotateNext();
		}

		// 取边界点一边的cot权重
		float cot_vp = 0, cot_vn = 0;
		if (pPre != nullptr && GetTriangleArea(*pPre, *from, *to) > EPSILON) {
			Ubpa::vecf3 preV2V = from->position - pPre->position;
			Ubpa::vecf3 preV2AdjV = to->position - pPre->position;
			cot_vp = preV2V.cot_theta(preV2AdjV);
		}
		
		if (pNext != nullptr && GetTriangleArea(*pNext, *from, *to) > EPSILON) {
			Ubpa::vecf3 nextV2V = from->position - pNext->position;
			Ubpa::vecf3 nextV2AdjV = to->position - pNext->position;
			// 当两个点重合时会出现问题
			cot_vn = nextV2V.cot_theta(nextV2AdjV);
		}

		return cot_vp + cot_vn;
	}

	float GetWeight(Vertex* from, Vertex* to,BoundaryWeightCalcMode mode = OneSide,bool ContainBoundary = true) {
		switch (mode)
		{
		case OneSide:
			return GetWeightOneSide(from, to);
		case DropOneSide:
			return GetWeightDropSide(from, to);
		}
		return 0;
	}

	float GetTriangleArea(const V& v0, const V& v1, const V& v2) {
		Ubpa::vecf3 e1 = v0 - v1;
		Ubpa::vecf3 e2 = v2 - v1;

		return e1.cross(e2).norm() * 0.5;
	}

	void UpdatePos() {
		if (IsOnBoundary()) return;
		this->position = std::move(this->newP);
	}

	void UpdateAllPos() {
		this->position = std::move(this->newP);
	}
};

struct Edge : Ubpa::TEdge<HEMeshXTraits> {
	// you can add any attributes and mothods to Edge

	// [example]

	// Ubpa::pointf3 Midpoint() const {
	//     auto* p = HalfEdge()->Origin();
    //     auto* q = HalfEdge()->End();
	//     return Ubpa::pointf3::combine(std::array{ p,q }, 0.5f);
	// }
};

struct Triangle : Ubpa::TPolygon<HEMeshXTraits> {
	// you can add any attributes and mothods to Triangle

	// [example]
	// 
	// float area{ 0.f };
	// 
	// bool IsTriangle() const {
	//     return Degree() == 3;
	// }
	// 
	// void UpdateArea() {
	//     assert(IsTriangle());
	//     auto* p0 = HalfEdge()->Origin();
	//     auto* p1 = HalfEdge()->HalfEdge()->Origin();
	//     auto* p2 = HalfEdge()->HalfEdge()->HalfEdge()->Origin();
	//     auto d01 = p1 - p0;
	//     auto d02 = p2 - p0;
	//     area = 0.5f * d02.cross(d01);
	// }
};

struct HEMeshX : Ubpa::HEMesh<HEMeshXTraits> {
	// you can add any attributes and mothods to HEMeshX

	// Use local laplace smoothing to update position.
	void UpdateVertexsPos(int iterCount, float lambda) {
		std::vector<V*> allVertexs = Vertices();

		for (int k = 0; k < iterCount; ++k) {
			for (int i = 0; i < allVertexs.size(); ++i) {
				allVertexs[i]->CalcNewPosOnce(lambda);
			}

			for (int i = 0; i < allVertexs.size(); ++i) {
				allVertexs[i]->UpdatePos();
			}
		}
	}

	// bug是边界点引起的
	void GLSSoftConstraintPos(BoundaryWeightCalcMode mode) {
		std::vector<V*> allVertexs = Vertices();
		std::vector<Tri> triplet;

		int size = allVertexs.size();
		int rows, cols;
		rows = size * 3;
		cols = size * 3;
		triplet.reserve(rows);

		int boundaryCount = 0;
		for (auto& v : allVertexs) {
			if (v->IsOnBoundary()) boundaryCount++;
		}

		//SpVec sv(rows + boundaryCount * 3);
		Eigen::VectorXd vx = Eigen::VectorXd::Zero(rows + boundaryCount * 3);

		// 构建非零元素
		int i = 0;
		float sumW;
		for (auto* v : allVertexs) {
			//float Amixed = v->GetAmixedAll(v);
			//float invAmixed = Amixed < EPSILON ? 0 : 1. / Amixed;

			sumW = 0;
			for (auto* adjV : v->AdjVertices()) {
				float w = v->GetWeight(v, adjV, mode);
				sumW += w;

				for (int j = 0; j < allVertexs.size(); ++j) {
					if (allVertexs[j] == adjV) {
						float coef = -w;
						triplet.push_back(Tri(i, j, coef));
						triplet.push_back(Tri(i + size, j + size, coef));
						triplet.push_back(Tri(i + size * 2, j + size * 2, coef));
					}
				}
			}

			triplet.push_back(Tri(i, i, sumW));
			triplet.push_back(Tri(i + size, i + size, sumW));
			triplet.push_back(Tri(i + size * 2, i + size * 2, sumW));
			i++;
		}

		// 添加边界点软约束
		for (int i = 0; i < allVertexs.size(); ++i) {
			Vertex* v = allVertexs[i];

			if (v->IsOnBoundary()) {
				triplet.push_back(Tri(rows, i, 1));
				triplet.push_back(Tri(rows + 1, size + i, 1));
				triplet.push_back(Tri(rows + 2, size * 2 + i, 1));
				//sv.insert(rows) = allVertexs[i]->position[0];
				//sv.insert(rows + 1) = allVertexs[i]->position[1];
				//sv.insert(rows + 2) = allVertexs[i]->position[2];
				vx[rows] = allVertexs[i]->position[0];
				vx[rows + 1] = allVertexs[i]->position[1];
				vx[rows + 2] = allVertexs[i]->position[2];
				rows += 3;
			}
		}

		SpMat sm(rows, cols);

		// 创建稀疏矩阵
		sm.setFromTriplets(triplet.begin(), triplet.end());
		sm.makeCompressed();

		// CG解稀疏矩阵
		LSCG lscgSolver;
		//SQR sqrSolver;
		lscgSolver.compute(sm);
		//sqrSolver.compute(sm);
		//lscgSolver.setMaxIterations(1000);

		if (lscgSolver.info() != Eigen::ComputationInfo
			::Success) {
			spdlog::info("CG algorithm compute matrix A failed!");
		}

		//if (sqrSolver.info() != Eigen::ComputationInfo
		//	::Success) {
		//	spdlog::info("QR algorithm compute matrix A failed!");
		//}

		//const auto& x = lscgSolver.solveWithGuess(vx, predictV).rhs();
		Eigen::VectorXd x = lscgSolver.solve(vx);
		//Eigen::VectorXd x1 = sqrSolver.solve(vx);
		if (lscgSolver.info() != Eigen::ComputationInfo
			::Success) {
			spdlog::info("CG algorithm solve x failed");
		}

		//if (sqrSolver.info() != Eigen::ComputationInfo
		//	::Success) {
		//	spdlog::info("CG algorithm solve x failed");
		//}

		assert(lscgSolver.iterations() > 0);

		// Update Pos;
		// Solve.rhs是Ax=b右边的b值向量
		for (int i = 0; i < allVertexs.size(); ++i) {
			allVertexs[i]->position = Ubpa::pointf3(x[i], x[i + size], x[i + size * 2]);
		}
	}

	void GLSHardConstraintPos(BoundaryWeightCalcMode mode) {
		std::vector<V*> allVertexs = Vertices();
		std::vector<Tri> triplet;

		int size = allVertexs.size();
		int rows, cols;
		rows = size * 3;
		cols = size * 3;
		triplet.reserve(rows);

		Eigen::VectorXd vx = Eigen::VectorXd::Zero(rows);

		int i = 0;
		float sumW;
		for (auto* v : allVertexs) {
			//float Amixed = v->GetAmixedAll(v);
			//float invAmixed = Amixed < EPSILON ? 0 : 1. / Amixed;
			if (v->IsOnBoundary()) {
				triplet.push_back(Tri(i, i, 1));
				triplet.push_back(Tri(i + size, i + size, 1));
				triplet.push_back(Tri(i + size * 2, i + size * 2, 1));
				vx[i] = v->position[0];
				vx[i + size] = v->position[1];
				vx[i + size * 2] = v->position[2];
				i++;
				continue;
			}

			sumW = 0;
			for (auto* adjV : v->AdjVertices()) {
				float w = v->GetWeight(v, adjV, mode);
				sumW += w;

				for (int j = 0; j < allVertexs.size(); ++j) {
					if (allVertexs[j] == adjV) {
						float coef = -w;
						triplet.push_back(Tri(i, j, coef));
						triplet.push_back(Tri(i + size, j + size, coef));
						triplet.push_back(Tri(i + size * 2, j + size * 2, coef));
					}
				}
			}

			triplet.push_back(Tri(i, i, sumW));
			triplet.push_back(Tri(i + size, i + size, sumW));
			triplet.push_back(Tri(i + size * 2, i + size * 2, sumW));
			i++;
		}

		SpMat sm(rows, cols);

		sm.setFromTriplets(triplet.begin(), triplet.end());
		sm.makeCompressed();

		LSCG lscgSolver;
		lscgSolver.compute(sm);

		if (lscgSolver.info() != Eigen::ComputationInfo
			::Success) {
			spdlog::info("CG algorithm compute matrix A failed!");
		}

		Eigen::VectorXd x = lscgSolver.solve(vx);
		if (lscgSolver.info() != Eigen::ComputationInfo
			::Success) {
			spdlog::info("CG algorithm solve x failed");
		}

		assert(lscgSolver.iterations() > 0);

		//int count = lscgSolver.iterations();
		for (int i = 0; i < allVertexs.size(); ++i) {
			allVertexs[i]->position = Ubpa::pointf3(x[i], x[i + size], x[i + size * 2]);
		}
	}

	void GLSNoContraint(BoundaryWeightCalcMode mode) {
		std::vector<V*> allVertexs = Vertices();
		std::vector<Tri> triplet;

		int size = allVertexs.size();
		int rows, cols;
		rows = size * 3;
		cols = size * 3;
		triplet.reserve(rows);

		Eigen::VectorXd vx = Eigen::VectorXd::Zero(rows);

		int i = 0;
		float sumW;
		for (auto* v : allVertexs) {
			//float Amixed = v->GetAmixedAll(v);
			//float invAmixed = Amixed < EPSILON ? 0 : 1. / Amixed;

			sumW = 0;
			for (auto* adjV : v->AdjVertices()) {
				float w = v->GetWeight(v, adjV, mode);
				sumW += w;

				for (int j = 0; j < allVertexs.size(); ++j) {
					if (allVertexs[j] == adjV) {
						float coef = -w;
						triplet.push_back(Tri(i, j, coef));
						triplet.push_back(Tri(i + size, j + size, coef));
						triplet.push_back(Tri(i + size * 2, j + size * 2, coef));
					}
				}
			}

			triplet.push_back(Tri(i, i, sumW));
			triplet.push_back(Tri(i + size, i + size, sumW));
			triplet.push_back(Tri(i + size * 2, i + size * 2, sumW));
			i++;
		}

		SpMat sm(rows, cols);

		sm.setFromTriplets(triplet.begin(), triplet.end());
		sm.makeCompressed();

		LSCG lscgSolver;
		lscgSolver.compute(sm);

		if (lscgSolver.info() != Eigen::ComputationInfo
			::Success) {
			spdlog::info("CG algorithm compute matrix A failed!");
		}

		Eigen::VectorXd x = lscgSolver.solve(vx);
		if (lscgSolver.info() != Eigen::ComputationInfo
			::Success) {
			spdlog::info("CG algorithm solve x failed");
		}

		assert(lscgSolver.iterations() > 0);
		// 齐次方程, 若A为非满秩(奇异)矩阵, 才有解
		// int count = lscgSolver.iterations();

		for (int i = 0; i < allVertexs.size(); ++i) {
			allVertexs[i]->position = Ubpa::pointf3(x[i], x[i + size], x[i + size * 2]);
		}
	}

	void GLS(int SLMmode, BoundaryWeightCalcMode mode = OneSide) {
		switch (SLMmode)
		{
		case 0:
			GLSNoContraint(mode);
			break;
		case 1:
			GLSHardConstraintPos(mode);
			break;
		case 2:
			GLSSoftConstraintPos(mode);
			break;
		}
	}

	void BoundaryMap(int convexShape, int edgeLen) {
		const std::vector<V*>& vertices = Vertices();
		std::vector<HalfEdge*> boundary;

		// Perimeter
		float perimeter = 0;
		switch (convexShape) {
		case 0:
			// Rectangle
			perimeter = edgeLen * 4;
			break;
		case 1:
			// Circle
			perimeter = MPI * 2 * edgeLen;
			break;
		}
		
		// Find first boundary vertex.
		Vertex* iterV = nullptr;
		Vertex* originBV = nullptr, * previousV = nullptr;
		for (auto* v : vertices) {
			if (v->IsOnBoundary()) {
				originBV = v;
				break;
			}
		}

		if (originBV == nullptr) return;

		// Find all boundary vertex. Calc boundary length.
		struct HalfEdge* originHe = originBV->HalfEdge();
		struct HalfEdge* he = originHe;
		iterV = originBV;
		float boundaryLen = 0;
		bool boundaryEnd = false;
		bool findNextVertex = false;

		if (he->End()->IsOnBoundary() && he->End() != previousV) {
			previousV = he->Origin();
			iterV = he->End();
			boundary.push_back(he);
			he = iterV->HalfEdge();
			originHe = he;
			boundaryLen += iterV->position.distance(previousV->position);
		}
		else {
			he = he->RotateNext();
		}
		while (he != originHe) {
			if (he->End()->IsOnBoundary() && he->End() != previousV) {
				previousV = he->Origin();
				iterV = he->End();
				boundary.push_back(he);
				he = iterV->HalfEdge();
				originHe = he;
				boundaryLen += iterV->position.distance(previousV->position);
				break;
			}
			else {
				he = he->RotateNext();
			}
		}

		while (iterV != originBV && !boundaryEnd) {
			if (he->End()->IsOnBoundary() && he->End() != previousV) {
				previousV = he->Origin();
				iterV = he->End();
				boundary.push_back(he);
				he = iterV->HalfEdge();
				originHe = he;
				boundaryLen += iterV->position.distance(previousV->position);
				continue;
			}
			else {
				he = he->RotateNext();
			}
			boundaryEnd = true;
			while (he != originHe) {
				if (he->End()->IsOnBoundary() && he->End() != previousV) {
					previousV = he->Origin();
					iterV = he->End();
					boundary.push_back(he);
					he = iterV->HalfEdge();
					originHe = he;
					boundaryLen += iterV->position.distance(previousV->position);
					boundaryEnd = false;
					break;
				}
				else {
					he = he->RotateNext();
				}
			}
		}

		//(Arc parameterization)
		// Assign per boundary vertex an uv coordinate.
		//if (boundary.size() > 0) {
		//	// First vertex
		//	boundary[0]->Origin()->newP = Ubpa::pointf3(0, 0, 0);
		//}
		// Start from (0, 0); top, right, botoom, left.
		/*float uvSumLen = 0;
		for (HalfEdge* edge : boundary) {
			Vertex* origin = edge->Origin();
			Vertex* end = edge->End();
			float length = origin->position.distance(end->position);
			float uvLen = length / boundaryLen * perimeter;
			uvSumLen += uvLen;
			int edgeCase = uvSumLen / (float)edgeLen;

			switch (edgeCase)
			{
			case 0:
				edge->End()->newP = Ubpa::pointf3(0, uvSumLen, 0);
				break;
			case 1:
				edge->End()->newP = Ubpa::pointf3(uvSumLen - edgeLen, edgeLen, 0);
				break;
			case 2:
				edge->End()->newP = Ubpa::pointf3(edgeLen, edgeLen * 3 - uvSumLen, 0);
				break;
			case 3:
				edge->End()->newP = Ubpa::pointf3(4 * edgeLen - uvSumLen, 0, 0);
				break;
			default:
				if (uvSumLen - perimeter < EPSILON) {
					edge->End()->newP = Ubpa::pointf3(0.001, 0, 0);
				}
				else {
					spdlog::error("uv length too long: ");
					spdlog::error(uvSumLen);
				}
				break;
			}
		}*/

		//(Uniform parameterization)
		if (convexShape == 0) {  // Rectangle
			int bvCount = boundary.size() + 1;
			if (boundary[0]->Origin() == boundary[boundary.size() - 1]->End()) {
				bvCount--;
			}
			int tmp = bvCount % 4;
			int bvCountPerEdge = tmp > 0 ? bvCount / 4 + 1 : bvCount / 4;
			float span = edgeLen * 0.125f;
			int externCount = bvCountPerEdge * 4 - bvCount;
			int ix = 0;
			for (HalfEdge* edge : boundary) {
				Vertex* end = edge->End();
				int edgeCase = ix / bvCountPerEdge;

				if (externCount > 0 && (ix + 1) % bvCount != 0) {
					ix++;
					externCount--;
				}

				switch (edgeCase)
				{
				case 0:
					edge->Origin()->newP = Ubpa::pointf3(0, span * ix, 0);
					break;
				case 1:
					edge->Origin()->newP = Ubpa::pointf3(span * (ix - bvCountPerEdge), edgeLen, 0);
					break;
				case 2:
					edge->Origin()->newP = Ubpa::pointf3(edgeLen, edgeLen - span * (ix - 2 * bvCountPerEdge), 0);
					break;
				case 3:
					edge->Origin()->newP = Ubpa::pointf3(edgeLen - span * (ix - 3 * bvCountPerEdge), 0, 0);
					break;
				}
				ix++;
			}
		}
		else if (convexShape == 1) {  // Circle
			float angle = MPI * 2 / boundary.size();
			int i = 0;
			for (HalfEdge* edge : boundary) {
				edge->Origin()->newP = Ubpa::pointf3(std::cos(angle * i) * 2 * edgeLen, std::sin(angle * i) * 2 * edgeLen, 0);
				i++;
			}
		}

		/*if (boundary.size() > 0) {
			boundary[boundary.size() - 1]->End()->newP = Ubpa::pointf3(0, 0, 0);
		}*/

		// Create matrix.
		std::vector<Tri> triplet;

		int size = vertices.size();
		int rows, cols;
		rows = size * 3;
		cols = size * 3;
		triplet.reserve(rows);

		Eigen::VectorXd vx = Eigen::VectorXd::Zero(rows);

		int i = 0;
		float sumW;
		for (auto* v : vertices) {
			//float Amixed = v->GetAmixedAll(v);
			//float invAmixed = Amixed < EPSILON ? 0 : 1. / Amixed;
			if (v->IsOnBoundary()) {
				triplet.push_back(Tri(i, i, 1));
				triplet.push_back(Tri(i + size, i + size, 1));
				triplet.push_back(Tri(i + size * 2, i + size * 2, 1));
				vx[i] = v->newP[0];
				vx[i + size] = v->newP[1];
				vx[i + size * 2] = v->newP[2];
				i++;
				continue;
			}

			sumW = 0;
			for (auto* adjV : v->AdjVertices()) {
				float w = v->GetWeight(v, adjV, OneSide);
				sumW += w;

				for (int j = 0; j < vertices.size(); ++j) {
					if (vertices[j] == adjV) {
						float coef = -w;
						triplet.push_back(Tri(i, j, coef));
						triplet.push_back(Tri(i + size, j + size, coef));
						triplet.push_back(Tri(i + size * 2, j + size * 2, coef));
					}
				}
			}

			triplet.push_back(Tri(i, i, sumW));
			triplet.push_back(Tri(i + size, i + size, sumW));
			triplet.push_back(Tri(i + size * 2, i + size * 2, sumW));
			i++;
		}

		SpMat sm(rows, cols);

		sm.setFromTriplets(triplet.begin(), triplet.end());
		sm.makeCompressed();

		LSCG lscgSolver;
		lscgSolver.compute(sm);

		if (lscgSolver.info() != Eigen::ComputationInfo
			::Success) {
			spdlog::info("CG algorithm compute matrix A failed!");
		}

		Eigen::VectorXd x = lscgSolver.solve(vx);
		if (lscgSolver.info() != Eigen::ComputationInfo
			::Success) {
			spdlog::info("CG algorithm solve x failed");
		}

		assert(lscgSolver.iterations() > 0);

		//int count = lscgSolver.iterations();
		for (int i = 0; i < vertices.size(); ++i) {
			vertices[i]->position = Ubpa::pointf3(x[i], x[i + size], x[i + size * 2]);
		}
	}
};
