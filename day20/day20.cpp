
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

template<typename T>
struct VectorWrapIterator {
	using Base = std::vector<T>::iterator;

	using difference_type = Base::difference_type;
	using value_type = Base::value_type;

	VectorWrapIterator() : it(), cont(nullptr) {}

	VectorWrapIterator(Base startIt, std::vector<T>& cont) : cont(&cont) {
		if (startIt == cont.end()) {
			it = cont.begin();
		} else {
			it = startIt;
		}
	}

	VectorWrapIterator(const VectorWrapIterator&) = default;
	VectorWrapIterator& operator=(const VectorWrapIterator&) = default;

	VectorWrapIterator(VectorWrapIterator&&) = default;
	VectorWrapIterator& operator=(VectorWrapIterator&&) = default;

	T& operator*() const {
		return *it;
	}
	T* operator->() const {
		return &(*it);
	}

	VectorWrapIterator& operator++() {
		++it;
		if (it == cont->end()) {
			it = cont->begin();
		}
		return *this;
	}
	VectorWrapIterator operator++(int) {
		auto self = *this;
		operator++();
		return self;
	}

	VectorWrapIterator& operator--() {
		if (it == cont->begin()) {
			it = std::prev(cont->end());
		} else {
			--it;
		}

		return *this;
	}
	VectorWrapIterator operator--(int) {
		auto self = *this;
		operator--();
		return self;
	}

	Base base() {
		return it;
	}

	bool operator==(const VectorWrapIterator& o) const = default;

private:
	Base it;
	std::vector<T>* cont;
};

static_assert(std::bidirectional_iterator<VectorWrapIterator<int>>);

struct Element {
	int64_t value;
	int64_t uniqueId;

	bool operator==(const Element&) const = default;
};

int64_t signMod(int64_t value, size_t size) {

	if (value < 0) {
		return -(std::abs(value) % size);
	} else {
		return value % size;
	}

}

int main(int argc, char* argv[]) {
	std::ifstream inputFile("inputs/day20.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto start = std::chrono::high_resolution_clock::now();

	auto inputIntoLines = std::string_view{ input } | std::views::split("\n"sv) | std::views::transform([](auto rng) { return std::string(rng.begin(), rng.end()); }) | std::views::filter([](auto str) { return !str.empty(); });

	std::vector<Element> elements;
	std::vector<Element> elementsToMix;

	for (auto line : inputIntoLines) {
		uint64_t num = std::stoi(line);

		num *= 811589153;

		Element element{ num, elements.size() };

		elements.push_back(element);

		elementsToMix.push_back(element);
	}

	for (int i = 0; i < 10; i++) {
		for (auto elemToMove : elementsToMix) {
			// get the value

			auto elemIt = std::find(elements.begin(), elements.end(), elemToMove);

			auto replaceIt = elements.erase(elemIt);

			VectorWrapIterator<Element> it(replaceIt, elements);

			if (elemToMove.value < 0) {
				auto toMove = signMod(elemToMove.value, elements.size());

				std::advance(it, toMove);

				if (it.base() == elements.begin()) {
					elements.insert(elements.end(), elemToMove);
				} else {
					elements.insert(it.base(), elemToMove);
				}
			} else {
				auto toMove = elemToMove.value % elements.size();

				std::advance(it, toMove);

				elements.insert(it.base(), elemToMove);
			}
		}
	}

	auto itOfZero = std::find_if(elements.begin(), elements.end(), [](auto e) {return e.value == 0; });

	VectorWrapIterator<Element> it(itOfZero, elements);

	int64_t quality = 0;

	for (int i = 0; i < 3; i++) {
		std::advance(it, 1000);

		fmt::print("Val {}\n", it->value);

		quality += it->value;
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
