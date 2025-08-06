#include "ParseUtils.hpp"
#include <vector>
#include <iostream>

int main() {
	std::vector<std::string>	data = ParseUtils::readFile("config.txt");
	std::vector<std::string>	splitData = ParseUtils::splitString(data[0], ' ');

	std::vector<std::string>	tokens = ParseUtils::splitAndAccumulate(data);


	return (0);
}