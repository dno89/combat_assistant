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
int extractInit(const std::string& str) {
    boost::regex r("<b>Init </b>(.[[:digit:]]+);");
    boost::match_results<std::string::const_iterator> res;

    boost::regex_search(str, res, r);

    return stoi(res[1]);
}
