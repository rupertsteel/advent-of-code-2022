
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

#include "core.hpp"

using namespace std::string_view_literals;
using namespace std::string_literals;

// constexpr int numRounds = 30; // part 1
constexpr int numRounds = 26; // part 2

struct Node {
	int flow_rate;
	std::vector<int> nextNodes;
};

struct SolutionData {
	int minute;
	int scoreLowerBound;
	int scoreUpperBound;
	int currentNode;
	int prevNode;

	int agent2CurrentNode;
	int agent2PrevNode;

	std::uint64_t openValves;
	std::uint64_t valvesStillToOpen;

	void updateUpperBound(const std::vector<Node>& map);
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

int calcScoreUpperBoundInternal(const std::vector<int>& openOrder, std::optional<int> agent1ReplacementFlowRate, std::optional<int> agent2ReplacementFlowRate, int turns) {
	int score = 0;

	// 0 is move, 1 is open
	int agent1CurrentAction = agent1ReplacementFlowRate ? 1 : 0;
	int agent2CurrentAction = agent2ReplacementFlowRate ? 1 : 0;

	std::optional<int> skipFlowRate1;
	std::optional<int> skipFlowRate2;

	std::span stillToOpen{openOrder};

	for (int i = 0; i < turns && !stillToOpen.empty(); i++) {
		if (agent1CurrentAction == 0) {
			agent1CurrentAction = 1;
		} else {
			// if we have a open override, do that first
			if (agent1ReplacementFlowRate) {
				// open this valve now
				score += *agent1ReplacementFlowRate * (turns - i - 1);
				skipFlowRate1 = agent1ReplacementFlowRate;
				agent1ReplacementFlowRate.reset();

				agent1CurrentAction = 0;
			} else {
				// see if there is anything to open
				// skipping the items we need to skip
				while (!stillToOpen.empty()) {
					auto rate = stillToOpen.front();
					if (skipFlowRate1.has_value() && skipFlowRate1 == rate) {
						stillToOpen = stillToOpen.subspan(1);
						continue;
					}
					if (skipFlowRate2.has_value() && skipFlowRate2 == rate) {
						stillToOpen = stillToOpen.subspan(1);
						continue;
					}

					stillToOpen = stillToOpen.subspan(1);

					score += rate * (turns - i - 1);
					agent1CurrentAction = 0;
					break;
				}
			}
		}
		if (agent2CurrentAction == 0) {
			agent2CurrentAction = 1;
		} else {
			// if we have a open override, do that first
			if (agent2ReplacementFlowRate) {
				// open this valve now
				score += *agent2ReplacementFlowRate * (turns - i - 1);
				skipFlowRate2 = agent2ReplacementFlowRate;
				agent2ReplacementFlowRate.reset();

				agent2CurrentAction = 0;
			} else {
				// see if there is anything to open
				// skipping the items we need to skip
				while (!stillToOpen.empty()) {
					auto rate = stillToOpen.front();
					if (skipFlowRate1.has_value() && skipFlowRate1 == rate) {
						stillToOpen = stillToOpen.subspan(1);
						continue;
					}
					if (skipFlowRate2.has_value() && skipFlowRate2 == rate) {
						stillToOpen = stillToOpen.subspan(1);
						continue;
					}

					stillToOpen = stillToOpen.subspan(1);

					score += rate * (turns - i - 1);
					agent2CurrentAction = 0;
					break;
				}
			}
		}
	}

	return score;
}

bool hasBitSet(uint64_t bitset, int bitNum) {
	return !!(bitset & (1ull << bitNum));
}

int calcScoreUpperBound(const std::vector<Node>& map, int currentLowerBound, int agent1CurrentNode, int agent2CurrentNode, std::uint64_t valvesToOpen, int turns) {
	// part 2 upper bound
	// lets just make a simpler calculation, since not allowing agents to move to their previous node without doing something leads to the biggest reduction in the number of nodes.

	if (valvesToOpen == 0) {
		return currentLowerBound;
	}

	// and handle only 1 valve left
	if (std::popcount(valvesToOpen) == 1) {
		if (hasBitSet(valvesToOpen, agent1CurrentNode) || hasBitSet(valvesToOpen, agent2CurrentNode)) {
			// open it now
			return currentLowerBound + map.at(std::countr_zero(valvesToOpen)).flow_rate * (turns - 1);
		}

		// open it next turn
		return currentLowerBound + map.at(std::countr_zero(valvesToOpen)).flow_rate * (turns - 2);
	}

	static std::vector<int> valveOpenOrder;
	valveOpenOrder.clear();
	core::TrueBitRange bitRange{ valvesToOpen };
	for (auto valve : bitRange) {
		valveOpenOrder.push_back(map.at(valve).flow_rate);
	}

	std::ranges::sort(valveOpenOrder, std::ranges::greater());

	// Each agent can do the following
	// C1: Move          -> Open(best) -> Move       -> ...
	// C1: Open(current) -> Move       -> Open(best) -> ... // if at a valve

	// We need to combine this for two agents

	// handle (C1, C1)
	auto bestScore = calcScoreUpperBoundInternal(valveOpenOrder, std::nullopt, std::nullopt, turns);

	
	// handle (C2, C1)
	if (map.at(agent1CurrentNode).flow_rate != 0) {
		bestScore = std::max(bestScore, calcScoreUpperBoundInternal(valveOpenOrder, map.at(agent1CurrentNode).flow_rate, std::nullopt, turns));
	}

	// handle (C1, C2)
	if (map.at(agent2CurrentNode).flow_rate != 0 && agent1CurrentNode != agent2CurrentNode) {
		bestScore = std::max(bestScore, calcScoreUpperBoundInternal(valveOpenOrder, std::nullopt, map.at(agent2CurrentNode).flow_rate, turns));
	}

	// handle (C2, C2)
	if (map.at(agent1CurrentNode).flow_rate != 0 && map.at(agent2CurrentNode).flow_rate != 0 && agent1CurrentNode != agent2CurrentNode) {
		bestScore = std::max(bestScore, calcScoreUpperBoundInternal(valveOpenOrder, map.at(agent1CurrentNode).flow_rate, map.at(agent2CurrentNode).flow_rate, turns));
	}

	return currentLowerBound + bestScore;


	// lest imagine both agents are at the best nodes to open next,
	// so we give them turns + 1

	/*int score = 0;

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

	return currentLowerBound + score;*/
}

void SolutionData::updateUpperBound(const std::vector<Node>& map) {
	scoreUpperBound = calcScoreUpperBound(map, scoreLowerBound, currentNode, agent2CurrentNode, valvesStillToOpen, numRounds - minute);

}

struct Move {
	bool openValve;
	int move;
};

void getMoves(std::vector<Move>& moves, const std::vector<Node>& nodes, int currentNode, int prevNode, std::uint64_t toOpen) {


	if (toOpen & 1ull << currentNode) {
		moves.emplace_back(true, -1);
	}

	for (auto next : nodes[currentNode].nextNodes) {
		if (next == prevNode && std::popcount(toOpen) > 1) {
			continue;
		}

		moves.emplace_back(false, next);
	}
}

int main(int argc, char* argv[]) {
	std::ifstream inputFile("inputs/day16.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto start = std::chrono::high_resolution_clock::now();

	auto inputIntoLines = std::string_view{ input } | std::views::split("\n"sv) | std::views::transform([](auto rng) { return std::string_view(rng.begin(), rng.end()); }) | std::views::filter([](auto str) { return !str.empty(); });

	std::vector<Node> graph;
	std::map<std::string, int> nameToId;
	std::map<std::string, std::vector<std::string>> nextNodesMap;
	std::uint64_t valvesToOpen = 0;

	const std::regex line_regex(R"(^Valve (..) has flow rate=(\d+); tunnels? leads? to valves? (.*)$)");

	for (auto line : inputIntoLines) {
		auto str = std::string{ line };

		std::smatch results;
		std::regex_match(str, results, line_regex);

		auto nodeName = results[1].str();
		auto flowRate = std::stoi(results[2].str());
		auto nextNodesStr = results[3].str();

		auto nextNodes = nextNodesStr | std::views::split(", "s) | std::views::transform([](auto rng) { return std::string(rng.begin(), rng.end()); }) | std::ranges::to<std::vector>();
		nextNodesMap.emplace(nodeName, nextNodes);

		nameToId.emplace(nodeName, graph.size());

		if (flowRate != 0) {
			valvesToOpen |= (1ull << graph.size());
		}
		graph.emplace_back(Node{ flowRate, {} });
	}

	for (auto& elem : nextNodesMap) {
		auto& src = graph[nameToId[elem.first]];

		for (auto& target : elem.second) {
			src.nextNodes.push_back(nameToId[target]);
		}
	}

	std::vector<std::unique_ptr<SolutionData>> wipSolutions;
	wipSolutions.emplace_back(std::make_unique<SolutionData>(SolutionData{ 0, 0, calcScoreUpperBound(graph, 0, nameToId.at("AA"), nameToId.at("AA"), valvesToOpen, numRounds), nameToId.at("AA"), -1, nameToId.at("AA"), -1, {}, valvesToOpen}));

	std::vector<std::jthread> cleanupThreads;

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
			option::MaxProgress{wipSolutions.size()},
			option::ShowElapsedTime{true},
			option::ShowRemainingTime{true}
		};

		//for (const auto& solution : wipSolutions) {
		for (size_t j = 0; j < wipSolutions.size(); j++) {
			auto& solution = *wipSolutions[j];

			lowestLowerBound = std::min(lowestLowerBound, solution.scoreLowerBound);
			highestLowerBound = std::max(highestLowerBound, solution.scoreLowerBound);
			highestUpperBound = std::max(highestUpperBound, solution.scoreUpperBound);
			lowestUpperBound = std::min(lowestUpperBound, solution.scoreUpperBound);
			
			// handle the case where we have finished
			if (solution.valvesStillToOpen == 0) {
				auto newSolution = solution;
				newSolution.minute++;
				newSolutions.push_back(std::make_unique<SolutionData>(newSolution));
				continue;
			}

			static std::vector<Move> meMoves;
			static std::vector<Move> elephantMoves;
			meMoves.clear();
			elephantMoves.clear();

			getMoves(meMoves, graph, solution.currentNode, solution.prevNode, solution.valvesStillToOpen);
			getMoves(elephantMoves, graph, solution.agent2CurrentNode, solution.agent2PrevNode, solution.valvesStillToOpen);

			for (auto& myMove : meMoves) {
				for (auto& elephantMove : elephantMoves) {
					if (myMove.openValve && elephantMove.openValve && solution.currentNode == solution.agent2CurrentNode) {
						continue; // we can't both open valves
					}
					if (solution.currentNode == solution.agent2CurrentNode && elephantMove.openValve) {
						continue; // prefer me to open valves when we both can
					}
					if (solution.currentNode == solution.agent2CurrentNode && !myMove.openValve) {
						// perfer me moving to the lowest numbered next valve (we can both travel in the same direction though)
						if (myMove.move > elephantMove.move) {
							continue;
						}
					}

					auto newSolution = solution;
					newSolution.minute++;

					if (myMove.openValve) {
						newSolution.openValves |= (1ull << solution.currentNode);
						newSolution.valvesStillToOpen ^= 1ull << solution.currentNode;
						newSolution.scoreLowerBound += graph.at(solution.currentNode).flow_rate * (numRounds - newSolution.minute);
						newSolution.prevNode = -1;
					} else {
						newSolution.currentNode = myMove.move;
						newSolution.prevNode = solution.currentNode;
					}

					if (elephantMove.openValve) {
						newSolution.openValves |= (1ull << solution.agent2CurrentNode);
						newSolution.valvesStillToOpen ^= 1ull << solution.agent2CurrentNode;
						newSolution.scoreLowerBound += graph.at(solution.agent2CurrentNode).flow_rate * (numRounds - newSolution.minute);
						newSolution.agent2PrevNode = -1;
					} else {
						newSolution.agent2CurrentNode = elephantMove.move;
						newSolution.agent2PrevNode = solution.agent2CurrentNode;
					}

					newSolution.updateUpperBound(graph);
					lowestUpperBound = std::min(lowestUpperBound, solution.scoreUpperBound);

					highestLowerBound = std::max(highestLowerBound, newSolution.scoreLowerBound);

					if (newSolution.scoreUpperBound < highestLowerBound) {
						solutionsRemovedBeforeAdd++;
						continue;
					}

					newSolutions.push_back(std::make_unique<SolutionData>(newSolution));
				}
			}

			if ((j & 0x1FFFF) == 0) {
				bar.set_progress(j);
				bar.set_option(option::PostfixText{ std::to_string(j) + "/" + std::to_string(wipSolutions.size()) });
			}
		}

		bar.set_option(option::PostfixText{ std::to_string(wipSolutions.size()) + "/" + std::to_string(wipSolutions.size()) });
		bar.set_progress(wipSolutions.size());
		

		auto solutionsBeforeRemove = newSolutions.size();

		std::erase_if(newSolutions, [highestLowerBound](auto& s) {
			return s->scoreUpperBound < highestLowerBound;
		});

		auto solutionsAfterRemove = newSolutions.size();

		fmt::print("Round {:2} LL: {:5} HL: {:5} LH: {:5} HH: {:5} #Sol: {:7} #Rem: {:7}\n", i, lowestLowerBound, highestLowerBound, lowestUpperBound, highestUpperBound, newSolutions.size(),
			solutionsBeforeRemove - solutionsAfterRemove + solutionsRemovedBeforeAdd);

		cleanupThreads.emplace_back([](auto vec) {}, std::move(wipSolutions));

		wipSolutions = std::move(newSolutions);
	}


	auto end = std::chrono::high_resolution_clock::now();

	if (!wipSolutions.empty()) {
		auto highestScore = std::ranges::max(wipSolutions | std::views::transform([](auto& s) { return s->scoreLowerBound; }));

		fmt::print("Part 1, highest score {}\n", highestScore);
	}

	auto dur = end - start;

	auto durSecs = std::chrono::duration_cast<std::chrono::duration<float>>(dur);

	fmt::print("Took {}\n", dur);
	fmt::print("Or {}\n", durSecs);

	return 0;
}
