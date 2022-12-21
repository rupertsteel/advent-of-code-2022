
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

class TreeNode {
public:
	virtual ~TreeNode() {}

	virtual bool isConst() const = 0;
	virtual bool isValue() const {
		return false;
	}
	virtual bool isHuman() const {
		return false;
	}
	virtual bool isOp() const {
		return false;
	}
	virtual int64_t calculate() const = 0;

	virtual std::string str() const = 0;
};

class ValueTreeNode : public TreeNode {
public:
	ValueTreeNode(int64_t value): value(value) {
	}

	int64_t value;

	bool isConst() const override {
		return true;
	}

	int64_t calculate() const override {
		return value;
	}

	std::string str() const override {
		return std::to_string(value);
	}

	bool isValue() const override {
		return true;
	}
};

class OperationTreeNode : public TreeNode {
public:
	OperationTreeNode(std::shared_ptr<TreeNode> left, std::shared_ptr<TreeNode> right, char op) : left(left), right(right), op(op) {}

	std::shared_ptr<TreeNode> left;
	std::shared_ptr<TreeNode> right;
	char op;

	bool isConst() const override {
		return left->isConst() && right->isConst();
	}

	int64_t calculate() const override {
		auto leftVal = left->calculate();
		auto rightVal = right->calculate();

		if (op == '+') {
			return leftVal + rightVal;
		} else if (op == '-') {
			return leftVal - rightVal;
		} else if (op == '*') {
			return leftVal * rightVal;
		} else {
			return leftVal / rightVal;
		}
	}

	std::string str() const override {
		return "(" + left->str() + " " + op + " " + right->str() + ")";
	}

	bool isOp() const override {
		return true;
	}
};

class RootTreeNode : public TreeNode {
public:
	RootTreeNode(std::shared_ptr<TreeNode> left, std::shared_ptr<TreeNode> right) : left(left), right(right) {}

	std::shared_ptr<TreeNode> left;
	std::shared_ptr<TreeNode> right;

	bool isConst() const override {
		return left->isConst() && right->isConst();
	}

	int64_t calculate() const override {
		throw std::runtime_error("calculate on root");
	}

	std::string str() const override {
		return left->str() + " == " + right->str();
	}
};

class HumanTreeNode : public TreeNode {
public:
	bool isConst() const override {
		return false;
	}

	int64_t calculate() const override {
		throw std::runtime_error("calculate on human");
	}

	std::string str() const override {
		return "X";
	}

	bool isHuman() const override {
		return true;
	}
};

bool tryReduceTree(std::shared_ptr<TreeNode>& node) {
	if (auto root = std::dynamic_pointer_cast<RootTreeNode>(node); root) {
		if (!root->left->isValue() && root->left->isConst()) {
			auto newNode = std::make_shared<ValueTreeNode>(root->left->calculate());
			root->left = newNode;
			return true;
		}

		if (!root->right->isValue() && root->right->isConst()) {
			auto newNode = std::make_shared<ValueTreeNode>(root->right->calculate());
			root->right = newNode;
			return true;
		}

		auto res = tryReduceTree(root->left);
		if (res) {
			return true;
		}

		return tryReduceTree(root->right);
	} else if (auto op = std::dynamic_pointer_cast<OperationTreeNode>(node); op) {
		if (!op->left->isValue() && op->left->isConst()) {
			auto newNode = std::make_shared<ValueTreeNode>(op->left->calculate());
			op->left = newNode;
			return true;
		}

		if (!op->right->isValue() && op->right->isConst()) {
			auto newNode = std::make_shared<ValueTreeNode>(op->right->calculate());
			op->right = newNode;
			return true;
		}

		auto res = tryReduceTree(op->left);
		if (res) {
			return true;
		}

		return tryReduceTree(op->right);
	}

	return false;
}

