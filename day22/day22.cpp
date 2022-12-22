
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

struct ConnectionMapping {
	int toTile;

	int newFacingDirection;
};


// Test map
//   0
// 123
//   45



// My map:
//  01
//  2
// 34
// 5



constexpr int tileSize = 4;
// constexpr int tileSize = 50;

struct Tile {
	int originalX;
	int originalY;

	char cubeFace = ' ';

	std::array<std::array<Cell, tileSize>, tileSize> cells;

	std::array<std::array<std::optional<int>, tileSize>, tileSize> lastMovementDirection;

	std::array<std::optional<ConnectionMapping>, 4> connectionMappings; // uses facing direction
};


void printGrid(const std::map<int, Tile>& tiles, const std::map<std::pair<int, int>, int>& tileXyMapping) {
	constexpr int mapWidth = 4; // test
	constexpr int mapHeight = 3; // test

	// constexpr int mapWidth = 3;
	// constexpr int mapHeight = 5;

	std::string printStr;
	printStr.reserve((mapWidth * tileSize + 1) * mapHeight * tileSize + 10);

	for (int y = 0; y < tileSize * mapHeight; y++) {
		for (int x = 0; x < tileSize * mapWidth; x++) {
			auto tileY = y / tileSize;
			auto tileX = x / tileSize;

			if (tileXyMapping.contains({tileX, tileY})) {
				auto tile = tileXyMapping.at({tileX, tileY});

				// print the tile cell
				auto inTileX = x % tileSize;
				auto inTileY = y % tileSize;

				auto directionVal = tiles.at(tile).lastMovementDirection[inTileY][inTileX].value_or(-1);

				if (directionVal == 0) {
					printStr.push_back('>');
				} else if (directionVal == 1) {
					printStr.push_back('V');
				} else if (directionVal == 2) {
					printStr.push_back('<');
				} else if (directionVal == 3) {
					printStr.push_back('^');
				} else if (tiles.at(tile).cells[inTileY][inTileX] == Cell::Wall) {
					printStr.push_back('#');
				} else {
					printStr.push_back('.');
				}
			} else {
				printStr.push_back(' ');
			}
		}

		printStr.push_back('\n');
	}

	fmt::print("{}\n\n", printStr);
}

