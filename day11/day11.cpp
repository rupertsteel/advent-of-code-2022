
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
#include <functional>


#include <fmt/ranges.h>
#include <fmt/chrono.h>

#include <gsl/gsl-lite.hpp>

using namespace std::string_view_literals;

struct Monkey {

	std::vector<uint64_t> items;

	// operation
	// new = old <OP> <old|CONSTANT>
	char op; // either '+' or '*'
	std::optional<int> rhs; // empty means old

	int divisor;
	int testTrueThrowMonkey;
	int testFalseThrowMonkey;
};

template<> struct fmt::formatter<Monkey> {
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
	auto format(const Monkey& m, FormatContext& ctx) const -> decltype(ctx.out()) {
		// ctx.out() is an output iterator to write to.

		fmt::format_to(ctx.out(), "Monkey\n");
		fmt::format_to(ctx.out(), "  Starting items: {}\n", m.items);
		fmt::format_to(ctx.out(), "  Operation: new = old {} {}\n", m.op, m.rhs.transform([](int a) {return std::to_string(a); }).value_or("rhs"));
		fmt::format_to(ctx.out(), "  Test: divisible by {}\n", m.divisor);
		fmt::format_to(ctx.out(), "    If true: throw to monkey {}\n", m.testTrueThrowMonkey);
		return fmt::format_to(ctx.out(), "    If false: throw to monkey {}\n", m.testFalseThrowMonkey);
	}
};

int main(int argc, char* argv[]) {
	std::ifstream inputFile("inputs/day11.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto start = std::chrono::high_resolution_clock::now();

	auto inputIntoLines = std::string_view{ input } | std::views::split("\n"sv) | std::views::transform([](auto rng) { return std::string_view(rng.begin(), rng.end()); }) | std::views::filter([](auto str) { return !str.empty(); });

	auto monkeyRangeLines = inputIntoLines | std::views::chunk_by([](auto, auto rightSv) {
		return rightSv[0] == ' ';
	});

	auto monkeysParsed = monkeyRangeLines | std::views::transform([](auto rng) {
		Monkey monkey;
		int id;

		for (auto line : rng) {
			if (line.starts_with("Monkey")) {
				id = line[7] - '0';
			} else if (line.starts_with("  Starting items")) {
				auto itemSv = line.substr(18);

				monkey.items = itemSv | std::views::split(", "sv) | std::views::transform([](auto rng) { return std::string_view(rng.begin(), rng.end()); }) | std::views::transform([](auto s) {return std::stoull(std::string(s)); }) | std::ranges::to<std::vector>();
			} else if (line.starts_with("  Operation")) {
				monkey.op = line[23];
				auto rhsSv = line.substr(25);
				if (std::ranges::all_of(rhsSv, isdigit)) {
					monkey.rhs = std::stoi(std::string{ rhsSv });
				}
			} else if (line.starts_with("  Test")) {
				monkey.divisor = std::stoi(std::string{ line.substr(21) });
			} else if (line.starts_with("    If true")) {
				monkey.testTrueThrowMonkey = line[29] - '0';
			} else if (line.starts_with("    If false")) {
				monkey.testFalseThrowMonkey = line[30] - '0';
			}
		}

		return std::make_pair(id, monkey);
	});

	std::map<int, Monkey> monkeys = monkeysParsed | std::ranges::to<std::map>();

	auto monkeyDivisors = monkeys | std::views::values | std::views::transform([](auto& m) {return m.divisor; }) | std::views::common;
	auto sanityModClamp = std::accumulate(monkeyDivisors.begin(), monkeyDivisors.end(), 1, std::multiplies<>());

	std::map<int, uint64_t> inspectionCounts;

#if 0 // part 1
	constexpr int rounds = 20;
#else
	constexpr int rounds = 10000;
#endif

	for (int i = 0; i < rounds; i++) {

		for (auto& [monkeyId, monkey] : monkeys) {
			while (!monkey.items.empty()) {
				inspectionCounts[monkeyId]++;

				auto item = monkey.items.front();
				monkey.items.erase(monkey.items.begin());

				if (monkey.op == '+') {
					item = item + monkey.rhs.value_or(item);
				}
				else {
					item = item * monkey.rhs.value_or(item);
				}

#if 0 // part 1
				item /= 3;
#else
				item %= sanityModClamp;
#endif
				if (item % monkey.divisor == 0) {
					monkeys.at(monkey.testTrueThrowMonkey).items.push_back(item);
				}
				else {
					monkeys.at(monkey.testFalseThrowMonkey).items.push_back(item);
				}
			}
		}

		//fmt::print("\n\n\nRound {} finished\n", i);
		//fmt::print("{}", monkeys);
	}
	
	auto end = std::chrono::high_resolution_clock::now();

	auto counts = inspectionCounts | std::views::values | std::ranges::to<std::vector>();
	std::ranges::sort(counts, std::ranges::greater());

	auto monkeyBusiness = counts[0] * counts[1];

	fmt::print("Inspection counts: {}\n", counts);
	fmt::print("Part 1, monkey business: {}\n", monkeyBusiness);


	auto dur = end - start;
	fmt::print("Took {}\n", dur);

	return 0;
}