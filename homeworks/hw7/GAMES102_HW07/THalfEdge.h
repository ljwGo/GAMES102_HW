#pragma once

#include "details/ForwardDecl.h"
#include <vector>

template <typename Traits>
class THalfEdge {
public:
	using V = typename HEMeshTraits_V<Traits>;
	using HE = typename  HEMeshTraits_HE<Traits>;
	using P = typename  HEMeshTraits_P<Traits>;

public:
	HE* Next() noexcept { return next; }
	HE* Pair() noexcept { return pair; }
	V* Origin() noexcept { return origin; }
	P* Polygon() noexcept { return polygon; }
	V* End() noexcept { return Next()->Origin(); }
	HE* Pre() noexcept;
	HE* RotateNext() noexcept { return Pair()->Next(); }
	HE* RotatePre() noexcept { return Pre()->Pair(); }

	const HE* Next() const noexcept { return const_cast<HE*>(This())->Next(); }
	const HE* Pair() const noexcept { return const_cast<HE*>(This())->Pair(); }
	const V* Origin() const noexcept { return const_cast<HE*>(This())->Origin(); }
	const P* Polygon() const noexcept { return const_cast<HE*>(This())->Polygon(); }
	const V* End() const noexcept { return const_cast<HE*>(This())->End(); }
	const HE* Pre() const noexcept { return const_cast<HE*>(This())->Pre(); }
	const HE* RotateNext() const noexcept { return const_cast<HE*>(This())->RotateNext(); }
	const HE* RotatePre() const noexcept { return const_cast<HE*>(This())->RotatePre(); }

	void SetNext(HE* he) noexcept { next = he; }
	void SetPair(HE* he) noexcept { pair = he; }
	void SetOrigin(V* v) noexcept { origin = v; }
	void SetPolygon(P* p) noexcept { polygon = p; }

	// next, pair, v, e can't be nullptr
	// p can be nullptr
	void Init(HE* next, HE* pair, V* v, P* p) noexcept;

	// polygon == nullptr
	bool IsOnBoundary() const noexcept { return !polygon; }

	// [begin, end), if begin == end, return a loop
	static std::vector<HE*> NextBetween(HE* begin, HE* end);

	// [this, end), NextBetween(this, end);
	std::vector<HE*> NextTo(HE* end) { return NextBetween(This(), end); }

	// NextBetween(this, this), a loop from this to this
	std::vector<HE*> NextLoop() { return NextTo(This()); }

	// [begin, end), if begin == end, return a loop
	static std::vector<HE*> RotateNextBetween(HE* begin, HE* end);

	// [this, end), RotateNextBetween(this, end);
	std::vector<HE*> RotateNextTo(HE* end) { return RotateNextBetween(This(), end); }

	// RotateNextBetween(this, this), a loop from this to this
	std::vector<HE*> RotateNextLoop() { return RotateNextTo(This()); }

private:
	friend THEMesh<Traits>;
	friend TVertex<Traits>;

	bool IsFree() const noexcept { return !polygon; }
	static bool MakeAdjacent(HE* inHE, HE* outHE);
	static HE* FindFreeIncident(HE* begin, HE* end);

	HE* This() noexcept { return static_cast<HE*>(this); }
	const HE* This() const noexcept { return static_cast<const HE*>(this); }

	HE* next{ nullptr };
	HE* pair{ nullptr };

	V* origin{ nullptr };
	P* polygon{ nullptr };
};

#include "details/THalfEdge.inl"