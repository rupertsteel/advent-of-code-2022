
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
#include <unordered_set>


#include <fmt/ranges.h>
#include <fmt/chrono.h>

#include <gsl/gsl-lite.hpp>

using namespace std::string_view_literals;

struct Instruction {
	int diffX;
	int diffY;
	int count;
};

template<>
struct std::hash<std::pair<int, int>> {
	std::size_t operator()(std::pair<int, int> const& s) const noexcept
	{
		std::size_t h1 = std::hash<int>{}(s.first);
		std::size_t h2 = std::hash<int>{}(s.second);
		return h1 ^ (h2 << 1); // or use boost::hash_combine
	}
};

struct Position {
	int x = 0;
	int y = y;
};

int main(int argc, char* argv[]) {
	std::ifstream inputFile("inputs/day9.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto start = std::chrono::high_resolution_clock::now();

	auto inputIntoInstructions = std::string_view{ input } | std::views::split("\n"sv) | std::views::transform([](auto rng) { return std::string_view(rng.begin(), rng.end()); }) | std::views::filter([](auto str) { return !str.empty(); })
		| std::views::transform([](auto sv) {
			Instruction ins;

			switch (sv[0]) {
			case 'U':
				ins.diffX = 0;
				ins.diffY = 1;
				break;
			case 'D':
				ins.diffX = 0;
				ins.diffY = -1;
				break;
			case 'R':
				ins.diffX = 1;
				ins.diffY = 0;
				break;
			case 'L':
				ins.diffX = -1;
				ins.diffY = 0;
				break;
			}

			auto countSv = sv.substr(2);
			std::string countStr{ countSv };
			auto count = std::stoi(countStr);

			ins.count = count;

			return ins;
		});

	// auto ropeLength = 1; // part 1
	auto ropeLength = 9;
	std::vector<Position> positions(ropeLength + 1);

	std::unordered_set<std::pair<int, int>> visitedLocations;

	for (auto instruction : inputIntoInstructions) {
		for (int i = 0; i < instruction.count; i++) {
			positions.front().x += instruction.diffX;
			positions.front().y += instruction.diffY;

			for (int j = 0; j < ropeLength; j++) {
				auto& headX = positions[j].x;
				auto& headY = positions[j].y;
				auto& tailX = positions[j + 1].x;
				auto& tailY = positions[j + 1].y;

				auto diffX = headX - tailX;
				auto diffY = headY - tailY;

				auto needMove = std::abs(diffX) > 1 || std::abs(diffY) > 1;

				if (needMove) {
					tailX += std::clamp(diffX, -1, 1);
					tailY += std::clamp(diffY, -1, 1);
				}
			}

			visitedLocations.emplace(positions.back().x, positions.back().y);
		}
	}

	auto end = std::chrono::high_resolution_clock::now();

	fmt::print("Part 1, # visited locations {}\n", visitedLocations.size());


	auto dur = end - start;
	fmt::print("Took {}\n", dur);

	return 0;
}