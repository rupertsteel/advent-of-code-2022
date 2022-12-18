
#include <cstdint>

#include <ranges>

namespace core {

class TrueBitRangeSentinel;
class TrueBitRangeIterator;

class TrueBitRange {
public:
	explicit TrueBitRange(std::uint64_t bits) : bits(bits) {}

	TrueBitRange(const TrueBitRange& other) = default;
	TrueBitRange& operator=(const TrueBitRange&) = default;

	~TrueBitRange() = default;

	bool operator==(const TrueBitRange&) const = default;

	[[nodiscard]] TrueBitRangeIterator begin() const;
	[[nodiscard]] TrueBitRangeSentinel end() const;

	[[nodiscard]] size_t size() const;
private:
	std::uint64_t bits;
};



class TrueBitRangeIterator {
public:
	using difference_type = std::ptrdiff_t;
	using value_type = int;

	TrueBitRangeIterator() : bits(0) {}
	explicit TrueBitRangeIterator(std::uint64_t bits) : bits(bits) {}

	TrueBitRangeIterator(const TrueBitRangeIterator&) = default;
	TrueBitRangeIterator& operator=(const TrueBitRangeIterator&) = default;

	~TrueBitRangeIterator() = default;

	bool operator==(const TrueBitRangeSentinel&) const;
	bool operator==(const TrueBitRangeIterator&) const = default;
	int operator*() const;
	TrueBitRangeIterator& operator++();
	TrueBitRangeIterator operator++(int) {
		auto tmp = *this;
		++*this;
		return tmp;
	}

private:
	std::uint64_t bits;
};

class TrueBitRangeSentinel {
	
};





}
