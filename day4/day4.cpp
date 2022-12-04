
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

int main(int argc, char* argv[]) {
	std::ifstream inputFile("inputs/day4.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto start = std::chrono::high_resolution_clock::now();

	auto inputIntoLines = std::string_view{ input } | std::views::split("\n"sv) | std::views::transform([](auto rng) { return std::string_view(rng.begin(), rng.end()); }) | std::views::filter([](auto str) { return !str.empty(); });

	//fmt::print("Processed 1: {}\n", inputIntoLines);

	auto inputIntoRanges = inputIntoLines | std::views::transform([](std::string_view sv) {
		const auto elf1StartRangeNumEnd = sv.find_first_of('-');

		const auto elf1StartRangeNumSv = sv.substr(0, elf1StartRangeNumEnd);

		const auto elf1EndRangeNumStart = elf1StartRangeNumEnd + 1;
		const auto elf1EndRangeNumEnd = sv.find_first_of(',', elf1EndRangeNumStart);

		const auto elf1EndRangeNumSv = sv.substr(elf1EndRangeNumStart, elf1EndRangeNumEnd - elf1EndRangeNumStart);

		const auto elf2StartRangeNumStart = elf1EndRangeNumEnd + 1;
		const auto elf2StartRangeNumEnd = sv.find_first_of('-', elf1EndRangeNumStart);

		const auto elf2StartRangeNumSv = sv.substr(elf2StartRangeNumStart, elf2StartRangeNumEnd - elf2StartRangeNumStart);

		const auto elf2EndRangeNumStart = elf2StartRangeNumEnd + 1;
		const auto elf2EndRangeNumEnd = sv.find_first_of(',', elf2EndRangeNumStart);

		const auto elf2EndRangeNumSv = sv.substr(elf2EndRangeNumStart, elf2EndRangeNumEnd - elf2EndRangeNumStart);


		const std::string elf1StartRangeNumStr{ elf1StartRangeNumSv };
		const std::string elf1EndRangeNumStr{ elf1EndRangeNumSv };
		const std::string elf2StartRangeNumStr{ elf2StartRangeNumSv };
		const std::string elf2EndRangeNumStr{ elf2EndRangeNumSv };

		return std::make_tuple(std::stoi(elf1StartRangeNumStr), std::stoi(elf1EndRangeNumStr), std::stoi(elf2StartRangeNumStr), std::stoi(elf2EndRangeNumStr));
	});

	auto rangesWhereFullOverlapOccurs = inputIntoRanges | std::views::filter([](auto input) {
		auto elf1FullyOverlapsElf2 = std::get<0>(input) >= std::get<2>(input) && std::get<1>(input) <= std::get<3>(input);
		auto elf2FullyOverlapsElf1 = std::get<2>(input) >= std::get<0>(input) && std::get<3>(input) <= std::get<1>(input);

		return  elf1FullyOverlapsElf2 || elf2FullyOverlapsElf1;
	});

	auto rangesWhereAnyOverlapOccurs = inputIntoRanges | std::views::filter([](auto input) {
		auto elf1StartOverlapsElf2 = std::get<0>(input) >= std::get<2>(input) && std::get<0>(input) <= std::get<3>(input);
		auto elf1EndOverlapsElf2 = std::get<1>(input) >= std::get<2>(input) && std::get<1>(input) <= std::get<3>(input);

		auto elf2StartOverlapsElf1 = std::get<2>(input) >= std::get<0>(input) && std::get<2>(input) <= std::get<1>(input);
		auto elf2EndOverlapsElf1 = std::get<3>(input) >= std::get<0>(input) && std::get<3>(input) <= std::get<1>(input);

		auto elf1FullyOverlapsElf2 = std::get<0>(input) <= std::get<2>(input) && std::get<1>(input) >= std::get<3>(input);
		auto elf2FullyOverlapsElf1 = std::get<2>(input) <= std::get<0>(input) && std::get<3>(input) >= std::get<1>(input);

		return elf1StartOverlapsElf2 || elf1EndOverlapsElf2 || elf2StartOverlapsElf1 || elf2EndOverlapsElf1 || elf1FullyOverlapsElf2 || elf2FullyOverlapsElf1;
	});

	auto numFullOverlaps = std::ranges::distance(rangesWhereFullOverlapOccurs);
	auto numOverlaps = std::ranges::distance(rangesWhereAnyOverlapOccurs);

	auto end = std::chrono::high_resolution_clock::now();

	fmt::print("Input into ranges: {}\n", inputIntoRanges);

	fmt::print("Part 1, num overlap pairs: {}\n", numFullOverlaps);
	fmt::print("Part 2, num overlap pairs: {}\n", numOverlaps);

	auto dur = end - start;
	fmt::print("Took {}\n", dur);

	return 0;
}