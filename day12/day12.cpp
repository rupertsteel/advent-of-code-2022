
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


#include <fmt/ranges.h>
#include <fmt/chrono.h>

#include <gsl/gsl-lite.hpp>

using namespace std::string_view_literals;

struct PathData {
	std::optional<int> distToEnd;
	bool isStart = false;
	bool isEnd = false;
};

int main(int argc, char* argv[]) {
	std::ifstream inputFile("inputs/day12.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto start = std::chrono::high_resolution_clock::now();

	auto inputIntoLines = std::string_view{ input } | std::views::split("\n"sv) | std::views::transform([](auto rng) { return std::string_view(rng.begin(), rng.end()); }) | std::views::filter([](auto str) { return !str.empty(); });

	std::vector<int> heightData;
	heightData.reserve(input.size());
	int width;
	int length = 0;

	int startX = 0;
	int startY = 0;
	int endX = 0;
	int endY = 0;

	for (auto line : inputIntoLines) {

		for (int i = 0; i < line.size(); i++) {
			auto ch = line[i];

			if (ch == 'S') {
				heightData.push_back(0);
				startX = i;
				startY = length;
			} else if (ch == 'E') {
				heightData.push_back(25);
				endX = i;
				endY = length;
			} else {

				heightData.push_back(ch - 'a');
			}
		}

		length++;
		width = line.size();
	}

	std::vector<PathData> pathData(heightData.size());

	auto arrLookup = [width](auto& vec, int x, int y) -> auto& {
		auto offset = y * width + x;

		return vec[offset];
	};

	arrLookup(pathData, startX, startY).isStart = true;
	arrLookup(pathData, endX, endY).isEnd = true;

	std::deque<std::pair<int, int>> locationsToProcess;

	locationsToProcess.push_back(std::make_pair(endX, endY));

	while (!locationsToProcess.empty()) {
		auto loc = locationsToProcess.front();
		locationsToProcess.pop_front();

		bool updated = false;

		if (arrLookup(pathData, loc.first, loc.second).isEnd) {
			arrLookup(pathData, loc.first, loc.second).distToEnd = 0;
			updated = true;
		} else {
			// if left exists
			// if left can travel to here
			// if left (dist + 1) < current (dist), then update current dist

			auto updatePathDistance = [&](int diffX, int diffY) {
				int testX = loc.first + diffX;
				int testY = loc.second + diffY;

				bool locationIsValid = testX >= 0 && testX < width&& testY >= 0 && testY < length;

				if (locationIsValid) {
					auto& currentPathData = arrLookup(pathData, loc.first, loc.second);
					auto& testPathData = arrLookup(pathData, testX, testY);

					bool targetCanTravelHere = arrLookup(heightData, loc.first, loc.second) - arrLookup(heightData, testX, testY) >= -1;

					if (targetCanTravelHere) {
						if (!currentPathData.distToEnd && testPathData.distToEnd) {
							currentPathData.distToEnd = testPathData.distToEnd.value() + 1;
							updated = true;
						} else if (testPathData.distToEnd && currentPathData.distToEnd.value() > testPathData.distToEnd.value() + 1) {
							currentPathData.distToEnd.value() = testPathData.distToEnd.value() + 1;
							updated = true;
						}
					}
				}
			};

			updatePathDistance(-1, 0);
			updatePathDistance(1, 0);
			updatePathDistance(0, -1);
			updatePathDistance(0, 1);
		}

		if (updated) {
			auto enqueueUpdate = [&](int diffX, int diffY) {
				auto newX = loc.first + diffX;
				auto newY = loc.second + diffY;

				if (newX >= 0 && newX < width && newY >= 0 && newY < length) {
					locationsToProcess.push_back(std::make_pair(newX, newY));
				}
			};

			enqueueUpdate(-1, 0);
			enqueueUpdate(1, 0);
			enqueueUpdate(0, -1);
			enqueueUpdate(0, 1);
		}
	}

	auto minDistFromLowestElevation = 999999;
	for (int i = 0; i < pathData.size(); i++) {
		if (heightData[i] == 0) {
			if (pathData[i].distToEnd) {
				if (pathData[i].distToEnd.value() < minDistFromLowestElevation) {
					minDistFromLowestElevation = pathData[i].distToEnd.value();
				}
			}
		}
	}
	
	auto end = std::chrono::high_resolution_clock::now();

	fmt::print("Part 1: Dist to finish {}\n", arrLookup(pathData, startX, startY).distToEnd.value());

	fmt::print("part 2: min dist {}\n", minDistFromLowestElevation);


	auto dur = end - start;
	fmt::print("Took {}\n", dur);

	return 0;
}