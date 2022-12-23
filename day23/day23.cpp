
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

std::optional<Point> proposePosition(const std::set<Point>& map, const Point& elf, int round) {
	bool neighboursEmpty = true;

	for (int y = -1; y <= 1; y++) {
		for (int x = -1; x <= 1; x++) {
			if (x == 0 && y == 0) {
				continue;
			}

			if (map.contains(Point{elf.x + x, elf.y + y})) {
				neighboursEmpty = false;
			}
		}
	}

	if (neighboursEmpty) {
		return std::nullopt;
	}

	for (int i = 0; i < 4; i++) {
		auto actualRound = (round + i) % 4;

		std::array<int, 6> diffs;

		if (actualRound == 0) {
			diffs = { -1, -1, 0, -1, 1, -1 };
		} else if (actualRound == 1) {
			diffs = { -1, 1, 0, 1, 1, 1 };
		} else if (actualRound == 2) {
			diffs = { -1, -1, -1, 0, -1, 1};
		} else {
			diffs = { 1, -1, 1, 0, 1, 1 };
		}

		if (!map.contains(Point{ elf.x + diffs[0], elf.y + diffs[1]}) && !map.contains(Point{elf.x + diffs[2], elf.y + diffs[3]}) && !map.contains(Point{elf.x + diffs[4], elf.y + diffs[5]})) {
			return Point{ elf.x + diffs[2], elf.y + diffs[3]};
		}
	}

	return std::nullopt;
}

void printBoard(const std::set<Point>& elves, int minX, int maxX, int minY, int maxY) {
	std::string printStr;

	for (int y = minY; y <= maxY; y++) {
		for (int x = minX; x <= maxX; x++) {
			if (elves.contains(Point{x, y})) {
				printStr += '#';
			} else {
				printStr += '.';
			}
		}

		printStr += '\n';
	}

	fmt::print("{}\n\n", printStr);
}

int main(int argc, char* argv[]) try {
	std::ifstream inputFile("inputs/day23.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto start = std::chrono::high_resolution_clock::now();

	auto inputIntoLines = std::string_view{ input } | std::views::split("\n"sv) | std::views::transform([](auto rng) { return std::string(rng.begin(), rng.end()); }) | std::views::filter([](auto str) { return !str.empty(); });

	std::set<Point> elfPositions;

	int lineNum = 0;
	for (auto line : inputIntoLines) {
		for (int x = 0; x < line.size(); x++) {
			if (line[x] == '#') {
				elfPositions.emplace(x, lineNum);
			}
		}

		lineNum++;
	}

	printBoard(elfPositions, -3, 10, -2, 9);

	
	int round = 0;

	while(true) {
		bool elfMoved = false;
		int numElvesMoved = 0;
		std::map<Point, int> proposials;

		for (auto& elf : elfPositions) {
			auto elfProposedPosition = proposePosition(elfPositions, elf, round);

			if (elfProposedPosition) {
				proposials[*elfProposedPosition]++;
			}
		}

		std::set<Point> newPositions;

		for (auto& elf : elfPositions) {
			auto elfProposedPosition = proposePosition(elfPositions, elf, round);

			if (elfProposedPosition && proposials[*elfProposedPosition] == 1) {
				newPositions.insert(*elfProposedPosition);
				elfMoved = true;
				numElvesMoved++;
			} else {
				newPositions.insert(elf);
			}
		}

		elfPositions = newPositions;

		

		fmt::print("== End of Round {:3} == {:4} elves moved\n", round + 1, numElvesMoved);
		//printBoard(elfPositions, -3, 10, -2, 9);

		if (elfMoved == false) {
			fmt::print("No elves moved\n");
			break;
		}

		round++;
	}

	int boundingBoxMinX = std::numeric_limits<int>::max();
	int boundingBoxMinY = std::numeric_limits<int>::max();
	int boundingBoxMaxX = std::numeric_limits<int>::min();
	int boundingBoxMaxY = std::numeric_limits<int>::min();

	for (auto& elf : elfPositions) {
		boundingBoxMinX = std::min(boundingBoxMinX, elf.x);
		boundingBoxMinY = std::min(boundingBoxMinY, elf.y);
		boundingBoxMaxX = std::max(boundingBoxMaxX, elf.x);
		boundingBoxMaxY = std::max(boundingBoxMaxY, elf.y);
	}

	auto area = (boundingBoxMaxX - boundingBoxMinX + 1) * (boundingBoxMaxY - boundingBoxMinY + 1);
	area -= elfPositions.size();

	auto end = std::chrono::high_resolution_clock::now();

	fmt::print("Ground area: {}\n", area);

	auto dur = end - start;

	auto durSecs = std::chrono::duration_cast<std::chrono::duration<float>>(dur);

	fmt::print("Took {}\n", dur);
	fmt::print("Or {}\n", durSecs);

	return 0;
} catch (std::exception& e) {
	fmt::print("Exception {}\n", e.what());
}
