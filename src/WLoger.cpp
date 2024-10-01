#define _CRT_SECURE_NO_WARNINGS
#include "../include/WLoger.h"
#include <signal.h> 
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <iostream>
#include <iomanip>
#include <numeric>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <atomic>

void __wloger_INIT_NATIVE();
void __wloger_INIT();

std::thread* sender_thread;
bool stop_sender = false;

struct Guard 
{
	Guard()
	{
		__wloger_INIT();
	}

	~Guard() 
	{
		stop_sender = true;
		sender_thread->join();
	}
};

Guard guard = Guard();

struct __wlog__locker
{
    void lock() { while (flag.test_and_set(std::memory_order_acquire)); }
    void unlock() { flag.clear(std::memory_order_release); }

	__wlog__locker() { flag.clear(); }

    std::atomic_flag flag{};

	struct __wlog__lock_guard_t
	{
		__wlog__lock_guard_t(__wlog__locker* ori)
		{
			parent = ori;
			parent->lock();
		}
		__wlog__locker* parent;
		~__wlog__lock_guard_t()
		{
			parent->unlock();
		}
	};

	__wlog__lock_guard_t lock_guard() { return __wlog__lock_guard_t(this); }
};

struct __profiler_t
{
	size_t acc = 0;
	size_t cnt = 0;
	std::string name = "";
    
    void lock() { while (flag.test_and_set(std::memory_order_acquire)); }
    void unlock() { flag.clear(std::memory_order_release); }

    std::atomic_flag flag{};

	__profiler_t() { flag.clear(); }
};

std::unordered_map<unsigned int, __profiler_t*> __profiler  = std::unordered_map<unsigned int, __profiler_t*>();

void __wlog_acc_stat(const char* file, const char* func, size_t acc)
{
	static __wlog__locker locker;

	unsigned int h1 = 5381;
    for (const unsigned char* ptr = (const unsigned char*)file; *ptr; ptr++) 
        h1 = ((h1 << 5) + h1) + *ptr;
    
    unsigned int h2 = 0;
	for (const unsigned char* ptr = (const unsigned char*)func; *ptr; ptr++) 
        h2 = *ptr + (h2 << 6) + (h2 << 16) - h2;
    
    auto h = h1 ^ h2;
	__profiler_t* t = new __profiler_t();
	locker.lock();
	auto r = __profiler.emplace(h, t);
	locker.unlock();
	if(r.second)
	{
		r.first->second->lock();
		r.first->second->acc = acc;
		r.first->second->cnt = 1;
		r.first->second->name = func;
		r.first->second->unlock();
	}
	else
	{
		r.first->second->lock();
		r.first->second->acc += acc;
		r.first->second->cnt += 1;
		r.first->second->unlock();
	}
}


#if WLOG_COMPILER_GCC
    #define va_start(v,l)   __builtin_va_start(v,l)
    #define va_end(v)       __builtin_va_end(v)
    #define va_arg(v,l)     __builtin_va_arg(v,l)
#endif

unsigned char __wlog_level = 0xffu;

unsigned char __wlog_get_log_level()
{
    return __wlog_level;
}

void __wlog_set_log_level(unsigned int val)
{
    __wlog_level = val;
}

__generate_prefix_func_type __wloger_generate_prefix_func;

__MESSAGE __BAD_BUFFER(nullptr);

struct __wlog__logger_data
{
	std::vector<std::ostream*> out_streams;
	std::string name;
	std::vector<wlmesasge_t*> messages;
	__wlog__locker locker;
};

__wlog__logger_data* __logers[0x100];

struct __MESSAGE_DATA
{
    std::stringstream message;
	size_t ns;
	std::string file_name;
	std::string func_name;
    std::string cond_str;
	unsigned int line;
	unsigned int level;
	uint32_t straem_id;
    bool in_process;
    
    void lock() { while (flag.test_and_set(std::memory_order_acquire)); }
    void unlock() { flag.clear(std::memory_order_release); }

	__MESSAGE_DATA() { flag.clear(); }

    std::atomic_flag flag{};

