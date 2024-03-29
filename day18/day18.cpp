
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
#include <thread>
#include <stack>

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
	int z;

	bool operator==(const Point&) const = default;
	std::strong_ordering operator<=>(const Point&) const = default;
};

int main(int argc, char* argv[]) {
	std::ifstream inputFile("inputs/day18.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto start = std::chrono::high_resolution_clock::now();

	auto inputIntoLines = std::string_view{ input } | std::views::split("\n"sv) | std::views::transform([](auto rng) { return std::string(rng.begin(), rng.end()); }) | std::views::filter([](auto str) { return !str.empty(); });

	const std::regex line_regex(R"(^(\d+),(\d+),(\d+)$)");

	std::set<Point> shape;

	std::set<Point> outsidePoints;

	int minX = std::numeric_limits<int>::max();
	int minY = std::numeric_limits<int>::max();
	int minZ = std::numeric_limits<int>::max();
	int maxX = std::numeric_limits<int>::min();
	int maxY = std::numeric_limits<int>::min();
	int maxZ = std::numeric_limits<int>::min();

	for (auto line : inputIntoLines) {
		auto str = std::string{ line };

		std::smatch results;
		std::regex_match(str, results, line_regex);

		auto x = std::stoi(results[1].str());
		auto y = std::stoi(results[2].str());
		auto z = std::stoi(results[3].str());

		minX = std::min(minX, x);
		minY = std::min(minY, y);
		minZ = std::min(minZ, z);

		maxX = std::max(maxX, x);
		maxY = std::max(maxY, y);
		maxZ = std::max(maxZ, z);

		shape.emplace(x, y, z);
	}

	// expand the max/min bounds to make sure there it at least 1 cell on every side
	minX--;
	minY--;
	minZ--;
	maxX++;
	maxY++;
	maxZ++;

	std::stack<Point> outsidePointsToProcess;

	outsidePointsToProcess.emplace(minX, minY, minZ);

	while (!outsidePointsToProcess.empty()) {
		auto point = outsidePointsToProcess.top();
		outsidePointsToProcess.pop();

		if (outsidePoints.contains(point)) {
			// we have already processed this point, no need to do it again
			continue;
		}

		// add this point to the outside shape
		outsidePoints.insert(point);

		// now add the up-to 6 orthogonal points touching this that are also in bound to the stack
		auto checkThenAdd = [&](int dx, int dy, int dz) {
			auto testPoint = point;
			testPoint.x += dx;
			testPoint.y += dy;
			testPoint.z += dz;

			if (testPoint.x < minX || testPoint.x > maxX) {
				return;
			}
			if (testPoint.y < minY || testPoint.y > maxY) {
				return;
			}
			if (testPoint.z < minZ || testPoint.z > maxZ) {
				return;
			}

			if (shape.contains(testPoint)) {
				return;
			}

			outsidePointsToProcess.push(testPoint);
		};

		checkThenAdd(1, 0, 0);
		checkThenAdd(-1, 0, 0);
		checkThenAdd(0, 1, 0);
		checkThenAdd(0, -1, 0);
		checkThenAdd(0, 0, 1);
		checkThenAdd(0, 0, -1);
	}

	int surfaceArea = 0;
	int outsideSurfaceArea = 0;

	for (const auto p : shape) {
		auto testAndApply = [&](int dx, int dy, int dz) {
			auto testPoint = p;
			testPoint.x += dx;
			testPoint.y += dy;
			testPoint.z += dz;

			if (!shape.contains(testPoint)) {
				surfaceArea++;
			}

			if (outsidePoints.contains(testPoint)) {
				outsideSurfaceArea++;
			}
		};

		testAndApply(1, 0, 0);
		testAndApply(-1, 0, 0);
		testAndApply(0, 1, 0);
		testAndApply(0, -1, 0);
		testAndApply(0, 0, 1);
		testAndApply(0, 0, -1);
	}

	auto end = std::chrono::high_resolution_clock::now();

	fmt::print("Part 1: surface area: {}\n", surfaceArea);
	fmt::print("Part 2: outside surface area: {}\n", outsideSurfaceArea);


	auto dur = end - start;

	auto durSecs = std::chrono::duration_cast<std::chrono::duration<float>>(dur);

	fmt::print("Took {}\n", dur);
	fmt::print("Or {}\n", durSecs);

	return 0;
}
