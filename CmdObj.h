#pragma once
#include <deque>
#include <queue>
#include <string>
#include <mutex>
class CmdObj
{
	std::mutex ctl;
	std::queue<std::string> cmds;

public:
	CmdObj();
	~CmdObj();

	int addCmd(const std::string);
	std::string getCmd();
	bool isEmpty() const;
};