int main(int argc, char* argv[]) try {
	std::ifstream inputFile("inputs/day22-test.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto start = std::chrono::high_resolution_clock::now();

	auto inputIntoLines = std::string_view{ input } | std::views::split("\n"sv) | std::views::transform([](auto rng) { return std::string(rng.begin(), rng.end()); }) | std::views::filter([](auto str) { return !str.empty(); });

	std::vector<std::vector<Cell>> map;

	std::map<std::pair<int, int>, int> tileXyToId;

	std::map<int, Tile> tiles;

	std::vector<Instruction> instructions;

	int loadY = 0;

	for (auto line : inputIntoLines) {
		if (line.starts_with(' ') || line.starts_with('.') || line.starts_with('#')) {
			// map line

			for (int i = 0; i < line.size(); i += tileSize) {
				if (line[i] == ' ') {
					continue;
				}

				int tileX = i / tileSize;
				int tileY = loadY / tileSize;

				if (!tileXyToId.contains({tileX, tileY})) {
					tileXyToId[{tileX, tileY}] = tileXyToId.size();
				}

				auto tileId = tileXyToId[{tileX, tileY}];
				if (!tiles.contains(tileId)) {
					tiles[tileId].originalX = i;
					tiles[tileId].originalY = loadY;
				}

				auto inTileY = loadY - tiles[tileId].originalY;

				for (int inTileX = 0; inTileX < tileSize; inTileX++) {
					auto ch = line[i + inTileX];

					if (ch == '.') {
						tiles[tileId].cells[inTileY][inTileX] = Cell::Empty;
					} else {
						tiles[tileId].cells[inTileY][inTileX] = Cell::Wall;
					}
				}
			}

			loadY++;

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


	tiles[0].cubeFace = 'F';

	std::set<char> setCubeFaces = { 'F' };

	// add all the current links between tiles
	for (auto& [xy, id] : tileXyToId) {
		// check the directions,

		if (tileXyToId.contains({xy.first + 1, xy.second})) {
			auto neighbourId = tileXyToId[{xy.first + 1, xy.second}];

			tiles[id].connectionMappings[0] = {
				neighbourId,
				0
			};
		}

		if (tileXyToId.contains({ xy.first, xy.second + 1})) {
			auto neighbourId = tileXyToId[{xy.first, xy.second + 1}];

			tiles[id].connectionMappings[1] = {
				neighbourId,
				1
			};
		}

		if (tileXyToId.contains({ xy.first - 1, xy.second })) {
			auto neighbourId = tileXyToId[{xy.first - 1, xy.second}];

			tiles[id].connectionMappings[2] = {
				neighbourId,
				2
			};
		}

		if (tileXyToId.contains({ xy.first, xy.second - 1 })) {
			auto neighbourId = tileXyToId[{xy.first, xy.second - 1}];

			tiles[id].connectionMappings[3] = {
				neighbourId,
				3
			};
		}
	}


	int yPos = 0;
	int xPos = 0;
	int tile = 0;
	int facing = 0;

	auto wrapPos = [&tiles](int tile, int xPos, int yPos, int facing) -> std::tuple<int, int, int, int> {
		int newXPos = xPos;
		int newYPos = yPos;

		if (facing == 0) {
			newXPos++;
		} else if (facing == 1) {
			newYPos++;
		} else if (facing == 2) {
			newXPos--;
		} else {
			newYPos--;
		}

		if (newXPos >= 0 && newXPos < tileSize && newYPos >= 0 && newYPos < tileSize) {
			return { tile, newXPos, newYPos, facing };
		}

		// so the facing determines the z coord on the new face (z is x or way, maybe with a negative to make things wrap correctly)

		auto nextTile = tiles[tile].connectionMappings[facing].value().toTile;
		auto nextFacing = tiles[tile].connectionMappings[facing].value().newFacingDirection;

		int newX;
		int newY;

		throw std::runtime_error("Not implemented");
		

		if (newXPos >= tileSize) {
			// handle right edge
		} else if (newYPos >= tileSize) {
			// handle bottom edge
		} else if (newXPos < 0) {
			// handle left edge
		} else {
			// handle top edge
		}

		return { 0, 0, 0, 0 };
	};

	auto cellFree = [&tiles](int tile, int xPos, int yPos) -> bool {
		return tiles[tile].cells[yPos][xPos] == Cell::Empty;
	};

	tiles[tile].lastMovementDirection[yPos][xPos] = facing;

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

			tiles[tile].lastMovementDirection[yPos][xPos] = facing;
		} else {
			auto dist = std::get<0>(ins);

			try {
				for (int i = 0; i < dist; i++) {
					auto [nextTile, nextX, nextY, nextFacing] = wrapPos(tile, xPos, yPos, facing);

					if (!cellFree(tile, nextX, nextY)) {
						break;
					}

					tile = nextTile;
					xPos = nextX;
					yPos = nextY;
					facing = nextFacing;

					tiles[tile].lastMovementDirection[yPos][xPos] = facing;
				}

				printGrid(tiles, tileXyToId);
			} catch (std::exception& e) {
				printGrid(tiles, tileXyToId);

				throw;
			}
		}


	}



	auto password = (tiles[tile].originalY + yPos + 1) * 1000 + (tiles[tile].originalX + xPos + 1) * 4 + facing;

	auto end = std::chrono::high_resolution_clock::now();

	fmt::print("Part 1: password {}\n", password);

	auto dur = end - start;

	auto durSecs = std::chrono::duration_cast<std::chrono::duration<float>>(dur);

	fmt::print("Took {}\n", dur);
	fmt::print("Or {}\n", durSecs);

	return 0;
} catch (std::exception& e) {
	fmt::print("Exception {}\n", e.what());
}
