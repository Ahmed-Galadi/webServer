#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <cstdlib>



class ParseUtils {
	private:
		ParseUtils();

	public:
		static std::vector<std::string> readFile(std::string fileName);
		static std::vector<std::string> splitAndAccumulate(std::vector<std::string> fileData);
		static std::vector<std::string> splitString(std::string &str, char del);
		static bool						bracketsCheck(std::vector<std::string> &tokens);
		static bool						dupsCheck(std::vector<std::string> &tokens);
		static bool						syntaxError(std::vector<std::string> &tokens);
		static int						toInt(std::vector<std::string>::iterator it);
};