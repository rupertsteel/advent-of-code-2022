
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

struct Point {
	int x;
	int y;

	std::strong_ordering operator<=>(const Point&) const = default;
	bool operator==(const Point&) const = default;
};

enum class Direction {
	Up,
	Right,
	Down,
	Left
};

struct Blizzard {
	Point position;

	Direction dir;
};

enum class Cell {
	Empty,
	Wall,
	Start,
	End
};

struct Map {
	std::vector<Cell> cells;

	int width;
	int height;

	Point start;
	Point dest;

	std::vector<Blizzard> blizzards;
	int blizzardTime = 0;

	std::map<int, std::set<Point>> blizzardPointsAtTime;

	int distStartToEnd;

	Cell getCell(const Point& point) const {
		auto i = (point.y * width) + point.x;
		return cells[i];
	}

	void addNextStep() {
		std::set<Point> currentPoints;
		for (auto& blizzard : blizzards) {
			currentPoints.insert(blizzard.position);

			if (blizzard.dir == Direction::Right) {
				blizzard.position.x++;
				if (blizzard.position.x >= (width - 1)) {
					blizzard.position.x = 1;
				}
			}
			if (blizzard.dir == Direction::Down) {
				blizzard.position.y++;
				if (blizzard.position.y >= (height - 1)) {
					blizzard.position.y = 1;
				}
			}
			if (blizzard.dir == Direction::Left) {
				blizzard.position.x--;
				if (blizzard.position.x < 1) {
					blizzard.position.x = (width - 2);
				}
			}
			if (blizzard.dir == Direction::Up) {
				blizzard.position.y--;
				if (blizzard.position.y < 1) {
					blizzard.position.y = (height - 2);
				}
			}
		}

		blizzardPointsAtTime[blizzardTime] = currentPoints;
		blizzardTime++;
	}
};

struct Path {
	int currentTime;
	Point currentPos;
	int estEndTime;

	bool reachedEndFirst;
	bool reachedStartAgain;

	std::vector<Point> positions;

	std::strong_ordering operator<=>(const Path& r) const {
		auto c1 = currentTime <=> r.currentTime;
		if (c1 != std::strong_ordering::equal) {
			return c1;
		}

		auto c2 = currentPos <=> r.currentPos;
		if (c2 != std::strong_ordering::equal) {
			return c2;
		}

		auto c3 = estEndTime <=> r.estEndTime;
		if (c3 != std::strong_ordering::equal) {
			return c3;
		}

		auto c4 = reachedEndFirst <=> r.reachedEndFirst;
		if (c4 != std::strong_ordering::equal) {
			return c4;
		}

		return reachedStartAgain <=> r.reachedStartAgain;
	}

	bool operator==(const Path& r) const {
		return currentTime == r.currentTime
			&& currentPos == r.currentPos
			&& estEndTime == r.estEndTime
			&& reachedEndFirst == r.reachedEndFirst
			&& reachedStartAgain == r.reachedStartAgain;
	}

	//std::strong_ordering operator<=>(const Path&) const = default;
	//bool operator==(const Path&) const = default;
};

int manhattenDistance(Point a, Point b) {
	auto distX = std::abs(a.x - b.x);
	auto distY = std::abs(a.y - b.y);

	return distX + distY;
}

void printBoard(const Map& map, const std::set<Point>& blizzards, const Point& pathPosition) {
	bool hasInvalidCells = false;

	std::string printStr;

	for (int y = 0; y < map.height; y++) {
		for (int x = 0; x < map.width; x++) {
			bool hasWall = map.getCell(Point{ x, y }) == Cell::Wall;

			bool hasBlizzard = blizzards.contains(Point{ x, y });

			bool hasPlayer = pathPosition == Point{ x, y };

			bool invalidCell = false;

			if ((hasWall && hasBlizzard) || (hasBlizzard && hasPlayer) || (hasWall && hasPlayer)) {
				invalidCell = true;
			}

			if (invalidCell) {
				printStr += '!';
			} else if (hasWall) {
				printStr += '#';
			} else if (hasBlizzard) {
				printStr += '@';
			} else if (hasPlayer) {
				printStr += 'E';
			} else {
				printStr += '.';
			}
		}

		printStr += '\n';
	}

	if (hasInvalidCells) {
		fmt::print("!! Invalid cells found\n");
	}
	fmt::print("{}\n", printStr);
	
}

