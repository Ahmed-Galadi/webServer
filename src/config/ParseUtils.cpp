
#include "ParseUtils.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

std::vector<std::string> ParseUtils::readFile(std::string fileName) {
    if (fileName.empty()) {
        std::cout << "Error: Enter config file!" << std::endl;
        exit(1);
    }

    std::ifstream inputFile(fileName.c_str());  // Use c_str() for C++98
    std::string line;
    std::vector<std::string> fileData;

    if (!inputFile.is_open()) {
        std::cout << "Error: Can't open file!" << std::endl;
        exit(1);
    }
    while (std::getline(inputFile, line))
        fileData.push_back(line);
    inputFile.close();
    return fileData;
}

std::vector<std::string> ParseUtils::splitString(std::string &str, char del) {
    std::vector<std::string> output;
    std::stringstream ss(str);
    std::string oneSplitedStr;

    while (std::getline(ss, oneSplitedStr, del))
        output.push_back(oneSplitedStr);
    return output;
}

std::vector<std::string> ParseUtils::splitAndAccumulate(std::vector<std::string> fileData) {
    std::vector<std::string> accumOut;
    unsigned int index_in;
    unsigned int index_out;

    for (index_in = 0; index_in < fileData.size(); index_in++) {
        std::vector<std::string> splitedStr = splitString(fileData[index_in], ' ');
        for (index_out = 0; index_out < splitedStr.size(); index_out++)
            if (!splitedStr[index_out].empty())
                accumOut.push_back(splitedStr[index_out]);
    }
    return accumOut;
}

bool ParseUtils::bracketsCheck(std::vector<std::string> &tokens) {
    int depth = 0;

    for (size_t i = 0; i < tokens.size(); ++i) {  // size_t for unsigned
        if (tokens[i] == "{")
            depth++;
        else if (tokens[i] == "}") {
            depth--;
            if (depth < 0)
                return false;
        }
    }
    return depth == 0;
}

int ParseUtils::toInt(std::vector<std::string>::iterator it) {
    int output;
    std::istringstream stream(*it);

    stream >> output;

    if (stream.fail() || !stream.eof()) {
        std::cout << "Error: Invalid integer conversion!" << std::endl;
        exit(1);
    }
    return output;
}