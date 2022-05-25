#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <map>
#include <cstdlib>
#include <fstream>

#define COMPARISON(jump,labelTrue,labelEnd) \
static std::size_t label = 0ul;\
return getComparisonImpl(label++, (jump), (labelTrue), (labelEnd));\

struct VMTranslator
{
	VMTranslator();
	void start(int argc, char** argv);

private:

	void initializeCommandMap();
	void initializeSegmentInAssemblyMap();

	std::string add() const;
	std::string sub() const;
	std::string eq() const;
	std::string neg() const;
	std::string gt() const;
	std::string lt() const;
	std::string _and() const;
	std::string _or() const;
	std::string _not() const;

	std::string addNeg(bool neg) const;

	std::string push() const;
	std::string pushImpl() const;
	std::string pushCommonPart(bool saveA = false) const;

	std::string pop(const std::string& popVarName = "") const;
	std::string popImpl() const;


	std::string getAInstruction(const std::string& varName) const;
	std::string getComparisonImpl(
		const std::size_t labelNumber,
		const std::string& jump,
		const std::string& labelTrue,
		const std::string& labelEnd) const;
	std::string getLogicImpl(const char op) const;
	std::string getUnaryImpl(const char op) const;

	std::string getTemplateSegment(const std::string& segment) const;

	std::vector<std::string> args;
	std::map<std::string, std::function<std::string()>> commandMap;
	std::map<std::string, std::string> segmentInAssemblyMap;
	std::string vmFileName;

};