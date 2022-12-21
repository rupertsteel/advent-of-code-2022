
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

struct Operation {
	std::string destValue;

	std::string leftValue;
	std::string rightValue;

	char op;
};

int main(int argc, char* argv[]) {
	std::ifstream inputFile("inputs/day21.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto start = std::chrono::high_resolution_clock::now();

	auto inputIntoLines = std::string_view{ input } | std::views::split("\n"sv) | std::views::transform([](auto rng) { return std::string(rng.begin(), rng.end()); }) | std::views::filter([](auto str) { return !str.empty(); });

	std::unordered_map<std::string, int64_t> knownNumbers;

	std::list<Operation> operationsToDo;

	const std::regex numberRegex(R"((\w{4}): (\d+))");
	const std::regex operationRegex(R"((\w{4}): (\w{4}) (.) (\w{4}))");

	for (auto line : inputIntoLines) {
		std::smatch matchResults;

		if (std::regex_match(line, matchResults, numberRegex)) {
			knownNumbers[matchResults[1].str()] = std::stoi(matchResults[2].str());
		}
		if (std::regex_match(line, matchResults, operationRegex)) {
			operationsToDo.emplace_back(
				matchResults[1].str(),
				matchResults[2].str(),
				matchResults[4].str(),

				*matchResults[3].first
			);
		}
	}

	while (!operationsToDo.empty()) {
		

		for (auto it = operationsToDo.begin(); it != operationsToDo.end();) {
			if (knownNumbers.contains(it->leftValue) && knownNumbers.contains(it->rightValue)) {
				auto left = knownNumbers[it->leftValue];
				auto right = knownNumbers[it->rightValue];

				long long result = 0;

				if (it->op == '+') {
					result = left + right;
				} else if (it->op == '-') {
					result = left - right;
				} else if (it->op == '*') {
					result = left * right;
				} else if (it->op == '/') {
					result = left / right;
				}

				knownNumbers[it->destValue] = result;

				it = operationsToDo.erase(it);
			} else {
				++it;
			}
		}
	}

	auto end = std::chrono::high_resolution_clock::now();

	fmt::print("Root {}\n", knownNumbers["root"]);

	auto dur = end - start;

	auto durSecs = std::chrono::duration_cast<std::chrono::duration<float>>(dur);

	fmt::print("Took {}\n", dur);
	fmt::print("Or {}\n", durSecs);

	return 0;
}
