#pragma once
#include <vector>
#include <array>
#include <cstdlib>

template<typename T, size_t BLOCK_SIZE = 1024>
class pool {
public:
	pool() noexcept = default;
	pool(pool&& pool) noexcept;
	pool& operator=(pool&& pool) noexcept;
	~pool();

	template<typename... Args>
	T* Request(Args&&... args);
	void Recycle(T* object);
	void Reserve(size_t n);
	// no ~T()
	void FastClear();
	void Clear();

private:
	void NewBlock();

private:
	std::vector<T*> blocks;
	std::vector<T*> freeAdresses;
};

#include "details/pool.inl"

