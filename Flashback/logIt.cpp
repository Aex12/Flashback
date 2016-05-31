/* 
	logIt 1.0.1
	-Created by: Aex12
	-Creation date: 26/02/2016 at 21:03
	-Last update: 27/02/2016 at 14:28 
*/

#include "logIt.h"
#include <fstream>
#include <sstream>

using namespace std;

void logIt::clear()
{
	remove("flashback-log.txt");
}

void logIt::writeStr(const std::string &text)
{
	std::ofstream log_file("flashback-log.txt", std::ios_base::out | std::ios_base::app);
	log_file << text << std::endl;
	log_file.close();
}

void logIt::writeFloat(float mfloat, int precision)
{
	std::ofstream log_file("flashback-log.txt", std::ios_base::out | std::ios_base::app);
	log_file.precision(precision);
	log_file << mfloat << std::endl;
	log_file.close();
}
