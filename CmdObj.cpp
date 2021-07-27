#include "CmdObj.h"

CmdObj::CmdObj()
{
}


CmdObj::~CmdObj()
{
}

/*
* Take: new command string
* Action: adds it to the cmdlist
* Return: command count (may be incorrect depending on how fast the commands are processed)
*/
int CmdObj::addCmd(const std::string newCmd)
{
	int count = 0;
	std::lock_guard<std::mutex> lock(ctl);
	cmds.push(newCmd);
	count = cmds.size();
	return count;
}

/*
* Take: nothing
* Action: if not empty, returns the oldest command and removes it from the list
* Return: command string
*/
std::string CmdObj::getCmd()
{
	std::string rtn = "";
	std::lock_guard<std::mutex> lock(ctl);
	if (!cmds.empty()) {
		rtn = cmds.front();
		cmds.pop();
	}
	return rtn;
}

/*
* Take: nothing
* Action: Check for cmds
* Return: true if empty
*/
bool CmdObj::isEmpty() const
{
	//	std::lock_guard<std::mutex> lock(ctl); /* problem with const-ness of the query method.  Lock may be unnecessary */
	return cmds.empty();
}
