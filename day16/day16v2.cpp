
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
#include <filesystem>
#include <stack>
#include <queue>

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

struct NodeLink {
	int nextNode;
	int weight;
};

struct NodeNameLink {
	std::string nextNodeName;
	int weight;
};

struct Node {
	int flow_rate;
	std::vector<NodeLink> nextNodes;
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

bool hasBitSet(uint64_t bitset, int bitNum) {
	return !!(bitset & (1ull << bitNum));
}

struct Move {
	bool openValve;
	bool endWait;
	int move;
	int time;
};

struct SimpleMap {
	std::map<std::string, std::vector<NodeNameLink>> nextNodesMap;
	std::map<std::string, int> flowRates;
};

struct AlgorithmMap {
	std::vector<Node> graph;
	std::map<std::string, int> nameToId;
	
	std::uint64_t valvesToOpen = 0;
};

SimpleMap readMap(std::filesystem::path filePath) {
	SimpleMap m;

	std::ifstream inputFile(filePath);

	if (!inputFile) {
		throw std::runtime_error("Failed to open file");
	}

	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto inputIntoLines = std::string_view{ input } | std::views::split("\n"sv) | std::views::transform([](auto rng) { return std::string_view(rng.begin(), rng.end()); }) | std::views::filter([](auto str) { return !str.empty(); });

	const std::regex line_regex(R"(^Valve (..) has flow rate=(\d+); tunnels? leads? to valves? (.*)$)");

	for (auto line : inputIntoLines) {
		auto str = std::string{ line };

		std::smatch results;
		std::regex_match(str, results, line_regex);

		auto nodeName = results[1].str();
		auto flowRate = std::stoi(results[2].str());
		auto nextNodesStr = results[3].str();

		auto nextNodes = nextNodesStr | std::views::split(", "s) | std::views::transform([](auto rng) { return NodeNameLink{ std::string(rng.begin(), rng.end()), 1 }; }) | std::ranges::to<std::vector>();
		m.nextNodesMap.emplace(nodeName, nextNodes);

		m.flowRates.emplace(nodeName, flowRate);
	}

	return m;
}

std::string mapToGraphviz(const SimpleMap& map) {
	std::string returnStr = "digraph G {\n";

	for (auto& [nodeName, flowRate] : map.flowRates) {
		if (flowRate > 0) {
			returnStr += fmt::format("    {0:} [style=\"filled\", color=\"lightgrey\", label=\"{0:}\\nflowrate={1:}\"];\n", nodeName, flowRate);
		} else {
			returnStr += fmt::format("    {} [];\n", nodeName);
		}
	}

	returnStr += "\n\n";

	for (auto& [nodeName, linkedNodes]: map.nextNodesMap) {
		for (auto& nextNode : linkedNodes) {
			returnStr += fmt::format("    {} -> {} [label=\"weight={}\"];\n", nodeName, nextNode.nextNodeName, nextNode.weight);
		}
	}

	returnStr += "}\n";

	return returnStr;
}

SimpleMap simplifyMapJoinNodesAndRenumber(const SimpleMap& input, int maxTime) {
	// use direct links
	// sort node ids by score

	std::set<std::string> sourceIds;
	std::set<std::string> destIds;

	sourceIds.insert("AA");

	for (auto& [name, flowRate] : input.flowRates) {
		if (flowRate > 0) {
			sourceIds.insert(name);
			destIds.insert(name);
		}
	}

	// from every source id, we breadth first search, tracking the depth, when we reach a dest id, we stop and record the distance
	// note, this means that going A -> B -> C can be faster than A -> C, we only track direct distance

	std::map<std::string, std::vector<NodeNameLink>> graphNewNodeLinks;

	for (auto src : sourceIds) {
		std::map<std::string, int> nodeToDist;
		std::stack<std::pair<std::string, int>> nodesToProcess;

		std::vector<NodeNameLink> newLinks;

		nodeToDist[src] = 0;
		for (auto srcNext : input.nextNodesMap.at(src)) {
			nodesToProcess.emplace(srcNext.nextNodeName, 1);
		}

		while (!nodesToProcess.empty()) {
			auto nextNode = nodesToProcess.top();
			nodesToProcess.pop();

			if (nodeToDist.contains(nextNode.first)) {
				continue;
			}

			nodeToDist[nextNode.first] = nextNode.second;

			if (destIds.contains(nextNode.first)) {
				// save the distance
				if (nextNode.second >= maxTime) {
					// For big inputs only:
					// this link means we would spend our entire time traveling along this, so we can't actually move here,
					// so skip
					// note, if a input has time = maxTime, we also can't do anything, as we need a turn to change the valve
					continue;
				}

				newLinks.emplace_back(NodeNameLink{ nextNode.first, nextNode.second });
			} else {
				for (auto links : input.nextNodesMap.at(nextNode.first)) {
					nodesToProcess.emplace(links.nextNodeName, nextNode.second + 1);
				}
			}
		}

		graphNewNodeLinks[src] = newLinks;
	}

	SimpleMap simplifiedMap;

	simplifiedMap.nextNodesMap = graphNewNodeLinks;

	for (auto node : sourceIds) {
		simplifiedMap.flowRates.emplace(node, input.flowRates.at(node));
	}

	return simplifiedMap;
}

SimpleMap simplifyMapRemoveNodesCantAccessInTime(const SimpleMap& input, int maxTime) {
	SimpleMap newMap = input;

	std::map<std::string, int> nodeAccessTimes;
	std::queue<std::tuple<std::string, std::string, int>> nodesToProcess;

	nodeAccessTimes.emplace("AA", 0);

	for (auto& next : input.nextNodesMap.at("AA")) {
		nodesToProcess.emplace("AA", next.nextNodeName, next.weight);
	}

	while (!nodesToProcess.empty()) {
		auto next = nodesToProcess.front();
		nodesToProcess.pop();

		auto newTime = nodeAccessTimes[std::get<0>(next)] + std::get<2>(next);

		bool timeUpdated = false;

		if (!nodeAccessTimes.contains(std::get<1>(next))) {
			nodeAccessTimes.emplace(std::get<1>(next), newTime);
			timeUpdated = true;
		} else if (newTime < nodeAccessTimes[std::get<1>(next)]) {
			nodeAccessTimes.emplace(std::get<1>(next), newTime);
			timeUpdated = true;
		}

		if (timeUpdated) {
			for (auto& nextLinks : input.nextNodesMap.at(std::get<1>(next))) {
				nodesToProcess.emplace(std::get<1>(next), nextLinks.nextNodeName, nextLinks.weight);
			}
		}
	}

	std::set<std::string> linksToRemove;

	for (auto& [nodeName, nodeAccessTime] : nodeAccessTimes) {
		if (nodeAccessTime >= maxTime) {
			fmt::print("Node removed {}, access time {}\n", nodeName, nodeAccessTime);
			newMap.nextNodesMap.erase(nodeName);
			newMap.flowRates.erase(nodeName);
			linksToRemove.emplace(nodeName);
		}
	}

	for (auto& [_, links] : newMap.nextNodesMap) {
		std::erase_if(links, [&](auto& link) {
			return linksToRemove.contains(link.nextNodeName);
		});
	}

	return newMap;
}

SimpleMap removeUnreachableNodes(const SimpleMap& input) {
	SimpleMap newMap = input;

	std::set<std::string> reachableNodes;
	std::stack<std::string> nodesToCheck;
	nodesToCheck.emplace("AA");

	while (!nodesToCheck.empty()) {
		auto toCheck = nodesToCheck.top();
		nodesToCheck.pop();

		if (!reachableNodes.contains(toCheck)) {
			reachableNodes.emplace(toCheck);

			for (auto& nextNodes : input.nextNodesMap.at(toCheck)) {
				nodesToCheck.push(nextNodes.nextNodeName);
			}
		}
	}

	std::set<std::string> nodesToRemove;
	for (auto& node : input.flowRates) {
		if (!reachableNodes.contains(node.first)) {
			newMap.flowRates.erase(node.first);
			newMap.nextNodesMap.erase(node.first);
		}
	}

	return newMap;
}

void printMapStats(const SimpleMap& map) {
	auto numNodes = map.flowRates.size();

	auto numLinks = std::transform_reduce(map.nextNodesMap.begin(), map.nextNodesMap.end(), 0ull, std::plus<>{}, [](const auto& vec) {
		return vec.second.size();
	});

	fmt::print("Num nodes {} num links {}\n", numNodes, numLinks);
}

SimpleMap simplifyMap(const SimpleMap& input, int maxTime) {
	printMapStats(input);
	auto m1 = simplifyMapJoinNodesAndRenumber(input, maxTime);

	printMapStats(m1);

	auto m2 = simplifyMapRemoveNodesCantAccessInTime(m1, maxTime);

	printMapStats(m2);

	size_t nodesBefore;
	SimpleMap m3 = m2;

	do {
		nodesBefore = m3.flowRates.size();

		m3 = removeUnreachableNodes(m3);
		
	} while (nodesBefore > m3.flowRates.size());

	printMapStats(m3);

	return m3;
}

AlgorithmMap toAlgorithmMap(const SimpleMap& input) {
	AlgorithmMap algorithmMap;

	std::multimap<int, std::string, std::greater<>> valveWeights;
	for (auto& [nodeName, nodeFlowRate] : input.flowRates) {
		valveWeights.emplace(nodeFlowRate, nodeName);
	}

	if (input.flowRates.size() > 64) {
		throw std::runtime_error("Too many nodes");
	}

	for (auto& [nodeWeight, nodeName] : valveWeights) {
		auto nodeId = algorithmMap.graph.size();
		algorithmMap.nameToId.emplace(nodeName, nodeId);

		algorithmMap.graph.emplace_back(Node{ nodeWeight, {} });

		if (nodeWeight > 0) {
			algorithmMap.valvesToOpen |= (1ull << nodeId);
		}
	}

	if (!algorithmMap.nameToId.contains("AA")) {
		algorithmMap.nameToId.emplace("AA", algorithmMap.graph.size());

		algorithmMap.graph.emplace_back(Node{ 0, {} });
	}

	for (auto& elem : input.nextNodesMap) {
		auto& src = algorithmMap.graph[algorithmMap.nameToId[elem.first]];

		for (auto& target : elem.second) {
			src.nextNodes.emplace_back(algorithmMap.nameToId[target.nextNodeName], target.weight);
		}
	}

	return algorithmMap;
}

struct WorkItem {

