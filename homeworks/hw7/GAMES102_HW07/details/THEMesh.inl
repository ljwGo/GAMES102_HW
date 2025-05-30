#pragma once

#include <set>
#include <string>
#include <algorithm>
#include <iterator>
#include <unordered_set>
#include <unordered_map>

template<typename Traits>
template<typename T, typename... Args>
T* THEMesh<Traits>::New(Args&&... args) {
	T* elem = MemVarOf<T>::pool(this).Request();
	new (elem) T(std::forward<Args>(args)...);
	MemVarOf<T>::set(this).insert(elem);
	return elem;
}

// clear and erase
template<typename Traits>
template<typename T>
void THEMesh<Traits>::Delete(T* elem) {
	MemVarOf<T>::pool(this).Recycle(elem);
	MemVarOf<T>::set(this).erase(elem);
}

template<typename Traits>
std::vector<size_t> THEMesh<Traits>::Indices(P* p) const {
	assert(p != nullptr);
	std::vector<size_t> indices;
	for (auto* v : p->AdjVertices())
		indices.push_back(Index(v));
	return indices;
}

template<typename Traits>
template<typename... Args>
HEMeshTraits_HE<Traits>* THEMesh<Traits>::AddEdge(V* v0, V* v1, Args&&... args) {
	//assert(v0 != nullptr && v1 != nullptr && v0 != v1 && !V::IsConnected(v0, v1));

	auto* he0 = New<HE>();
	auto* he1 = New<HE>();

	// [init]
	he0->SetNext(he1);
	he0->SetPair(he1);
	he0->SetOrigin(v0);

	he1->SetNext(he0);
	he1->SetPair(he0);
	he1->SetOrigin(v1);

	// 构建两个封闭图形很复杂和精妙
	// [link he0]
	if (!v0->IsIsolated()) {
		auto* inV0 = v0->FindFreeIncident();
		if (inV0 == nullptr)
			return nullptr;
		auto* outV0 = inV0->Next();

		inV0->SetNext(he0);
		he1->SetNext(outV0);
	}
	else
		v0->SetHalfEdge(he0);

	// [link he1]
	if (!v1->IsIsolated()) {
		auto* inV1 = v1->FindFreeIncident();
		if (inV1 == nullptr)
			return nullptr;
		auto* outV1 = inV1->Next();

		inV1->SetNext(he1);
		he0->SetNext(outV1);
	}
	else
		v1->SetHalfEdge(he1);

	return he0;
}

template<typename Traits>
template<typename... Args>
HEMeshTraits_P<Traits>* THEMesh<Traits>::AddPolygon(const std::vector<HE*>& heLoop, Args&&... args) {
	assert(!heLoop.empty() && "heLoop must be non-empty");
#ifndef NDEBUG
	for (size_t i = 0; i < heLoop.size(); i++) {
		assert(heLoop[i] != nullptr);
		assert(heLoop[i]->IsFree());
		size_t next = (i + 1) % heLoop.size();
		assert(heLoop[i]->End() == heLoop[next]->Origin());
	}
#endif // !NDEBUG

	// reorder link
	for (size_t i = 0; i < heLoop.size(); i++) {
		size_t next = (i + 1) % heLoop.size();
		bool success = HE::MakeAdjacent(heLoop[i], heLoop[next]);
		assert(success && "the polygon would introduce a non - monifold condition");
	}

	// link polygon and heLoop
	auto* polygon = New<P>(std::forward<Args>(args)...);

	polygon->SetHalfEdge(heLoop[0]);
	for (auto* he : heLoop)
		he->SetPolygon(polygon);

	return polygon;
}

template<typename Traits>
void THEMesh<Traits>::RemovePolygon(P* polygon) {
	assert(polygon != nullptr);
	for (auto* he : polygon->AdjHalfEdges())
		he->SetPolygon(nullptr);
	Delete<P>(polygon);
}

