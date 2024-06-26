#ifndef __WLOGER_H__
#define __WLOGER_H__

#pragma once

#include <ostream>
#include <string>
#include <chrono>
#include <sstream>

#define WLOG_COMPILER_MSVC 0
#define WLOG_COMPILER_GCC 0
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
#if (defined(__GNUC__) && !defined(__clang__))
#undef WLOG_COMPILER_GCC
#define WLOG_COMPILER_GCC 1
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

const static char WLOG_VERSION[] = "0.0.3";

static const auto wlogger_start_time = std::chrono::high_resolution_clock::now();

struct __MESSAGE_DATA;

struct __MESSAGE
{
    void *data;

    void print(std::string);

    template <typename T>
    __MESSAGE& operator<<(const T& val)
    {
        std::stringstream message;
        message << val;
        print(message.str());
        return *this;
    }

    __MESSAGE& operator<<(std::basic_ostream<char, std::char_traits<char>>&(*_Pfn)(std::basic_ostream<char, std::char_traits<char>>&))
    {
        std::ostringstream omes;
        _Pfn(omes);
        print(omes.str());
        return *this;
    }

    __MESSAGE(void* = 0);
    ~__MESSAGE();
};

typedef __MESSAGE_DATA wlmesage_t;


typedef std::string(*__generate_prefix_func_type)(unsigned int, std::string file, std::string func, std::string cond, unsigned int line, std::chrono::system_clock::time_point time, uint32_t straem_id);

extern __generate_prefix_func_type __wloger_generate_prefix_func;

__MESSAGE __wloger_generate_loger_buffer(unsigned int level, bool cond, const char* cond_str, const char* file, const char* func, unsigned int line);

void __wloger_printf(unsigned int level, bool cond, const char* cond_str, const char* file, const char* func, unsigned int line, const char* format, ...);

void __wloger_generate_loger(unsigned int, std::string);

void __wloger_rename_loger(unsigned int, std::string);

bool __wloger_cond(unsigned int level);

bool __wloger_attach_stream(unsigned int, std::ostream*);

bool __wloger_detach_stream(unsigned int, std::ostream*);

void __wloger_generate_log_files(std::string path);

void __wloger_generate_log_files(std::string path);

unsigned int __wlog_get_log_level();

void __wlog_set_log_level(unsigned int);

static const unsigned int WL_FATAL = 0x1000u;
static const unsigned int WL_ERROR = 0x2000u;
static const unsigned int WL_WARNING = 0x3000u;
static const unsigned int WL_INFO = 0x4000u;
static const unsigned int WL_DEBUG = 0x5000u;

#define __WLOG_VALUE_TSTR(val) #val

#define WLOG_LEVEL __wlog_get_log_level()

#define WLOG_SET_LEVEL(level) __wlog_set_log_level(level)

#define WLOG_ATTACH_STRAEM(level, stream) __wloger_attach_stream(level, &(stream));

#define WLOG_GENERATE_LOG_FILE(name) __wloger_generate_log_files(name);

#define WLOG_COND(level) (__wloger_cond(level))

#define WLOG_NOT_COND(level) !WLOG_COND(level)

#define WLOG_LEVEL_COND(level) (WLOG_LEVEL >= level &&  WLOG_COND(level))

#define WLOG_LEVE_NOT_COND(level) (WLOG_LEVEL < level || WLOG_NOT_COND(level))

#define WLOG_GENERATE_LOGER(level, name) __wloger_generate_loger(level, name)

#define WLOG_RENAME_LOGER(level, name) __wloger_rename_loger(level, name)

#define IF_WLOG(level, cond) __wloger_generate_loger_buffer(level, cond, __WLOG_VALUE_TSTR(cond), __FILE__, WLOG_FUNC, __LINE__)

#define WLOG(level) IF_WLOG(level, true)

#define IF_PWLOG(level, cond, ...) __wloger_printf(level, cond, __WLOG_VALUE_TSTR(cond), __FILE__, WLOG_FUNC, __LINE__, __VA_ARGS__)

#define PWLOG(level, ...) IF_PWLOG(level, true, __VA_ARGS__)




#define WLD WLOG(WL_DEBUG)
#define PWLD(...) PWLOG(WL_DEBUG, __VA_ARGS__)

#define WLI WLOG(WL_INFO)
#define PWLI(...) PWLOG(WL_INFO, __VA_ARGS__)

#define WLW WLOG(WL_WARNING)
#define PWLW(...) PWLOG(WL_WARNING, __VA_ARGS__)

#define WLE WLOG(WL_ERROR)
#define PWLE(...) PWLOG(WL_ERROR, __VA_ARGS__)

#define WLF WLOG(WL_FATAL)
#define PWLF(...) PWLOG(WL_FATAL, __VA_ARGS__)

#define WLOG_VALUE(val) __WLOG_VALUE_TSTR(val) << " = " << val << "; "

#undef max
#undef min
#endif // __WLOGER_H__