#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <map>
#include <cstdlib>
#include <fstream>

//TODO
//Think about concurrency, i think this is possible

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

	//varNum should be in range 5-12.
	std::string pop(int varNum = -1) const;
	std::string popImpl() const;

	std::string label(const std::string& labelName) const;
	std::string _goto() const;
	std::string if_goto() const;

	std::string func() const;
	std::string call() const;
	std::string _return() const;

	std::string getAInstruction(const std::string& varName) const;
	std::string getComparisonImpl(
		const std::size_t labelNumber,
		const std::string& jump,
		const std::string& labelTrue,
		const std::string& labelEnd) const;
	std::string getLogicImpl(const char op) const;
	std::string getUnaryImpl(const char op) const;

	std::string getTemplateSegment(const std::string& segment) const;

	std::string getVMFileName(const std::string& vmFile) const;
	std::string getFirstDirFromThePathEnd(const std::string& dirPath) const;
	std::string getBootstrapCode() const;

	bool isVMFile(const std::string& file) const noexcept;
	char getDirectionSeparateDel() const;
	std::string getFullPath(const std::string& vmFile) const;

	std::vector<std::string> args;
	std::map<std::string, std::function<std::string()>> commandMap;
	std::map<std::string, std::string> segmentInAssemblyMap;
	std::string vmFileName;
	mutable std::string funcName;
	mutable int retFuncRunNum;

};