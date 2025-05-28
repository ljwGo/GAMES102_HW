#include "HEMeshX.h"
#include "QuadricErrorSimplication.h"
#include "Common.h"

MyPair::MyPair() {
	pair = std::pair<Vertex*, Vertex*>(nullptr, nullptr);
	e = nullptr;
	Q = Matrix4f::Ones();
	optinumPos = Vector3f::Zero();
	energy = 0;
};

MyPair::MyPair(Vertex* v1, Vertex* v2) {
	pair = std::pair<Vertex*, Vertex*>(v1, v2);
	e = Vertex::EdgeBetween(v1, v2);
	Q = v1->Q + v2->Q;
	optinumPos = toVector3f(HEMeshX::qem->CalcOptimumPos(v1, v2, Q));
	energy = HEMeshX::qem->CalcQuadricError(Q, toVector4f(optinumPos));
};

MyPair::MyPair(HalfEdge* he) {
	pair = std::pair<Vertex*, Vertex*>(he->Origin(), he->End());
	e = he->Edge();
	Q = he->Origin()->Q + he->End()->Q;
	optinumPos = toVector3f(HEMeshX::qem->CalcOptimumPos(he->Origin(), he->End(), Q));
	energy = HEMeshX::qem->CalcQuadricError(Q, toVector4f(optinumPos));
}

void MyPair::UpdateQ()
{
	Q = pair.first->Q + pair.second->Q;
	optinumPos = toVector3f(HEMeshX::qem->CalcOptimumPos(pair.first, pair.second, Q));
	energy = HEMeshX::qem->CalcQuadricError(Q, toVector4f(optinumPos));
};

bool MyPair::operator==(const MyPair& other) const {
	return pair.first == other.pair.first && pair.second == other.pair.second ||
		pair.second == other.pair.first && pair.first == other.pair.second;
}

bool MyPair::operator<(const MyPair& other) const {
	return energy < other.energy;
}

void QEM::Init()
{
	heap.Clear();
	pool.Clear();

	using Pair = std::pair<Vertex*, Vertex*>;
	using Pairs = std::vector<Pair>;

	assert(heMesh != nullptr);

	UpdateVertexQ();

	Pairs allPairs;
	for (Vertex* v : heMesh->Vertices()) {
		Pairs vPairs = SelectPair(v);
		allPairs.insert(allPairs.end(), vPairs.begin(), vPairs.end());
	}

	// 去重
	std::vector<bool> flags(allPairs.size(), true);

	for (int i = 0; i < allPairs.size(); ++i) {
		if (!flags[i]) continue;

		MyPair* myPair = pool.Request(allPairs[i].first, allPairs[i].second);
		heap.Push(myPair);
		allPairs[i].first->pairs.insert(myPair);
		allPairs[i].second->pairs.insert(myPair);

		for (int j = i + 1; j < allPairs.size(); ++j) {
			if (!flags[j]) continue;

			if (allPairs[i].first == allPairs[j].first && allPairs[i].second == allPairs[j].second || 
				allPairs[i].first == allPairs[j].second && allPairs[i].second == allPairs[j].first) {
				flags[j] = false;
				break;
			}
		}
	}
}

Matrix4f QEM::CalcQ(Vertex* v)
{
	Matrix4f Q = Matrix4f::Zero();
	for (Triangle* t : v->AdjPolygons()) {
		if (t == nullptr) continue;
		Q += CalcK(t);
	}
	return Q;
}

Matrix4f QEM::CalcK(Triangle* t)
{
	const std::vector<Vertex*>& vs = t->AdjVertices();
	assert(vs.size() == 3);
	Vector4f planeCoef = CalcPlaneExpression(vs[0]->position, vs[1]->position, vs[2]->position);
	Matrix4f K = planeCoef * planeCoef.transpose();
	return K;
};

Vector4f QEM::CalcOptimumPos(Vertex* vs, Vertex* ve)
{
	Matrix4f Q1 = CalcQ(vs);
	Matrix4f Q2 = CalcQ(ve);
	Matrix4f Q = Q1 + Q2;

	//assert(ClampOrNot(Q.determinant(), 0, 0, 1e-4));
	if (ClampOrNot(Q.determinant(), 0, 0, 1e-5)) {
		const Vector3f& p1 = vs->position;
		const Vector3f& p2 = ve->position;
		//spdlog::info("Q determinant eq 0");
		return Vector4f((p1[0] + p2[0]) * 0.5f, (p1[1] + p2[1]) * 0.5f, (p1[2] + p2[2]) * 0.5f, 1);
	}

	Q(3, 0) = 0;
	Q(3, 1) = 0;
	Q(3, 2) = 0;
	Q(3, 3) = 1;
	Vector4f b(0, 0, 0, 1);
	Vector4f v = Q.inverse() * b;
	assert(ClampOrNot(v[3], 1, 1, 1e-5));
	return v;
}

Vector4f QEM::CalcOptimumPos(Vertex* vs, Vertex* ve, Matrix4f Q)
{
	//assert(ClampOrNot(Q.determinant(), 0, 0, 1e-4));
	if (ClampOrNot(Q.determinant(), 0, 0, 1e-5)) {
		const Vector3f& p1 = vs->position;
		const Vector3f& p2 = ve->position;
		//spdlog::info("Q determinant eq 0");
		return Vector4f((p1[0] + p2[0]) * 0.5f, (p1[1] + p2[1]) * 0.5f, (p1[2] + p2[2]) * 0.5f, 1);
	}

	Q(3, 0) = 0;
	Q(3, 1) = 0;
	Q(3, 2) = 0;
	Q(3, 3) = 1;
	Vector4f b(0, 0, 0, 1);
	Vector4f v = Q.inverse() * b;
	assert(ClampOrNot(v[3], 1, 1, 1e-5));
	return v;
}

