
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


#include <fmt/ranges.h>
#include <fmt/chrono.h>

#include <gsl/gsl-lite.hpp>

using namespace std::string_view_literals;

struct visability {
	bool top = false;
	bool right = false;
	bool bot = false;
	bool left = false;
};

int outSizeVisabilityMap(const std::vector<int> data, int width, int length) {
	std::vector<visability> visabilityMap(data.size());

	auto arrLookup = [width](auto& vec, int x, int y) -> auto& {
		auto offset = y * width + x;

		return vec[offset];
	};

	// top down
	for (int i = 0; i < width; i++) {
		int currentHeight = -1;

		for (int j = 0; j < length; j++) {
			if (arrLookup(data, i, j) > currentHeight) {
				currentHeight = arrLookup(data, i, j);
				arrLookup(visabilityMap, i, j).top = true;
			}

			if (currentHeight == 9) {
				break;
			}
		}
	}

	// right left
	for (int i = 0; i < length; i++) {
		int currentHeight = -1;

		for (int j = width - 1; j >= 0; j--) {
			if (arrLookup(data, j, i) > currentHeight) {
				currentHeight = arrLookup(data, j, i);
				arrLookup(visabilityMap, j, i).right = true;
			}

			if (currentHeight == 9) {
				break;
			}
		}
	}

	// bot top
	for (int i = 0; i < width; i++) {
		int currentHeight = -1;

		for (int j = length - 1; j >= 0; j--) {
			if (arrLookup(data, i, j) > currentHeight) {
				currentHeight = arrLookup(data, i, j);
				arrLookup(visabilityMap, i, j).bot = true;
			}

			if (currentHeight == 9) {
				break;
			}
		}
	}

	// left right
	for (int i = 0; i < length; i++) {
		int currentHeight = -1;

		for (int j = 0; j < width; j++) {
			if (arrLookup(data, j, i) > currentHeight) {
				currentHeight = arrLookup(data, j, i);
				arrLookup(visabilityMap, j, i).left = true;
			}

			if (currentHeight == 9) {
				break;
			}
		}
	}

	return std::ranges::count_if(visabilityMap, [](auto v) {
		return v.top || v.right || v.bot || v.left;
	});
}

int insideTreehouseScore(const std::vector<int>& data, int width, int length) {
	std::vector<visability> visabilityMap(data.size());

	auto arrLookup = [width](auto& vec, int x, int y) -> auto& {
		auto offset = y * width + x;

		return vec[offset];
	};

	auto bestScore = 0;

	for (int x = 0; x < width; x++) {
		for (int y = 0; y < length; y++) {

			auto heightOfSampleTree = arrLookup(data, x, y);

			int leftViewingDistance = 0;
			for (auto leftSample = x - 1; leftSample >= 0; leftSample--) {
				auto testHeight = arrLookup(data, leftSample, y);

				if (testHeight < heightOfSampleTree) {
					leftViewingDistance++;
				} else {
					leftViewingDistance++;
					break;
				}
			}

			int rightViewingDistance = 0;
			for (auto rightSample = x + 1; rightSample < width; rightSample++) {
				auto testHeight = arrLookup(data, rightSample, y);

				if (testHeight < heightOfSampleTree) {
					rightViewingDistance++;
				}
				else {
					rightViewingDistance++;
					break;
				}
			}

			int topViewingDistance = 0;
			for (auto topSample = y - 1; topSample >= 0; topSample--) {
				auto testHeight = arrLookup(data, x, topSample);

				if (testHeight < heightOfSampleTree) {
					topViewingDistance++;
				}
				else {
					topViewingDistance++;
					break;
				}
			}

			int botViewingDistance = 0;
			for (auto botSample = y + 1; botSample < length; botSample++) {
				auto testHeight = arrLookup(data, x, botSample);

				if (testHeight < heightOfSampleTree) {
					botViewingDistance++;
				}
				else {
					botViewingDistance++;
					break;
				}
			}

			auto treeScore = leftViewingDistance * rightViewingDistance * botViewingDistance * topViewingDistance;

			bestScore = std::max(bestScore, treeScore);
		}
	}

	return bestScore;
}

int main(int argc, char* argv[]) {
	std::ifstream inputFile("inputs/day8.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto start = std::chrono::high_resolution_clock::now();

	auto inputIntoLines = std::string_view{ input } | std::views::split("\n"sv) | std::views::transform([](auto rng) { return std::string_view(rng.begin(), rng.end()); }) | std::views::filter([](auto str) { return !str.empty(); });

	std::vector<int> data;
	data.reserve(input.size());
	int width;
	int length = 0;

	for (auto line : inputIntoLines) {
		length++;
		width = line.size();

		for (auto ch : line) {
			data.push_back(ch - '0');
		}
	}

	auto countVisible = outSizeVisabilityMap(data, width, length);

	auto bestTreeScore = insideTreehouseScore(data, width, length);

	auto end = std::chrono::high_resolution_clock::now();

	fmt::print("Part 1, count visible: {}\n", countVisible);
	fmt::print("Part 2, best tree score: {}\n", bestTreeScore);


	auto dur = end - start;
	fmt::print("Took {}\n", dur);

	return 0;
}
