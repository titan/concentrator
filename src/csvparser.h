#ifndef _CVSPARSER_H
#define _CVSPARSER_H
#include <string>
#include <vector>
using namespace std;

void csvline_populate(vector<string> &record, const string& line, char delimiter);
#endif
