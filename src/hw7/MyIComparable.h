#pragma once

template <typename T>
class MyIComparable
{
public:
	virtual bool operator==(const T& other) const = 0;
	virtual bool operator<(const T& other) const = 0;
};