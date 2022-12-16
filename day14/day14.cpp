
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

enum class CellType {
	// Empty = 0, // not defined here, as we use a empty map value
	Wall = 1,
	Sand = 2,
};

struct Point {
	int x;
	int y;
	std::strong_ordering operator<=>(const Point& point) const = default;
};



using namespace std::string_view_literals;

void addWall(std::map<int, std::map<int, CellType>>& map, const Point& point) {
	map[point.x][point.y] = CellType::Wall;
}

void addSand(std::map<int, std::map<int, CellType>>& map, const Point& point) {
	map[point.x][point.y] = CellType::Sand;
}


Point getDiff(const Point& point, const Point& lastPoint) {
	return Point{ point.x - lastPoint.x, point.y - lastPoint.y };
}

void movePoint(Point& point, const Point& diff) {
	point.x += std::clamp(diff.x, -1, 1);
	point.y += std::clamp(diff.y, -1, 1);
}

int main(int argc, char* argv[]) {
	std::ifstream inputFile("inputs/day14.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto start = std::chrono::high_resolution_clock::now();

	auto inputIntoLines = std::string_view{ input } | std::views::split("\n"sv) | std::views::transform([](auto rng) { return std::string_view(rng.begin(), rng.end()); }) | std::views::filter([](auto str) { return !str.empty(); });

	auto inputIntoRangesOfPoints = inputIntoLines | std::views::transform([](auto sv) {
		return sv
			| std::views::split(" -> "sv)
			| std::views::transform([](auto rng) { return std::string_view(rng.begin(), rng.end()); })
			| std::views::transform([](std::string_view sv) {
				auto comma = sv.find(',');
				auto xSv = sv.substr(0, comma);
				auto ySv = sv.substr(comma + 1);

				auto x = std::stoi(std::string{ xSv });
				auto y = std::stoi(std::string{ ySv });

				return Point{ x, y };
			});
	});


	// First map handles horizontal rows,
	// Second map handles vertical rows, allows us to use map lookup functions for fast sand falling.
	std::map<int, std::map<int, CellType>> grid;

	// Place the walls
	for (auto line : inputIntoRangesOfPoints) {

		auto lastPoint = line.front();

		addWall(grid, lastPoint);

		for (auto point : line | std::views::drop(1)) {
			auto diff = getDiff(point, lastPoint);

			auto currentPoint = lastPoint;

			while (currentPoint != point) {
				movePoint(currentPoint, diff);

				addWall(grid, currentPoint);
			}

			lastPoint = point;
		}
	}

	int numGrainsPlaced = 0;

	bool finished = false;

	while (!finished) {
		int currentX = 500;
		int currentY = 0;

		while (true) {
			// drop grain
			auto dropIt = grid[currentX].upper_bound(currentY);

			if (dropIt == grid[currentX].end()) {
				// sand will drop forever, break out of both loops
				finished = true;
				break;
			}

			// move currentY to just above the blocking element
			currentY = dropIt->first - 1;

			// now test if we can move left or right
			// test left first
			if (!grid[currentX - 1].contains(currentY + 1)) {
				// bot left is free, update and loop
				currentX -= 1;
				currentY += 1;
			} else if (!grid[currentX + 1].contains(currentY + 1)) {
				// bot right is free, update and loop
				currentX += 1;
				currentY += 1;
			} else {
				// sand is blocked, add some more sand
				addSand(grid, { currentX, currentY });
				numGrainsPlaced++;
				break;
			}
		}
	}

	
	auto end = std::chrono::high_resolution_clock::now();


	fmt::print("Part 1: num grins placed {}\n", numGrainsPlaced);

	auto dur = end - start;
	fmt::print("Took {}\n", dur);

	return 0;
}
