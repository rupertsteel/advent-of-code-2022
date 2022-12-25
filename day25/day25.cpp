
#include <fmt/format.h>
#include <iostream>
#include <fstream>
#include <ranges>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <regex>
#include <map>
#include <set>
#include <deque>
#include <functional>
#include <queue>
#include <variant>
#include <thread>
#include <stack>
#include <list>

#include <fmt/ranges.h>
#include <fmt/chrono.h>
#include <fmt/std.h>

#include <gsl/gsl-lite.hpp>

#include <indicators/progress_bar.hpp>

#include <boost/container/flat_set.hpp>

using namespace std::string_view_literals;
using namespace std::string_literals;

struct Point {
	int x;
	int y;

	std::strong_ordering operator<=>(const Point&) const = default;
	bool operator==(const Point&) const = default;
};

ptrdiff_t fromSnafu(const std::string& line) {
	ptrdiff_t digitPower = 1;

	ptrdiff_t sum = 0;

	for (int i = line.size() - 1; i >= 0; i--) {
		auto ch = line[i];

		if (ch == '2') {
			sum += (2 * digitPower);
		} else if (ch == '1') {
			sum += (1 * digitPower);
		} else if (ch == '-') {
			sum -= (1 * digitPower);
		} else if (ch == '=') {
			sum -= (2 * digitPower);
		}

		digitPower *= 5;
	}

	return sum;
}

auto positiveMod(auto a, auto b) {
	return (a % b + b) % b;
}

std::string toSnafu(ptrdiff_t sum) {
	std::string str;

	bool needCarry = false;
	while (sum > 0) {
		auto [rem, digit] = std::div(sum + 2, 5ll);

		fmt::print("Sum: {}, rem: {}, digit: {}\n", sum, rem, digit);

		if (digit == 0) {
			str.push_back('=');
		} else if (digit == 1) {
			str.push_back('-');
		} else {
			str += std::to_string(digit - 2);
		}

		sum = rem;
	}

	if (needCarry) {
		//str.push_back('1');
	}

	std::ranges::reverse(str);

	return str;
}

int main(int argc, char* argv[]) try {
	std::ifstream inputFile("inputs/day25.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto start = std::chrono::high_resolution_clock::now();

	auto inputIntoLines = std::string_view{ input } | std::views::split("\n"sv) | std::views::transform([](auto rng) { return std::string(rng.begin(), rng.end()); }) | std::views::filter([](auto str) { return !str.empty(); });

	ptrdiff_t sum = 0;

	for (auto line : inputIntoLines) {
		auto normalNum = fromSnafu(line);
		sum += normalNum;
	}

	auto result = toSnafu(sum);

	auto end = std::chrono::high_resolution_clock::now();

	fmt::print("Part 1: {}\n", sum);
	fmt::print("SNAFU: {}\n", result);
	fmt::print("Back again: {}\n", fromSnafu(result));
	
	auto dur = end - start;

	auto durSecs = std::chrono::duration_cast<std::chrono::duration<float>>(dur);

	fmt::print("Took {}\n", dur);
	fmt::print("Or {}\n", durSecs);

	return 0;
} catch (std::exception& e) {
	fmt::print("Exception {}\n", e.what());
}
