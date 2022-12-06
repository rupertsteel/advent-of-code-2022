
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
	std::ifstream inputFile("inputs/day6.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto start = std::chrono::high_resolution_clock::now();

	auto inputSv = std::string_view{ input };

	auto slide = inputSv | std::views::slide(4) | std::views::transform([](auto rng) {
		for (int i = 0; i < 3; i++) {
			for (int j = i; j < 4; j++) {
				if (i == j) {
					continue;
				}
				if (rng[i] == rng[j]) {
					return false;
				}
			}
		}

		return true;
	});

	auto slide14 = inputSv | std::views::slide(14) | std::views::transform([](auto rng) {
		auto vec = rng | std::ranges::to<std::vector>();

		std::ranges::sort(vec);

		for (auto&& elems : vec | std::views::slide(2)) {
			if (elems[0] == elems[1]) {
				return false;
			}
		}

		return true;
	});

	auto offset = std::ranges::find(slide, true);
	auto offsetCount = std::distance(slide.begin(), offset) + 4;

	auto offset14 = std::ranges::find(slide14, true);
	auto offsetCount14 = std::distance(slide14.begin(), offset14) + 14;

	auto end = std::chrono::high_resolution_clock::now();

	fmt::print("Part 1 offset: {}\n", offsetCount);
	fmt::print("Part 2 offset: {}\n", offsetCount14);

	auto dur = end - start;
	fmt::print("Took {}\n", dur);

	return 0;
}