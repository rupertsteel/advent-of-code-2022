
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

#include <indicators/progress_bar.hpp>

#include <boost/container/flat_set.hpp>

using namespace std::string_view_literals;
using namespace std::string_literals;

// constexpr int numRounds = 30; // part 1
constexpr int numRounds = 26; // part 2

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

	std::string agent2CurrentNode;
	std::string agent2PrevNode;

	//std::set<std::string> openValves;
	//std::set<std::string> valvesStillToOpen;
	boost::container::flat_set<std::string> openValves;
	boost::container::flat_set<std::string> valvesStillToOpen;

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

int calcScoreUpperBoundPart1(const std::map<std::string, Node>& map, int currentLowerBound, const std::string& currentNodeName, const std::set<std::string>& valvesToOpen, int turns) {
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

int calcScoreUpperBound(const std::map<std::string, Node>& map, int currentLowerBound, const std::string& currentNodeName, const boost::container::flat_set<std::string>& valvesToOpen, int turns) {
	// part 2 upper bound
	// lets just make a simpler calculation, since not allowing agents to move to their previous node without doing something leads to the biggest reduction in the number of nodes.

	if (valvesToOpen.empty()) {
		return currentLowerBound;
	}

	std::vector<int> valveOpenOrder;
	for (auto& valve : valvesToOpen) {
		valveOpenOrder.push_back(map.at(valve).flow_rate);
	}

	std::ranges::sort(valveOpenOrder, std::ranges::greater());

	// If I want to make this faster, I can do the following:

	// if both agents are at valves, and both are best valves
	// give both +1 move

	// if agent 1 at best valve, agent 2 not at valve, give agent 1 +1 move

	// if agent 2 at best valve, agent 2 not at valve, give agent 2 +1 move

	// if agent 1 at best valve, agent 2 at valve
	// +1 move agent 1, +1 move agent 2 & make agent 2 valve second
	// +1 move agent 1, standard agent 2

	// if agent 2 at best valve, agent 1 at valve
	// +1 move agent 2, +1 move agent 1 & make agent 1 valve second
	// +1 move agent 2, standard agent 1

	// if agent 1 at 2nd best valve, agent 2 not at valve
	// +1 move agent 1, 2nd best valve first
	// standard

	// if agent 2 at 2nd best valve, agent 1 not at valve
	// +1 move agent 2, 2nd best valve first
	// standard

	// if agent 1 at 2nd best valve, agent 2 at valve
	// +1 move agent 1, +1 move agent 2, current valves first
	// +1 move agent 1, agent 1 valve first, standard agent 2
	// standard

	// if agent 2 at 2nd best valve, agent 1 at valve
	// +1 move agent 2, +1 move agent 1, current valves first
	// +1 move agent 2, agent 2 valve first, standard agent 1
	// standard

	// both agents at valves
	// +1 move agent 1, +1 move agent 2, current valves first
	// +1 move agent 1, agent 1 valve first
	// +1 move agent 2, agent 2 valve first
	// standard

	// agent 1 at valve
	// +1 move agent 1, agent 1 valve first
	// standard

	// agent 2 at valve
	// +1 move agent 2, agent 2 valve first
	// standard

	// otherwise standard

	// lest imagine both agents are at the best nodes to open next,
	// so we give them turns + 1

	int score = 0;

	int calcRounds = turns + 1;

	while (!valveOpenOrder.empty() && calcRounds > 2) {
		score += valveOpenOrder.front() * (calcRounds - 2);
		valveOpenOrder.erase(valveOpenOrder.begin());

		if (!valveOpenOrder.empty()) {
			score += valveOpenOrder.front() * (calcRounds - 2);
			valveOpenOrder.erase(valveOpenOrder.begin());
		}

		calcRounds -= 2;
	}

	return currentLowerBound + score;
}

void SolutionData::updateUpperBound(const std::map<std::string, Node>& map) {
	scoreUpperBound = calcScoreUpperBound(map, scoreLowerBound, currentNode, valvesStillToOpen, numRounds - minute);

}

int main(int argc, char* argv[]) {
	std::ifstream inputFile("inputs/day16.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto start = std::chrono::high_resolution_clock::now();

	auto inputIntoLines = std::string_view{ input } | std::views::split("\n"sv) | std::views::transform([](auto rng) { return std::string_view(rng.begin(), rng.end()); }) | std::views::filter([](auto str) { return !str.empty(); });

	std::map<std::string, Node> graph;
	boost::container::flat_set<std::string> valvesToOpen;

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

	std::vector<std::unique_ptr<SolutionData>> wipSolutions;
	wipSolutions.emplace_back(std::make_unique<SolutionData>(SolutionData{ 0, 0, calcScoreUpperBound(graph, 0, "AA", valvesToOpen, numRounds), "AA", "", "AA", "", {}, valvesToOpen }));

	for (int i = 0; i < numRounds; i++) {
		std::vector<std::unique_ptr<SolutionData>> newSolutions;

		// for each solution
		//   create a new solution for each possible option, updating the upper and lower bound scores
		// find the highest lower bound
		// if any solution has a upper bound less than the highest lower bound, remove it.

		int lowestLowerBound = std::numeric_limits<int>::max();
		int highestLowerBound = 0;
		int lowestUpperBound = std::numeric_limits<int>::max();
		int highestUpperBound = 0;

		size_t solutionsRemovedBeforeAdd = 0;

		using namespace indicators;
		ProgressBar bar{
			option::BarWidth{60},
			option::ForegroundColor{Color::white},
			option::MaxProgress{wipSolutions.size()}
		};

		//for (const auto& solution : wipSolutions) {
		for (size_t j = 0; j < wipSolutions.size(); j++) {
			auto& solution = *wipSolutions[j];

			lowestLowerBound = std::min(lowestLowerBound, solution.scoreLowerBound);
			highestLowerBound = std::max(highestLowerBound, solution.scoreLowerBound);
			highestUpperBound = std::max(highestUpperBound, solution.scoreUpperBound);
			lowestUpperBound = std::min(lowestUpperBound, solution.scoreUpperBound);
			
			// handle the case where we have finished
			if (solution.valvesStillToOpen.empty()) {
				auto newSolution = solution;
				newSolution.minute++;
				newSolutions.push_back(std::make_unique<SolutionData>(newSolution));
				continue;
			}

			// handle both agents open
			{
				if (solution.valvesStillToOpen.contains(solution.currentNode) && solution.valvesStillToOpen.contains(solution.agent2CurrentNode) && solution.currentNode != solution.agent2CurrentNode) {
					// open both valves
					auto newSolution = solution;
					newSolution.openValves.emplace(solution.currentNode);
					newSolution.valvesStillToOpen.erase(solution.currentNode);
					newSolution.openValves.emplace(solution.agent2CurrentNode);
					newSolution.valvesStillToOpen.erase(solution.agent2CurrentNode);

					newSolution.minute++;
					newSolution.scoreLowerBound += graph.at(solution.currentNode).flow_rate * (numRounds - newSolution.minute);
					newSolution.scoreLowerBound += graph.at(solution.agent2CurrentNode).flow_rate * (numRounds - newSolution.minute);

					newSolution.updateUpperBound(graph);
					lowestUpperBound = std::min(lowestUpperBound, solution.scoreUpperBound);
					newSolution.prevNode.erase();
					newSolution.agent2PrevNode.erase();

					highestLowerBound = std::max(highestLowerBound, newSolution.scoreLowerBound);

					newSolutions.push_back(std::make_unique<SolutionData>(newSolution));
				}
			}
			// handle agent 1 open, agent 2 move
			{
				if (solution.valvesStillToOpen.contains(solution.currentNode)) {
					for (auto nextNode : graph.at(solution.agent2CurrentNode).nextNodes) {
						// if there is only 1 valve to open, allow agents to travel anywhere to prevent them from getting stuck
						if (nextNode == solution.agent2PrevNode && solution.valvesStillToOpen.size() > 1) {
							continue;
						}

						auto newSolution = solution;
						newSolution.openValves.emplace(solution.currentNode);
						newSolution.valvesStillToOpen.erase(solution.currentNode);

						newSolution.agent2CurrentNode = nextNode;
						newSolution.agent2PrevNode = solution.agent2CurrentNode;

						newSolution.minute++;
						newSolution.scoreLowerBound += graph.at(solution.currentNode).flow_rate * (numRounds - newSolution.minute);
						newSolution.updateUpperBound(graph);

						if (newSolution.scoreUpperBound < highestLowerBound) {
							solutionsRemovedBeforeAdd++;
							continue;
						}

						lowestUpperBound = std::min(lowestUpperBound, solution.scoreUpperBound);
						newSolution.prevNode.erase();

						highestLowerBound = std::max(highestLowerBound, newSolution.scoreLowerBound);

						newSolutions.push_back(std::make_unique<SolutionData>(newSolution));
					}
				}
			}

			// handle agent 1 move, agent 2 open
			{
				if (solution.valvesStillToOpen.contains(solution.agent2CurrentNode)) {
					for (auto nextNode : graph.at(solution.currentNode).nextNodes) {
						if (nextNode == solution.prevNode && solution.valvesStillToOpen.size() > 1) {
							continue;
						}

						auto newSolution = solution;
						newSolution.openValves.emplace(solution.agent2CurrentNode);
						newSolution.valvesStillToOpen.erase(solution.agent2CurrentNode);

						newSolution.minute++;
						newSolution.scoreLowerBound += graph.at(solution.agent2CurrentNode).flow_rate * (numRounds - newSolution.minute);
						newSolution.updateUpperBound(graph);

						newSolution.currentNode = nextNode;
						newSolution.prevNode = solution.currentNode;

						if (newSolution.scoreUpperBound < highestLowerBound) {
							solutionsRemovedBeforeAdd++;
							continue;
						}

						lowestUpperBound = std::min(lowestUpperBound, solution.scoreUpperBound);
						newSolution.prevNode.erase();

						highestLowerBound = std::max(highestLowerBound, newSolution.scoreLowerBound);

						newSolutions.push_back(std::make_unique<SolutionData>(newSolution));
					}
				}
			}

			// handle both agents move
			{
				for (auto agent1NextNode : graph.at(solution.currentNode).nextNodes) {
					if (agent1NextNode == solution.prevNode && solution.valvesStillToOpen.size() > 1) {
						continue;
					}

					for (auto agent2NextNode : graph.at(solution.agent2CurrentNode).nextNodes) {
						if (agent2NextNode == solution.agent2PrevNode && solution.valvesStillToOpen.size() > 1) {
							continue;
						}

						auto newSolution = solution;

						newSolution.minute++;
						newSolution.currentNode = agent1NextNode;
						newSolution.prevNode = solution.currentNode;
						newSolution.agent2CurrentNode = agent2NextNode;
						newSolution.agent2PrevNode = solution.agent2CurrentNode;

						newSolution.updateUpperBound(graph);

						if (newSolution.scoreUpperBound < highestLowerBound) {
							solutionsRemovedBeforeAdd++;
							continue;
						}

						lowestUpperBound = std::min(lowestUpperBound, solution.scoreUpperBound);
						newSolutions.push_back(std::make_unique<SolutionData>(newSolution));
					}
				}
			}

			
			if (j % 1000 == 0) {
				bar.set_progress(j);
				bar.set_option(option::PostfixText{ std::to_string(j) + "/" + std::to_string(wipSolutions.size()) });
			}
		}

		bar.set_option(option::PostfixText{ std::to_string(wipSolutions.size()) + "/" + std::to_string(wipSolutions.size()) });
		bar.mark_as_completed();

		auto solutionsBeforeRemove = newSolutions.size();

		std::erase_if(newSolutions, [highestLowerBound](auto& s) {
			return s->scoreUpperBound < highestLowerBound;
		});

		auto solutionsAfterRemove = newSolutions.size();

		fmt::print("Round {:2} LL: {:5} HL: {:5} LH: {:5} HH: {:5} #Sol: {:7} #Rem: {:7}\n", i, lowestLowerBound, highestLowerBound, lowestUpperBound, highestUpperBound, newSolutions.size(),
			solutionsBeforeRemove - solutionsAfterRemove + solutionsRemovedBeforeAdd);

		wipSolutions = std::move(newSolutions);
	}

	auto highestScore = std::ranges::max(wipSolutions | std::views::transform([](auto& s) { return s->scoreLowerBound; }));
	
	auto end = std::chrono::high_resolution_clock::now();

	fmt::print("Part 1, highest score {}\n", highestScore);

	auto dur = end - start;
	fmt::print("Took {}\n", dur);

	return 0;
}
