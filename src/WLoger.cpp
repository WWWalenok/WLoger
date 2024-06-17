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

unsigned int __wlog_level = 0xffffu;

unsigned int __wlog_get_log_level()
{
    return __wlog_level;
}

void __wlog_set_log_level(unsigned int val)
{
    __wlog_level = val;
}

__generate_prefix_func_type __wloger_generate_prefix_func;

__MESSAGE __BAD_BUFFER(nullptr);

std::thread* sender_thread;

bool stop_sender = false;

std::unordered_map<unsigned int, std::vector<std::ostream*>> __loger__out__streams = std::unordered_map<unsigned int, std::vector<std::ostream*>>();
std::unordered_map<unsigned int, std::string> __loger__name = std::unordered_map<unsigned int, std::string>();
std::unordered_map<unsigned int, std::vector<wlmesage_t*>> __loger__buffers = std::unordered_map<unsigned int, std::vector<wlmesage_t*>>();
std::unordered_map<unsigned int, std::mutex*> __loger__buffers_mutex = std::unordered_map<unsigned int, std::mutex*>();


struct __MESSAGE_DATA
{
    std::stringstream message;
	std::chrono::system_clock::time_point time;
	std::string file_name;
	std::string func_name;
    std::string cond_str;
	unsigned int line;
	unsigned int level;
	uint32_t straem_id;
    std::mutex mutex;
    bool in_process;
};

__MESSAGE::__MESSAGE(void* dt)
{
    data = dt;
    if(!data)
        return;
    auto message = (__MESSAGE_DATA*)data;
    std::lock_guard<std::mutex> lg(message->mutex);
    message->in_process = true;
}

void __MESSAGE::print(std::string str)
{
    if(!data)
        return;
    auto message = (__MESSAGE_DATA*)data;
    std::lock_guard<std::mutex> lg(message->mutex);
    message->in_process = true;

    message->message << str;
}

__MESSAGE::~__MESSAGE()
{
    if(!data)
        return;
    auto message = (__MESSAGE_DATA*)data;
    std::lock_guard<std::mutex> lg(message->mutex);
    message->in_process = false;
    std::lock_guard<std::mutex> guard(*__loger__buffers_mutex[message->level]);
    __loger__buffers[message->level].push_back(message);
}


void __wloger_generate_loger(unsigned int level, std::string name)
{
	__loger__name[level] = name; 
	if (!__wloger_cond(level))
	{
		__loger__out__streams[level] = std::vector<std::ostream*>();
		__loger__buffers[level] = std::vector<wlmesage_t*>();
		__loger__buffers_mutex[level] = new std::mutex();
	}
}

void __wloger_rename_loger(unsigned int level, std::string name)
{
	if(__wloger_cond(level))
		__loger__name[level] = name;
}


bool __wloger_cond(unsigned int level)
{
	return __loger__out__streams.find(level) != __loger__out__streams.end() 
		&& __loger__name.find(level) != __loger__name.end() 
		&& __loger__buffers.find(level) != __loger__buffers.end()
		&& __loger__buffers_mutex.find(level) != __loger__buffers_mutex.end();
}

