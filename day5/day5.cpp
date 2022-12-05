
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

#include <fmt/ranges.h>
#include <fmt/chrono.h>

using namespace std::string_view_literals;

struct Command {
	int count;
	int from;
	int to;
};

int main(int argc, char* argv[]) {
	std::ifstream inputFile("inputs/day5.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto start = std::chrono::high_resolution_clock::now();

	auto inputIntoLines = std::string_view{ input } | std::views::split("\n"sv) | std::views::transform([](auto rng) { return std::string_view(rng.begin(), rng.end()); });

	std::vector<std::string_view> stack_input;
	std::vector<std::string_view> command_input;

	bool isInStack = true;

	for (auto line : inputIntoLines) {
		if (line.empty()) {
			isInStack = false;
			continue;
		}

		if (isInStack) {
			stack_input.push_back(line);
		} else {
			command_input.push_back(line);
		}
	}

	auto numStacks = (stack_input[0].length() + 1) / 4;

	std::vector<std::vector<char>> stacks(numStacks);

	// populate the stacks
	auto minStackInput = stack_input | std::views::reverse | std::views::drop(1) | std::views::transform([numStacks](auto sv) {
		std::string returnStr;

		for (int i = 0; i < numStacks; i++) {
			returnStr.push_back(sv[i * 4 + 1]);
		}

		return returnStr;
	});

	for (auto minInputLine : minStackInput) {
		for (int i = 0; i < numStacks; i++) {
			if (minInputLine[i] != ' ') {
				stacks[i].push_back(minInputLine[i]);
			}
		}
	}

	const std::regex command_regex(R"(move (\d+) from (\d+) to (\d+))");

	auto processedCommands = command_input | std::views::transform([command_regex](auto sv) {
		auto str = std::string{ sv };

		std::smatch results;
		std::regex_match(str, results, command_regex);

		auto count = std::stoi(results[1].str());
		auto from = std::stoi(results[2].str()) - 1;
		auto to = std::stoi(results[3].str()) - 1;

		return Command{ count, from, to };
	});

	for (auto command : processedCommands) {
#if 0 // Part 1
		for (int i = 0; i < command.count; i++) {
			if (stacks[command.from].empty()) {
				break;
			}

			stacks[command.to].push_back(stacks[command.from].back());
			stacks[command.from].pop_back();
		}
#endif
		std::vector<char> tempStack;

		for (int i = 0; i < command.count; i++) {
			if (stacks[command.from].empty()) {
				break;
			}

			tempStack.push_back(stacks[command.from].back());
			stacks[command.from].pop_back();
		}

		while (!tempStack.empty()) {
			stacks[command.to].push_back(tempStack.back());
			tempStack.pop_back();
		}
	}

	std::string end_result;

	for (int i = 0; i < numStacks; i++) {
		if (stacks[i].empty()) {
			end_result.push_back(' ');
		} else {
			end_result.push_back(stacks[i].back());
		}
	}

	auto end = std::chrono::high_resolution_clock::now();

	fmt::print("Num stacks {}\n", numStacks);

	fmt::print("Part 1 stacks: {}\n", end_result);

	auto dur = end - start;
	fmt::print("Took {}\n", dur);

	return 0;
}