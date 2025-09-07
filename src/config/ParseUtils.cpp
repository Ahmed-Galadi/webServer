
#include "ParseUtils.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>

std::string ParseUtils::toString(int number) {
    std::ostringstream oss;
    oss << number;
    return oss.str();
}

int		ParseUtils::htoi(const std::string &hexStr) {
    int result = 0;
    for (size_t i = 0; i < hexStr.size(); ++i) {
        char c = std::toupper(hexStr[i]);
        int value;

        if (std::isdigit(c))
            value = c - '0';
        else if (c >= 'A' && c <= 'F')
            value = 10 + (c - 'A');
        else
            break;
        result = result * 16 + value;
    }
    return result;
}

std::vector<std::string> ParseUtils::readFile(std::string fileName) {
	if (fileName.empty()) {
		std::cout << "Error: Enter config file!" << std::endl;
		exit(1);
	}

	std::ifstream				inputFile(fileName.c_str());
	std::string 				line;
	std::vector<std::string>	fileData;

	if (!inputFile.is_open()) {
		std::cout << "Error: Can't open file!" << std::endl;
		exit(1);
	}
	while (std::getline(inputFile, line))
		fileData.push_back(line);
	inputFile.close();
	return (fileData);
}

std::vector<std::string>	ParseUtils::splitString(std::string &str, char del) {
	std::vector<std::string>	output;
	std::stringstream			ss(str);
	std::string					oneSplitedStr;

	while (std::getline(ss, oneSplitedStr, del))
		output.push_back(oneSplitedStr);
	return (output);
}

std::string ParseUtils::trim(const std::string &s) {
    size_t start = 0;
    while (start < s.size() && isspace((unsigned char)s[start])) ++start;
    size_t end = s.size();
    while (end > start && isspace((unsigned char)s[end - 1])) --end;
    return s.substr(start, end - start);
}

std::vector<std::string> ParseUtils::splitAndAccumulate(std::vector<std::string> fileData) {
	std::vector<std::string>	accumOut;
	unsigned int				index_in;
	unsigned int				index_out;

	for (index_in = 0; index_in < fileData.size(); index_in++) {
		std::vector<std::string> splitedStr = splitString(fileData[index_in], ' ');
		for(index_out = 0; index_out < splitedStr.size(); index_out++)
			if (!splitedStr[index_out].empty())
				accumOut.push_back(splitedStr[index_out]);
	}
	return (accumOut);
}

bool	ParseUtils::bracketsCheck(std::vector<std::string> &tokens) {
	int depth = 0;

	for (size_t i = 0; i < tokens.size(); ++i) {
		if (tokens[i] == "{")
			depth++;
		else if (tokens[i] == "}") {
			depth--;
			if (depth < 0)
				return (false);
		}
	}
	if (depth == 0)
		return (true);
	return (false);
}

int		ParseUtils::toInt(std::vector<std::string>::iterator it) {
	int		output;
	std::istringstream stream(*it);

	stream >> output;

	if (stream.fail() || !stream.eof())
		exit(1);
	return (output);
}