	struct lock_guard_t
	{
		lock_guard_t(__MESSAGE_DATA* ori)
		{
			parent = ori;
			parent->lock();
		}
		__MESSAGE_DATA* parent;
		~lock_guard_t()
		{
			parent->unlock();
		}
	};

	lock_guard_t lock_guard() { return lock_guard_t(this); }
};

__MESSAGE::__MESSAGE(void* dt)
{
    data = dt;
    if(!data)
        return;
    auto message = (__MESSAGE_DATA*)data;
	message->lock_guard();
    message->in_process = true;
}

void __MESSAGE::print(std::string str)
{
    if(!data)
        return;
    auto message = (__MESSAGE_DATA*)data;
	message->lock_guard();
    message->in_process = true;

    message->message << str;
}

__MESSAGE::~__MESSAGE()
{
    if(!data)
        return;
    auto message = (__MESSAGE_DATA*)data;
	message->lock_guard();
    message->in_process = false;
    __logers[message->level]->locker.lock();
    __logers[message->level]->messages.push_back(message);
	__logers[message->level]->locker.unlock();
}


void __wloger_generate_loger(unsigned int level, std::string name)
{
	if(__logers[level] == nullptr)
	{
		__logers[level] = new __wlog__logger_data();
		__logers[level]->name = name;
	}
}

void __wloger_rename_loger(unsigned int level, std::string name)
{
	if(__logers[level] == nullptr)
	{
		__logers[level] = new __wlog__logger_data();
	}
	__logers[level]->name = name;
}


bool __wloger_cond(unsigned int level)
{
	return __logers[level] != nullptr;
}

bool __wloger_attach_stream(unsigned int level, std::ostream* stream)
{
	if (__wloger_cond(level))
	{
		auto *strams = &__logers[level]->out_streams;
		bool unic = true;
		for (int i = 0; i < strams->size() && unic; i++)
			unic = (stream != strams->operator[](i));
		if(unic)
			__logers[level]->out_streams.push_back(stream);
	}
	else
		return false;
	return true;
}

bool __wloger_detach_stream(unsigned int level, std::ostream* stream)
{
	if (__wloger_cond(level))
	{
		auto s = &__logers[level]->out_streams;
		for(int i = 0; i < s->size(); i++)
			if (s->operator[](i) == stream)
			{
				s->erase(s->begin() + i);
				return true;
			}
	}
	return false;
}

static std::string __base_generate_prefix_func(unsigned int level, std::string file, std::string func, std::string cond, unsigned int line, size_t ns, uint32_t straem_id)
{
	size_t lenght = file.length();
	std::string _file = "";
	for (size_t i = 0; i < lenght; i++)
	{
		_file.append(1, file[i]);
		if (file[i] == '\\' || file[i] == '/')
			_file.clear();
	};

	constexpr double hrt2us = std::chrono::high_resolution_clock::period::num * 1e6 / double(std::chrono::high_resolution_clock::period::den);
	constexpr double st2us  = std::chrono::system_clock::period::num * 1e6 / double(std::chrono::system_clock::period::den);
	constexpr double st2hrt = 
		double(std::chrono::system_clock::period::num * std::chrono::high_resolution_clock::period::den) / 
		double(std::chrono::system_clock::period::den * std::chrono::high_resolution_clock::period::num);
	constexpr double hrt2st = 
		double(std::chrono::high_resolution_clock::period::num * std::chrono::system_clock::period::den) / 
		double(std::chrono::high_resolution_clock::period::den * std::chrono::system_clock::period::num);
	static const size_t st_offset = size_t(wlogger_start_data_time.time_since_epoch().count() * st2hrt) % size_t(1e10);
	int us = size_t((ns - wlogger_start_ns + st_offset) * hrt2us) % 1000000;

	auto tm = wlogger_start_data_time + std::chrono::system_clock::duration(size_t((ns - wlogger_start_ns) * hrt2st));
	
	std::time_t tp = std::chrono::system_clock::to_time_t(tm);
	char buff[256];
    auto lct = std::localtime(&tp);
	auto count = snprintf(buff, 256, "[%.2i:%.2i:%.2i.%.6i] ", lct->tm_hour, lct->tm_min, lct->tm_sec, us);


	std::string out;
	out.append(buff);
	out.append(_file);
	out.append(":");
	out.append(std::to_string(line));
	out.append(" ");
	out.append(std::to_string(straem_id));
    out.append(" ");
    if(func.size() > 100)
        func = func.substr(0,97) + "...";
    out.append(func);
    if(cond == "true")
	    out.append(": ");
    else
    {
        out.append("if(");
        out.append(cond);
        out.append("): ");
    }
    
	if (__wloger_cond(level))
	{
		auto name = __logers[level]->name;
		if (name.length() > 0)
		{
			out.append(name);
			out.append(": ");
		}
	}
	return out;
}

