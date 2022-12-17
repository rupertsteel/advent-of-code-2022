
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

bool canMove(const std::map<ptrdiff_t, std::array<bool, 7>>& map, ptrdiff_t startX, ptrdiff_t startY, const std::vector<Point>& testPoints) {
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

void applyShape(std::map<ptrdiff_t, std::array<bool, 7>>& map, ptrdiff_t startX, ptrdiff_t startY, const std::vector<Point>& solid) {
	for (auto point : solid) {
		auto actualX = startX + point.x;
		auto actualY = startY + point.y;

		map[actualX][actualY] = true;
	}
}


void printMap(const std::map<ptrdiff_t, std::array<bool, 7>>& map) {

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

struct StateInfo {
	int blockIndex;
	int windIndex;
	uint64_t topBlocks;

	std::strong_ordering operator<=>(const StateInfo&) const = default;
	bool operator==(const StateInfo&) const = default;
};

template<>
struct std::hash<StateInfo> {
	std::size_t operator()(const StateInfo& s) const noexcept {
		auto h1 = std::hash<int>{}(s.blockIndex);
		auto h2 = std::hash<int>{}(s.windIndex);
		auto h3 = std::hash<uint64_t>{}(s.topBlocks);

		return h1 ^ (h2 << 1) ^ (h3 << 2);
	}
};

struct RepeatInfo {
	ptrdiff_t height;
	ptrdiff_t numBlocks;
};

uint64_t getTopBitset(const std::map<ptrdiff_t, std::array<bool, 7>>& map) {
	uint64_t bits = 0;

	auto it = map.rbegin();

	for (int i = 0; i < 9; i++) {
		auto layerBits = it->second;

		for (int j = 0; j < 7; j++) {
			bits |= (it->second[j] ? 1 : 0) >> (i * 7 + j);
		}

		++it;
	}

	return bits;
}



std::optional<RepeatInfo> tryDetectRepeats(const std::unordered_map<StateInfo, std::vector<RepeatInfo>>::mapped_type& mapped) {
	fmt::print("Detecting repeats\n");

	fmt::print("Input data:\n");
	fmt::print("Height blocks\n");
	for (auto& in : mapped | std::views::reverse | std::views::take(10) | std::views::reverse) {
		fmt::print("{:6} {:6}\n", in.height, in.numBlocks);
	}

	fmt::print("Diffs\n");
	fmt::print("Height blocks\n");

	std::vector<ptrdiff_t> heightDifferences;
	std::vector<ptrdiff_t> blockDifferences;

	//for (int i = 1; i < mapped.size(); i++) {
	for (auto pair : mapped | std::views::reverse | std::views::take(10) | std::views::reverse | std::views::slide(2)) {


		auto diffHeight = pair[1].height - pair[0].height;
		auto diffBlocks = pair[1].numBlocks - pair[0].numBlocks;

		heightDifferences.push_back(diffHeight);
		blockDifferences.push_back(diffBlocks);

		fmt::print("{:6} {:6}\n", diffHeight, diffBlocks);
	}

	auto allHeightsDiffsTheSame = std::ranges::all_of(heightDifferences, [&](auto elem) {
		return elem == heightDifferences.front();
	});

	auto allBlocksDiffsTheSame = std::ranges::all_of(blockDifferences, [&](auto elem) {
		return elem == blockDifferences.front();
	});

	if (allHeightsDiffsTheSame && allBlocksDiffsTheSame) {
		return RepeatInfo{ heightDifferences.front(), blockDifferences.front() };
	}

	return std::nullopt;
}

int main(int argc, char* argv[]) {
	std::ifstream inputFile("inputs/day17.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto start = std::chrono::high_resolution_clock::now();

	std::string_view inputSv{ input };

	auto newline = inputSv.find('\n');
	auto windInputRange = inputSv.substr(0, newline);


	std::map<ptrdiff_t, std::array<bool, 7>> map;

	//constexpr ptrdiff_t numRocksCount = 2022;
	//constexpr int numRocksCount = 10;
	constexpr ptrdiff_t numRocksCount = 1514285714288;
	ptrdiff_t maxHeight = 0;

	std::unordered_map<StateInfo, std::vector<RepeatInfo>> detectedRepeats;

	ptrdiff_t windCount = 0;
	ptrdiff_t currentBlockCount = 0;

	bool running = true;

	std::optional<RepeatInfo> repeatInfo;

	while(running) {
		ptrdiff_t startX = maxHeight + 3;
		ptrdiff_t startY = 2;

		int shapeIndex = currentBlockCount % 5;

		int windIndex;

		while (true) {
			windIndex = windCount % windInputRange.size();
			auto windDir = windInputRange[windIndex];
			++windCount;

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
		maxHeight = map.rbegin()->first + 1;

		currentBlockCount++;

		if (maxHeight > 9) {
			auto topBitset = getTopBitset(map);

			StateInfo state{
				shapeIndex,
				windIndex,
				topBitset
			};

			detectedRepeats[state].push_back(RepeatInfo{ maxHeight, currentBlockCount });

			if (detectedRepeats[state].size() >= 10) {
				repeatInfo = tryDetectRepeats(detectedRepeats[state]);

				if (repeatInfo) {
					running = false;
					break;
				}
			}
		}
	}



	auto end = std::chrono::high_resolution_clock::now();

	//printMap(map);

	fmt::print("Part 1: max height {}\n", maxHeight);

	auto dur = end - start;
	fmt::print("Took {}\n", dur);

	return 0;
}