void QEM::UpdateVertexQ()
{
	for (Vertex* v : heMesh->Vertices()) {
		v->Q = CalcQ(v);
	}
}

float QEM::CalcQuadricError(const Matrix4f& Q, const Vector4f& optimumPos)
{
	VectorXf energy = optimumPos.transpose() * Q * optimumPos;
	assert(energy.size() == 1);
	//if (!ClampOrNot(Q.determinant(), 0, 0, 1e-5)) assert(energy(0, 0) >= 0);
	//return energy[0, 0];
	return abs(energy[0, 0]);
}

void QEM::ContractOnce()
{
	auto& pool = HEMeshX::qem->GetPool();

	MyPair* root = heap.PopRoot();
	assert(root != nullptr);

	std::pair<Vertex*, Vertex*> pair = root->pair;

	// 因为一些边合并后会生成游离边(形成不了三角形), 跳过这条边的合并
	/*
		------------
		\    /\    /                ------
		 \  /  \  /                 \    /
		  \/    \/                   \  /
		   ------(合并这条边)  --->   \/
		   \    /                     | (游离边)
		    \  /					  |
		     \/                       
	*/
	if (!heMesh->IsCollapsable(root->e)) {
		pair.first->pairs.erase(root);
		pair.second->pairs.erase(root);
		return;
	}

	// 删除原先的pair
	for (MyPair* vPair : pair.first->pairs) {
		if (vPair != root) {
			// 处理小根堆
			heap.Erase(vPair);
			// 删除邻接顶点的这条对
			Vertex* adjV = vPair->pair.first == pair.first ? vPair->pair.second : vPair->pair.first;
			adjV->pairs.erase(vPair);
			// 删除总容器的这条对	
			pool.Recycle(vPair);
		}
	}
	for (MyPair* vPair : pair.second->pairs) {
		if (vPair != root) {
			heap.Erase(vPair);
			Vertex* adjV = vPair->pair.second == pair.second ? vPair->pair.first : vPair->pair.second;
			adjV->pairs.erase(vPair);
			pool.Recycle(vPair);
		}
	}

	Vertex* v = heMesh->CollapseEdge(root->e);  // 只更新了topology关系
	// 此时v1和v2空间已经被回收了(并不是真正回收, 只是逻辑上回收了)

	assert(v != pair.first && v != pair.second);

	// 更新v的信息0x000001e137cebb58
	v->position = std::move(root->optinumPos);
	v->Q = HEMeshX::qem->CalcQ(v);

	// 添加新的pair
	for (HalfEdge* outHe : v->OutHalfEdges()) {
		// 更新的临近点信息
		Vertex* adjV = outHe->End();
		adjV->Q = HEMeshX::qem->CalcQ(adjV);

		// 跟新pair
		MyPair* newP = pool.Request(outHe);
		// 顶点添加pair
		adjV->pairs.insert(newP);
		v->pairs.insert(newP);
		heap.Push(newP);
	}

	std::vector<MyPair*> neighborhood2;
	// 更新与1领域顶点有关的pair
	for (HalfEdge* he : v->OutHalfEdges()) {
		Vertex* adjV = he->End();
		neighborhood2.insert(neighborhood2.end(), adjV->pairs.vec().begin(), adjV->pairs.vec().end());
	}

	// 去重
	std::vector<bool> flags(neighborhood2.size(), true);

	for (int ix = 0; ix < neighborhood2.size(); ++ix) {
		if (flags[ix] == false) continue;
		
		const std::pair<Vertex*, Vertex*>& pair = neighborhood2[ix]->pair;
		if (pair.first == v || pair.second == v) {
			flags[ix] = false;
			continue;
		}

		for (int nix = ix + 1; nix < neighborhood2.size(); ++nix) {
			if (flags[nix] == false) continue;

			if (neighborhood2[ix] == neighborhood2[nix]) {
				flags[nix] = false;
				break;
			}
		}
	}
	
	// 更新pair
	for (int i = 0; i < neighborhood2.size(); ++i) {
		if (flags[i]) {
			neighborhood2[i]->UpdateQ();
		}
	}

	pool.Recycle(root);
}

void QEM::Contract(size_t iterCount)
{
	for (size_t ix = 0; ix < iterCount; ++ix) {
		ContractOnce();
	}
}

std::vector<MyPair*> QEM::SelectMyPair(Vertex* v)
{
	v->pairs.clear();
	std::vector<MyPair*> result;
	const std::vector<HalfEdge*>& outHes = v->OutHalfEdges();
	result.reserve(outHes.size());

	return std::vector<MyPair*>();
}

std::vector<std::pair<Vertex*, Vertex*>> QEM::SelectPair(Vertex* v)
{
	// 边合并
	using Pair = std::pair<Vertex*, Vertex*>;

	std::vector<Pair> result;
	const std::vector<HalfEdge*>& outHes = v->OutHalfEdges();
	result.reserve(outHes.size());

	for (HalfEdge* outHe : outHes) {
		result.emplace_back(Pair(outHe->Origin(), outHe->End()));
	}

	return result;
}

