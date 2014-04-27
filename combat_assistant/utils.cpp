#include <boost/regex.hpp>
#include "utils.h"

//global functions
std::string processHP(const std::string& str) {
        boost::regex r("([[:digit:]]+)d([[:digit:]]+)");
        return regex_replace(str, r, "d(\\2, \\1)");
}
std::string formatDescription(std::string str) {
    boost::regex r("<link.*?>");
    return boost::regex_replace(str, r, "");
}
