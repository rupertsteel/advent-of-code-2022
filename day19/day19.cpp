
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

#include <fmt/ranges.h>
#include <fmt/chrono.h>
#include <fmt/std.h>

#include <gsl/gsl-lite.hpp>

#include <indicators/progress_bar.hpp>

#include <boost/container/flat_set.hpp>

using namespace std::string_view_literals;
using namespace std::string_literals;

struct Blueprint {
	int id;
	int oreRobotOreCost;
	int clayRobotOreCost;
	int obsidianRobotOreCost;
	int obsidianRobotClayCost;
	int geodeRobotOreCost;
	int geodeRobotObsidianCost;
};

struct Solution {
	int minute;

	int numOreMiners;
	int numClayMiners;
	int numObsidianMiners;
	int numGeodeMiners;

	int amountOre;
	int amountClay;
	int amountObsidian;
	int amountGeode;

	int nextRobot;
};

int main(int argc, char* argv[]) {
	std::ifstream inputFile("inputs/day19.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto start = std::chrono::high_resolution_clock::now();

	auto inputIntoLines = std::string_view{ input } | std::views::split("\n"sv) | std::views::transform([](auto rng) { return std::string(rng.begin(), rng.end()); }) | std::views::filter([](auto str) { return !str.empty(); });

	const std::regex line_regex(R"(^\D+(\d+)\D+(\d+)\D+(\d+)\D+(\d+)\D+(\d+)\D+(\d+)\D+(\d+)\D+)");

	auto blueprints = inputIntoLines | std::views::transform([&line_regex](const std::string& s) {
		std::smatch results;

		std::regex_match(s, results, line_regex);

		return Blueprint{
			std::stoi(results[1].str()),
			std::stoi(results[2].str()),
			std::stoi(results[3].str()),
			std::stoi(results[4].str()),
			std::stoi(results[5].str()),
			std::stoi(results[6].str()),
			std::stoi(results[7].str())
		};
	});

#if 0 // part 1
	constexpr int stopTime = 24;
#else
	constexpr int stopTime = 32;
#endif

	int quality = 0;
	int multiplyScore = 1;

#if 0 // part 1
	for (auto blueprint : blueprints) {
#else
	for (auto blueprint : blueprints | std::views::take(3)) {
#endif
		std::stack<Solution> solutions;

		for (int i = 0; i < 4; i++) {
			Solution start{
				1, 1, 0, 0, 0, 0, 0, 0, 0, i
			};

			solutions.push(start);
		}

		int bestScore = 0;


		size_t iter = 0;

		while (!solutions.empty()) {
			auto baseSolution = solutions.top();
			solutions.pop();

			auto printProgress = [&]() {
				std::vector<int> minuteNums;

				for (auto& sol : solutions._Get_container()) {
					minuteNums.push_back(sol.minute);
				}

				fmt::print("Progress: {}\n", minuteNums);
			};

			// work out what robots can be made
			bool canOreMinerBeMade = baseSolution.amountOre >= blueprint.oreRobotOreCost;
			bool canClayMinerBeMade = baseSolution.amountOre >= blueprint.clayRobotOreCost;
			bool canObsidianMinerBeMade = baseSolution.amountOre >= blueprint.obsidianRobotOreCost && baseSolution.amountClay >= blueprint.obsidianRobotClayCost;
			bool canGeodeMinerBeMade = baseSolution.amountOre >= blueprint.geodeRobotOreCost && baseSolution.amountObsidian >= blueprint.geodeRobotObsidianCost;

			// if we can make any robot, then force a robot to be made, as delaying will always be sub-optional
			bool mustMakeRobot = canOreMinerBeMade && canClayMinerBeMade && canObsidianMinerBeMade && canGeodeMinerBeMade;

			baseSolution.amountOre += baseSolution.numOreMiners;
			baseSolution.amountClay += baseSolution.numClayMiners;
			baseSolution.amountObsidian += baseSolution.numObsidianMiners;
			baseSolution.amountGeode += baseSolution.numGeodeMiners;

			iter++;

			if (baseSolution.minute == stopTime) {
				// we are done here
				bestScore = std::max(bestScore, baseSolution.amountGeode);

				if ((iter & 0xFFFFFFF) == 0) {
					printProgress();
				}

				continue;
			}
			baseSolution.minute++;

			if (baseSolution.nextRobot == 0 && canOreMinerBeMade) {
				auto newSolution = baseSolution;
				newSolution.amountOre -= blueprint.oreRobotOreCost;
				newSolution.numOreMiners++;

				for (int i = 0; i < 4; i++) {
					newSolution.nextRobot = i;
					solutions.push(newSolution);
				}
			} else if (baseSolution.nextRobot == 1 && canClayMinerBeMade) {
				auto newSolution = baseSolution;
				newSolution.amountOre -= blueprint.clayRobotOreCost;
				newSolution.numClayMiners++;
				for (int i = 0; i < 4; i++) {
					newSolution.nextRobot = i;
					solutions.push(newSolution);
				}
			} else if (baseSolution.nextRobot == 2 && canObsidianMinerBeMade) {
				auto newSolution = baseSolution;
				newSolution.amountOre -= blueprint.obsidianRobotOreCost;
				newSolution.amountClay -= blueprint.obsidianRobotClayCost;
				newSolution.numObsidianMiners++;
				for (int i = 0; i < 4; i++) {
					newSolution.nextRobot = i;
					solutions.push(newSolution);
				}
			} else if (baseSolution.nextRobot == 3 && canGeodeMinerBeMade) {
				auto newSolution = baseSolution;
				newSolution.amountOre -= blueprint.geodeRobotOreCost;
				newSolution.amountObsidian -= blueprint.geodeRobotObsidianCost;
				newSolution.numGeodeMiners++;
				for (int i = 0; i < 4; i++) {
					newSolution.nextRobot = i;
					solutions.push(newSolution);
				}
			} else {
				solutions.push(baseSolution);
			}

			if ((iter & 0xFFFFFFF) == 0) {
				printProgress();
			}
		}

		fmt::print("Blueprint {}: score {}\n", blueprint.id, bestScore);

		quality += blueprint.id * bestScore;
		multiplyScore *= bestScore;
	}

	auto end = std::chrono::high_resolution_clock::now();

	fmt::print("Part 1: quality {}\n", quality);
	fmt::print("Part 2: best score {}\n", multiplyScore);

	auto dur = end - start;

	auto durSecs = std::chrono::duration_cast<std::chrono::duration<float>>(dur);

	fmt::print("Took {}\n", dur);
	fmt::print("Or {}\n", durSecs);

	return 0;
}
