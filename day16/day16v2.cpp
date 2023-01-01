
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

struct Map {
	std::vector<Node> graph;
	std::map<std::string, int> nameToId;
	std::map<std::string, std::vector<NodeNameLink>> nextNodesMap;
	std::uint64_t valvesToOpen = 0;
};

Map readMap(std::filesystem::path filePath) {
	Map m;

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

		m.nameToId.emplace(nodeName, m.graph.size());

		if (flowRate != 0) {
			m.valvesToOpen |= (1ull << m.graph.size());
		}
		m.graph.emplace_back(Node{ flowRate, {} });
	}

	for (auto& elem : m.nextNodesMap) {
		auto& src = m.graph[m.nameToId[elem.first]];

		for (auto& target : elem.second) {
			src.nextNodes.emplace_back(m.nameToId[target.nextNodeName], target.weight);
		}
	}

	return m;
}

std::string mapToGraphviz(const Map& map) {
	std::string returnStr = "digraph G {\n";

	for (auto& [nodeName, nodeId] : map.nameToId) {
		auto& node = map.graph[nodeId];

		returnStr += fmt::format("    {} [{}];\n", nodeName, node.flow_rate > 0 ? "style=filled, color=lightgrey" : "");
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

Map simplifyMap(const Map& input) {
	// use direct links
	// sort node ids by score

	std::set<std::string> sourceIds;
	std::set<std::string> destIds;

	sourceIds.insert("AA");

	for (auto& [name, id] : input.nameToId) {
		auto& node = input.graph[id];

		if (node.flow_rate > 0) {
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
				newLinks.emplace_back(NodeNameLink{ nextNode.first, nextNode.second });
			} else {
				for (auto links : input.nextNodesMap.at(nextNode.first)) {
					nodesToProcess.emplace(links.nextNodeName, nextNode.second + 1);
				}
			}
		}

		graphNewNodeLinks[src] = newLinks;
	}

	std::multimap<int, std::string, std::greater<>> valveWeights;
	for (auto& [nodeName, nodeId] : input.nameToId) {
		if (input.graph[nodeId].flow_rate > 0) {
			valveWeights.emplace(input.graph[nodeId].flow_rate, nodeName);
		}
	}

	Map simplifiedMap;

	simplifiedMap.nextNodesMap = graphNewNodeLinks;

	for (auto& [nodeWeight, nodeName] : valveWeights) {
		simplifiedMap.nameToId.emplace(nodeName, simplifiedMap.graph.size());

		simplifiedMap.graph.emplace_back(Node{ nodeWeight, {} });
	}

	if (!simplifiedMap.nameToId.contains("AA")) {
		simplifiedMap.nameToId.emplace("AA", simplifiedMap.graph.size());

		simplifiedMap.graph.emplace_back(Node{ 0, {} });
	}

	for (auto& elem : simplifiedMap.nextNodesMap) {
		auto& src = simplifiedMap.graph[simplifiedMap.nameToId[elem.first]];

		for (auto& target : elem.second) {
			src.nextNodes.emplace_back(simplifiedMap.nameToId[target.nextNodeName], target.weight);
		}
	}

	return simplifiedMap;
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

	fmt::print("Map has {} nodes\n", map.graph.size());

	fmt::print("{}\n", mapToGraphviz(map));

	auto simplifiedMap = simplifyMap(map);

	fmt::print("{}\n", mapToGraphviz(simplifiedMap));

	auto end = std::chrono::high_resolution_clock::now();


	auto dur = end - start;

	auto durSecs = std::chrono::duration_cast<std::chrono::duration<float>>(dur);

	fmt::print("Took {}\n", dur);
	fmt::print("Or {}\n", durSecs);

	return 0;
}
