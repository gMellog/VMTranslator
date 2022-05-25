#include "VMTranslator.h"
#include "utils.h"

VMTranslator::VMTranslator()
{
    initializeCommandMap();
    initializeSegmentInAssemblyMap();
}

void VMTranslator::start(int argc, char** argv)
{
    std::string line;
    bool first = true;

    if (argc != 2)
    {
        std::cerr << "usage: ./VMTranslator [VMFILENAME].vm\n";
        std::exit(1);
    }

    const std::string vmFileNameEx = argv[1];
    std::ifstream vmFile{ vmFileNameEx };

    vmFileName = split(vmFileNameEx, '.')[0];

    const std::string asmFileName = vmFileName + ".asm";
    std::ofstream asmFile{ asmFileName, std::ios::out | std::ios::app };

    while (std::getline(vmFile, line))
    {
        if (line.size() != 0 && line.find("//") != 0)
        {
            args = split(line, ' ');

            if (args.size() != 0)
            {
                auto it = commandMap.find(args[0]);

                if (it != std::end(commandMap))
                {
                    if (first)
                    {
                        first = false;
                    }
                    else
                    {
                        asmFile << "\n";
                    }

                    asmFile << "//" + args[0];
                    
                    if (args.size() == 3)
                    {
                        asmFile << " " << args[1] << " " << args[2];
                    }

                    asmFile << "\n";

                    asmFile << it->second();
                }
                else
                {
                    std::cerr << "Unknown command: " << args[0] << '\n';
                    std::cerr << "Terminating...\n";

                    break;
                }
            }
        }
    }
}

void VMTranslator::initializeCommandMap()
{
    commandMap.insert({ "add", [this]() -> std::string { return add(); } });
    commandMap.insert({ "sub", [this]() -> std::string { return sub(); } });
    commandMap.insert({ "eq", [this]() ->  std::string { return eq(); } });
    commandMap.insert({ "push", [this]() -> std::string { return push(); } });
    commandMap.insert({ "pop", [this]() -> std::string { return pop(); } });
    commandMap.insert({ "neg", [this]() -> std::string { return neg(); } });
    commandMap.insert({ "gt", [this]() -> std::string { return gt(); } });
    commandMap.insert({ "lt", [this]() -> std::string { return lt(); } });
    commandMap.insert({ "and", [this]() -> std::string { return _and(); } });
    commandMap.insert({ "or", [this]() -> std::string { return _or(); } });
    commandMap.insert({ "not", [this]() -> std::string { return _not(); } });
}

void VMTranslator::initializeSegmentInAssemblyMap()
{
    segmentInAssemblyMap.insert( {"local", "LCL"});
    segmentInAssemblyMap.insert({ "argument", "ARG" });
    segmentInAssemblyMap.insert({ "this", "THIS" });
    segmentInAssemblyMap.insert({ "that", "THAT" });
    segmentInAssemblyMap.insert({ "temp", "5" });
    segmentInAssemblyMap.insert({"pointer","3"});
}

std::string VMTranslator::add() const 
{
    return addNeg(false);
}

std::string VMTranslator::sub() const
{
    return addNeg(true);
}

std::string VMTranslator::addNeg(bool neg) const
{
    const std::string v1 = "pop_var1";
    const std::string v2 = "pop_var2";

    const auto popCodeV2 = pop(v2);
    const auto popCodeV1 = pop(v1);

    const char sign = neg ? '-' : '+';

    const std::string ourImpl =
        getAInstruction(v1) +
        "D=M\n" +
        getAInstruction(v2) +
        "D=D" + sign + "M\n"
        ;

    const auto pushCode = push();

    return popCodeV2 + popCodeV1 + ourImpl + pushCode;
}

std::string VMTranslator::eq() const
{
    COMPARISON("JEQ","EQ","EQ_END");
}

std::string VMTranslator::gt() const
{
    COMPARISON("JGT", "GT", "GT_END");
}

std::string VMTranslator::lt() const
{
    COMPARISON("JLT", "LT", "LT_END");
}

std::string VMTranslator::getComparisonImpl(const std::size_t labelNumber, const std::string& jump, const std::string& labelTrue, const std::string& labelEnd) const
{
    const std::string v1 = "pop_var1";
    const std::string v2 = "pop_var2";

    const auto popCodeV2 = pop(v2);
    const auto popCodeV1 = pop(v1);

    const std::string truelabel = labelTrue + std::to_string(labelNumber);
    const std::string endLabel = labelEnd + std::to_string(labelNumber);

    const std::string ourImpl =
        getAInstruction(v1) +
        "D=M\n" +
        getAInstruction(v2) +
        "D=D-M\n" +
        getAInstruction(truelabel) +
        "D;" + jump + "\n"
        "D=0\n" +
        getAInstruction(endLabel) +
        "0;JMP\n"
        "(" + truelabel + ")\n" +
        "D=-1\n"
        "(" + endLabel + ")\n"
        ;

    const auto pushCode = push();

    return popCodeV2 + popCodeV1 + ourImpl + pushCode;
}

