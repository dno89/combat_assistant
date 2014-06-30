#ifndef UTILS_H
#define UTILS_H

#include <string>
#include "txtDatabase.h"

std::string processHP(const std::string& str);
std::string formatDescription(std::string str);
int extractInit(const std::string &str);
void SortTxtDB(txtDatabase::DatabaseType& db, txtDatabase::LabelMapType lbl, std::string key, bool integer = false);

#endif // UTILS_H
