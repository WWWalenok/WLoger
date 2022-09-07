#pragma once
#include <string>
#include <sstream>
#include <ostream>
#include <chrono>
#include <thread>
#include <queue>
#include <vector>
#include <map>
#include <mutex>


static auto wlogger_start_time = std::chrono::high_resolution_clock::now();

#define WLOG_COMPILER_MSVC 0
#define WLOG_COMPILER_MSVC 0
#define WLOG_COMPILER_CLANG 0
#define WLOG_COMPILER_INTEL 0

#define WLOG_MINGW 0
#define WLOG_CYGWIN 0

#define WLOG_OS_WINDOWS 0
#define WLOG_OS_LINUX 0
#define WLOG_OS_MAC 0
#define WLOG_OS_FREEBSD 0
#define WLOG_OS_SOLARIS 0
#define WLOG_OS_AIX 0
#define WLOG_OS_NETBSD 0
#define WLOG_OS_EMSCRIPTEN 0
#define WLOG_OS_UNIX 0
#define WLOG_OS_ANDROID 0
#define WLOG_OS_QNX 0

#if defined(_MSC_VER)
#define WLOG_COMPILER_MSVC 1
#endif
#if (defined(__clang__) && (__clang__ == 1))
#define WLOG_COMPILER_CLANG 1
#endif
#if (defined(__MINGW32__) || defined(__MINGW64__))
#define WLOG_MINGW 1
#endif
#if (defined(__CYGWIN__) && (__CYGWIN__ == 1))
#define WLOG_CYGWIN 1
#endif
#if (defined(__INTEL_COMPILER))
#define WLOG_COMPILER_INTEL 1
#endif
// Operating System Evaluation
#if (defined(_WIN32) || defined(_WIN64))
#define WLOG_OS_WINDOWS 1
#endif
// Linux
#if (defined(__linux) || defined(__linux__))
#define WLOG_OS_LINUX 1
#endif
#if (defined(__APPLE__))
#define WLOG_OS_MAC 1
#endif
#if (defined(__FreeBSD__) || defined(__FreeBSD_kernel__))
#define WLOG_OS_FREEBSD 1
#endif
#if (defined(__sun))
#define WLOG_OS_SOLARIS 1
#endif
#if (defined(_AIX))
#define WLOG_OS_AIX 1
#endif
#if (defined(__NetBSD__))
#define WLOG_OS_NETBSD 1
#endif
#if defined(__EMSCRIPTEN__)
#define WLOG_OS_EMSCRIPTEN 1
#endif
#if (defined(__QNX__) || defined(__QNXNTO__))
#define WLOG_OS_QNX 1
#endif
#if ((WLOG_OS_LINUX || WLOG_OS_MAC || WLOG_OS_FREEBSD || WLOG_OS_NETBSD || WLOG_OS_SOLARIS || WLOG_OS_AIX || WLOG_OS_EMSCRIPTEN || WLOG_OS_QNX) && (!WLOG_OS_WINDOWS))
#define WLOG_OS_UNIX 1
#endif
#if (defined(__ANDROID__))
#define WLOG_OS_ANDROID 1
#endif

std::string WLOG_GET_EXE_PATH();




#undef WLOG_FUNC
#if defined(_MSC_VER)  // Visual C++
#define WLOG_FUNC __FUNCSIG__
#elif WLOG_COMPILER_GCC  // GCC
#define WLOG_FUNC __PRETTY_FUNCTION__
#elif WLOG_COMPILER_INTEL  // Intel C++
#define WLOG_FUNC __PRETTY_FUNCTION__
#elif (defined(__clang__) && (__clang__ == 1))  // Clang++
#define WLOG_FUNC __PRETTY_FUNCTION__
#else
#if defined(__func__)
#define WLOG_FUNC __func__
#else
#define WLOG_FUNC ""
#endif
#endif

#if WLOG_OS_WINDOWS
#include <windows.h>
#include "dbghelp.h"

#include <windows.h>
#include "dbghelp.h"

// based on dbghelp.h
typedef BOOL(WINAPI* WLOG_MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
	CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
	CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam
	);

LONG WINAPI TopLevelFilter(struct _EXCEPTION_POINTERS* pExceptionInfo);

#endif

struct __WLOGER__SINGLTONE {

	bool stop_sender = false;

	void INIT();

	__WLOGER__SINGLTONE()
	{
		INIT();
	}

	std::map<unsigned int, std::vector<std::ostream*>> __loger__out__streams = std::map<unsigned int, std::vector<std::ostream*>>();
	std::map<unsigned int, std::string> __loger__name = std::map<unsigned int, std::string>();
	std::map<unsigned int, std::queue<std::stringstream*>> __loger__buffers = std::map<unsigned int, std::queue<std::stringstream*>>();
	std::map<unsigned int, std::mutex*> __loger__buffers_mutex = std::map<unsigned int, std::mutex*>();

	std::stringstream __BAD_BUFFER = std::stringstream("");

	typedef std::string(*__generate_prefix_func_type)(unsigned int level, const char* file, const char* func, int line);

	__generate_prefix_func_type __generate_prefix_func;

	std::stringstream* __generate_loger_buffer(unsigned int level, bool cond);

	~__WLOGER__SINGLTONE();
private:

	std::thread* sender_thread;

	void __send(unsigned int level);
	void __send();

	void sender();


};

extern __WLOGER__SINGLTONE* wloger;


#define WL_ERROR 0x0000u
#define WL_WARNING 0x1000u
#define WL_INFO 0x2000u

#define WLOG_LEVEL 0xffff

#define __WLOG_GETPREFIX(level) wloger->__generate_prefix_func(level, __FILE__, WLOG_FUNC, __LINE__)

#define WLOG_COND(level) (wloger->__loger__out__streams.find(level) != wloger->__loger__out__streams.end() && wloger->__loger__name.find(level) != wloger->__loger__name.end() && wloger->__loger__buffers.find(level) != wloger->__loger__buffers.end())

#define WLOG_NOT_COND(level) (wloger->__loger__out__streams.find(level) == wloger->__loger__out__streams.end() || wloger->__loger__name.find(level) == wloger->__loger__name.end() || wloger->__loger__buffers.find(level) == wloger->__loger__buffers.end())

#define WLOG_LEVEL_COND(level) (WLOG_LEVEL >= level && WLOG_COND(level))

#define WLOG_LEVE_NOT_COND(level) (WLOG_LEVEL < level || WLOG_NOT_COND(level))


#define WLOG_GENERATE_LOGER(level, name){ \
	wloger->__loger__out__streams[level] = std::vector<std::ostream*>(); \
	wloger->__loger__name[level] = name; \
	wloger->__loger__buffers[level] = std::queue<std::stringstream*>();\
	wloger->__loger__buffers_mutex[level] = new std::mutex(); \
}

#define IF_WLOG(level, cond) *wloger->__generate_loger_buffer(level, cond) << __WLOG_GETPREFIX(level)

#define WLOG(level) IF_WLOG(level, true)

#define ATTACH_STRAEM(level, stream) if(WLOG_COND(level)) wloger->__loger__out__streams[level].push_back(&(stream));
