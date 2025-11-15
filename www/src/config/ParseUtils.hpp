#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <cctype>   // for std::isdigit
#include <limits>   // for std::numeric_limits


class ParseUtils {
	private:
		ParseUtils();

	public:
		static std::vector<std::string> readFile(std::string fileName);
		static std::vector<std::string> splitAndAccumulate(std::vector<std::string> fileData);
		static std::vector<std::string> splitString(std::string &str, char del);
		static bool						bracketsCheck(std::vector<std::string> &tokens);
		// static bool						dupsCheck(std::vector<std::string> &tokens);
		// static bool						syntaxError(std::vector<std::string> &tokens);
		static int 						htoi(const std::string &hexStr);
		static int						toInt(std::vector<std::string>::iterator it);
		static std::string				toString(int number);
		static std::string				trim(const std::string &s);
		static size_t					parseMaxBodySize(const std::string &value);
		static size_t 					parseMaxBodySize(size_t value);
};