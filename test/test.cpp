
#include "../include/WLoger.h"

#include <iostream>

void temp() 
{
	WLE << "123123" << __WLOG_VALUE_TSTR(asdasdasdasd);
}
#include <thread>
int main()
{
	WLOG_ATTACH_STRAEM(WL_ERROR, std::cout);
	WLOG_ATTACH_STRAEM(WL_WARNING, std::cout);
	WLOG_ATTACH_STRAEM(WL_INFO, std::cout);
	system("mkdir build");
	system("mkdir build/log");

	WLOG_GENERATE_LOG_FILE("build/log");

	std::thread* ths[2];
	
	for (int i = 0; i < 2; i++)
	{
		ths[i] = new std::thread(temp);

	}
	for (int i = 0; i < 2; i++)
	{
		ths[i]->join();

	}
}