std::string generate_prefix_func_from_message(wlmesasge_t* m)
{
    return __wloger_generate_prefix_func(m->level, m->file_name, m->func_name, m->cond_str, m->line, m->ns, m->straem_id);
}

void __wloger_send()
{
	static __wlog__locker locker;
	
	locker.lock();

	std::vector<unsigned int> loger__buffers__levels;
	std::vector<std::vector<wlmesasge_t*>> loger__buffers__queue;
	std::vector<uint32_t> loger__buffers__lasts;
	for (int level = 0; level < 0x100; ++level) if(__logers[level] != nullptr)
	{
		auto data = __logers[level];
		{
			data->locker.lock();
			loger__buffers__queue.push_back(data->messages);
			loger__buffers__levels.push_back(level);
			data->messages = std::vector<wlmesasge_t*>();
			loger__buffers__lasts.push_back(0);
			data->locker.unlock();
		}
	}
	bool count = true;

	std::unordered_map<std::ostream*, std::string> str_buffers;
    std::unordered_map<std::ostream*, std::ostream*> stream_buffers;

	for (int level = 0; level < 0x100; ++level) if(__logers[level] != nullptr)
		for (auto stream : __logers[level]->out_streams)
        {
			str_buffers[stream] = "";
            stream_buffers[stream] = stream;
        }


	size_t size = loger__buffers__queue.size();

    struct temp_t
    {
        wlmesasge_t* message = 0;
        int i = -1;
        int64_t ts = -1;

        bool operator<(const temp_t& other)
        {
            return ts < other.ts;
        }
    };

    std::vector<temp_t> mes_buffer;
    
    for (int i = 0; i < size; i++)
    {
        for(auto el : loger__buffers__queue[i])
        {
            int64_t ns = el->ns;
            mes_buffer.push_back({
                el,
                i,
                ns
            });
        }
    }

    std::sort(mes_buffer.begin(), mes_buffer.end(), 
		[](temp_t a, temp_t b)
		{
			return a.ts < b.ts;
		});

	for(auto& mes : mes_buffer)
	{
		count = false;
		wlmesasge_t* best = mes.message;
		int best_i = mes.i;
		if (best)
		{
            auto& buffer = best->message;
			if (!buffer)
				continue;
			std::string str_buffer = generate_prefix_func_from_message(best);
			int i = 0;
			std::string str = " ";
            std::string ostr = " ";
			std::getline(buffer, str);
			while (buffer)
			{
                ostr = str;
                std::getline(buffer, str);
                if(buffer || ostr.size() > 0)
                {
                    if (i != 0)
                        str_buffer.append("                  ");
                    
                    str_buffer.append(ostr).append("\n");
                    i++;
                }
				
			}

			for (auto stream : __logers[best->level]->out_streams)
				str_buffers[stream].append(str_buffer);

			buffer.clear();
			delete best;
		}
	}

    for (auto stream_pair : stream_buffers)
		{
			*stream_pair.second << str_buffers[stream_pair.second];
			str_buffers[stream_pair.second] = "";
            stream_pair.second->flush();
		};
			
	locker.unlock();
}

void __wloger_sender()
{

	bool _stop_sender = false;
	
	while (!_stop_sender)
	{
		auto ts = std::chrono::high_resolution_clock::now();
		static const std::chrono::nanoseconds timespan(size_t(20 * 10e6));
		_stop_sender = stop_sender;
		if(_stop_sender)
			__wloger_generate_loger_buffer(WL_INFO, true, "true", "END", "", 0) << __wlog_profiler_get_stat();
		__wloger_send();
		auto ts2 = std::chrono::high_resolution_clock::now();
		std::this_thread::sleep_for(timespan - (ts2 - ts));
	}
}


