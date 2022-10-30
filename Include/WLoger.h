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

const static char WLOG_VERSION[] = "0.0.1";

static const auto wlogger_start_time = std::chrono::high_resolution_clock::now();

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
#undef WLOG_COMPILER_MSVC
#define WLOG_COMPILER_MSVC 1
#endif
#if (defined(__clang__) && (__clang__ == 1))
#undef WLOG_COMPILER_CLANG
#define WLOG_COMPILER_CLANG 1
#endif
#if (defined(__MINGW32__) || defined(__MINGW64__))
#undef WLOG_MINGW
#define WLOG_MINGW 1
#endif
#if (defined(__CYGWIN__) && (__CYGWIN__ == 1))
#undef WLOG_CYGWIN
#define WLOG_CYGWIN 1
#endif
#if (defined(__INTEL_COMPILER))
#undef WLOG_COMPILER_INTEL
#define WLOG_COMPILER_INTEL 1
#endif
// Operating System Evaluation
#if (defined(_WIN32) || defined(_WIN64))
#undef WLOG_OS_WINDOWS
#define WLOG_OS_WINDOWS 1
#endif
#if (defined(__linux) || defined(__linux__))
#undef WLOG_OS_LINUX
#define WLOG_OS_LINUX 1
#endif
#if (defined(__APPLE__))
#undef WLOG_OS_MAC
#define WLOG_OS_MAC 1
#endif
#if (defined(__FreeBSD__) || defined(__FreeBSD_kernel__))
#undef WLOG_OS_FREEBSD
#define WLOG_OS_FREEBSD 1
#endif
#if (defined(__sun))
#undef WLOG_OS_SOLARIS
#define WLOG_OS_SOLARIS 1
#endif
#if (defined(_AIX))
#undef WLOG_OS_AIX
#define WLOG_OS_AIX 1
#endif
#if (defined(__NetBSD__))
#undef WLOG_OS_NETBSD
#define WLOG_OS_NETBSD 1
#endif
#if defined(__EMSCRIPTEN__)
#undef WLOG_OS_EMSCRIPTEN
#define WLOG_OS_EMSCRIPTEN 1
#endif
#if (defined(__QNX__) || defined(__QNXNTO__))
#undef WLOG_OS_QNX
#define WLOG_OS_QNX 1
#endif
#if ((WLOG_OS_LINUX || WLOG_OS_MAC || WLOG_OS_FREEBSD || WLOG_OS_NETBSD || WLOG_OS_SOLARIS || WLOG_OS_AIX || WLOG_OS_EMSCRIPTEN || WLOG_OS_QNX) && (!WLOG_OS_WINDOWS))
#undef WLOG_OS_UNIX
#define WLOG_OS_UNIX 1
#endif
#if (defined(__ANDROID__))
#undef WLOG_OS_ANDROID
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

static void __WLogerShutdown();

struct __WLOGER__SINGLTONE {

	struct __MESSAGE
	{
		std::stringstream* message;
		std::chrono::system_clock::time_point time;
		std::string file_name;
		std::string func_name;
		unsigned int line;
		unsigned int level;
		uint32_t straem_id;
	};

	bool stop_sender = false;

	void INIT();

	__WLOGER__SINGLTONE()
	{
		INIT();
	}

	std::stringstream __BAD_BUFFER = std::stringstream("");

	typedef std::string(*__generate_prefix_func_type)(__MESSAGE*);

	__generate_prefix_func_type __generate_prefix_func;

	std::stringstream* __generate_loger_buffer(unsigned int level, bool cond, const char* file, const char* func, unsigned int line);

	void generate_loger(unsigned int, std::string);

	void rename_loger(unsigned int, std::string);

	bool cond(unsigned int level);

	bool attach_stream(unsigned int, std::ostream*);

	bool detach_stream(unsigned int, std::ostream*);

	void generate_log_files(std::string path);

	~__WLOGER__SINGLTONE();
private:

	std::thread* sender_thread;

	void __send(unsigned int level);
	void __send();

	void sender();


};
typedef __WLOGER__SINGLTONE::__MESSAGE wlmesage_t;

extern __WLOGER__SINGLTONE* wloger;


#define WL_ERROR 0x1000u
#define WL_WARNING 0x2000u
#define WL_INFO 0x3000u

#define WLOG_LEVEL 0xffff

#define WLOG_COND(level) (wloger->cond(level))

#define WLOG_NOT_COND(level) !WLOG_COND(level)

#define WLOG_LEVEL_COND(level) (WLOG_LEVEL >= level &&  WLOG_COND(level))

#define WLOG_LEVE_NOT_COND(level) (WLOG_LEVEL < level || WLOG_NOT_COND(level)

#define WLOG_GENERATE_LOGER(level, name) generate_loger(level, name);

#define WLOG_RENAME_LOGER(level, name) rename_loger(level, name);

#define IF_WLOG(level, cond) *wloger->__generate_loger_buffer(level, cond, __FILE__, WLOG_FUNC, __LINE__)

#define WLOG(level) IF_WLOG(level, true)

#define ATTACH_STRAEM(level, stream) if(WLOG_COND(level)) wloger->attach_stream(level, &(stream));

#define GENERATE_LOG_FILE(name) wloger->generate_log_files(name);

#define WLI WLOG(WL_INFO)
#define WLW WLOG(WL_WARNING)
#define WLE WLOG(WL_ERROR)