
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
		throw std::runtime_error("Error: Config file name is empty");
	}

	std::ifstream				inputFile(fileName.c_str());
	std::string 				line;
	std::vector<std::string>	fileData;

	if (!inputFile.is_open()) {
		throw std::runtime_error("Error: Cannot open config file: " + fileName);
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
		std::string trimmedLine = trim(fileData[index_in]);
		std::vector<std::string> splitedStr = splitString(trimmedLine, ' ');
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
		throw std::runtime_error("Error: Invalid integer value in config: " + *it);
	return (output);
}


size_t ParseUtils::parseMaxBodySize(const std::string &value)
{
	if (value.empty())
		throw std::invalid_argument("Max body size value is empty");

	std::string num = value;
	size_t multiplier = 1;
	char unit = value[value.size() - 1];

	if (unit == 'k' || unit == 'K')
	{
		multiplier = 1024UL;
		num = value.substr(0, value.size() - 1);
	}
	else if (unit == 'm' || unit == 'M')
	{
		multiplier = 1024UL * 1024UL;
		num = value.substr(0, value.size() - 1);
	}
	else if (unit == 'g' || unit == 'G')
	{
		multiplier = 1024UL * 1024UL * 1024UL;
		num = value.substr(0, value.size() - 1);
	}

	size_t start = num.find_first_not_of(" \t");
	size_t end = num.find_last_not_of(" \t");
	if (start == std::string::npos)
		throw std::invalid_argument("Invalid body size format (no number)");
	num = num.substr(start, end - start + 1);

	for (size_t i = 0; i < num.size(); ++i)
	{
		if (!std::isdigit(num[i]))
			throw std::invalid_argument("Invalid character in body size: " + value);
	}

	unsigned long numeric_value = 0;
	try
	{
		numeric_value = strtoull(num.c_str(), NULL, 10);
	}
	catch (const std::exception &)
	{
		throw std::out_of_range("Body size number is too large");
	}

	if (numeric_value > std::numeric_limits<size_t>::max() / multiplier)
		throw std::out_of_range("Body size value overflow");

	size_t result = numeric_value * multiplier;

	const size_t MAX_ALLOWED = 1024UL * 1024UL * 1024UL;
	if (result > MAX_ALLOWED)
		throw std::runtime_error("Max body size exceeds 1 GB limit");

	return result;
}

