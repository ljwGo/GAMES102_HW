#pragma once

#include <type_traits>
#include <UContainer/Pool.h>
#include <assert.h>

#include "UHEMesh/detail/random_set.h"
#include "MyIComparable.h"

using namespace Ubpa;

template <typename T>
class SmallRootHeap {

	// 更好的解决办法https://zplutor.github.io/2020/11/21/how-to-check-if-a-type-is-comparable/
	bool static constexpr IsVaild() {
		return std::is_base_of_v<MyIComparable<T>, T>;
	}

	static_assert(IsVaild());

public:
	void Push(T* val) {
		if (root == nullptr) {
			root = val;
			nodeSets.insert(val);
		}
		else {
			nodeSets.insert(val);
			LocalUpUpdateT(val);
		}
	};
	T* Root() { return nodeSets.size() > 0 ? nodeSets.at(0) : nullptr; };
	T* PopRoot() {
		const std::vector<T*>& vec = nodeSets.vec();
		if (nodeSets.size() == 0) return nullptr;
		else if (nodeSets.size() == 1) {
			T* val = vec[0];
			root = nullptr;
			nodeSets.clear();
			return val;
		}
		else {
			T* val = vec[0];
			// earse的操作,删除当前点,并把结尾添加到删除位置. 这和小根堆一次自顶向下相同
			nodeSets.erase(val);
			LocalDownUpdate(0);
			assert(val != vec[0]);
			root = vec[0];
			return val;
		}
	};
	void Update(T* val) {
		LocalUpUpdate(val);
		LocalDownUpdate(val);
	};
	void Erase(T* val) {
		size_t ix = nodeSets.idx(val);
		if (ix >= nodeSets.size()) return;
		nodeSets.erase(val);
		LocalDownUpdate(ix);
	}
	void Clear() {
		nodeSets.clear();
		root = nullptr;
	}

private:
	void LocalUpUpdateT(T* node) {
		int ix = GetIx(node);
		LocalUpUpdate(ix);
	};
	void LocalUpUpdate(size_t ix) {
		assert(ix != -1);
		if (ix >= nodeSets.size()) return;

		int parentIx = GetParentIx(ix);
		T* node = nodeSets.vec()[ix];

		while (parentIx != -1) {
			T* pNode = GetNode(parentIx);

			if (*pNode < *node) {
				break;
			}

			// Swap two node.
			std::swap(pNode, node);

			ix = parentIx;
			parentIx = GetParentIx(ix);
		}
	}
	void LocalDownUpdateT(T* node) {
		int ix = GetIx(node);
		LocalDownUpdate(ix);
	};
	void LocalDownUpdate(size_t ix) {
		assert(ix != -1);
		if (ix >= nodeSets.size()) return;

		T* node = nodeSets.vec()[ix];
		int leftIx = GetLeftChildIx(ix);
		int rightIx = GetRightChildIx(ix);

		while (rightIx != -1) {

			T* lNode = GetNode(leftIx);
			T* rNode = GetNode(rightIx);

			T* minNode = node;
			minNode = *lNode < *minNode ? lNode : minNode;
			minNode = *rNode < *minNode ? rNode : minNode;

			if (minNode != node) {
				std::swap(node, minNode);
				ix = minNode == lNode ? leftIx : rightIx;
				leftIx = GetLeftChildIx(ix);
				rightIx = GetRightChildIx(ix);
			}
			else {
				break;
			}
		}

		if (leftIx != -1 && rightIx == -1) {
			T* lNode = GetNode(leftIx);
			if (*lNode < *node) std::swap(node, lNode);;
		}
	}
	int GetIx(T* val) { return val != nullptr ? nodeSets.idx(val) : -1; }
	int GetParentIx(size_t ix) { return ix > 0 ? (ix - 1) >> 2 : -1; }
	int GetLeftChildIx(size_t ix) { 
		size_t leftIx = ix * 2 + 1;
		return leftIx >= nodeSets.size() ? -1 : leftIx;
	};
	int GetRightChildIx(size_t ix) {
		size_t rightIx = ix * 2 + 2;
		return rightIx >= nodeSets.size() ? -1 : rightIx;
	};
	
	T* GetParent(T* node) {
		size_t ix = nodeSets.idx(node);
		int parentIx = GetParentIx(ix);
		
		if (parentIx != -1) {
			return nodeSets.at(parentIx);
		}
		return nullptr;
	}
	T* GetLeftChild(T* node) {
		size_t ix = nodeSets.idx(node);
		int leftIx = GetLeftChildIx(ix);

		if (leftIx != -1) {
			return nodeSets.at(leftIx);
		}
		return nullptr;
	}
	T* GetRightChild(T* node) {
		size_t ix = nodeSets.idx(node);
		int rightIx = GetRightChildIx(ix);

		if (rightIx != -1) {
			return nodeSets.at(rightIx);
		}
		return nullptr;
	}
	
	T* GetParent(size_t ix) {
		int parentIx = GetParentIx(ix);

		if (parentIx != -1) {
			return nodeSets.at(parentIx);
		}
		return nullptr;
	}
	T* GetLeftChild(size_t ix) {
		int leftIx = GetLeftChildIx(ix);

		if (leftIx != -1) {
			return nodeSets.at(leftIx);
		}
		return nullptr;
	}
	T* GetRightChild(size_t ix) {
		int rightIx = GetRightChildIx(ix);

		if (rightIx != -1) {
			return nodeSets.at(rightIx);
		}
		return nullptr;
	}
	// {}中的分号必须要有
	T* GetNode(size_t ix) { return nodeSets.at(ix); };

private:
	T* root = nullptr;
	random_set<T*> nodeSets;
};