std::string __wlog_profiler_get_stat()
{
	auto buff = std::stringstream();
	int i = 0;
	buff << "profiling statistic:[";
	for(auto& el: __profiler)
	{
		auto& t = el.second;
		t->lock();
		buff << (i == 0 ? "" : ",")
			<< "{"
			<< "\"Name\": \"" << t->name << "\","
			<< "\"Time_resolution\": \"ms\","
			<< "\"Total\": " << t->acc * 1e-6 << ","
			<< "\"Total_calls\": " << t->cnt << ","
			<< "\"Avg_for_call\": " << t->acc * 1e-6 / double(t->cnt) << ""
			<< "}"
		;
		t->unlock();
		++i;
	}
	buff << "]";

	return buff.str();
}

void __wlog_profiler_push_stat()
{
	__wloger_generate_loger_buffer(WL_INFO, true, "true", "PROFILER", "", 0) << __wlog_profiler_get_stat();
	__wlog_force_flush_buffers();
}

void __wlog_force_flush_buffers()
{
	__wloger_send();
}

typedef	void (*__sig_fn_t)(int);

void __wloger_INIT()
{
	for(int i = 0; i < 0x100; ++i)
		__logers[i] = nullptr;
	__wloger_generate_prefix_func = __base_generate_prefix_func;
    WLOG_GENERATE_LOGER(WL_FATAL, "FATAL");
	WLOG_GENERATE_LOGER(WL_ERROR, "ERROR");
	WLOG_GENERATE_LOGER(WL_WARNING, "WARNING");
	WLOG_GENERATE_LOGER(WL_INFO, "INFO");
    WLOG_GENERATE_LOGER(WL_DEBUG, "DEBUG");

	sender_thread = new std::thread(&__wloger_sender);
	__wloger_INIT_NATIVE();
}

__MESSAGE __wloger_generate_loger_buffer(unsigned int level, bool cond, const char* cond_str, const char* file, const char* func, unsigned int line)
{
	size_t ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	if (cond && __wlog_level >= level && __logers[level] != nullptr) {

		auto id = std::this_thread::get_id();
		uint32_t _id = *((uint32_t*)((void*)(&id)));

		wlmesasge_t* message = new wlmesasge_t;
		message->file_name = file;
        message->func_name = func;
        message->cond_str = cond_str;
        message->level = level;
        message->line = line;
        message->straem_id = _id;
		message->ns = ns;
        return __MESSAGE(message);
	}
	return __MESSAGE(nullptr);
}

void __wloger_printf(unsigned int level, bool cond, const char* cond_str, const char* file, const char* func, unsigned int line, const char* format, ...)
{
	size_t ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    if (!cond || !__wlog_level >= level || __logers[level] == nullptr)
        return;
    va_list args;
    va_start(args, format);
    int len = vsnprintf(NULL, 0, format, args);
    va_end(args);
    std::vector<char> buff(len + 5, 0);
    va_start(args, format);
    vsnprintf(buff.data(), len + 2, format, args);
    va_end(args);


    auto id = std::this_thread::get_id();
    uint32_t _id = *((uint32_t*)((void*)(&id)));

    wlmesasge_t* message = new wlmesasge_t;
    message->file_name = file;
    message->func_name = func;
    message->cond_str = cond_str;
    message->level = level;
    message->line = line;
    message->straem_id = _id;
	message->ns = ns;
	
    __MESSAGE(message).print(buff.data());
}



#include <fstream>

#if WLOG_OS_WINDOWS
const char sep = '\\';
#else
const char sep = '/';
#endif
void __wloger_generate_log_files(std::string path)
{
	std::time_t tp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto lct = std::localtime(&tp);
    char time_buff[512];
    snprintf(time_buff, 512, "Y%.2iM%.2iD%.2iTh%.2im%.2is%.2i", lct->tm_year % 100, lct->tm_mon, lct->tm_mday, lct->tm_hour, lct->tm_min, lct->tm_sec);
	for (int level = 0; level < 0x100; ++level) if(__logers[level] != nullptr)
	{
		auto name = __logers[level]->name;

		if (WLOG_LEVEL >= level)
		{
			auto filename = path;
			

			filename.append(name).append("_").append(time_buff).append(".log");
			std::ofstream* fout = new std::ofstream(filename);

			__wloger_attach_stream(level, fout);
		}
	}
}

