
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
};

struct Shape {
	std::vector<Point> solid;

	std::vector<Point> needFreeMoveDown;
	std::vector<Point> needFreeMoveLeft;
	std::vector<Point> needFreeMoveRight;

	int width;
};

std::array<Shape, 5> shapes {
	Shape{
		//  0123
		// l####r
		// .dddd.
		{
			{0,0},
			{0,1},
			{0,2},
			{0,3},
		},
		{
			{-1, 0},
			{-1, 1},
			{-1, 2},
			{-1, 3},
		},
		{
			{0,-1}
		},
		{
			{0, 4}
		},
		4
	},
	{
		//  012
		// .l#r.
		// l###r
		// .L#R.
		// ..d..
		{
			{2, 1},
			{1, 0},
			{1, 1},
			{1, 2},
			{0, 1}
		},
		{
			{0, 0},
			{0, 2},
			{-1, 1}
		},
		{
			{2, 0},
			{1, -1},
			{0, 0}
		},
		{
			{2, 2},
			{1, 3},
			{0, 2}
		},
		3
	},
	{
		//  012
		// ..l#r
		// ..l#r
		// l###r
		// .ddd.
		{
			{0, 0},
			{0, 1},
			{0, 2},
			{1, 2},
			{2, 2}
		},
		{
			{-1, 0},
			{-1, 1},
			{-1, 2},
		},
		{
			{0, -1},
			{1, 1},
			{2, 1}
		},
		{
			{0, 3},
			{1, 3},
			{2, 3}
		},
		3
	},
	{
		//  0
		// l#r
		// l#r
		// l#r
		// l#r
		// .d.
		{
			{0, 0},
			{1, 0},
			{2, 0},
			{3, 0},
		},
		{
			{-1, 0}
		},
		{
			{0, -1},
			{1, -1},
			{2, -1},
			{3, -1}
		},
		{
			{0, 1},
			{1, 1},
			{2, 1},
			{3, 1}
		},
		1
	},
	{
		//  0
		// l##r
		// l##r
		// .dd.
		{
			{0, 0},
			{0, 1},
			{1, 0},
			{1, 1}
		},
		{
			{-1, 0},
			{-1, 1},
		},
		{
			{0, -1},
			{1, -1},
		},
		{
			{0, 2},
			{1, 2}
		},
		2
	}
};

bool canMove(const std::map<int, std::array<bool, 7>>& map, int startX, int startY, const std::vector<Point>& testPoints) {
	return std::ranges::all_of(testPoints, [&](auto testPoint) {
		auto actualX = startX + testPoint.x;
		auto actualY = startY + testPoint.y;

		if (actualX < 0) {
			return false;
		}

		if (actualY < 0) {
			return false;
		}

		if (actualY > 6) {
			return false;
		}

		if (!map.contains(actualX)) {
			return true;
		}

		return !map.at(actualX)[actualY];
	});
}

void applyShape(std::map<int, std::array<bool, 7>>& map, int startX, int startY, const std::vector<Point>& solid) {
	for (auto point : solid) {
		auto actualX = startX + point.x;
		auto actualY = startY + point.y;

		map[actualX][actualY] = true;
	}
}


void printMap(const std::map<int, std::array<bool, 7>>& map) {

	for (auto line : map | std::views::reverse) {
		fmt::print("{:4} |{}{}{}{}{}{}{}|\n",
			line.first,
			line.second[0] ? '#' : '.',
			line.second[1] ? '#' : '.',
			line.second[2] ? '#' : '.',
			line.second[3] ? '#' : '.',
			line.second[4] ? '#' : '.',
			line.second[5] ? '#' : '.',
			line.second[6] ? '#' : '.'
		);
	}

	fmt::print("     +-------+\n\n");
}

int main(int argc, char* argv[]) {
	std::ifstream inputFile("inputs/day17.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto start = std::chrono::high_resolution_clock::now();

	std::string_view inputSv{ input };

	auto newline = inputSv.find('\n');
	auto inputRange = inputSv.substr(0, newline);

	auto jankRepeatView = std::views::iota(0) | std::views::transform([&inputRange](auto i) {
		return inputRange[i % inputRange.size()];
	});

	std::map<int, std::array<bool, 7>> map;

	constexpr int numRocksCount = 2022;
	//constexpr int numRocksCount = 10;
	int maxHeight = 0;

	auto windIt = jankRepeatView.begin();

	for (int i = 0; i < numRocksCount; i++) {
		int startX = maxHeight + 3;
		int startY = 2;

		auto shapeIndex = i % 5;

		while (true) {
			auto windDir = *windIt;
			++windIt;
			if (windDir == '<') {
				// test left
				if (startY > 0 && canMove(map, startX, startY, shapes[shapeIndex].needFreeMoveLeft)) {
					startY -= 1;
				}
			} else {
				if (startY + shapes[shapeIndex].width < 7 && canMove(map, startX, startY, shapes[shapeIndex].needFreeMoveRight)) {
					startY += 1;
				}
			}

			if (canMove(map, startX, startY, shapes[shapeIndex].needFreeMoveDown)) {
				startX--;
			} else {
				break;
			}
		}

		applyShape(map, startX, startY, shapes[shapeIndex].solid);
		maxHeight = map.size();

		//printMap(map);

		// add rock to map
	}

	auto end = std::chrono::high_resolution_clock::now();

	printMap(map);

	fmt::print("Part 1: max height {}\n", maxHeight);

	auto dur = end - start;
	fmt::print("Took {}\n", dur);

	return 0;
}
