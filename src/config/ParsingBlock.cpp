#include "ParsingBlock.hpp"

std::string ParsingBlock::getName() const {
	return (this->name);
}

std::vector<std::string> ParsingBlock::getArgs() const {
	return (this->args);
}

std::vector<std::string> ParsingBlock::getTokens() const {
	return (this->tokens);
}

void	ParsingBlock::setName(std::string name) {
	this->name = name;
}

void	ParsingBlock::addToArgs(std::string arg) {
	args.push_back(arg);
}

void	ParsingBlock::addToTokens(std::string token) {
	tokens.push_back(token);
}