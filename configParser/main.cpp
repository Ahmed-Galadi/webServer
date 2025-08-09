#include "ParseUtils.hpp"
#include <vector>
#include <iostream>
#include "ConfigParser.hpp"

int main() {
	/*std::vector<std::string>	data = ParseUtils::readFile("config.txt");*/
	/*std::vector<std::string>	splitData = ParseUtils::splitString(data[0], ' ');*/
	/**/
	/*std::vector<std::string>	tokens = ParseUtils::splitAndAccumulate(data);*/
	/**/
	/*for (int i = 0; i < tokens.size(); i++)*/
	/*	std::cout << tokens[i] << std::endl;*/
	/**/
	/*std::string s("123457");*/
	/*std::vector<std::string> vs;*/
	/*vs.push_back(s);*/
	/**/
	/*int intiger = ParseUtils::toInt(vs.begin());*/
	/**/
	/*std::cout << "the number is : " << intiger << std::endl;*/
	
	ConfigParser WSconfig;

	WSconfig.parse("config.txt");
	return (0);
}