void __WLogerShutdown() {
	stop_sender = true;
	sender_thread->join();
}

#ifndef __APPLE__
void __wloger_INIT_NATIVE() 
{
	#define SIGNAL_HANDLER(SIGNAL) static const __sig_fn_t __##SIGNAL##__base_handler = signal(SIGNAL, [](int){ \
    std::cout << "Unhandled exception: " #SIGNAL "\n" << std::flush; \
    __wloger_generate_loger_buffer(WL_FATAL, true, "true", "SIGNAL_HANDLER", "", 0) << "Unhandled exception: " #SIGNAL; \
    __WLogerShutdown(); \
    signal(SIGNAL, __##SIGNAL##__base_handler); \
    raise(SIGNAL); \
})
#ifdef SIGHUP
    SIGNAL_HANDLER(SIGHUP);
#endif
#ifdef SIGINT
    SIGNAL_HANDLER(SIGINT);
#endif
#ifdef SIGQUIT
    SIGNAL_HANDLER(SIGQUIT);
#endif
#ifdef SIGILL
    SIGNAL_HANDLER(SIGILL);
#endif
#ifdef SIGTRAP
    SIGNAL_HANDLER(SIGTRAP);
#endif
#ifdef SIGABRT
    SIGNAL_HANDLER(SIGABRT);
#endif
#ifdef SIGEMT
    SIGNAL_HANDLER(SIGEMT);
#endif
#ifdef SIGFPE
    SIGNAL_HANDLER(SIGFPE);
#endif
#ifdef SIGKILL
    SIGNAL_HANDLER(SIGKILL);
#endif
#ifdef SIGBUS
    SIGNAL_HANDLER(SIGBUS);
#endif
#ifdef SIGSEGV
    SIGNAL_HANDLER(SIGSEGV);
#endif
#ifdef SIGSYS
    SIGNAL_HANDLER(SIGSYS);
#endif
#ifdef SIGPIPE
    SIGNAL_HANDLER(SIGPIPE);
#endif
#ifdef SIGALRM
    SIGNAL_HANDLER(SIGALRM);
#endif
#ifdef SIGTERM
    SIGNAL_HANDLER(SIGTERM);
#endif
#ifdef SIGURG
    SIGNAL_HANDLER(SIGURG);
#endif
#ifdef SIGSTOP
    SIGNAL_HANDLER(SIGSTOP);
#endif
#ifdef SIGTSTP
    SIGNAL_HANDLER(SIGTSTP);
#endif
#ifdef SIGCONT
    SIGNAL_HANDLER(SIGCONT);
#endif
#ifdef SIGCHLD
    SIGNAL_HANDLER(SIGCHLD);
#endif
#ifdef SIGTTIN
    SIGNAL_HANDLER(SIGTTIN);
#endif
#ifdef SIGTTOU
    SIGNAL_HANDLER(SIGTTOU);
#endif
#ifdef SIGIO
    SIGNAL_HANDLER(SIGIO);
#endif
#ifdef SIGXCPU
    SIGNAL_HANDLER(SIGXCPU);
#endif
#ifdef SIGXFSZ
    SIGNAL_HANDLER(SIGXFSZ);
#endif
#ifdef SIGVTALRM
    SIGNAL_HANDLER(SIGVTALRM);
#endif
#ifdef SIGPROF
    SIGNAL_HANDLER(SIGPROF);
#endif
#ifdef SIGWINCH
    SIGNAL_HANDLER(SIGWINCH);
#endif
#ifdef SIGINFO
    SIGNAL_HANDLER(SIGINFO);
#endif
#ifdef SIGUSR1
    SIGNAL_HANDLER(SIGUSR1);
#endif
#ifdef SIGUSR2
    SIGNAL_HANDLER(SIGUSR2);
#endif

#undef SIGNAL_HANDLER
}
#endif