
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

int main(int argc, char* argv[]) {
	std::cout << fmt::format("Hello world!");


	std::ifstream inputFile("inputs/day1.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto processed1 = std::string_view{ input } | std::views::split("\n"sv) | std::views::transform([](auto rng) { return std::string_view(rng.begin(), rng.end()); });

	fmt::print("Processed 1: {}\n", processed1);


	auto processed2 = processed1 | std::views::chunk_by([](auto left, auto right) {
		return !left.empty();
	});

	auto processed3 = processed2 | std::views::transform([](auto rng) { return rng | std::views::filter([](auto str) { return !str.empty(); }); });


	//auto processed3 = processed2 | std::views::transform([](auto subRange) { return subRange | std::views::reverse | std::views::drop(1) | std::views::reverse; });

	auto processed4 = processed3 | std::views::transform([](auto rng1) { return rng1 | std::views::transform([](auto sv) { return std::stoi(std::string(sv)); }); });

	fmt::print("Processed 2: {}\n", processed2);

	//fmt::print("Processed 4: {}\n", processed4);

	// now, processed 4 has all values converted to ints, then get the max sum for problem 1

	auto sumCalc1 = processed4 | std::views::transform([](auto rng) { return std::accumulate(rng.begin(), rng.end(), 0); });
	auto sumMax = std::ranges::max(sumCalc1);

	fmt::print("Max calories: {}\n", sumMax);

	// I Give up using ranges now,
	auto processedEnd = processed4 | std::views::transform([](auto subRange) { return std::vector( subRange.begin(), subRange.end() ); });
	auto output = std::vector(processedEnd.begin(), processedEnd.end() );



	//auto processed15 = processed1 | std::views::transform([](auto sv) { return std::stoi(std::string{ sv }); });
	//

	return 0;
}