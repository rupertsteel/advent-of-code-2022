
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


#include <fmt/ranges.h>
#include <fmt/chrono.h>

#include <gsl/gsl-lite.hpp>

using namespace std::string_view_literals;

int main(int argc, char* argv[]) {
	std::ifstream inputFile("inputs/day10.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto start = std::chrono::high_resolution_clock::now();

	auto inputIntoLines = std::string_view{ input } | std::views::split("\n"sv) | std::views::transform([](auto rng) { return std::string_view(rng.begin(), rng.end()); }) | std::views::filter([](auto str) { return !str.empty(); });

	std::set<int> printCycles = { 20, 60, 100, 140, 180, 220 };

	int signalStrengthSum = 0;

	int xReg = 1;
	int cycle = 1;

	auto printCycleIfNeeded = [&]() {
		if (printCycles.contains(cycle)) {
			signalStrengthSum += cycle * xReg;
		}

		
	};

	auto printScreen = [&]() {
		auto xPosition = (cycle - 1) % 40;

		auto xDiff = xReg - xPosition;
		if (xDiff >= -1 && xDiff <= 1) {
			fmt::print("#");
		}
		else {
			fmt::print(" ");
		}

		if (xPosition == 39) {
			fmt::print("\n");
		}
	};

	for (auto ins : inputIntoLines) {
		if (ins == "noop") {
			printScreen();

			cycle++;

			printCycleIfNeeded();
		} else {
			printScreen();

			cycle++;

			printCycleIfNeeded();

			auto valueSs = ins.substr(5);
			std::string valueStr{ valueSs };
			auto value = std::stoi(valueStr);

			printScreen();

			cycle++;
			xReg += value;

			printCycleIfNeeded();
		}
	}



	auto end = std::chrono::high_resolution_clock::now();

	fmt::print("Part 1, signal strength: {}\n", signalStrengthSum);

	auto dur = end - start;
	fmt::print("Took {}\n", dur);

	return 0;
}