template<typename Traits>
void THEMesh<Traits>::RemoveEdge(HE* he0) {
	assert(he0 != nullptr);
	auto* he1 = he0->Pair();

	if (!he0->IsFree())
		RemovePolygon(he0->Polygon());
	if (!he1->IsFree())
		RemovePolygon(he1->Polygon());

	// init
	auto* v0 = he0->Origin();
	auto* v1 = he1->Origin();
	auto* inV0 = he0->Pre();
	auto* inV1 = he1->Pre();
	auto* outV0 = he0->RotateNext();
	auto* outV1 = he1->RotateNext();

	// [link off he0]
	if (v0->HalfEdge() == he0)
		v0->SetHalfEdge(outV0 == he0 ? nullptr : outV0);

	inV0->SetNext(outV0);

	// [link off he1]
	if (v1->HalfEdge() == he1)
		v1->SetHalfEdge(outV1 == he1 ? nullptr : outV1);

	inV1->SetNext(outV1);

	// delete
	Delete<HE>(he0);
	Delete<HE>(he1);
}

template<typename Traits>
void THEMesh<Traits>::RemoveVertex(V* v) {
	assert(v != nullptr);
	for (auto* he : v->AdjHalfEdges())
		RemoveEdge(he);
	Delete<V>(v);
}

template<typename Traits>
bool THEMesh<Traits>::Init(const std::vector<std::vector<size_t>>& polygons) {
	assert(!polygons.empty());

	Clear();

	size_t max = 0;
	size_t min = std::numeric_limits<size_t>::max();
	for (const auto& polygon : polygons) {
		assert(polygon.size() > 2);

		for (auto idx : polygon) {
			if (idx > max)
				max = idx;
			if (idx < min)
				min = idx;
		}
	}

	assert(min == 0);

	// No Instantiate
	for (size_t i = 0; i <= max; i++)
		New<V>();

	for (const auto& polygon : polygons) {
		std::vector<HE*> heLoop;
		for (size_t i = 0; i < polygon.size(); i++) {
			size_t next = (i + 1) % polygon.size();
			assert(polygon[i] != polygon[next]);
			auto* u = vertices[polygon[i]];
			auto* v = vertices[polygon[next]];
			auto* he = u->HalfEdgeTo(v);
			if (!he)
				he = AddEdge(u, v);
			heLoop.push_back(he);
		}
		auto* p = AddPolygon(heLoop);
		assert(p != nullptr);
	}

	return true;
}

template<typename Traits>
bool THEMesh<Traits>::Init(const std::vector<size_t>& polygons, size_t sides) {
	assert(polygons.size() % sides == 0);
	std::vector<std::vector<size_t>> arrangedPolygons;
	for (size_t i = 0; i < polygons.size(); i += sides) {
		std::vector<size_t> indices;
		for (size_t j = 0; j < sides; j++)
			indices.push_back(polygons[i + j]);
		arrangedPolygons.push_back(std::move(indices));
	}
	return Init(arrangedPolygons);
}

template<typename Traits>
std::vector<std::vector<size_t>> THEMesh<Traits>::Export() const {
	if (!IsValid())
		return {};

	std::vector<std::vector<size_t>> rst;
	for (auto* polygon : polygons.vec()) {
		std::vector<size_t> indices;
		for (auto* v : polygon->AdjVertices())
			indices.push_back(Index(v));
		rst.push_back(std::move(indices));
	}
	return rst;
}

template<typename Traits>
void THEMesh<Traits>::Clear() noexcept {
	if constexpr (std::is_trivially_destructible_v<V>)
		poolV.FastClear();
	else {
		for (auto* v : vertices.vec())
			poolV.Recycle(v);
	}
	vertices.clear();

	if constexpr (std::is_trivially_destructible_v<HE>)
		poolHE.FastClear();
	else {
		for (auto* he : halfEdges.vec())
			poolHE.Recycle(he);
	}
	halfEdges.clear();

	if constexpr (std::is_trivially_destructible_v<P>)
		poolP.FastClear();
	else {
		for (auto* p : polygons.vec())
			poolP.Recycle(p);
	}
	polygons.clear();
}

template<typename Traits>
void THEMesh<Traits>::Reserve(size_t n) {
	vertices.reserve(n);
	poolV.Reserve(n);

	halfEdges.reserve(6 * n);
	poolHE.Reserve(6 * n);

	polygons.reserve(2 * n);
	poolP.Reserve(2 * n);
}

template<typename Traits>
bool THEMesh<Traits>::HasBoundary() const noexcept {
	for (auto* he : halfEdges) {
		if (he->IsOnBoundary())
			return true;
	}
	return false;
}