	uint64_t scoreLowerBound;
	uint64_t scoreUpperBound;
	int agent1CurrentNode;
	int agent1PrevNode;
	int agent2CurrentNode;
	int agent2PrevNode;
	uint64_t valvesStillToOpen;
	int minute;

	int agent1Time;
	int agent2Time;

	std::strong_ordering operator<=>(const WorkItem& other) const {
		return scoreUpperBound <=> other.scoreUpperBound;
	}
};

uint64_t calcUpperBoundScore(const WorkItem& workItem, const AlgorithmMap& map, int rounds);

void addInitialStates(std::priority_queue<WorkItem>& priorityQueue, const AlgorithmMap& map, int rounds) {
	const auto startNode = map.nameToId.at("AA");

	WorkItem initialState{
		0,
		0, // Will be updated
		startNode,
		-1,
		startNode,
		-1,
		map.valvesToOpen,
		0,
		0,
		0
	};

	initialState.scoreUpperBound = calcUpperBoundScore(initialState, map, rounds);

	priorityQueue.push(initialState);
}

void getMoves(std::vector<Move>& moves, const AlgorithmMap& map, int rounds, int agentCurrentNode, int agentPrevNode, int currentTime, uint64_t valvesToOpen) {
	if (valvesToOpen & (1ull << agentCurrentNode)) {
		moves.push_back(Move{
			true,
			false,
			-1,
			1
		});
	}

	for (const auto& nextNode : map.graph[agentCurrentNode].nextNodes) {
		if (nextNode.nextNode == agentPrevNode) {
			continue;
		}

		if (currentTime + nextNode.weight >= rounds) {
			continue; // can't reach it in time
		}

		moves.push_back(Move{
			false,
			false,
			nextNode.nextNode,
			nextNode.weight
		});
	}

	if (moves.empty()) {
		// add a backup move that waits for 1 turn so the other actor can finish.
		moves.push_back(Move{
			false,
			true,
			-1,
			1
		});
	}
}

void applyMove(const Move& move, const AlgorithmMap& map, WorkItem& updateState, int& agentCurrentNode, int& agentPrevNode, int& agentArrivalTime, int rounds) {
	if (move.openValve) {
		updateState.valvesStillToOpen ^= (1ull << agentCurrentNode);

		agentPrevNode = -1;
		agentArrivalTime++;

		const auto valveFlowRate = map.graph[agentCurrentNode].flow_rate;

		updateState.scoreLowerBound += valveFlowRate * (rounds - agentArrivalTime);
	} else if (!move.endWait) {
		agentPrevNode = agentCurrentNode;
		agentCurrentNode = move.move;
		agentArrivalTime += move.time;
	} else {
		agentArrivalTime++;
	}
}

uint64_t calcUpperBoundScore(const WorkItem& workItem, const AlgorithmMap& map, int rounds) {
	// simple upper bound,
	// we assume that when each agent arrives at a node, it is the highest flow rate unopened node, so it is always optimal
	// to open that node, and the next best nodes are only 1 cost away.

	int agent1ArrivalTime = workItem.agent1Time;
	int agent2ArrivalTime = workItem.agent2Time;

	auto upperBoundScore = workItem.scoreLowerBound;

	auto valvesToOpen = workItem.valvesStillToOpen;

	while (valvesToOpen && agent1ArrivalTime < rounds && agent2ArrivalTime < rounds) {
		const auto nextValveToOpen = std::countr_zero(valvesToOpen);
		const auto valveFlowRate = map.graph[nextValveToOpen].flow_rate;

		valvesToOpen ^= (1ull << nextValveToOpen);

		if (agent1ArrivalTime <= agent2ArrivalTime) {
			// run agent 1

			// it will take us 1 round to open this valve
			// so if rounds is 26, then the valve will be open on turn 1,
			// so score = flow_rate * (rounds - agentArrivalTime - 1)
			upperBoundScore += valveFlowRate * (rounds - agent1ArrivalTime - 1);
			// it will take us 1 round to open the valve, and 1 to move to the next node.
			agent1ArrivalTime += 2;
		} else {
			upperBoundScore += valveFlowRate * (rounds - agent2ArrivalTime - 1);
			agent2ArrivalTime += 2;
		}
	}

	return upperBoundScore;
}

uint64_t highestScoreTwoAgents(const AlgorithmMap& map, int rounds) {
	std::priority_queue<WorkItem> itemsToProcess;

	addInitialStates(itemsToProcess, map, rounds);

	size_t solutionsRemovedBeforeAdd = 0;
	size_t solutionsRemovedOnProcess = 0;

	size_t itemsProcessed = 0;
	uint64_t highestScore = 0;

	while (!itemsToProcess.empty()) {
		auto state = itemsToProcess.top();
		itemsToProcess.pop();

		++itemsProcessed;

		if (state.scoreUpperBound < highestScore) {
			solutionsRemovedOnProcess++;

			if ((itemsProcessed & 0xFFFF) == 0) {
				fmt::print("{} items processed, {} in queue, Best upper bound {}\n", itemsProcessed, itemsToProcess.size(), highestScore);
			}

			continue;
		}

		if (state.minute + 1 == rounds) {
			// no more moves possible
			continue;
		}
		if (state.minute >= rounds) [[unlikely]] {
			fmt::print("Minute over rounds\n");
			return 0;
		}

		// expand future states
		const bool needAgent1States = state.agent1Time <= state.agent2Time;
		const bool needAgent2States = state.agent2Time <= state.agent1Time;

		static std::vector<Move> agent1Moves;
		static std::vector<Move> agent2Moves;

		if (needAgent1States) {
			agent1Moves.clear();
			getMoves(agent1Moves, map, rounds, state.agent1CurrentNode, state.agent1PrevNode, state.minute, state.valvesStillToOpen);
		}
		if (needAgent2States) {
			agent2Moves.clear();
			getMoves(agent2Moves, map, rounds, state.agent2CurrentNode, state.agent2PrevNode, state.minute, state.valvesStillToOpen);
		}

		if (needAgent1States && needAgent2States) {
			for (auto& agent1Move : agent1Moves) {
				for (auto& agent2Move : agent2Moves) {
					if (agent1Move.openValve && agent2Move.openValve && state.agent1CurrentNode == state.agent2CurrentNode) {
						continue; // both agents can't open the same valve
					}
					if (state.agent1CurrentNode == state.agent2CurrentNode && agent2Move.openValve) {
						continue; // prefer agent1 to open valves
					}
					if (state.agent1CurrentNode == state.agent2CurrentNode && !agent1Move.openValve && !agent1Move.endWait && !agent2Move.endWait) {
						// prefer me moving to the lowest numbered next valve (we can both travel in the same direction though)
						if (agent1Move.move > agent2Move.move) {
							continue;
						}
					}

					auto newState = state;

					applyMove(agent1Move, map, newState, newState.agent1CurrentNode, newState.agent1PrevNode, newState.agent1Time, rounds);
					applyMove(agent2Move, map, newState, newState.agent2CurrentNode, newState.agent2PrevNode, newState.agent2Time, rounds);

					newState.minute += std::min(agent1Move.time, agent2Move.time);

					newState.scoreUpperBound = calcUpperBoundScore(newState, map, rounds);

					if (newState.scoreUpperBound < highestScore) {
						solutionsRemovedBeforeAdd++;
						continue;
					}

					highestScore = std::max(highestScore, newState.scoreLowerBound);

					itemsToProcess.push(newState);
				}
			}
		} else if (needAgent1States) {
			for (auto& agent1Move : agent1Moves) {
				auto newState = state;

				applyMove(agent1Move, map, newState, newState.agent1CurrentNode, newState.agent1PrevNode, newState.agent1Time, rounds);

				newState.minute = std::min(newState.minute + agent1Move.time, newState.agent2Time);

				newState.scoreUpperBound = calcUpperBoundScore(newState, map, rounds);

				if (newState.scoreUpperBound < highestScore) {
					solutionsRemovedBeforeAdd++;
					continue;
				}

				highestScore = std::max(highestScore, newState.scoreLowerBound);

				itemsToProcess.push(newState);
			}
		} else if (needAgent2States) {
			for (auto& agent2Move : agent2Moves) {
				auto newState = state;

				applyMove(agent2Move, map, newState, newState.agent2CurrentNode, newState.agent2PrevNode, newState.agent2Time, rounds);

				newState.minute = std::min(newState.minute + agent2Move.time, newState.agent1Time);

				newState.scoreUpperBound = calcUpperBoundScore(newState, map, rounds);

				if (newState.scoreUpperBound < highestScore) {
					solutionsRemovedBeforeAdd++;
					continue;
				}

				highestScore = std::max(highestScore, newState.scoreLowerBound);

				itemsToProcess.push(newState);
			}
		}

		if ((itemsProcessed & 0xFFFF) == 0) {
			fmt::print("{} items processed, {} in queue, Best upper bound {}\n", itemsProcessed, itemsToProcess.size(), highestScore);
		}
	}

	return highestScore;
}

int main(int argc, char* argv[]) {

	std::filesystem::path mapPath;
	if (argc >= 2) {
		mapPath = argv[1];
	} else {
		mapPath = "inputs/day16.txt";
	}

	auto start = std::chrono::high_resolution_clock::now();

	auto map = readMap(mapPath);

	fmt::print("Map has {} nodes\n", map.nextNodesMap.size());

	fmt::print("{}\n", mapToGraphviz(map));

	auto simplifiedMap = simplifyMap(map, numRounds);

	fmt::print("{}\n", mapToGraphviz(simplifiedMap));

	auto algorithmMap = toAlgorithmMap(simplifiedMap);

	auto twoAgentHighestScore = highestScoreTwoAgents(algorithmMap, numRounds);

	auto end = std::chrono::high_resolution_clock::now();

	fmt::print("2 agent result: {}\n", twoAgentHighestScore);

	auto dur = end - start;

	auto durSecs = std::chrono::duration_cast<std::chrono::duration<float>>(dur);

	fmt::print("Took {}\n", dur);
	fmt::print("Or {}\n", durSecs);

	return 0;
}
