
#include "catch2/catch_test_macros.hpp"

auto positiveMod(auto a, auto b) {
	return (a % b + b) % b;
}

TEST_CASE("Day 2 part 2, change hand", "") {
	auto func = [](char leftIn, char rightIn) {
		auto left = leftIn - 'A' + 1;

		auto rightChange = rightIn - 'X' - 1;

		return positiveMod(left - 1 + rightChange, 3) + 1;
	};

	CHECK(func('A', 'X') == 3);
	CHECK(func('B', 'X') == 1);
	CHECK(func('C', 'X') == 2);

	CHECK(func('A', 'Y') == 1);
	CHECK(func('B', 'Y') == 2);
	CHECK(func('C', 'Y') == 3);

	CHECK(func('A', 'Z') == 2);
	CHECK(func('B', 'Z') == 3);
	CHECK(func('C', 'Z') == 1);
}
