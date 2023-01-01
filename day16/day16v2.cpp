
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
	std::map<std::string, std::vector<std::string>> nextNodesMap;
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

		auto nextNodes = nextNodesStr | std::views::split(", "s) | std::views::transform([](auto rng) { return std::string(rng.begin(), rng.end()); }) | std::ranges::to<std::vector>();
		m.nextNodesMap.emplace(nodeName, nextNodes);

		m.nameToId.emplace(nodeName, m.graph.size());

		if (flowRate != 0) {
			m.valvesToOpen |= (1ull << m.graph.size());
		}
		m.graph.emplace_back(Node{ flowRate, {} });
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
			returnStr += fmt::format("    {} -> {};\n", nodeName, nextNode);
		}
	}

	returnStr += "}\n";

	return returnStr;
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

	auto end = std::chrono::high_resolution_clock::now();


	auto dur = end - start;

	auto durSecs = std::chrono::duration_cast<std::chrono::duration<float>>(dur);

	fmt::print("Took {}\n", dur);
	fmt::print("Or {}\n", durSecs);

	return 0;
}
