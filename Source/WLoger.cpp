#define _CRT_SECURE_NO_WARNINGS
#include "..\Include\WLoger.h"

__WLOGER__SINGLTONE* wloger = new __WLOGER__SINGLTONE();


struct Guard {
	~Guard() {
		delete wloger;
	}
};

Guard guard = Guard();

#if WLOG_OS_WINDOWS
#include <libloaderapi.h>

WCHAR app_path[MAX_PATH];

std::string WLOG_GET_EXE_PATH() {
	char buff[MAX_PATH];

	GetModuleFileNameA(NULL, buff, MAX_PATH);

	auto len = strlen(buff);
	for (int i = len - 1; i >= 0; i--)
	{
		if (buff[i] == '\\')
		{
			buff[i + 1] = 0;
			break;
		}
	}
	mbstowcs(app_path, buff, MAX_PATH);
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

#if WLOG_OS_WINDOWS

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
			szResult = L"DBGHELP.DLL is outdated";
		}
	}
	else
	{
		printf("DBGHELP.DLL was not found");
		szResult = L"DBGHELP.DLL was not found";
	}

	if (szResult)
		MessageBoxW(NULL, szResult, szDumpPath, MB_OK);

	delete wloger;

	return retval;
}

#endif

static std::string __base_generate_prefix_func(unsigned int level, const char* file, const char* func, int line)
{
	int lenght = strlen(file);
	std::string _file = "";
	for (int i = 0; i < lenght; i++)
	{
		_file.append(1, file[i]);
		if (file[i] == '\\')
			_file.clear();
	}
	auto time = std::chrono::high_resolution_clock::now();
	int ms = std::chrono::duration<double, std::milli>(time - wlogger_start_time).count();
	std::string out;
	out.append("[");
	out.append(std::to_string(ms / 10000));
	out.append(std::to_string((ms / 1000) % 10));
	out.append(":");
	out.append(std::to_string((ms % 1000) / 100));
	out.append(std::to_string((ms % 100) / 10));
	out.append(std::to_string((ms % 10)));
	out.append("] ");
	out.append(_file);
	out.append(":");
	out.append(std::to_string(line));
	out.append(" ");
	auto id = std::this_thread::get_id();
	unsigned int _id = *((unsigned int*)((void*)(&id)));
	out.append(std::to_string(_id));
	out.append(" ");
	out.append(func);
	out.append(" => ");
	if (wloger->__loger__name.find(level) != wloger->__loger__name.end())
		if (wloger->__loger__name[level].length() > 0)
		{
			out.append("\'");
			out.append(wloger->__loger__name[level]);
			out.append("\' => ");
		}
	return out;
}

void __WLOGER__SINGLTONE::INIT()
{
	WLOG_GET_EXE_PATH();
#if WLOG_OS_WINDOWS
	SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)TopLevelFilter); //���������� ���� �� ����
#endif
	__generate_prefix_func = __base_generate_prefix_func;
	wloger = this;

	WLOG_GENERATE_LOGER(WL_ERROR, "ERROR");
	WLOG_GENERATE_LOGER(WL_WARNING, "WARNING");
	WLOG_GENERATE_LOGER(WL_INFO, "INFO");

	sender_thread = new std::thread(&__WLOGER__SINGLTONE::sender, this);
}

void  __WLOGER__SINGLTONE::__send(unsigned int level)
{
	if (WLOG_NOT_COND(level))
		return;



	auto& streams = __loger__out__streams[level];
	std::queue<std::stringstream*> buffers;
	{
		std::lock_guard<std::mutex> guard(*__loger__buffers_mutex[level]);
		buffers = __loger__buffers[level];
		__loger__buffers[level] = std::queue<std::stringstream*>();
	}


	if (buffers.empty())
		return;

	std::string str_buffer = "";

	while (buffers.size() > 0)
	{
		auto& buffer = buffers.front();
		buffers.pop();
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
		buffer->clear();
		delete buffer;
	}


	for (auto stream : streams)
	{
		*stream << str_buffer;
	}

}

void __WLOGER__SINGLTONE::__send()
{
	__BAD_BUFFER = std::stringstream("");
	for (auto pair_streams : __loger__out__streams)
		__send(pair_streams.first);

	for (auto pair_streams : __loger__out__streams)
		for (auto stream : pair_streams.second)
			stream->flush();
}

void  __WLOGER__SINGLTONE::sender()
{
	bool _stop_sender = stop_sender;
	while (!_stop_sender)
	{
		_stop_sender = stop_sender;
		__send();
		std::chrono::milliseconds timespan(100);
		std::this_thread::sleep_for(timespan);
	}
}

std::stringstream* __WLOGER__SINGLTONE::__generate_loger_buffer(unsigned int level, bool cond)
{
	std::stringstream* ret = &__BAD_BUFFER;
	if (cond && WLOG_LEVEL_COND(level)) {
		ret = new std::stringstream;
		std::lock_guard<std::mutex> guard(*__loger__buffers_mutex[level]);
		__loger__buffers[level].push(ret);
	}
	return ret;
}

__WLOGER__SINGLTONE::~__WLOGER__SINGLTONE() {
	stop_sender = true;
	sender_thread->join();
}
