#include "Log.h"

Log::Log(std::string filename) {
	fp = std::ofstream(filename);
	if (!fp.is_open()) {
		std::cerr << "Error: IO operation - file not open" << std::endl;
		fp.clear();
	}
}

Log::~Log() {
	fp.close();
}

Log& Log::operator()(std::string newMsg)
{
	setLog(newMsg);
	return *this;
}

void Log::setLog(std::string s) {
	if (fp.is_open() && fp.good())
		fp << s << std::endl;
}
