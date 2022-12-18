
#include "core.hpp"

#include <bit>

static_assert(std::ranges::forward_range<core::TrueBitRange>);

core::TrueBitRangeIterator core::TrueBitRange::begin() const {
	return TrueBitRangeIterator{ bits };
}

core::TrueBitRangeSentinel core::TrueBitRange::end() const {
	return TrueBitRangeSentinel{};
}

size_t core::TrueBitRange::size() const {
	return std::popcount(bits);
}

bool core::TrueBitRangeIterator::operator==(const TrueBitRangeSentinel&) const {
	return bits == 0;
}

int core::TrueBitRangeIterator::operator*() const {
	return std::countr_zero(bits);
}

core::TrueBitRangeIterator& core::TrueBitRangeIterator::operator++() {
	bits ^= 1ull << std::countr_zero(bits);

	return *this;
}