bool tryFoldTree(const std::shared_ptr<RootTreeNode>& node) {
	// assume that right is always a value (it is for me)
	// also assume we have only one human node in the tree

	// then we check left, if it is a human node, we are done
	if (node->left->isHuman()) {
		return false;
	}

	// then it will be a op node,
	// and one of its nodes will be a value, and the other another tree

	auto leftOp = std::dynamic_pointer_cast<OperationTreeNode>(node->left);

	// forms are:
	// C: const
	// U: non const
	// C1 + U = C2  => U = (C2 - C1)
	if (leftOp->left->isConst() && !leftOp->right->isConst() && leftOp->op == '+') {
		auto c1 = leftOp->left;
		auto c2 = node->right;

		auto u = leftOp->right;

		auto newRight = std::make_shared<OperationTreeNode>(c2, c1, '-');

		node->left = u;
		node->right = newRight;

		return true;
	}

	// U + C1 = C2  => U = (C2 - C1)
	if (!leftOp->left->isConst() && leftOp->right->isConst() && leftOp->op == '+') {
		auto u = leftOp->left;
		auto c1 = leftOp->right;

		auto c2 = node->right;

		auto newRight = std::make_shared<OperationTreeNode>(c2, c1, '-');

		node->left = u;
		node->right = newRight;

		return true;
	}

	// C1 - U = C2  => C1 = C2 + U => C2 + U = C1
	if (leftOp->left->isConst() && !leftOp->right->isConst() && leftOp->op == '-') {
		auto c1 = leftOp->left;
		auto u = leftOp->right;

		auto c2 = node->right;

		auto newLeft = std::make_shared<OperationTreeNode>(c2, u, '+');

		node->left = newLeft;
		node->right = c1;

		return true;
	}

	// U - C1 = C2  => U = C2 + C1
	if (!leftOp->left->isConst() && leftOp->right->isConst() && leftOp->op == '-') {
		auto u = leftOp->left;
		auto c1 = leftOp->right;
		auto c2 = node->right;

		auto newRight = std::make_shared<OperationTreeNode>(c2, c1, '+');
		node->left = u;
		node->right = newRight;

		return true;
	}

	// C1 * U = C2  => U = C2 / C1
	if (leftOp->left->isConst() && !leftOp->right->isConst() && leftOp->op == '*') {
		auto c1 = leftOp->left;
		auto u = leftOp->right;
		auto c2 = node->right;

		auto newRight = std::make_shared<OperationTreeNode>(c2, c1, '/');

		node->left = u;
		node->right = newRight;

		return true;
	}

	// U * C1 = C2  => U = C2 / C1
	if (!leftOp->left->isConst() && leftOp->right->isConst() && leftOp->op == '*') {
		auto u = leftOp->left;

		auto c1 = leftOp->right;

		auto c2 = node->right;

		auto newRight = std::make_shared<OperationTreeNode>(c2, c1, '/');

		node->left = u;
		node->right = newRight;

		return true;
	}

	// C1 / U = C2  => C1 = C2 * U => C2 * U = C1
	// this isn't actually needed, but I implemented it anyway
	if (leftOp->left->isConst() && !leftOp->right->isConst() && leftOp->op == '/') {
		auto c1 = leftOp->left;
		auto u = leftOp->right;
		auto c2 = node->right;

		auto newLeft = std::make_shared<OperationTreeNode>(c2, u, '*');
		node->left = newLeft;
		node->right = c1;

		return true;
	}

	// U / C1 = C2  => U = C2 * C1
	if (!leftOp->left->isConst() && leftOp->right->isConst() && leftOp->op == '/') {
		auto u = leftOp->left;
		auto c1 = leftOp->right;

		auto c2 = node->right;

		auto newRight = std::make_shared<OperationTreeNode>(c2, c1, '*');

		node->left = u;
		node->right = newRight;

		return true;
	}

	return false;
}

int main(int argc, char* argv[]) {
	std::ifstream inputFile("inputs/day21.txt");
	std::string input(std::istreambuf_iterator{ inputFile }, std::istreambuf_iterator<char>{});

	auto start = std::chrono::high_resolution_clock::now();

	auto inputIntoLines = std::string_view{ input } | std::views::split("\n"sv) | std::views::transform([](auto rng) { return std::string(rng.begin(), rng.end()); }) | std::views::filter([](auto str) { return !str.empty(); });

	std::unordered_map<std::string, int64_t> knownNumbers;
	std::unordered_map<std::string, Operation> knownOperations;

	const std::regex numberRegex(R"((\w{4}): (\d+))");
	const std::regex operationRegex(R"((\w{4}): (\w{4}) (.) (\w{4}))");

	// step 1 parse all elements

	for (auto line : inputIntoLines) {
		std::smatch matchResults;

		if (std::regex_match(line, matchResults, numberRegex)) {
			knownNumbers[matchResults[1].str()] = std::stoi(matchResults[2].str());
		}
		if (std::regex_match(line, matchResults, operationRegex)) {

			knownOperations[matchResults[1].str()] = Operation{
				matchResults[1].str(),
				matchResults[2].str(),
				matchResults[4].str(),

				*matchResults[3].first
			};
		}
	}

	// step 2: build tree

	std::shared_ptr<TreeNode> root;
	{

		std::unordered_map<std::string, std::shared_ptr<TreeNode>> builtTreeElements;
		for (auto& num : knownNumbers) {
			if (num.first == "humn") {
				builtTreeElements[num.first] = std::make_shared<HumanTreeNode>();
			} else {
				builtTreeElements[num.first] = std::make_shared<ValueTreeNode>(num.second);
			}
		}

		while (!knownOperations.empty()) {
			for (auto it = knownOperations.begin(); it != knownOperations.end();) {
				if (builtTreeElements.contains(it->second.leftValue) && builtTreeElements.contains(it->second.rightValue)) {
					auto left = builtTreeElements[it->second.leftValue];
					auto right = builtTreeElements[it->second.rightValue];

					if (it->first == "root") {
						builtTreeElements[it->first] = std::make_shared<RootTreeNode>(left, right);
					} else {
						builtTreeElements[it->first] = std::make_shared<OperationTreeNode>(left, right, it->second.op);
					}

					it = knownOperations.erase(it);
				} else {
					++it;
				}
			}
		}

		root = builtTreeElements["root"];
	}

	// step 3: reduce tree

	while (true) {
		auto didReduce = tryReduceTree(root);

		if (!didReduce) {
			break;
		}
	}

	fmt::print("Root {}\n", root->str());

	auto rootTreeNodeAsRootType = std::dynamic_pointer_cast<RootTreeNode>(root);

	// step 4: equality folding
	while (true) {
		auto didFold = tryFoldTree(rootTreeNodeAsRootType);

		if (!didFold) {
			break;
		}

		while (true) {
			auto didReduce = tryReduceTree(root);

			if (!didReduce) {
				break;
			}
		}

		fmt::print("Root {}\n", root->str());
	}

	auto end = std::chrono::high_resolution_clock::now();

	

	auto dur = end - start;

	auto durSecs = std::chrono::duration_cast<std::chrono::duration<float>>(dur);

	fmt::print("Took {}\n", dur);
	fmt::print("Or {}\n", durSecs);

	return 0;
}
