
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

using namespace std::string_view_literals;

std::string makeFileString(const std::vector<std::string_view>& parentPath, std::string_view itemPath) {
	std::string outStr = "";

	auto v = parentPath | std::views::join_with("/"sv);

	std::ranges::copy(v, std::back_inserter(outStr));

	outStr.push_back('/');
	outStr.append(itemPath);

	return outStr;
}

std::vector<std::string> getAllDirectories(std::string str) {
	std::vector<std::string> returnVec;


	do {
		auto pos = str.find_last_of('/');
		str.erase(pos, str.size());

		returnVec.push_back(str + "/");
	} while (str.find('/') != std::string::npos);

	return returnVec;
}

int main(int argc, char* argv[]) {
	std::ifstream inputFile("inputs/day7.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto start = std::chrono::high_resolution_clock::now();

	auto inputIntoLines = std::string_view{ input } | std::views::split("\n"sv) | std::views::transform([](auto rng) { return std::string_view(rng.begin(), rng.end()); }) | std::views::filter([](auto str) { return !str.empty(); });


	// get file sizes
	std::vector<std::string_view> currentPath;
	currentPath.push_back("");

	std::map<std::string, uint64_t> fileSizeMap;

	bool inLsListing = false;

	for (auto line : inputIntoLines) {
		if (inLsListing && line.starts_with('$')) {
			inLsListing = false;
		}
		if (inLsListing) {
			if (!line.starts_with("dir")) {
				auto split = line.find(' ');
				auto sizeSv = line.substr(0, split);
				auto itemSv = line.substr(split + 1);

				auto size = std::stoi(std::string{ sizeSv });

				fileSizeMap[makeFileString(currentPath, itemSv)] = size;
			}
		}

		if (line == "$ cd /") {
			currentPath.clear();
			currentPath.push_back("");
		} else if (line == "$ cd ..") {
			currentPath.pop_back();
		} else if (line.starts_with("$ cd")) {
			auto pushPath = line.substr(5);
			currentPath.push_back(pushPath);
		} else if (line == "$ ls") {
			inLsListing = true;
		}
	}


	// get directory sizes
	std::map<std::string, uint64_t> directorySizeMap;

	for (const auto& file : fileSizeMap) {
		// get all of the directories it is part of
		const auto allDirectoriesOfFile = getAllDirectories(file.first);

		for (auto& directory : allDirectoriesOfFile) {
			directorySizeMap[directory] += file.second;
		}
	}

	uint64_t totalSize = 0;
	for (auto& directory : directorySizeMap) {
		if (directory.second <= 100000) {
			totalSize += directory.second;
		}
	}

	auto diskUsage = directorySizeMap.at("/");
	auto diskFree = 70000000 - diskUsage;
	auto diskNeeded = 30000000 - diskFree;

	fmt::print("diskUsage:  {}\n", diskUsage);
	fmt::print("diskFree:   {}\n", diskFree);
	fmt::print("diskNeeded: {}\n", diskNeeded);

	std::map<std::string, uint64_t>::const_pointer smallestDirOverTarget = nullptr;
	for (auto it = directorySizeMap.begin(); it != directorySizeMap.end(); ++it) {
		if (it->second >= diskNeeded) {
			if (smallestDirOverTarget == nullptr) {
				smallestDirOverTarget = &(*it);
			} else if (it->second < smallestDirOverTarget->second) {
				smallestDirOverTarget = &(*it);
			}
		}
	}

	auto end = std::chrono::high_resolution_clock::now();

	fmt::print("Part 1: {}\n", totalSize);

	if (smallestDirOverTarget) {
		fmt::print("Part 2: {}    ---  {}\n", smallestDirOverTarget->first, smallestDirOverTarget->second);
	}


	auto dur = end - start;
	fmt::print("Took {}\n", dur);

	return 0;
}