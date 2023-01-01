
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
	int move;
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

		returnStr += fmt::format("    {} [{}];\n", nodeName, flowRate > 0 ? "style=filled, color=lightgrey" : "");
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

	for (auto& [nodeWeight, nodeName] : valveWeights) {
		algorithmMap.nameToId.emplace(nodeName, algorithmMap.graph.size());

		algorithmMap.graph.emplace_back(Node{ nodeWeight, {} });
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

	auto end = std::chrono::high_resolution_clock::now();


	auto dur = end - start;

	auto durSecs = std::chrono::duration_cast<std::chrono::duration<float>>(dur);

	fmt::print("Took {}\n", dur);
	fmt::print("Or {}\n", durSecs);

	return 0;
}
