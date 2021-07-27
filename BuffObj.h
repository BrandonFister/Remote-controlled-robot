#pragma once
#include <deque>
#include <queue>
#include <mutex>
#include <memory>
struct BuffInfo {
	char* buff = nullptr;
	int size = 0;
};
class BuffObj
{
	std::mutex ctl;
	std::queue<BuffInfo> buffSet;
public:
	BuffObj();
	~BuffObj();
	int add(const char*, int size = sizeof(int));
	//char* get(); /* be sure to delete this pointer after use */
	//std::unique_ptr<char> get();
	BuffInfo get();
	bool isEmpty() const;
};