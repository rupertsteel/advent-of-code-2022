
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

using namespace std::string_view_literals;
using namespace std::string_literals;

struct Node {
	int flow_rate;
	std::vector<std::string> nextNodes;
};

struct SolutionData {
	int minute;
	int scoreLowerBound;
	int scoreUpperBound;
	std::string currentNode;
	std::string prevNode;

	std::set<std::string> openValves;
	std::set<std::string> valvesStillToOpen;

	void updateUpperBound(const std::map<std::string, Node>& map);
};

// idea: have a set of in progress solutions, each with upper and lower bound of score, if the upper bound of any solution is below the lower bound of another, then discard it.
// the lower bound is the current release score at the end of the game of the currently open valves
// the upper bound is the score if we opened all valves at once (this could be better?)

int calcPossibleScore(std::vector<int> valveScores, int rounds) {
	// there are at most 30 turns,
	// subtract 2, then open the next valve
	int score = 0;

	while (!valveScores.empty() && rounds > 2) {
		score += valveScores.front() * (rounds - 2);
		valveScores.erase(valveScores.begin());
		rounds -= 2;
	}

	return score;
}

int calcScoreUpperBound(const std::map<std::string, Node>& map, int currentLowerBound, const std::string& currentNodeName, const std::set<std::string>& valvesToOpen, int turns) {
	// the upper bound is
	// lowerBound + the max score we could get with opening the other valves

	// we need to take into account the current node name
	// if the current node doesn't have a valve, we must move to a node with a valve
	// if the current node has a valve, but it isn't the biggest unopened valve, then not opening the current valve might be best
	// if the current node is the biggest unopened valve, we should open it now.

	// it takes 1 turn to move, and 1 turn to open a valve, a valve only contributes score to the turn after it opens
	// if you open a valve on turn 30, then it contributes nothing
	// if you open a valve on turn 29, then it contributes its score once.

	if (valvesToOpen.empty()) {
		return currentLowerBound;
	}

	std::vector<int> valveOpenOrder;
	for (auto& valve : valvesToOpen) {
		valveOpenOrder.push_back(map.at(valve).flow_rate);
	}

	std::ranges::sort(valveOpenOrder, std::ranges::greater());

	// handle the case where the current node has the biggest valve
	if (map.at(currentNodeName).flow_rate == valveOpenOrder[0]) {
		// the +1 handles a "free" move to the largest valve (we are already here)
		return currentLowerBound + calcPossibleScore(valveOpenOrder, turns + 1);
	}

	const auto scoreForPickingBiggestValveFirst = calcPossibleScore(valveOpenOrder, turns);

	auto scoreForPickingCurrentValveFirst = 0;

	if (valvesToOpen.contains(currentNodeName)) {
		// this also assumes there are no duplicate flow rates
		valveOpenOrder.erase(std::ranges::find(valveOpenOrder, map.at(currentNodeName).flow_rate));
		valveOpenOrder.insert(valveOpenOrder.begin(), map.at(currentNodeName).flow_rate);

		scoreForPickingCurrentValveFirst = calcPossibleScore(valveOpenOrder, turns + 1);
	}

	return currentLowerBound + std::max(scoreForPickingBiggestValveFirst, scoreForPickingCurrentValveFirst);
}

void SolutionData::updateUpperBound(const std::map<std::string, Node>& map) {
	scoreUpperBound = calcScoreUpperBound(map, scoreLowerBound, currentNode, valvesStillToOpen, 30 - minute);

}

