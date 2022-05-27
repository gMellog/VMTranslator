#include "VMTranslator.h"
#include "utils.h"
#include <filesystem>

//DONE
//Process directory +
//Process vm file only + 
//Process multiple vm files +

//TODO
//implement BRANCHING
//if-goto
//goto
//label

//implement function
//function
//call
//return

VMTranslator::VMTranslator()
{
    initializeCommandMap();
    initializeSegmentInAssemblyMap();
}

void VMTranslator::start(int argc, char** argv)
{
    std::string line;
    bool first = true;
    if (argc == 1)
    {
        std::cerr << "usage: ./VMTranslator {[VMFILENAME].vm...} or [FULL_PATH_TO_DIRECTORY]\n";
        std::cerr << "use . -> to refer to the current directory\n";
        std::exit(1);
    }

    std::vector<std::string> vmFiles;

    if(argc == 2)
    {
        const std::string file = argv[1];

        if (isVMFile(file))
        {
            vmFiles.push_back(getFullPath(file));
        }
        else
        {
            const std::string path = argv[1];
            try
            {
                for (const auto& file : std::filesystem::directory_iterator(path))
                {
                    const std::string filePath{ file.path().u8string() };

                    if (isVMFile(filePath))
                    {
                        vmFiles.push_back(filePath);
                    }
                }
            }
            catch (const std::filesystem::filesystem_error& ex)
            {
                const std::string exceptionMessage = ex.what();
                std::cerr << exceptionMessage.substr(exceptionMessage.find(' ') + 1) << '\n';
                std::exit(1);
            }
        }
    }

    if (argc >= 3)
    {
        const int allFiles = argc - 1;
        for (int i = 0; i < allFiles; ++i)
        {
            const std::string file = argv[1 + i];

            if (isVMFile(file))
            {
                vmFiles.push_back(getFullPath(file));
            }
            else
            {
                std::cerr << "file: " << file << " isn't vm file, terminating...\n";
                std::exit(1);
            }
        }
    }

    if (vmFiles.size() == 0)
    {
        std::cerr << "All files aren't vm\n";
        std::cerr << "Try again\n";
        std::exit(0);
    }

    std::string dirName;
    const std::string firstParam = argv[1];

    if (isVMFile(firstParam) || std::string(firstParam) == ".")
    {
        dirName = getFirstDirFromThePathEnd(std::filesystem::current_path().u8string());
    }
    else
    {
        dirName = getFirstDirFromThePathEnd(firstParam);
    }

    const std::string asmFileName = dirName + ".asm";
    std::ofstream asmFile{ asmFileName, std::ios::out | std::ios::app};

    asmFile << getBootstrapCode() << '\n';
    
    for (const auto& originalPath : vmFiles)
    {
        std::ifstream vmFile{ originalPath };
        vmFileName = getVMFileName(originalPath);
     
        if (vmFileName.size() == 0)
        {
            std::cerr << originalPath << '\n';
            std::cerr << "Wow, somewhat went dirty, unrecognized path\n";
            std::cerr << "Terminating...\n";
            std::exit(1);
        }
        
        while (std::getline(vmFile, line))
        {
            const auto startCommentIndex = line.find("//");

            if (line.size() != 0 && startCommentIndex != 0)
            {
                if (startCommentIndex != std::string::npos)
                {
                    line = line.substr(0u, startCommentIndex);
                }

                //Erasing spaces at the end of line
                int i;
                for (i = line.size() - 1; i >= 0 && line[i] == ' '; --i);

                if (i == -1)
                {
                    std::cerr << "Empty command\n";
                    std::cerr << "Terminating...\n";
                    std::exit(1);
                }

                line = line.substr(0u, i + 1);

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
}

void VMTranslator::initializeCommandMap()
{
    //TODO essentials checks should be done here, i mean for args, not inside function, separate them
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
    
    commandMap.insert({ "label", [this]() -> std::string 
        { 
            std::string r;

            if (args.size() == 2 )
            {
                r = label(args[1]);
            }
            else
            {
                for (const auto& i : args)
                {
                    std::cerr << i << ' ';
                }

                std::cerr << " isn't valid command\n";
                std::cerr << "Terminating...\n";
                std::exit(1);
            }

            return r;
        } 
    });


    commandMap.insert({ "if-goto", [this]() -> std::string { return if_goto(); } });
    commandMap.insert({ "goto", [this]() -> std::string { return _goto(); } });
    commandMap.insert({ "function", [this]() -> std::string { return func(); } });
    commandMap.insert({ "call", [this]() -> std::string { return call(); } });
    commandMap.insert({ "return", [this]() -> std::string { return _return(); } });
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
    constexpr int v1 = 5;
    constexpr int v2 = 6;

    const auto popCodeV2 = pop(v2);
    const auto popCodeV1 = pop(v1);

    const char sign = neg ? '-' : '+';

    const std::string ourImpl =
        getAInstruction(std::to_string(v1)) +
        "D=M\n" +
        getAInstruction(std::to_string(v2)) +
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
    const int v1 = 5;
    const int v2 = 6;
    const auto popCodeV2 = pop(v2);
    const auto popCodeV1 = pop(v1);

    const std::string truelabel = labelTrue + std::to_string(labelNumber);
    const std::string endLabel = labelEnd + std::to_string(labelNumber);

    const std::string ourImpl =
        getAInstruction(std::to_string(v1)) +
        "D=M\n" +
        getAInstruction(std::to_string(v2)) +
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
    const int v1 = 5;
    const int v2 = 6;

    const auto popCode1 = pop(v1);
    const auto popCode2 = pop(v2);

    const std::string ourImpl =
        getAInstruction(std::to_string(v1)) +
        "D=M\n" +
        getAInstruction(std::to_string(v2)) +
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

std::string VMTranslator::label(const std::string& labelName) const
{
    return funcName.size() != 0 ? "(" + funcName + "$" + labelName + ")\n" : "(" + labelName + ")\n";
}

std::string VMTranslator::_goto() const
{
    std::string r;
    if (args.size() == 2)
    {

        const auto& label = args[1];
        r =
            getAInstruction(funcName + "$" + label) +
            "0;JMP\n";
    }
    else
    {
        for (const auto& i : args)
        {
            std::cerr << i << ' ';
        }

        std::cerr << " isn't valid command\n";
        std::cerr << "Terminating...\n";
        std::exit(1);
    }

    return r;
}

std::string VMTranslator::if_goto() const
{
    std::string r;

    if (args.size() == 2)
    {
        constexpr int tempVal = 5;
        const auto popCode = pop(tempVal);
        const auto& label = args[1];

        r =
            popCode +
            getAInstruction(funcName + "$" + label) +
            "D;JNE\n"
            ;
    }
    else
    {
        for (const auto& i : args)
        {
            std::cerr << i << ' ';
        }

        std::cerr << " isn't valid command\n";
        std::cerr << "Terminating...\n";
        std::exit(1);
    }

    return r;
}

std::string VMTranslator::func() const
{
    std::string r;

    funcName = args[1];
    retFuncRunNum = 0;

    r = "(" + funcName + ")\n";

    const auto localVarsAmount = std::stoi(args[2]);

    if (localVarsAmount > 0)
    {
        r += "D=0\n"; // D prepared for to be pushed

        for (int i = 0; i < localVarsAmount; ++i)
            r += push();
    }

    return r;
}

//For every label inside func you should add to it functionName$ where functionName is Xxx.foo

std::string VMTranslator::call() const
{
    std::string r;
 
    const std::string return_address =  funcName + "$" + "ret." + std::to_string(retFuncRunNum);
    ++retFuncRunNum;

    //essential for caller (things to remember when we will comback after callee)
    std::vector<std::string> pushValues
    {
        "LCL", "ARG", "THIS", "THAT"
    };

    r +=
        getAInstruction(return_address) +
        "D=A\n" +
        push()
        ;

    for (const auto& i : pushValues)
    {
        r +=
            getAInstruction(i) +
            "D=M\n" +
            push()
            ;
    }

    //ARG=SP-n-5
    r +=
        "@SP\n"
        "D=M\n" +
        getAInstruction(args[2]) +
        "D=D-A\n" +
        "@5\n"
        "D=D-A\n"
        "@ARG\n"
        "M=D\n"
        ;

    //LCL=SP
    r +=
        "@SP\n"
        "D=M\n"
        "@LCL\n"
        "M=D\n"
        ;

    //goto func declaration
    r +=
        getAInstruction(args[1]) +
        "0;JMP\n"
        ;

    r +=
        "(" + return_address + ")"
        ;

    return r;
}

std::string VMTranslator::_return() const
{
    std::string r;

    //@5(FRAME) and @6(RET) are just temp vars followed by RAM organization of Hack computer
    r =
        "@LCL\n"
        "D=M\n"
        "@5\n"
        "M=D\n"
        "A=D-A\n"
        "D=M\n"
        "@6\n" 
        "M=D\n" + // save return-address
        pop(7)  + // D already holds value of 7th temp var
        "@ARG\n" //reposition of ret value into argument[0]
        "A=M\n"
        "M=D\n"
        "@ARG\n"
        "D=M+1\n"
        "@SP\n"
        "M=D\n" // SP for caller restored
        ;

    //order matters
    const std::vector<std::string> restoreValues
    {
        "THAT", "THIS", "ARG", "LCL"
    };

    for (std::size_t i = 0; i < restoreValues.size(); ++i)
    {
        r +=
            "@5\n"
            "D=M\n" +
            getAInstruction(std::to_string(i + 1)) +
            "A=D-A\n"
            "D=M\n" +
            getAInstruction(restoreValues[i]) +
            "M=D\n"
            ;
    }

    r +=
        "@6\n"
        "A=M\n"
        "0;JMP\n"
        ;

    return r;
}

std::string VMTranslator::getUnaryImpl(const char op) const
{
    constexpr int tempVal = 5;
    const auto popCode = pop(tempVal);

    const std::string ourImpl =
        getAInstruction(std::to_string(tempVal)) +
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

//TODO we can forbid store in temp var when it's just popped in D (pop impl without segments)
std::string VMTranslator::pop(int varNum) const
{
    std::string r;

    if (varNum != -1)
    {
        r =
            "@SP\n"
            "M=M-1\n"
            "A=M\n"
            "D=M\n" +
            getAInstruction(std::to_string(varNum)) +
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

std::string VMTranslator::getVMFileName(const std::string& vmFile) const
{
    std::string r;
    
    if (vmFile.size() != 0)
    {
        int i;
        for (i = vmFile.size() - 1;
            i >= 0 && vmFile[i] != '/' && vmFile[i] != '\\';
            --i);

        if (i != -1)
        {
            const std::string vmFileNameEx = vmFile.substr(i + 1);
            r = split(vmFileNameEx, '.')[0];
        }
    }

    return r;
}

std::string VMTranslator::getFirstDirFromThePathEnd(const std::string& dirPath) const
{
    std::string r;

    if (dirPath.size() != 0)
    {
        int i;
        for (i = dirPath.size() - 1;
            i >= 0 && dirPath[i] != '/' && dirPath[i] != '\\';
            --i);
        
        r = dirPath.substr(i + 1);
    }

    return r;
}

std::string VMTranslator::getBootstrapCode() const
{
    std::string r;

    r =
        "@256\n"
        "D=A\n"
        "@SP\n"
        "M=D\n"
        "@Sys.init\n"
        "0;JMP\n"
        ;

    return r;
}

bool VMTranslator::isVMFile(const std::string& file) const noexcept
{
    return file.find(".vm") != std::string::npos;
}

char VMTranslator::getDirectionSeparateDel() const
{
    return std::filesystem::current_path().u8string().find('/') == std::string::npos ? '\\' : '/';
}

std::string VMTranslator::getFullPath(const std::string& vmFile) const
{
    return std::filesystem::current_path().u8string() + getDirectionSeparateDel() + vmFile;
}