template<typename Traits>
std::vector<HEMeshTraits_HE<Traits>*> THEMesh<Traits>::Boundaries() const {
	std::vector<HE*> boundaries;
	std::unordered_set<HE*> found;
	for (auto* he : halfEdges) {
		if (he->IsOnBoundary() && found.find(he) == found.end()) {
			auto* cur = he;
			do {
				found.insert(cur);
				cur = cur->Next();
			} while (cur != he);
			boundaries.push_back(he);
		}
	}
	return boundaries;
}

template<typename Traits>
bool THEMesh<Traits>::IsValid() const {
	for (auto* he : halfEdges) {
		if (!he->Next() || !he->Pair() || !he->Origin())
			return false;
	}

	std::unordered_set<HE*> uncheckHalfEdges(halfEdges.begin(), halfEdges.end());
	HE* headHE = nullptr;
	HE* curHE = nullptr;
	while (!uncheckHalfEdges.empty() || headHE != nullptr) {
		if (!headHE) {
			auto iter = uncheckHalfEdges.begin();
			headHE = curHE = *iter;
			uncheckHalfEdges.erase(iter);
		}
		curHE = curHE->Next();
		if (curHE == headHE)
			headHE = nullptr;
		else {
			auto target = uncheckHalfEdges.find(curHE);
			if (target == uncheckHalfEdges.end())
				return false;
			uncheckHalfEdges.erase(target);
		}
	}

	for (auto* he : halfEdges) {
		if (he->Pair()->Pair() != he)
			return false;
	}

	for (auto* v : vertices) {
		for (auto* he : v->OutHalfEdges()) {
			if (he->Origin() != v)
				return false;
		}
	}

	for (auto* p : polygons) {
		if (!p->HalfEdge())
			return false;
		for (auto* he : p->AdjHalfEdges()) {
			if (he->Polygon() != p)
				return false;
		}
	}

	return true;
}

template<typename Traits>
bool THEMesh<Traits>::IsTriMesh() const {
	if (!IsValid())
		return false;

	for (auto* poly : polygons) {
		if (poly->Degree() != 3)
			return false;
	}

	return true;
}

template<typename Traits>
const HEMeshTraits_P<Traits>* THEMesh<Traits>::EraseVertex(V* v) {
	if (v->IsOnBoundary()) {
		RemoveVertex(v);
		return nullptr;
	}

	auto* he = v->HalfEdge();
	while (he->Next()->End() == v) {
		he = he->RotateNext();
		if (he == v->HalfEdge()) {
			RemoveVertex(v);
			return nullptr;
		}
	}
	he = he->Next();

	RemoveVertex(v);
	return AddPolygon(he->NextLoop());
}

// 一条边2个顶点变成两条边3个顶点
template<typename Traits>
template<typename... Args>
HEMeshTraits_V<Traits>* THEMesh<Traits>::AddEdgeVertex(HE* he01, Args&&... args) {
	// prepare
	auto* he10 = he01->Pair();

	auto* v0 = he01->Origin();
	auto* v1 = he10->Origin();

	auto* v0d = v0->Degree();
	auto* v1d = v1->Degree();

	auto* p01 = he01->Polygon();
	auto* p10 = he10->Polygon();

	auto* he02 = New<HE>();
	auto* he21 = New<HE>();
	auto* he12 = New<HE>();
	auto* he20 = New<HE>();

	auto* v2 = New<V>(std::forward<Args>(args)...);

	// basic set
	if (v0->HalfEdge() == he01)
		v0->SetHalfEdge(he02);
	if (v1->HalfEdge() == he10)
		v1->SetHalfEdge(he12);
	v2->SetHalfEdge(he21);

	he02->SetNext(he21);
	he02->SetPair(he20);
	he02->SetOrigin(v0);
	he02->SetPolygon(p01);

	//he21->SetNext();
	he21->SetPair(he12);
	he21->SetOrigin(v2);
	he21->SetPolygon(p01);

	he12->SetNext(he20);
	he12->SetPair(he21);
	he12->SetOrigin(v1);
	he12->SetPolygon(p10);

	//he20->SetNext();
	he20->SetPair(he02);
	he20->SetOrigin(v2);
	he20->SetPolygon(p10);

	if (!P::IsBoundary(p01) && p01->HalfEdge() == he01)
		p01->SetHalfEdge(he02);
	if (!P::IsBoundary(p10) && p10->HalfEdge() == he10)
		p10->SetHalfEdge(he12);

	// 4 case
	if (v0d == 1 && v1d == 1) {
		he21->SetNext(he12);
		he20->SetNext(he02);
	}
	else if (v0d == 1) {
		auto* he01Next = he01->Next();
		auto* he10Pre = he10->Pre();

		he21->SetNext(he01Next);
		he20->SetNext(he02);
		he10Pre->SetNext(he12);
	}
	else if (v1d == 1) {
		auto* he01Pre = he01->Pre();
		auto* he10Next = he10->Next();

		he01Pre->SetNext(he02);
		he21->SetNext(he12);
		he20->SetNext(he10Next);
	}
	else {
		auto* he01Pre = he01->Pre();
		auto* he01Next = he01->Next();
		auto* he10Pre = he10->Pre();
		auto* he10Next = he10->Next();

		he01Pre->SetNext(he02);
		he10Pre->SetNext(he12);
		he21->SetNext(he01Next);
		he20->SetNext(he10Next);
	}

	// delete
	Delete<HE>(he01);
	Delete<HE>(he10);

	return v2;
}