int main(int argc, char* argv[]) {
	std::ifstream inputFile("inputs/day16.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto start = std::chrono::high_resolution_clock::now();

	auto inputIntoLines = std::string_view{ input } | std::views::split("\n"sv) | std::views::transform([](auto rng) { return std::string_view(rng.begin(), rng.end()); }) | std::views::filter([](auto str) { return !str.empty(); });

	std::map<std::string, Node> graph;
	std::set<std::string> valvesToOpen;

	const std::regex line_regex(R"(^Valve (..) has flow rate=(\d+); tunnels? leads? to valves? (.*)$)");

	for (auto line : inputIntoLines) {
		auto str = std::string{ line };

		std::smatch results;
		std::regex_match(str, results, line_regex);

		auto nodeName = results[1].str();
		auto flowRate = std::stoi(results[2].str());
		auto nextNodesStr = results[3].str();

		auto nextNodes = nextNodesStr | std::views::split(", "s) | std::views::transform([](auto rng) { return std::string(rng.begin(), rng.end()); }) | std::ranges::to<std::vector>();

		graph.emplace(nodeName, Node{ flowRate, nextNodes });

		if (flowRate != 0) {
			valvesToOpen.emplace(nodeName);
		}
	}

	std::vector<SolutionData> wipSolutions;
	wipSolutions.emplace_back(SolutionData{ 0, 0, calcScoreUpperBound(graph, 0, "AA", valvesToOpen, 30), "AA", "", {}, valvesToOpen});

	for (int i = 0; i < 30; i++) {
		std::vector<SolutionData> newSolutions;

		// for each solution
		//   create a new solution for each possible option, updating the upper and lower bound scores
		// find the highest lower bound
		// if any solution has a upper bound less than the highest lower bound, remove it.

		int lowestLowerBound = std::numeric_limits<int>::max();
		int highestLowerBound = 0;
		int lowestUpperBound = std::numeric_limits<int>::max();
		int highestUpperBound = 0;

		for (const auto& solution : wipSolutions) {
			lowestLowerBound = std::min(lowestLowerBound, solution.scoreLowerBound);
			highestLowerBound = std::max(highestLowerBound, solution.scoreLowerBound);
			highestUpperBound = std::max(highestUpperBound, solution.scoreUpperBound);
			lowestUpperBound = std::min(lowestUpperBound, solution.scoreUpperBound);

			// handle the case where we have finished
			if (solution.valvesStillToOpen.empty()) {
				auto newSolution = solution;
				newSolution.minute++;
				newSolutions.push_back(newSolution);
				continue;
			}

			// if the valve is unopened, open it
			if (solution.valvesStillToOpen.contains(solution.currentNode)) {
				auto newSolution = solution;
				newSolution.openValves.emplace(solution.currentNode);
				newSolution.valvesStillToOpen.erase(solution.currentNode);

				newSolution.minute++;
				newSolution.scoreLowerBound += graph.at(solution.currentNode).flow_rate * (30 - newSolution.minute);
				newSolution.updateUpperBound(graph);
				lowestUpperBound = std::min(lowestUpperBound, solution.scoreUpperBound);
				newSolution.prevNode.erase();

				highestLowerBound = std::max(highestLowerBound, newSolution.scoreLowerBound);

				newSolutions.push_back(newSolution);
			}

			for (auto nextNode : graph.at(solution.currentNode).nextNodes) {
				if (nextNode == solution.prevNode) {
					continue;
				}

				auto newSolution = solution;

				newSolution.minute++;
				newSolution.currentNode = nextNode;
				newSolution.prevNode = solution.currentNode;

				newSolution.updateUpperBound(graph);
				lowestUpperBound = std::min(lowestUpperBound, solution.scoreUpperBound);
				newSolutions.push_back(newSolution);
			}
		}

		auto solutionsBeforeRemove = newSolutions.size();

		std::erase_if(newSolutions, [highestLowerBound](auto s) {
			return s.scoreUpperBound < highestLowerBound;
		});

		auto solutionsAfterRemove = newSolutions.size();

		fmt::print("Round {:2} LL: {:5} HL: {:5} LH: {:5} HH: {:5} #Sol: {:7} #Rem: {:7}\n", i, lowestLowerBound, highestLowerBound, lowestUpperBound, highestUpperBound, newSolutions.size(),
			solutionsBeforeRemove - solutionsAfterRemove);

		wipSolutions = newSolutions;
	}

	auto highestScore = std::ranges::max(wipSolutions | std::views::transform([](auto s) { return s.scoreLowerBound; }));
	
	auto end = std::chrono::high_resolution_clock::now();

	fmt::print("Part 1, highest score {}\n", highestScore);

	auto dur = end - start;
	fmt::print("Took {}\n", dur);

	return 0;
}
