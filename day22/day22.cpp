
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

enum class Cell {
	Empty,
	Wall,
	OutOfBounds,
};

enum class Direction {
	Left,
	Right
};

using Instruction = std::variant<int, Direction>;

int main(int argc, char* argv[]) {
	std::ifstream inputFile("inputs/day22.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto start = std::chrono::high_resolution_clock::now();

	auto inputIntoLines = std::string_view{ input } | std::views::split("\n"sv) | std::views::transform([](auto rng) { return std::string(rng.begin(), rng.end()); }) | std::views::filter([](auto str) { return !str.empty(); });

	std::vector<std::vector<Cell>> map;

	std::vector<Instruction> instructions;

	for (auto line : inputIntoLines) {
		if (line.starts_with(' ') || line.starts_with('.') || line.starts_with('#')) {
			// map line
			std::vector<Cell> mapLine;

			for (auto ch : line) {
				if (ch == ' ') {
					mapLine.push_back(Cell::OutOfBounds);
				} else if (ch == '.') {
					mapLine.push_back(Cell::Empty);
				} else {
					mapLine.push_back(Cell::Wall);
				}
			}

			map.push_back(mapLine);
		} else {
			// instruction line
			std::string_view insSv = line;

			while (!insSv.empty()) {
				if (insSv[0] == 'L') {
					instructions.push_back(Direction::Left);
					insSv = insSv.substr(1);
				} else if (insSv[0] == 'R') {
					instructions.push_back(Direction::Right);
					insSv = insSv.substr(1);
				} else {
					auto numEnd = insSv.find_first_not_of("0123456789");

					if (numEnd == std::string_view::npos) {
						std::string tempStr{ insSv.begin(), insSv.end() };
						auto num = std::stoi(tempStr);
						instructions.push_back(num);

						insSv = std::string_view{};
					} else {

						std::string tempStr{ insSv.begin(), insSv.begin() + numEnd };
						auto num = std::stoi(tempStr);
						instructions.push_back(num);

						insSv = insSv.substr(numEnd);
					}

					
				}
			}
		}
	}


	int yPos = 0;
	int xPos = std::find(map[0].begin(), map[0].end(), Cell::Empty) - map[0].begin();
	int facing = 0;

	auto wrapPos = [&map](int xPos, int yPos, int deltaX, int deltaY) -> std::pair<int, int> {
		xPos += deltaX;
		yPos += deltaY;

		if (deltaY == 0) {
			while (true) {
				if (xPos < 0) {
					xPos = map[yPos].size() - 1;
				}
				if (xPos >= map[yPos].size()) {
					xPos = 0;
				}

				if (map[yPos][xPos] == Cell::OutOfBounds) {
					xPos += deltaX;
					continue;
				}

				return { xPos, yPos };
			}
		} else {
			while (true) {
				if (yPos < 0) {
					yPos = map.size() - 1;
				}
				if (yPos >= map.size()) {
					yPos = 0;
				}

				if (xPos < map[yPos].size() && map[yPos][xPos] != Cell::OutOfBounds) {
					return { xPos, yPos };
				}

				yPos += deltaY;
			}
		}
	};

	auto cellFree = [&map](int xPos, int yPos) -> bool {
		return map[yPos][xPos] == Cell::Empty;
	};

	for (auto ins : instructions) {
		if (ins.index() == 1) {
			if (std::get<1>(ins) == Direction::Left) {
				facing -= 1;
				if (facing < 0) {
					facing = 3;
				}
			} else {
				facing += 1;
				if (facing > 3) {
					facing = 0;
				}
			}
		} else {
			auto dist = std::get<0>(ins);

			int deltaX;
			int deltaY;

			if (facing == 0) {
				deltaX = 1;
				deltaY = 0;
			} else if (facing == 1) {
				deltaX = 0;
				deltaY = 1;
			} else if (facing == 2) {
				deltaX = -1;
				deltaY = 0;
			} else {
				deltaX = 0;
				deltaY = -1;
			}

			for (int i = 0; i < dist; i++) {
				auto [nextX, nextY] = wrapPos(xPos, yPos, deltaX, deltaY);

				if (!cellFree(nextX, nextY)) {
					break;
				}

				xPos = nextX;
				yPos = nextY;
			}
		}
	}

	auto passWord = (yPos + 1) * 1000 + (xPos + 1) * 4 + facing;

	auto end = std::chrono::high_resolution_clock::now();

	fmt::print("Part 1: password {}\n", passWord);

	auto dur = end - start;

	auto durSecs = std::chrono::duration_cast<std::chrono::duration<float>>(dur);

	fmt::print("Took {}\n", dur);
	fmt::print("Or {}\n", durSecs);

	return 0;
}