// 划分多一条边, 比如矩形, 中间加一条对角线, 分割为两个三角形
template<typename Traits>
template<typename... Args>
HEMeshTraits_HE<Traits>* THEMesh<Traits>::ConnectVertex(HE* he0, HE* he1, Args&&... args) {
	assert(he0->Polygon() == he1->Polygon());

	auto* p = he0->Polygon();

	assert(!P::IsBoundary(p));

	auto* v0 = he0->Origin();
	auto* v1 = he1->Origin();

	assert(v0 != v1 && !V::IsConnected(v0, v1));

	RemovePolygon(p);

	auto* he0Pre = he0->Pre();
	auto* he1Pre = he1->Pre();

	auto* he0Loop = he0->NextTo(he1);
	auto* he1Loop = he1->NextTo(he0);

	auto* he01 = New<HE>();
	auto* he10 = New<HE>();

	he01->SetNext(he1);
	he01->SetPair(he10);
	he01->SetOrigin(v0);

	he10->SetNext(he0);
	he10->SetPair(he01);
	he10->SetOrigin(v1);

	he0Pre->SetNext(he01);
	he1Pre->SetNext(he10);

	he0Loop.push_back(he10);
	he1Loop.push_back(he01);

	AddPolygon(he0Loop);
	AddPolygon(he1Loop);

	return he01;
}

template<typename Traits>
bool THEMesh<Traits>::FlipEdge(HE* he01) {
	assert(he01 != nullptr && !he01->IsOnBoundary());

	// 1. prepare
	auto* he10 = he01->Pair();
	auto* he02 = he10->Next();
	auto* he13 = he01->Next();
	auto* he01Pre = he01->Pre();
	auto* he10Pre = he10->Pre();
	auto* he02Next = he02->Next();
	auto* he13Next = he13->Next();

	auto* p01 = he01->Polygon();
	auto* p10 = he10->Polygon();

	auto* v0 = he01->Origin();
	auto* v1 = he01->End();
	auto* v2 = he02->End();
	auto* v3 = he13->End();

	// 2. change
	if (v0->HalfEdge() == he01)
		v0->SetHalfEdge(he02);
	if (v1->HalfEdge() == he10)
		v1->SetHalfEdge(he13);

	auto* he23 = he01;
	auto* he32 = he10;

	he01Pre->SetNext(he02);

	he02->SetNext(he23);
	he02->SetPolygon(p01);

	he23->SetOrigin(v2);
	he32->SetOrigin(v3);

	he23->SetNext(he13Next);

	he10Pre->SetNext(he13);

	he13->SetNext(he32);
	he13->SetPolygon(p10);

	he32->SetNext(he02Next);

	if (p01->HalfEdge() == he13)
		p01->SetHalfEdge(he02);
	if (p10->HalfEdge() == he02)
		p10->SetHalfEdge(he13);

	return true;
}

