
#include <fmt/format.h>
#include <iostream>
#include <fstream>
#include <ranges>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>
#include <chrono>

#include <fmt/ranges.h>
#include <fmt/chrono.h>

using namespace std::string_view_literals;

auto positiveMod(auto a, auto b) {
	return (a % b + b) % b;
}

int main(int argc, char* argv[]) {
	std::ifstream inputFile("inputs/day3.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto start = std::chrono::high_resolution_clock::now();

	auto inputIntoLines = std::string_view{ input } | std::views::split("\n"sv) | std::views::transform([](auto rng) { return std::string_view(rng.begin(), rng.end()); }) | std::views::filter([](auto str) { return !str.empty(); });

	//fmt::print("Processed 1: {}\n", inputIntoLines);

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


	//fmt::print("Processed 2: {}\n", inputIntoRangePairs);

	auto packsSortedUnique = inputIntoLines | std::views::transform([](std::string_view sv) {
		std::string str{ sv };
		std::ranges::sort(str);
		auto res = std::ranges::unique(str);
		str.erase(res.begin(), res.end());

		return str;
	});

	auto group3 = packsSortedUnique | std::views::chunk(3);

	auto commonItem = group3 | std::views::transform([](auto rng) -> std::string {
		auto func = [](const std::string& left, const std::string& right) {
			std::string common;
			std::ranges::set_intersection(left, right, std::back_inserter(common));
			return common;
		};

		auto commonRange = std::ranges::common_view{rng};

		auto front = *commonRange.begin();

		return std::accumulate(std::next(commonRange.begin()), commonRange.end(), front, func);
	});

	auto commonItemToSingle = commonItem | std::views::transform([](auto str) {
		if (str.empty()) {
			throw std::runtime_error("Unexpected");
		}

		return str[0];
	});

	auto commonItemValues = commonItemToSingle | std::views::transform([](char c) -> int {
		if (c >= 'a' && c <= 'z') {
			return c - 'a' + 1;
		}

		return c - 'A' + 27;
	});

	auto commonItemSum = std::accumulate(commonItemValues.begin(), commonItemValues.end(), 0);

	auto end = std::chrono::high_resolution_clock::now();

	fmt::print("Part 1: {}\n", mismatchSum);
	fmt::print("Part 2: {}\n", commonItemSum);

	auto dur = end - start;
	fmt::print("Took {}\n", dur);

	return 0;
}