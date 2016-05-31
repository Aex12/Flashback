/*
logIt 1.0
-Created by: Aex12
-Creation date: 26/02/2016 at 21:03
-Last update:
*/
#pragma once
#include <fstream>
#include <sstream>
using namespace std;

class logIt {
public:
	void clear();
	void writeStr(const std::string &text);
	void writeFloat(float mfloat, int precision);
};