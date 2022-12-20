
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

struct ListWrapIterator {
	using Base = std::list<int>::iterator;

	using difference_type = Base::difference_type;
	using value_type = Base::value_type;

	ListWrapIterator() : it(), cont(nullptr) {}

	ListWrapIterator(Base startIt, std::list<int>& cont) : it(startIt), cont(&cont) {}

	ListWrapIterator(const ListWrapIterator&) = default;
	ListWrapIterator& operator=(const ListWrapIterator&) = default;

	ListWrapIterator(ListWrapIterator&&) = default;
	ListWrapIterator& operator=(ListWrapIterator&&) = default;

	int& operator*() const {
		return *it;
	}
	int* operator->() const {
		return &(*it);
	}

	ListWrapIterator& operator++() {
		++it;
		if (it == cont->end()) {
			it = cont->begin();
		}
		return *this;
	}
	ListWrapIterator operator++(int) {
		auto self = *this;
		operator++();
		return self;
	}

	ListWrapIterator& operator--() {
		if (it == cont->begin()) {
			it = std::prev(cont->end());
		} else {
			--it;
		}

		return *this;
	}
	ListWrapIterator operator--(int) {
		auto self = *this;
		operator--();
		return self;
	}

	Base base() {
		return it;
	}

	bool operator==(const ListWrapIterator& o) const = default;

private:
	Base it;
	std::list<int>* cont;
};

static_assert(std::bidirectional_iterator<ListWrapIterator>);

int main(int argc, char* argv[]) {
	std::ifstream inputFile("inputs/day20.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto start = std::chrono::high_resolution_clock::now();

	auto inputIntoLines = std::string_view{ input } | std::views::split("\n"sv) | std::views::transform([](auto rng) { return std::string(rng.begin(), rng.end()); }) | std::views::filter([](auto str) { return !str.empty(); });

	std::list<int> elements;
	std::vector<std::list<int>::iterator> elementsToMix;

	for (auto line : inputIntoLines) {
		auto num = std::stoi(line);

		elements.push_back(num);

		elementsToMix.push_back(std::prev(elements.end()));
	}

	for (auto elemToMove : elementsToMix) {
		// get the value
		auto val = *elemToMove;
		// reduce it a bit

		//auto res = std::div(val, elements.size());

		//val %= elements.size();

		// now we need to move this element this many elements forward (or backward)
		ListWrapIterator it(elemToMove, elements);
		if (val > 0) {
			std::advance(it, val + 1);

			if (it.base() == elements.begin()) {
				elements.splice(elements.end(), elements, elemToMove);
			} else {
				elements.splice(it.base(), elements, elemToMove);
			}
		} else if (val < 0) {
			std::advance(it, val);

			if (it.base() == elements.begin()) {
				elements.splice(elements.end(), elements, elemToMove);
			} else {
				elements.splice(it.base(), elements, elemToMove);
			}
		}
	}

	auto itOfZero = std::find(elements.begin(), elements.end(), 0);

	ListWrapIterator it(itOfZero, elements);

	int quality = 0;

	for (int i = 0; i < 3; i++) {
		std::advance(it, 1000);

		fmt::print("Val {}\n", *it);

		quality += *it;
	}


	auto end = std::chrono::high_resolution_clock::now();

	//fmt::print("List {}\n", elements);
	fmt::print("Part 1: {}\n", quality);

	auto dur = end - start;

	auto durSecs = std::chrono::duration_cast<std::chrono::duration<float>>(dur);

	fmt::print("Took {}\n", dur);
	fmt::print("Or {}\n", durSecs);

	return 0;
}
