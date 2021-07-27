#include "BuffObj.h"

BuffObj::BuffObj()
{
}


BuffObj::~BuffObj()
{
	int size = buffSet.size();
	for (int i = 0; i < size; i++) {
		delete[] buffSet.front().buff;
		buffSet.pop();
	}
}

int BuffObj::add(const char * rcvBuff, int size)
{
	BuffInfo temp;
	temp.buff = new char[size];
	temp.size = size;
	memcpy(temp.buff, rcvBuff, size);
	std::lock_guard<std::mutex> lock(ctl);
	buffSet.push(temp);
	return buffSet.size();
}

BuffInfo BuffObj::get()
//std::unique_ptr<char> BuffObj::get()
//char * BuffObj::get()
{
	std::lock_guard<std::mutex> lock(ctl);
	//  std::unique_ptr<char> temp(buffSet.front().buff);
	//	char * temp = buffSet.front();
	BuffInfo temp = buffSet.front();
	buffSet.pop();
	return temp;
}

bool BuffObj::isEmpty() const
{
	return buffSet.empty();
}