bool __wloger_attach_stream(unsigned int level, std::ostream* stream)
{
	if (__wloger_cond(level))
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

bool __wloger_detach_stream(unsigned int level, std::ostream* stream)
{
	if (__wloger_cond(level))
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

static std::string __base_generate_prefix_func(unsigned int level, std::string file, std::string func, std::string cond, unsigned int line, std::chrono::system_clock::time_point time, uint32_t straem_id)
{
	size_t lenght = file.length();
	std::string _file = "";
	for (size_t i = 0; i < lenght; i++)
	{
		_file.append(1, file[i]);
		if (file[i] == '\\')
			_file.clear();
	};

	size_t ms = std::round(double(std::chrono::duration<double, std::milli>(time.time_since_epoch()).count()));
	std::time_t tp = std::chrono::system_clock::to_time_t(time);
	char buff[256];
    auto lct = std::localtime(&tp);
	auto count = snprintf(buff, 256, "[%.2i:%.2i:%.2i.%.3i] ", lct->tm_hour, lct->tm_min, lct->tm_sec, int(ms % 1000));


	std::string out;
	out.append(buff);
	out.append(_file);
	out.append(":");
	out.append(std::to_string(line));
	out.append(" ");
	out.append(std::to_string(straem_id));
    out.append(" ");
    // if(func.size() > 50)
    //     func = func.substr(0,47) + "...";
    // out.append(func);
    if(cond == "true")
	    out.append(": ");
    else
    {
        out.append("if(");
        out.append(cond);
        out.append("): ");
    }
    
	if (__loger__name.find(level) != __loger__name.end())
		if (__loger__name[level].length() > 0)
		{
			out.append(__loger__name[level]);
			out.append(": ");
		}
	return out;
}

std::string generate_prefix_func_from_message(wlmesage_t* m)
{
    return __wloger_generate_prefix_func(m->level, m->file_name, m->func_name, m->cond_str, m->line, m->time, m->straem_id);
}

void __wloger_send()
{
	std::vector<unsigned int> loger__buffers__levels;
	std::vector<std::vector<wlmesage_t*>> loger__buffers__queue;
	std::vector<uint32_t> loger__buffers__lasts;
	for (auto pair_streams : __loger__out__streams)
	{
		auto level = pair_streams.first;
		{
			std::lock_guard<std::mutex> guard(*__loger__buffers_mutex[level]);
			loger__buffers__queue.push_back(__loger__buffers[level]);
			loger__buffers__levels.push_back(level);
			__loger__buffers[level] = std::vector<wlmesage_t*>();
			loger__buffers__lasts.push_back(0);
		}
	}
	bool count = true;

	std::unordered_map<std::ostream*, std::string> str_buffers;
    std::unordered_map<std::ostream*, std::ostream*> stream_buffers;

	for (auto _pair : __loger__out__streams)
		for (auto stream : _pair.second)
        {
			str_buffers[stream] = "";
            stream_buffers[stream] = stream;
        }


	size_t size = loger__buffers__queue.size();

    struct temp_t
    {
        wlmesage_t* message = 0;
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
            int64_t ns = (std::chrono::nanoseconds(el->time.time_since_epoch())).count();
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
		wlmesage_t* best = mes.message;
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
                        str_buffer.append("|            | ");
                    
                    str_buffer.append(ostr).append("\n");
                    i++;
                }
				
			}

			for (auto stream : __loger__out__streams[best->level])
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
			
}

void  __wloger_sender()
{
	bool _stop_sender = false;
	while (!_stop_sender)
	{
		std::chrono::milliseconds timespan(20);
		std::this_thread::sleep_for(timespan);
		_stop_sender = stop_sender;
		__wloger_send();

	}
}

typedef	void (*__sig_fn_t)(int);

void __wloger_INIT()
{
#define SIGNAL_HANDLER(SIGNAL) static const __sig_fn_t __##SIGNAL##__base_handler = signal(SIGNAL, [](int){ \
    std::cout << "Unhandled exception: " #SIGNAL "\n" << std::flush; \
    __wloger_generate_loger_buffer(WL_FATAL, true, "true", "SIGNAL_HANDLER", "", 0) << "Unhandled exception: " #SIGNAL; \
    __WLogerShutdown(); \
    signal(SIGNAL, __##SIGNAL##__base_handler); \
    raise(SIGNAL); \
})
#ifdef SIGTERM
    SIGNAL_HANDLER(SIGTERM);
#endif
#ifdef SIGSEGV
    SIGNAL_HANDLER(SIGSEGV);
#endif
#ifdef SIGINT
    SIGNAL_HANDLER(SIGINT);
#endif
#ifdef SIGILL
    SIGNAL_HANDLER(SIGILL);
#endif
#ifdef SIGABRT
    SIGNAL_HANDLER(SIGABRT);
#endif
#ifdef SIGFPE
    SIGNAL_HANDLER(SIGFPE);
#endif
#undef SIGNAL_HANDLER

	__wloger_generate_prefix_func = __base_generate_prefix_func;
    WLOG_GENERATE_LOGER(WL_FATAL, "FATAL");
	WLOG_GENERATE_LOGER(WL_ERROR, "ERROR");
	WLOG_GENERATE_LOGER(WL_WARNING, "WARNING");
	WLOG_GENERATE_LOGER(WL_INFO, "INFO");
    WLOG_GENERATE_LOGER(WL_DEBUG, "DEBUG");

	sender_thread = new std::thread(&__wloger_sender);
}

__MESSAGE __wloger_generate_loger_buffer(unsigned int level, bool cond, const char* cond_str, const char* file, const char* func, unsigned int line)
{
	if (cond && WLOG_LEVEL_COND(level)) {
		std::chrono::system_clock::time_point tp;

		tp = std::chrono::system_clock::now();

		auto id = std::this_thread::get_id();
		uint32_t _id = *((uint32_t*)((void*)(&id)));

		wlmesage_t* mesage = new wlmesage_t;
		mesage->file_name = file;
        mesage->func_name = func;
        mesage->cond_str = cond_str;
        mesage->level = level;
        mesage->line = line;
        mesage->straem_id = _id;
        mesage->time = tp;
        return __MESSAGE(mesage);
	}
	return __MESSAGE(nullptr);
}

void __wloger_printf(unsigned int level, bool cond, const char* cond_str, const char* file, const char* func, unsigned int line, const char* format, ...)
{
    if (!cond || !WLOG_LEVEL_COND(level))
        return;
    va_list args;
    va_start(args, format);
    int len = vsnprintf(NULL, 0, format, args);
    va_end(args);
    std::vector<char> buff(len + 5, 0);
    va_start(args, format);
    vsnprintf(buff.data(), len + 2, format, args);
    va_end(args);

    std::chrono::system_clock::time_point tp;

    tp = std::chrono::system_clock::now();

    auto id = std::this_thread::get_id();
    uint32_t _id = *((uint32_t*)((void*)(&id)));

    wlmesage_t* mesage = new wlmesage_t;
    mesage->file_name = file;
    mesage->func_name = func;
    mesage->cond_str = cond_str;
    mesage->level = level;
    mesage->line = line;
    mesage->straem_id = _id;
    mesage->time = tp;

    auto ret = __MESSAGE(mesage);

    ret.print(buff.data());
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
	for (auto pair_name : __loger__name)
	{
		auto level = pair_name.first;
		auto name = pair_name.second;

		if (WLOG_LEVEL >= level)
		{
			auto filename = path;
			

			filename.append(name).append("_").append(time_buff).append(".log");
			std::ofstream* fout = new std::ofstream(filename);

			__wloger_attach_stream(level, fout);
		}
	}
}

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

static void __WLogerShutdown() {
	guard.~Guard();
}
