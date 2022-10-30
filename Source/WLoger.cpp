#define _CRT_SECURE_NO_WARNINGS
#include "../Include/WLoger.h"

#include <iostream>
#include <iomanip>
#include <vector>
#include <numeric>
#include <chrono>

std::map<unsigned int, std::vector<std::ostream*>> __loger__out__streams = std::map<unsigned int, std::vector<std::ostream*>>();
std::map<unsigned int, std::string> __loger__name = std::map<unsigned int, std::string>();
std::map<unsigned int, std::vector<wlmesage_t*>> __loger__buffers = std::map<unsigned int, std::vector<wlmesage_t*>>();
std::map<unsigned int, std::mutex*> __loger__buffers_mutex = std::map<unsigned int, std::mutex*>();

void __WLOGER__SINGLTONE::generate_loger(unsigned int level, std::string name)
{
	__loger__name[level] = name; 
	if (!cond(level))
	{
		__loger__out__streams[level] = std::vector<std::ostream*>();
		__loger__buffers[level] = std::vector<wlmesage_t*>();
		__loger__buffers_mutex[level] = new std::mutex();
	}
}

void __WLOGER__SINGLTONE::rename_loger(unsigned int level, std::string name)
{
	if(cond(level))
		__loger__name[level] = name;
}


bool __WLOGER__SINGLTONE::cond(unsigned int level)
{
	return __loger__out__streams.find(level) != __loger__out__streams.end() 
		&& __loger__name.find(level) != __loger__name.end() 
		&& __loger__buffers.find(level) != __loger__buffers.end()
		&& __loger__buffers_mutex.find(level) != __loger__buffers_mutex.end();
}

bool __WLOGER__SINGLTONE::attach_stream(unsigned int level, std::ostream* stream)
{
	if (cond(level))
	{
		auto *strams = &__loger__out__streams[level];
		bool unic = true;
		for (int i = 0; i < strams->size() && unic; i++)
			unic = (stream != strams->operator[](i));
		if(unic)
			__loger__out__streams[level].push_back(stream);
	}
	else
		return false;
	return true;
}

bool __WLOGER__SINGLTONE::detach_stream(unsigned int level, std::ostream* stream)
{
	if (cond(level))
	{
		auto s = &__loger__out__streams[level];
		for(int i = 0; i < s->size(); i++)
			if (s->operator[](i) == stream)
			{
				s->erase(s->begin() + i);
				return true;
			}
	}
	return false;
}

__WLOGER__SINGLTONE* wloger = new __WLOGER__SINGLTONE();


struct Guard {
	~Guard() {
		if (wloger)
		{

			delete wloger;
		}
	}
};

Guard guard = Guard();

static void __WLogerShutdown() {
	guard.~Guard();
}

#ifdef EXEPT_CAPTION_WLOG

#if WLOG_OS_WINDOWS

WCHAR app_path[MAX_PATH];

LONG WINAPI TopLevelCycleFilter(struct _EXCEPTION_POINTERS* pExceptionInfo)
{
	int retval = EXCEPTION_EXECUTE_HANDLER;
	int rt;
	for (;;) {
		rt = 5;
		if (rt == 4)
			break;
		Sleep(100);
	}
	return retval;
}
LONG WINAPI TopLevelEmptyFilter(struct _EXCEPTION_POINTERS* pExceptionInfo)
{
	int retval = EXCEPTION_EXECUTE_HANDLER;
	return retval;
}