template<typename Traits>
template<typename... Args>
HEMeshTraits_V<Traits>* THEMesh<Traits>::SplitEdge(HE* he01, Args&&... args) {
	assert(he01 != nullptr);

	auto* he10 = he01->Pair();

	assert((!he01->IsOnBoundary() || !he10->IsOnBoundary())
		&& "two side of edge are boundaries");

	if (he01->IsOnBoundary() || he10->IsOnBoundary()) {
		if (he01->IsOnBoundary())
			std::swap(he01, he10);

		auto* p01 = he01->Polygon();

		assert(p01->Degree() == 3);

		/*
		*     v1         v1
		*    / |        / |
		*   /  |       /  |
		* v2   | ==> v2--v3
		*   \  |       \  |
		*    \ |        \ |
		*     v0         v0
		*/

		// prepare
		auto* he12 = he01->Next();
		auto* he20 = he12->Next();
		auto* he10Next = he10->Next();
		auto* v0 = he01->Origin();
		auto* v1 = he10->Origin();
		auto* v2 = he20->Origin();

		// old to new
		auto* he03 = he01;
		auto* he13 = he10;
		auto* p03 = p01;

		// new
		auto* v3 = New<V>(std::forward<Args>(args)...);
		auto* he30 = New<HE>();
		auto* he31 = New<HE>();
		auto* he32 = New<HE>();
		auto* he23 = New<HE>();
		auto* p31 = New<P>();

		// set
		v3->SetHalfEdge(he31);

		he12->SetNext(he23);
		he12->SetPolygon(p31);
		he03->Init(he32, he30, v0, p03);
		he13->Init(he30, he31, v1, nullptr);
		he30->Init(he10Next, he03, v3, nullptr);
		he31->Init(he12, he13, v3, p31);
		he32->Init(he20, he23, v3, p03);
		he23->Init(he31, he32, v2, p31);

		if (p03->HalfEdge() == he12)
			p03->SetHalfEdge(he03);
		p31->SetHalfEdge(he31);

		return v3;
	}

	auto* p01 = he01->Polygon();
	auto* p10 = he10->Polygon();

	assert(p01->Degree() == 3 && p10->Degree() == 3);

	/*
	*     v1             v1
	*    /||\           /||\
	*   / || \         / || \
	* v2  ||  v3 ==> v2--v4--v3
	*   \ || /         \ || /
	*    \||/           \||/
	*     v0             v0
	*/

	// prepare
	auto* he12 = he01->Next();
	auto* he20 = he12->Next();
	auto* he03 = he10->Next();
	auto* he31 = he03->Next();
	auto* v0 = he01->Origin();
	auto* v1 = he10->Origin();
	auto* v2 = he20->Origin();
	auto* v3 = he31->Origin();

	// old to new
	auto* he04 = he01;
	auto* he14 = he10;
	auto* p04 = p01;
	auto* p14 = p10;

	// new
	auto* v4 = New<V>(std::forward<Args>(args)...);
	auto* he40 = New<HE>();
	auto* he41 = New<HE>();
	auto* he42 = New<HE>();
	auto* he24 = New<HE>();
	auto* he43 = New<HE>();
	auto* he34 = New<HE>();
	auto* p41 = New<P>();
	auto* p40 = New<P>();

	// set
	v4->SetHalfEdge(he41);

	he12->SetNext(he24);
	he12->SetPolygon(p41);
	he03->SetNext(he34);
	he03->SetPolygon(p40);
	he04->Init(he42, he40, v0, p04);
	he14->Init(he43, he41, v1, p14);
	he40->Init(he03, he04, v4, p40);
	he41->Init(he12, he14, v4, p41);
	he42->Init(he20, he24, v4, p04);
	he24->Init(he41, he42, v2, p41);
	he43->Init(he31, he34, v4, p14);
	he34->Init(he40, he43, v3, p40);

	if (p04->HalfEdge() == he12)
		p04->SetHalfEdge(he04);
	if (p14->HalfEdge() == he03)
		p14->SetHalfEdge(he14);
	p41->SetHalfEdge(he41);
	p40->SetHalfEdge(he40);

	return v4;
}

