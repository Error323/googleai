#include <ctime>
#include <iostream>
#include <sstream>

#include "./Logger.h"

Logger* Logger::instance = NULL;

Logger::Logger(std::string n = "") {
	if (n.empty())
		name = GetLogName();
	else
		name = n;
	log.open(name.c_str());
}

std::string Logger::GetLogName() {
	if (!name.empty()) {
		return name;
	}

	//time_t t;
	//time(&t);
	//struct tm* lt = localtime(&t);

	char buf[1024] = {0};

	snprintf(
		buf,
		1024 - 1,
		"MyBot-Error323.txt"//-%02d-%02d-%04d_%02d%02d.txt",
		//lt->tm_mon + 1,
		//lt->tm_mday,
		//lt->tm_year + 1900,
		//lt->tm_hour,
		//lt->tm_min
	);

	return (std::string(buf));
}
