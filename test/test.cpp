
#include "../include/WLoger.h"

#include <iostream>

#include "ghc_filesystem.hpp"

void temp() 
{
    WL_START_TYMETRACE;
    for(int i = 0; i < 5; i++)
    {
        WLE << "WL 123123 " << __WLOG_VALUE_TSTR(asdasdasdasd) " " << WLOG_VALUE(i);
        WLW << "WL 123123 " << __WLOG_VALUE_TSTR(asdasdasdasd) " " << WLOG_VALUE(i);
        PWLI("PWL 123123 asdasdasdasd i = %i;", i);
    }
}



#include <thread>
int main()
{
    WL_START_TYMETRACE;
	WLOG_ATTACH_STRAEM(WL_ERROR, std::cout);
	WLOG_ATTACH_STRAEM(WL_WARNING, std::cout);
	WLOG_ATTACH_STRAEM(WL_INFO, std::cout);

    ghc::filesystem::create_directories("./build/log");

	WLOG_GENERATE_LOG_FILE("./build/log/");

	std::thread* ths[20];
	
	for (int i = 0; i < 20; i++)
	{
		ths[i] = new std::thread(temp);

	}
	for (int i = 0; i < 20; i++)
	{
		ths[i]->join();

	}
}