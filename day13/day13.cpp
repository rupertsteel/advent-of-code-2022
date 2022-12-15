
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
#include <variant>


#include <fmt/ranges.h>
#include <fmt/chrono.h>
#include <fmt/std.h>

#include <gsl/gsl-lite.hpp>

using namespace std::string_view_literals;

struct ListItem {
	std::variant<int, std::vector<ListItem>> item;
};

template<> struct fmt::formatter<ListItem> {
	constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
		// [ctx.begin(), ctx.end()) is a character range that contains a part of
		// the format string starting from the format specifications to be parsed,
		// e.g. in
		//
		//   fmt::format("{:f} - point of interest", point{1, 2});
		//
		// the range will contain "f} - point of interest". The formatter should
		// parse specifiers until '}' or the end of the range. In this example
		// the formatter should parse the 'f' specifier and return an iterator
		// pointing to '}'.

		// Please also note that this character range may be empty, in case of
		// the "{}" format string, so therefore you should check ctx.begin()
		// for equality with ctx.end().

		// Parse the presentation format and store it in the formatter:
		auto it = ctx.begin(), end = ctx.end();

		// Check if reached the end of the range:
		if (it != end && *it != '}') throw format_error("invalid format");

		// Return an iterator past the end of the parsed range:
		return it;
	}

	// Formats the point p using the parsed format specification (presentation)
	// stored in this formatter.
	template <typename FormatContext>
	auto format(const ListItem& m, FormatContext& ctx) const -> decltype(ctx.out()) {
		// ctx.out() is an output iterator to write to.

		return fmt::format_to(ctx.out(), "{}", m.item);
	}
};

ListItem parseListInternal(std::string_view& sv) {
	std::vector<ListItem> item;

	while (!sv.empty()) {
		if (sv.front() == '[') {
			// start list
			// parse nested

			sv = sv.substr(1);
			auto internalList = parseListInternal(sv);

			item.push_back(internalList);
		} else if (sv.front() == ']') {
			// end list
			sv = sv.substr(1);
			return ListItem{ item };
		} else if (sv.front() == ',') {
			sv = sv.substr(1);
		} else {
			auto numLength = std::min({ sv.size(), sv.find(','), sv.find(']') });

			auto numSv = sv.substr(0, numLength);
			std::string numStr{ numSv };
			auto num = std::stoi(numStr);

			item.push_back(ListItem{ num });

			sv = sv.substr(numLength);
		}
	}

	return ListItem{ item };
}

ListItem parseList(std::string_view sv) {
	return parseListInternal(sv);
}

std::strong_ordering operator<=>(const ListItem& left, const ListItem& right) {
	if (left.item.index() == 0 && right.item.index() == 0) {
		// both items are ints, compare direct
		return std::get<int>(left.item) <=> std::get<int>(right.item);
	} else if (left.item.index() == 1 && right.item.index() == 1) {
		// both items are lists, compare item by item
		auto& leftList = std::get<1>(left.item);
		auto& rightList = std::get<1>(right.item);

		int minSize = std::min(leftList.size(), rightList.size());

		// we have the same number of items, compare item by item
		for (int i = 0; i < minSize; i++) {
			auto cmp = leftList[i] <=> rightList[i];

			if (cmp != std::strong_ordering::equal) {
				return cmp;
			}
		}

		// out of elements,
		return leftList.size() <=> rightList.size();
	}

	if (left.item.index() == 0) {
		// pack item into a list, and re do
		std::vector<ListItem> packList;
		packList.push_back(left);

		ListItem packedItem{ packList };

		return packedItem <=> right;
	}

	// pack item into a list, and re do
	std::vector<ListItem> packList;
	packList.push_back(right);

	ListItem packedItem{ packList };

	return left <=> packedItem;
}


int main(int argc, char* argv[]) {
	std::ifstream inputFile("inputs/day13.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto start = std::chrono::high_resolution_clock::now();

	auto inputIntoLines = std::string_view{ input } | std::views::split("\n"sv) | std::views::transform([](auto rng) { return std::string_view(rng.begin(), rng.end()); });

	auto pairs = inputIntoLines | std::views::chunk_by([](auto left, auto right) {
		return !left.empty();
	}) | std::views::transform([](auto rng) {
		return rng | std::views::filter([](auto str) { return !str.empty(); });
	});

	int pairNumber = 1;

	int lessPairNumberSum = 0;

	for (auto pair : pairs) {
		

		auto leftList = parseList(pair.front());
		auto rightList = parseList(*std::ranges::next(pair.begin()));

		//fmt::print("Left:  {}\nRight: {}\n", leftList, rightList);

		auto cmp = leftList <=> rightList;

		if (cmp == std::strong_ordering::less) {
			lessPairNumberSum += pairNumber;
		}

		//fmt::print("Pair {}: {}: {}\n", pairNumber, pair, cmp._Value);

		pairNumber++;
	}
	

	
	auto end = std::chrono::high_resolution_clock::now();

	fmt::print("Pair number sum: {}\n", lessPairNumberSum);

	auto dur = end - start;
	fmt::print("Took {}\n", dur);

	return 0;
}