LONG WINAPI TopLevelFilter(struct _EXCEPTION_POINTERS* pExceptionInfo)
{
	WLOG(WL_ERROR) << "Catch exeption TopLevelFilter";

	WCHAR szDumpPath[_MAX_PATH];
	WCHAR szScratch[_MAX_PATH];
	LONG retval = EXCEPTION_CONTINUE_SEARCH;
	HWND hParent = NULL;						// find a better value for your app

	HMODULE hDll = NULL;

	// load any version we can

	hDll = LoadLibraryW(L"DBGHELP.DLL");

	WCHAR* szResult = NULL;

	if (hDll)
	{
		WLOG_MINIDUMPWRITEDUMP pDump = (WLOG_MINIDUMPWRITEDUMP)GetProcAddress(hDll, "MiniDumpWriteDump");
		if (pDump)
		{

			ZeroMemory(szDumpPath, _MAX_PATH);
			ZeroMemory(szScratch, _MAX_PATH);

			wcscpy(szDumpPath, app_path);


			wcscat(szDumpPath, L"app.dmp");



			// ask the user if they want to save a dump file
			if (MessageBoxW(NULL, L"A fatal exception has occured, would you like to save a diagnostic file?", szDumpPath, MB_YESNO) == IDYES)
			{
				// create the file
				HANDLE hFile = CreateFileW(szDumpPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL, NULL);

				if (hFile != INVALID_HANDLE_VALUE)
				{
					_MINIDUMP_EXCEPTION_INFORMATION ExInfo;

					ExInfo.ThreadId = GetCurrentThreadId();
					ExInfo.ExceptionPointers = pExceptionInfo;
					ExInfo.ClientPointers = NULL;

					// write the dump
					BOOL bOK = pDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &ExInfo, NULL, NULL);
					if (bOK)
					{
						wprintf(szScratch, "Successfully saved %s", szDumpPath);
						szResult = szScratch;
						retval = EXCEPTION_EXECUTE_HANDLER;
					}
					else
					{
						wprintf(szScratch, "Failed to save %s (error %d)", szDumpPath, GetLastError());
						szResult = szScratch;
					}
					CloseHandle(hFile);
				}
				else
				{
					wprintf(szScratch, "Failed to create %s (error %d)", szDumpPath, GetLastError());
					szResult = szScratch;
				}
			}
		}
		else
		{
			szResult = (WCHAR*)L"DBGHELP.DLL is outdated";
		}
	}
	else
	{
		printf("DBGHELP.DLL was not found");
		szResult = (WCHAR*)L"DBGHELP.DLL was not found";
	}

	if (szResult)
		MessageBoxW(NULL, szResult, szDumpPath, MB_OK);

	delete wloger;

	return retval;
}

#endif

#endif

#if WLOG_OS_WINDOWS
#include <libloaderapi.h>

std::string WLOG_GET_EXE_PATH() {
	char buff[MAX_PATH];

	GetModuleFileNameA(NULL, buff, MAX_PATH);

	size_t len = strlen(buff);
	for (size_t i = len - 1; i >= 0; i--)
	{
		if (buff[i] == '\\')
		{
			buff[i + 1] = 0;
			break;
		}
	}
	#ifdef EXEPT_CAPTION_WLOG
	mbstowcs(app_path, buff, MAX_PATH);
	#endif
	return buff;
}
#endif

#if WLOG_OS_LINUX
std::string get_selfpath() {
	char buff[1024];
	ssize_t len = ::readlink("/proc/self/exe", buff, sizeof(buff) - 1);
	if (len != -1) {
		buff[len] = '\0';
		return std::string(buff);
	}
	/* handle error condition */
}
#endif

static std::string __base_generate_prefix_func(wlmesage_t* message)
{
	size_t lenght = message->file_name.length();
	std::string _file = "";
	for (size_t i = 0; i < lenght; i++)
	{
		_file.append(1, message->file_name[i]);
		if (message->file_name[i] == '\\')
			_file.clear();
	};

	size_t ms = std::lround(double(std::chrono::duration<double, std::milli>(message->time.time_since_epoch()).count()));
	std::time_t tp = std::chrono::system_clock::to_time_t(message->time);
	char buff[256];
	auto count = strftime(buff, 256, "%T", std::localtime(&tp));


	std::string out;
	out.append("[");
	out.append(buff);
	out.append(".");
	out.append(std::to_string((ms % 1000) / 100));
	out.append(std::to_string((ms % 100) / 10));
	out.append(std::to_string((ms % 10)));
	out.append("] ");
	out.append(_file);
	out.append(":");
	out.append(std::to_string(message->line));
	out.append(" ");
	out.append(std::to_string(message->straem_id));
	if (message->func_name.size() < 50)
	{
		out.append(" ");
		out.append(message->func_name);
	}
	out.append(" => ");
	if (__loger__name.find(message->level) != __loger__name.end())
		if (__loger__name[message->level].length() > 0)
		{
			out.append("\'");
			out.append(__loger__name[message->level]);
			out.append("\' => ");
		}
	return out;
}

void __WLOGER__SINGLTONE::INIT()
{
	WLOG_GET_EXE_PATH();
#ifdef EXEPT_CAPTION_WLOG
#if WLOG_OS_WINDOWS
	SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)TopLevelFilter);
#endif
#endif
	__generate_prefix_func = __base_generate_prefix_func;

	WLOG_GENERATE_LOGER(WL_ERROR, "ERROR");
	WLOG_GENERATE_LOGER(WL_WARNING, "WARNING");
	WLOG_GENERATE_LOGER(WL_INFO, "INFO");

	sender_thread = new std::thread(&__WLOGER__SINGLTONE::sender, this);
}

