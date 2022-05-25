#include "utils.h"


std::vector<std::string> split(const std::string& str, char deli)
{
    std::vector<std::string> r;

    auto from = 0u;
    auto nextSpace = str.find(deli);

    while (nextSpace != std::string::npos)
    {
        auto part = str.substr(from, nextSpace - from);
        r.push_back(std::move(part));

        from = nextSpace + 1;
        nextSpace = str.find(' ', from);
    }

    if (from <= (str.size() - 1))
        r.push_back(str.substr(from));

    return r;
}