#pragma once
#include <iostream>
#include <string>
#include <fstream>

class Log {
	std::ofstream fp;
public:
	Log() = delete;
	Log(std::string filename);
	~Log();
	void setLog(std::string s);
	Log& operator()(std::string newMsg);
};