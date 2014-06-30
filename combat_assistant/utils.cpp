#include <boost/regex.hpp>
#include <algorithm>
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
void SortTxtDB(txtDatabase::DatabaseType& db, txtDatabase::LabelMapType lbl, std::string key, bool integer) {
    assert(integer == false && "Integer = true not yet implemented!");
    std::sort(db.begin(), db.end(), [&](txtDatabase::DatabaseType::const_reference e1, txtDatabase::DatabaseType::const_reference e2){return e1[lbl[key]] < e2[lbl[key]];});
}