int main(int argc, char* argv[]) try {
	std::ifstream inputFile("inputs/day24.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto start = std::chrono::high_resolution_clock::now();

	auto inputIntoLines = std::string_view{ input } | std::views::split("\n"sv) | std::views::transform([](auto rng) { return std::string(rng.begin(), rng.end()); }) | std::views::filter([](auto str) { return !str.empty(); });

	Map map;

	int lineNum = 0;
	for (auto line : inputIntoLines) {
		int numWallsOnLine = 0;
		int column = 0;
		for (auto ch : line) {
			if (ch == '#') {
				map.cells.push_back(Cell::Wall);
				numWallsOnLine++;
			} else if (ch == '.' && lineNum == 0) {
				map.cells.push_back(Cell::Start);
				map.start = Point{ column, lineNum };
			} else if (ch == '.' && numWallsOnLine > 1) {
				map.cells.push_back(Cell::End);
				map.dest = Point{ column, lineNum };
			} else {
				map.cells.push_back(Cell::Empty);
			}

			if (ch == '>') {
				map.blizzards.emplace_back(Point{ column, lineNum }, Direction::Right);
			}
			if (ch == 'v') {
				map.blizzards.emplace_back(Point{ column, lineNum }, Direction::Down);
			}
			if (ch == '<') {
				map.blizzards.emplace_back(Point{ column, lineNum }, Direction::Left);
			}
			if (ch == '^') {
				map.blizzards.emplace_back(Point{ column, lineNum }, Direction::Up);
			}
			
			column++;
		}
		lineNum++;
		map.width = line.size();
	}
	map.height = lineNum;

	map.distStartToEnd = manhattenDistance(map.start, map.dest);

	std::map<int, std::set<Path>> processingPaths;

	auto initialPathEstDistance = manhattenDistance(map.start, map.dest);
	processingPaths[initialPathEstDistance].emplace(0, map.start, initialPathEstDistance, false, false, std::vector{map.start});

	Path fastestPath;

	bool running = true;
	while (running) {
		map.addNextStep();

		auto currentBestPathsIt = processingPaths.lower_bound(0);

		auto paths = std::move(currentBestPathsIt->second);
		processingPaths.erase(currentBestPathsIt);

		for (const auto& path : paths) {
			bool switchToStart = false;
			if (path.currentPos == map.dest && !path.reachedEndFirst) {
				switchToStart = true;
			}
			bool switchToEnd = false;
			if (path.currentPos == map.start && path.reachedEndFirst && !path.reachedStartAgain) {
				switchToEnd = true;
			}

			if (path.currentPos == map.dest && path.reachedEndFirst && path.reachedStartAgain) {
				fmt::print("Found path at dest, time {}\n", path.currentTime);
				running = false;
				fastestPath = path;
			}

			if (map.blizzardPointsAtTime[path.currentTime].contains(path.currentPos)) {
				continue; // path in blizzard, not valid
			}

			auto tryAddPath = [&](int dx, int dy) {
				auto newPath = Path{ path.currentTime + 1, Point{path.currentPos.x + dx, path.currentPos.y + dy}, 0 , path.reachedEndFirst, path.reachedStartAgain, path.positions};

				if (newPath.currentPos.x < 0 || newPath.currentPos.y < 0 || newPath.currentPos.x >= map.width || newPath.currentPos.y >= map.height) {
					return;
				}
				if (map.getCell(newPath.currentPos) == Cell::Wall) {
					return;
				}

				if (switchToStart) {
					newPath.reachedEndFirst = true;
				}
				if (switchToEnd) {
					newPath.reachedStartAgain = true;
				}

				if (!newPath.reachedEndFirst) {
					newPath.estEndTime = newPath.currentTime + manhattenDistance(newPath.currentPos, map.dest);
				} else if (!newPath.reachedStartAgain) {
					newPath.estEndTime = newPath.currentTime + manhattenDistance(newPath.currentPos, map.start);
				} else {
					newPath.estEndTime = newPath.currentTime + manhattenDistance(newPath.currentPos, map.dest);
				}

				newPath.positions.push_back(newPath.currentPos);

				processingPaths[newPath.estEndTime].insert(newPath);
			};

			tryAddPath(0, 0);
			tryAddPath(1, 0);
			tryAddPath(-1, 0);
			tryAddPath(0, 1);
			tryAddPath(0, -1);
		}
	}

	auto end = std::chrono::high_resolution_clock::now();

	for (int i = 0; i <= fastestPath.currentTime; i++) {
		fmt::print("Minute {}\n", i);
		printBoard(map, map.blizzardPointsAtTime.at(i), fastestPath.positions.at(i));
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	auto dur = end - start;

	auto durSecs = std::chrono::duration_cast<std::chrono::duration<float>>(dur);

	fmt::print("Took {}\n", dur);
	fmt::print("Or {}\n", durSecs);

	return 0;
} catch (std::exception& e) {
	fmt::print("Exception {}\n", e.what());
}
