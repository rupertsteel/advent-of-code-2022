
#include <fmt/format.h>
#include <iostream>
#include <fstream>
#include <ranges>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>

#include <fmt/ranges.h>

using namespace std::string_view_literals;

auto positiveMod(auto a, auto b) {
	return (a % b + b) % b;
}

int main(int argc, char* argv[]) {
	std::ifstream inputFile("inputs/day3.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto inputIntoLines = std::string_view{ input } | std::views::split("\n"sv) | std::views::transform([](auto rng) { return std::string_view(rng.begin(), rng.end()); }) | std::views::filter([](auto str) { return !str.empty(); });

	fmt::print("Processed 1: {}\n", inputIntoLines);

	auto inputIntoRangePairs = inputIntoLines | std::views::transform([](std::string_view sv) {
		auto leftView = sv.substr(0, sv.length() / 2);
		auto rightView = sv.substr(sv.length() / 2, sv.length() / 2);

		std::string left{ leftView };
		std::string right{ rightView };

		std::ranges::sort(left);
		std::ranges::sort(right);

		return std::make_pair(left, right);
	});

	auto mismatchedCharacters = inputIntoRangePairs | std::views::transform([](const std::pair<std::string, std::string>& pair) {
		std::string mismatches;

		std::ranges::set_intersection(pair.first, pair.second, std::back_inserter(mismatches));

		return mismatches;
	});

	auto uniqueMismatches = mismatchedCharacters | std::views::transform([](const std::string& str) {
		std::string uniqueStr;
		std::ranges::unique_copy(str, std::back_inserter(uniqueStr));

		return uniqueStr;
	});

	auto mismatchesToValues = uniqueMismatches | std::views::transform([](const std::string& str) {
		auto values = str | std::views::transform([](char c) -> int {
			if (c >= 'a' && c <= 'z') {
				return c - 'a' + 1;
			}

			return c - 'A' + 27;
		});

		return std::accumulate(values.begin(), values.end(), 0);
	});

	auto mismatchedValues = mismatchesToValues; // | std::views::join;

	auto mismatchSum = std::accumulate(mismatchedValues.begin(), mismatchedValues.end(), 0);

	fmt::print("Part 1: {}", mismatchSum);


	//fmt::print("Processed 2: {}\n", inputIntoRangePairs);


	return 0;
}