void __WLOGER__SINGLTONE::__send()
{
	__BAD_BUFFER = std::stringstream("");

	std::vector<unsigned int> loger__buffers__levels;
	std::vector<std::vector<wlmesage_t*>> loger__buffers__queue;
	std::vector<uint32_t> loger__buffers__lasts;
	for (auto pair_streams : __loger__out__streams)
	{
		auto level = pair_streams.first;
		{
			std::lock_guard<std::mutex> guard(*__loger__buffers_mutex[level]);
			auto buffer = __loger__buffers[level];
			loger__buffers__queue.push_back(buffer);
			loger__buffers__levels.push_back(level);
			__loger__buffers[level] = std::vector<wlmesage_t*>();
			loger__buffers__lasts.push_back(0);
		}
	}
	bool count = true;

	std::map<std::ostream*, std::string> str_buffers;

	for (auto _pair : __loger__out__streams)
		for (auto stream : _pair.second)
			str_buffers[stream] = "";


	size_t size = loger__buffers__queue.size();
	while (count)
	{
		count = false;
		wlmesage_t* best = 0;
		int best_i = 0;
		for (int i = 0; i < size; i++)
		{
			if (loger__buffers__lasts[i] != loger__buffers__queue[i].size())
			{
				auto select = loger__buffers__queue[i][loger__buffers__lasts[i]];
				if (!count)
				{
					best = select;
					best_i = i;
				}
				else
					if (best->time > select->time)
					{
						best = select;
						best_i = i;
					}
				count = true;
			}
		}

		if (best)
		{
			loger__buffers__lasts[best_i]++;
			std::string str_buffer = wloger->__generate_prefix_func(best);
			auto buffer = best->message;
			if (!buffer)
				continue;

			int i = 0;
			std::string str = " ";
			std::getline(*buffer, str);
			while (*buffer)
			{
				if (i != 0)
					str_buffer.append("||\t");

				str_buffer.append(str).append("\n");
				i++;
				std::getline(*buffer, str);
			}

			for (auto stream : __loger__out__streams[best->level])
				str_buffers[stream].append(str_buffer);

			buffer->clear();
			delete buffer;
			delete best;
		}

	}

	for (auto _pair : __loger__out__streams)
		for (auto stream : _pair.second)
		{
			*stream << str_buffers[stream];
			str_buffers[stream] = "";
		};

	for (auto pair_streams : __loger__out__streams)
		for (auto stream : pair_streams.second)
			stream->flush();
}

void  __WLOGER__SINGLTONE::sender()
{
	bool _stop_sender = false;
	while (!_stop_sender)
	{
		std::chrono::milliseconds timespan(20);
		std::this_thread::sleep_for(timespan);
		_stop_sender = stop_sender;
		__send();
		
	}
}

std::stringstream* __WLOGER__SINGLTONE::__generate_loger_buffer(unsigned int level, bool cond, const char* file, const char* func, unsigned int line)
{
	std::stringstream* ret = &__BAD_BUFFER;
	if (cond && WLOG_LEVEL_COND(level)) {
		std::chrono::system_clock::time_point tp;

		tp = std::chrono::system_clock::now();

		auto id = std::this_thread::get_id();
		uint32_t _id = *((uint32_t*)((void*)(&id)));

		ret = new std::stringstream;
		wlmesage_t* mesage = new wlmesage_t;
		*mesage = wlmesage_t{
			ret, 
			tp,
			file,
			func,
			line,
			level,
			_id
		};
		std::lock_guard<std::mutex> guard(*__loger__buffers_mutex[level]);
		__loger__buffers[level].push_back(mesage);
	}
	return ret;
}

__WLOGER__SINGLTONE::~__WLOGER__SINGLTONE() {
	stop_sender = true;
	sender_thread->join();
	if (wloger == this)
		wloger = 0;
}

#include <fstream>

void __WLOGER__SINGLTONE::generate_log_files(std::string path)
{
	size_t pl = path.length();
	if (path.length())
	{
		if (path[pl - 1] != '\\')
			path.append("\\");
	}
	std::time_t tp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	for (auto pair_name : __loger__name)
	{
		auto level = pair_name.first;
		auto name = pair_name.second;

		if (WLOG_LEVEL >= level)
		{
			auto filename = path;
			
			char buff[512];
			auto count = strftime(buff, 512, "%y%m%d_%H%M%S", std::localtime(&tp));
			filename.append(name).append("_").append(buff).append(".log");
			std::ofstream* fout = new std::ofstream(filename);

			count = strftime(buff, 512, "[%T]", std::localtime(&tp));

			*fout << "LOGING START : " << buff << "\n-=-=-=-=-=-=-=-=-=-=-=-=-=\n\n";

			attach_stream(level, fout);
		}
	}
}
