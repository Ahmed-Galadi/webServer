#pragma once

#include <vector>
#include <string>

class ParsingBlock {
	private:
		std::string name;
		std::vector<std::string> args;
		std::vector<std::string> tokens;
	public:
		std::string getName() const;
		std::vector<std::string> getArgs() const;
		std::vector<std::string> getTokens() const;

		void	setName(std::string &name);
		void	addToArgs(std::string &arg);
		void	addToTokens(std::string &token);
};