template<typename Traits>
bool THEMesh<Traits>::IsCollapsable(HE* he01) const {
	assert(he01 != nullptr);

	auto* he10 = he01->Pair();

	auto* v0 = he01->Origin();
	auto* v1 = he01->End();

	size_t p01D = he01->NextLoop().size();
	size_t p10D = he10->NextLoop().size();

	std::vector<V*> comVs;
	auto v0AdjVs = v0->AdjVertices();
	auto v1AdjVs = v1->AdjVertices();
	sort(v0AdjVs.begin(), v0AdjVs.end());
	sort(v1AdjVs.begin(), v1AdjVs.end());
	std::set_intersection(v0AdjVs.begin(), v0AdjVs.end(), v1AdjVs.begin(), v1AdjVs.end(),
		std::insert_iterator<std::vector<V*>>(comVs, comVs.begin()));

	size_t limit = 2;
	if (p01D > 3)
		limit -= 1;
	if (p10D > 3)
		limit -= 1;
	if (comVs.size() > limit)
		return false;

	if (v0->IsOnBoundary() && v1->IsOnBoundary() && !he01->IsOnBoundary())
		return false;

	return true;
}

template<typename Traits>
template<typename... Args>
HEMeshTraits_V<Traits>* THEMesh<Traits>::CollapseEdge(HE* e, Args&&... args) {
	assert(IsCollapsable(e) && "use IsCollapsable before CollapseEdge");

	auto* he01 = e->HalfEdge();
	auto* he10 = he01->Pair();

	auto* v0 = he01->Origin();
	auto* v1 = he01->End();

	auto* p01 = he01->Polygon();
	auto* p10 = he10->Polygon();
	size_t p01D = he01->NextLoop().size();
	size_t p10D = he10->NextLoop().size();

	if (v0->IsOnBoundary() && v1->IsOnBoundary() && !e->IsOnBoundary())
		return nullptr;

	if (v0->Degree() == 1) {
		EraseVertex(v0);
		return v1;
	}

	if (v1->Degree() == 1) {
		EraseVertex(v1);
		return v0;
	}

	// set v
	auto* v = New<V>(std::forward<Args>(args)...);
	if (he01->Pre()->Pair()->Polygon() != p10)
		v->SetHalfEdge(he01->Pre()->Pair());
	else
		v->SetHalfEdge(he10->Pre()->Pair());

	for (auto* he : v0->OutHalfEdges())
		he->SetOrigin(v);
	for (auto* he : v1->OutHalfEdges())
		he->SetOrigin(v);

	if (p01D == 3) { // p01->Degree() == 3
		auto* he01Next = he01->Next();
		auto* he01Pre = he01->Pre();
		auto* he01NextPair = he01Next->Pair();
		auto* he01PrePair = he01Pre->Pair();

		auto* v2 = he01Pre->Origin();
		if (v2->HalfEdge() == he01Pre)
			v2->SetHalfEdge(he01NextPair);

		he01NextPair->SetPair(he01PrePair);
		he01PrePair->SetPair(he01NextPair);

		Delete<HE>(he01Next);
		Delete<HE>(he01Pre);
		if (!P::IsBoundary(p01))
			Delete<P>(p01);
	}
	else { //p01->Degree >= 4
		if (!P::IsBoundary(p01) && p01->HalfEdge() == he01)
			p01->SetHalfEdge(he01->Next());
		he01->Pre()->SetNext(he01->Next());
	}

	if (p10D == 3) { // p10->Degree() == 3
		auto* he10Next = he10->Next();
		auto* he10Pre = he10->Pre();
		auto* he10NextPair = he10Next->Pair();
		auto* he10PrePair = he10Pre->Pair();

		auto* v2 = he10Pre->Origin();
		if (v2->HalfEdge() == he10Pre)
			v2->SetHalfEdge(he10NextPair);

		he10NextPair->SetPair(he10PrePair);
		he10PrePair->SetPair(he10NextPair);

		Delete<HE>(he10Next);
		Delete<HE>(he10Pre);
		if (!P::IsBoundary(p10))
			Delete<P>(p10);
	}
	else { //p10->Degree >= 4
		if (!P::IsBoundary(p10) && p10->HalfEdge() == he10)
			p10->SetHalfEdge(he10->Next());
		he10->Pre()->SetNext(he10->Next());
	}

	Delete<V>(v0);
	Delete<V>(v1);
	Delete<HE>(he01);
	Delete<HE>(he10);

	return v;
}