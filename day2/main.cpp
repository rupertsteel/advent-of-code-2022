
#include <fmt/format.h>
#include <iostream>
#include <fstream>
#include <ranges>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>

#include <fmt/ranges.h>

using namespace std::string_view_literals;

int main(int argc, char* argv[]) {
	std::ifstream inputFile("inputs/day2.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto inputIntoLines = std::string_view{ input } | std::views::split("\n"sv) | std::views::transform([](auto rng) { return std::string_view(rng.begin(), rng.end()); }) | std::views::filter([](auto str) { return !str.empty(); });

	fmt::print("Processed 1: {}\n", inputIntoLines);


	auto inputIntoGamePairs = inputIntoLines | std::views::transform([](std::string_view sv) {
		auto left = sv[0] - 'A' + 1;

		auto right = sv[2] - 'X' + 1;

		return std::make_pair(left, right);
	});

	auto calcGameScoresFollowingStrategy = inputIntoGamePairs | std::views::transform([](std::pair<int, int> game) {
		bool isDraw = game.first == game.second;

		bool isWin = (game == std::make_pair(1, 2)) || (game == std::make_pair(2, 3)) || (game == std::make_pair(3, 1));

		return game.second + (isDraw ? 3 : 0) + (isWin ? 6 : 0);
	});

	auto totalScore = std::accumulate(calcGameScoresFollowingStrategy.begin(), calcGameScoresFollowingStrategy.end(), 0);

	fmt::print("Part 1, Score: {}", totalScore);

	return 0;
}