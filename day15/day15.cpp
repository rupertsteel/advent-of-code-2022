
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

struct Sensor {
	int posX;
	int posY;

	int beaconX;
	int beaconY;
};

// ranges are inclusive, so [1, 1] only has 1 block in it
struct Range {
	int lower;
	int upper;
};

bool rangesTouchOrOverlap(const Range& rng1, const Range& rng2) {
	// lets see if they touch
	if (rng1.upper + 1 == rng2.lower) {
		return true;
	}
	if (rng2.upper + 1 == rng1.lower) {
		return true;
	}

	// now they don't just touch, so they either fully overlap, or don't overlap at all
	if (rng1.lower <= rng2.upper && rng1.upper >= rng2.lower) {
		return true;
	}
	if (rng2.lower <= rng1.upper && rng2.upper >= rng1.lower) {
		return true;
	}

	return false;
}

Range createMergedRange(const Range& rng1, const Range& rng2) {
	return {
		std::min(rng1.lower, rng2.lower),
		std::max(rng1.upper, rng2.upper)
	};
}

struct Position {
	int x;
	int y;

	bool operator==(const Position& other) const = default;
	std::strong_ordering operator<=>(const Position& other) const = default;
};

int main(int argc, char* argv[]) {
	std::ifstream inputFile("inputs/day15.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto start = std::chrono::high_resolution_clock::now();

	auto inputIntoLines = std::string_view{ input } | std::views::split("\n"sv) | std::views::transform([](auto rng) { return std::string(rng.begin(), rng.end()); }) | std::views::filter([](auto str) { return !str.empty(); });

	const std::regex line_regex(R"(Sensor at x=(-?\d+), y=(-?\d+): closest beacon is at x=(-?\d+), y=(-?\d+))");

	std::set<Position> beacons;

	std::vector<Position> possibleTargetPositions;

	auto sensors = inputIntoLines | std::views::transform([&line_regex](const std::string& s) {
		std::smatch results;

		std::regex_match(s, results, line_regex);

		return Sensor{
			std::stoi(results[1].str()),
			std::stoi(results[2].str()),
			std::stoi(results[3].str()),
			std::stoi(results[4].str())
		};
	});


	for (auto sensor : sensors) {
		beacons.emplace(sensor.beaconX, sensor.beaconY);

		auto deltaX = sensor.posX - sensor.beaconX;
		auto deltaY = sensor.posY - sensor.beaconY;

		auto distToBeacon = std::abs(deltaX) + std::abs(deltaY);

		// add all required target cells,
		// these have dist + 1

		// top, down right
		{
			int placeX = sensor.posX;
			int placeY = sensor.posY + distToBeacon + 1;

			if (placeX >= 0 && placeX <= 4000000 && placeY >= 0 && placeY <= 4000000) {
				possibleTargetPositions.emplace_back(placeX, placeY);
			}
			for (int i = 0; i < distToBeacon; i++) {
				placeX++;
				placeY--;

				if (placeX >= 0 && placeX <= 4000000 && placeY >= 0 && placeY <= 4000000) {
					possibleTargetPositions.emplace_back(placeX, placeY);
				}
			}
		}
		// bot, up left
		{
			int placeX = sensor.posX;
			int placeY = sensor.posY - distToBeacon - 1;

			if (placeX >= 0 && placeX <= 4000000 && placeY >= 0 && placeY <= 4000000) {
				possibleTargetPositions.emplace_back(placeX, placeY);
			}
			for (int i = 0; i < distToBeacon; i++) {
				placeX--;
				placeY++;
				
				if (placeX >= 0 && placeX <= 4000000 && placeY >= 0 && placeY <= 4000000) {
					possibleTargetPositions.emplace_back(placeX, placeY);
				}
			}
		}
		// right, down left
		{
			int placeX = sensor.posX + distToBeacon + 1;
			int placeY = sensor.posY;

			if (placeX >= 0 && placeX <= 4000000 && placeY >= 0 && placeY <= 4000000) {
				possibleTargetPositions.emplace_back(placeX, placeY);
			}
			for (int i = 0; i < distToBeacon; i++) {
				placeX--;
				placeY--;
				
				if (placeX >= 0 && placeX <= 4000000 && placeY >= 0 && placeY <= 4000000) {
					possibleTargetPositions.emplace_back(placeX, placeY);
				}
			}
		}
		// left, up right
		{
			int placeX = sensor.posX - distToBeacon - 1;
			int placeY = sensor.posY;

			if (placeX >= 0 && placeX <= 4000000 && placeY >= 0 && placeY <= 4000000) {
				possibleTargetPositions.emplace_back(placeX, placeY);
			}
			for (int i = 0; i < distToBeacon; i++) {
				placeX++;
				placeY++;
				
				if (placeX >= 0 && placeX <= 4000000 && placeY >= 0 && placeY <= 4000000) {
					possibleTargetPositions.emplace_back(placeX, placeY);
				}
			}
		}
	}

	// now drop all targets that are within range
	for (auto sensor : sensors) {
		auto deltaX = sensor.posX - sensor.beaconX;
		auto deltaY = sensor.posY - sensor.beaconY;

		auto distToBeacon = std::abs(deltaX) + std::abs(deltaY);

		std::erase_if(possibleTargetPositions, [&](auto& pos) {
			auto posDx = sensor.posX - pos.x;
			auto posDy = sensor.posY - pos.y;

			auto posDistToBeacon = std::abs(posDx) + std::abs(posDy);
			return posDistToBeacon <= distToBeacon;
		});
	}


	// On the target line, each sensor may have a range where beacons cannot be located.
	// We get all of the ranges
	// Then we merge ranges that touch or overlap

	constexpr int targetLine = 2000000;
	//constexpr int targetLine = 10;

	std::vector<Range> ranges = sensors | std::views::transform([targetLine](auto sensor) -> std::optional<Range> {
		auto deltaX = sensor.posX - sensor.beaconX;
		auto deltaY = sensor.posY - sensor.beaconY;

		auto distToBeacon = std::abs(deltaX) + std::abs(deltaY);

		auto sensorDistToTargetLine = std::abs(targetLine - sensor.posY);

		// when deltaY to line is 0, then we have dist to the left, then 1, then dist to the right
		// we lose 1 block on each side for each distance away deltaY
		auto rangeDistOnLine = 1 + (distToBeacon - sensorDistToTargetLine) * 2;

		if (rangeDistOnLine <= 0) {
			return std::nullopt;
		}

		return Range{
			sensor.posX - (distToBeacon - sensorDistToTargetLine),
			sensor.posX + (distToBeacon - sensorDistToTargetLine)
		};
	}) | std::views::filter([](auto o) { return o.has_value(); }) | std::views::transform([](auto o) {
		return o.value();
	}) | std::ranges::to<std::vector>();


	while (true) {
		bool updated = false;

		for (auto it1 = ranges.begin(); !updated && it1 != ranges.end(); ++it1) {
			for (auto it2 = std::next(it1); !updated && it2 != ranges.end(); ++it2) {
				if (rangesTouchOrOverlap(*it1, *it2)) {

					auto newRange = createMergedRange(*it1, *it2);
					ranges.erase(it2);
					ranges.erase(it1);

					ranges.push_back(newRange);

					it1 = it2 = ranges.begin();
					updated = true;
				}
			}
		}

		if (!updated) {
			break;
		}
	}

	int numCells = 0;

	for (auto& rng : ranges) {
		numCells += (rng.upper - rng.lower + 1);

		for (auto& beacon : beacons) {
			if (beacon.y == targetLine) {
				if (beacon.x >= rng.lower && beacon.x <= rng.upper) {
					--numCells;
				}
			}
		}
	}

	auto end = std::chrono::high_resolution_clock::now();

	fmt::print("Part 1, num cells {}\n", numCells);

	fmt::print("Part 2, num cells {}\n", possibleTargetPositions.size());

	if (!possibleTargetPositions.empty()) {
		size_t freq = possibleTargetPositions.front().x * 4000000ll + possibleTargetPositions.front().y;

		fmt::print("Part 2, freq {}\n", freq);
	}

	auto dur = end - start;

	auto durSecs = std::chrono::duration_cast<std::chrono::duration<float>>(dur);

	fmt::print("Took {}\n", dur);
	fmt::print("Or {}\n", durSecs);

	return 0;
}
