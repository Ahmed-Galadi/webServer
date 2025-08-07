#include "ParseUtils.hpp"
#include <vector>
#include <iostream>

int main() {
	std::vector<std::string>	data = ParseUtils::readFile("config.txt");
	std::vector<std::string>	splitData = ParseUtils::splitString(data[0], ' ');

	std::vector<std::string>	tokens = ParseUtils::splitAndAccumulate(data);

	for (int i = 0; i < tokens.size(); i++)
		std::cout << tokens[i] << std::endl;

	return (0);
}