std::string VMTranslator::_and() const
{
    return getLogicImpl('&');
}

std::string VMTranslator::_or() const
{
    return getLogicImpl('|');
}

std::string VMTranslator::getLogicImpl(const char op) const
{
    const std::string v1 = "pop_var1";
    const std::string v2 = "pop_var2";

    const auto popCode1 = pop(v1);
    const auto popCode2 = pop(v2);

    const std::string ourImpl =
        getAInstruction(v1) +
        "D=M\n" +
        getAInstruction(v2) +
        "D=D" + op + "M\n"
        ;

    const auto pushCode = push();

    return popCode1 + popCode2 + ourImpl + pushCode;
}

std::string VMTranslator::neg() const
{
    return getUnaryImpl('-');
}

std::string VMTranslator::_not() const
{
    return getUnaryImpl('!');
}

std::string VMTranslator::pushImpl() const
{
    std::string r;
    const auto hackSegment = getTemplateSegment(args[1]);

    if (hackSegment != "")
    {
        const std::string assignValue = (args[1] == "pointer" || args[1] == "temp" ? "A" : "M");

        r =
            getAInstruction(hackSegment) +
            "D=" + assignValue + "\n" +
            getAInstruction(args[2]) +
            "D=D+A\n"
            "A=D\n" +
            pushCommonPart()
            ;
    }
    else
    {
        if (args[1] == "constant")
        {
            r = getAInstruction(args[2]) +
                pushCommonPart(true)
                ;
        }
        else if (args[1] == "static")
        {
            r =
                getAInstruction(vmFileName + '.' + args[2]) +
                pushCommonPart()
                ;
        }
    }

    return r;
}

std::string VMTranslator::popImpl() const
{
    std::string r;
    const auto hackSegment = getTemplateSegment(args[1]);

    if (hackSegment != "")
    {
        const std::string assignValue = (args[1] == "pointer" || args[1] == "temp" ? "A" : "M");

        r =
            getAInstruction(hackSegment) +
            "D=" + assignValue + "\n" +
            getAInstruction(args[2]) +
            "D=D+A\n"
            "@tempAddr\n"
            "M=D\n"
            "@SP\n"
            "M=M-1\n"
            "A=M\n"
            "D=M\n"
            "@tempAddr\n"
            "A=M\n"
            "M=D\n"
            ;
    }
    else
    {
        if (args[1] == "static")
        {
            r =
                "@SP\n"
                "M=M-1\n"
                "A=M\n"
                "D=M\n" +
                getAInstruction(vmFileName + '.' + args[2]) +
                "M=D\n"
                ;
        }
    }

    return r;
}

std::string VMTranslator::getUnaryImpl(const char op) const
{
    const std::string v = "pop_var";
    const auto popCode = pop(v);

    const std::string ourImpl =
        getAInstruction(v) +
        "D=" + op + "M\n"
        ;

    const auto pushCode = push();

    return popCode + ourImpl + pushCode;
}

std::string VMTranslator::push() const 
{
    std::string r;

    if (args.size() != 0 && args[0] != "push")
    {
        //already with value in D register from previous operation
        r =
            "@SP\n"
            "A=M\n"
            "M=D\n"
            "@SP\n"
            "M=M+1\n"
            ;
    }
    else if (args.size() == 3)
    {
        r = pushImpl();
    }

    return r;
}

std::string VMTranslator::pop(const std::string& popVarName) const
{
    std::string r;

    if (popVarName != "")
    {
        r =
            "@SP\n"
            "M=M-1\n"
            "A=M\n"
            "D=M\n" +
            getAInstruction(popVarName) +
            "M=D\n"
            ;
    }
    else if (args.size() == 3)
    {
        r = popImpl();
    }

    return r;
}

std::string VMTranslator::pushCommonPart(bool saveA) const
{
    const std::string assignValue = saveA ? "A" : "M";

    const std::string r =
        "D=" + assignValue + "\n"
        "@SP\n"
        "A=M\n"
        "M=D\n"
        "@SP\n"
        "M=M+1\n"
        ;

    return r;
}

std::string VMTranslator::getAInstruction(const std::string& varName) const
{
    return "@" + varName + "\n";
}

std::string VMTranslator::getTemplateSegment(const std::string& segment) const
{
    const auto it = segmentInAssemblyMap.find(segment);
    return it != std::end(segmentInAssemblyMap) ? it->second : "";
}
