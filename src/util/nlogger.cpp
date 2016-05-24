// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#define YBUTIL_SOURCE

#include "util/util_config.h"
#if defined(YBUTIL_WINDOWS) || defined(__CYGWIN__)
#include <windows.h>
#endif
#if defined(__unix__)
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#endif
#if defined(__linux__)
#include <sys/syscall.h>
#endif
#if defined(__FreeBSD__)
#include <pthread_np.h>
#endif
#include <time.h>
#include <stdio.h>
#include <sstream>
#include "util/nlogger.h"

namespace Yb {

InvalidLogLevel::InvalidLogLevel()
    : ValueError(_T("Invalid log level"))
{}

InvalidLoggerName::InvalidLoggerName()
    : ValueError(_T("Invalid logger name"))
{}

YBUTIL_DECL unsigned long
get_process_id()
{
#if defined(YBUTIL_WINDOWS)
    return GetCurrentProcessId();
#elif defined(__unix__)
    return getpid();
#else
    return -1;
#endif
}

YBUTIL_DECL unsigned long
get_thread_id()
{
#if defined(YBUTIL_WINDOWS) || defined(__CYGWIN__)
    return GetCurrentThreadId();
#elif defined(__linux__)
    return syscall(SYS_gettid);
#elif defined(__FreeBSD__)
    if (pthread_main_np() == 0)
        return pthread_getthreadid_np();
    return getpid();
#elif defined(__unix__)
    // dirty hack
    pthread_t tid = pthread_self();
    return *(unsigned long *)&tid;
#else
    return -1;
#endif
}

YBUTIL_DECL MilliSec
get_cur_time_millisec()
{
#if defined(__unix__)
    struct timeval tv;
    gettimeofday(&tv, NULL);
    MilliSec r = tv.tv_sec;
    r *= 1000;
    r += tv.tv_usec / 1000;
    return r;
#elif defined(YBUTIL_WINDOWS)
    // quick & dirty
    time_t t0 = time(NULL);
    SYSTEMTIME st;
    GetSystemTime(&st);
    time_t t1 = time(NULL);
    MilliSec r = t1;
    r *= 1000;
    if (t1 == t0)
        r += st.wMilliseconds;
    return r;
#else
    MilliSec r = time(NULL);
    r *= 1000;
    return r;
#endif
}

YBUTIL_DECL struct tm *localtime_safe(const time_t *clock, struct tm *result)
{
    if (!clock || !result)
        return NULL;
#if defined(__unix__)
    if (!localtime_r(clock, result))
        return NULL;
#elif defined(_MSC_VER)
    errno_t err = localtime_s(result, clock);
    if (err)
        return NULL;
#else
    static Mutex m;
    ScopedLock lock(m);
    struct tm *r = localtime(clock);
    if (!r)
        return NULL;
    memcpy(result, r, sizeof(*result));
#endif
    return result;
}

int LogRecord::check_level(int level)
{
    if (level <= ll_NONE || level >= ll_ALL)
        throw InvalidLogLevel();
    return level;
}

const Yb::String &LogRecord::get_level_name(int level)
{
    static Yb::String *log_level_names = NULL;
    if (!log_level_names) {
        Yb::String *names = new Yb::String[7];
        names[0] = _T("CRI");
        names[1] = _T("ERR");
        names[2] = _T("WRN");
        names[3] = _T("INF");
        names[4] = _T("DBG");
        names[5] = _T("TRC");
        names[6] = _T("XXX");
        log_level_names = names;
    }
    try {
        return log_level_names[check_level(level) - 1];
    }
    catch (const InvalidLogLevel &) {
        return log_level_names[6];
    }
}

LogRecord::LogRecord(int level, const Yb::String &component,
                     const Yb::String &msg)
    : t_(get_cur_time_millisec())
    , pid_(get_process_id())
    , tid_(get_thread_id())
    , level_(check_level(level))
    , component_(component)
    , msg_(msg)
{}

ILogAppender::~ILogAppender()
{}

ILogger::~ILogger()
{}

Logger::Logger(ILogAppender *appender, const Yb::String &name)
    : appender_(appender)
    , name_(name)
{}

ILogger::Ptr Logger::new_logger(const Yb::String &name)
{
    if (!valid_name(name))
        throw InvalidLoggerName();
    Yb::String new_name = name;
    if (!str_empty(name_))
        new_name = name_ + _T(".") + new_name;
    ILogger::Ptr p(new Logger(appender_, new_name));
    if (p->get_level() == ll_ALL) {
        int parent_level = get_level();
        if (parent_level != ll_ALL)
            p->set_level(parent_level);
    }
    return p;
}

ILogger::Ptr Logger::get_logger(const Yb::String &name)
{
    if (!valid_name(name, true))
        throw InvalidLoggerName();
    ILogger::Ptr p(new Logger(appender_, name));
    return p;
}

int Logger::get_level()
{
    return appender_->get_level(name_);
}

void Logger::set_level(int level)
{
    appender_->set_level(name_, level);
}

void Logger::log(int level, const Yb::String &msg)
{
    if (level <= ll_NONE || level > ll_TRACE)
        throw InvalidLogLevel();
    LogRecord rec(level, get_name(), msg);
    appender_->append(rec);
}

const Yb::String Logger::get_name() const
{
    return str_empty(name_)? Yb::String(_T("main")): name_;
}

bool Logger::valid_name(const Yb::String &name, bool allow_dots)
{
    if (str_empty(name))
        return false;
    for (size_t i = 0; i < str_length(name); ++i) {
        int c = char_code(name[(int)i]);
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || (c == '_') || (c == '-') ||
              (c == '.' && allow_dots)))
        {
            return false;
        }
    }
    return true;
}

void LogAppender::output(std::ostream &s, const LogRecord &rec,
                         const char *time_str)
{
    const Yb::String &msg = rec.msg();
    s << time_str << " "
        << "P" << rec.pid() << " T" << rec.tid() << " "
        << NARROW(rec.level_name()) << " "
        << NARROW(rec.component()) << ": "
        << NARROW(msg);
    if (str_empty(msg) || char_code(msg[(int)str_length(msg) - 1]) != '\n')
        s << '\n';
}

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif // _MSC_VER

void LogAppender::do_flush(time_t now)
{
    std::ostringstream s;
    Queue::const_iterator it = queue_.begin(),
        end = queue_.end();
    time_t prev_time = 0;
    char time_str[64] = "";
    for (; it != end; ++it) {
        if (prev_time != it->get_sec()) {
            prev_time = it->get_sec();
            struct tm split_time;
            localtime_safe(&prev_time, &split_time);
            strftime(time_str, sizeof(time_str),
                "%y-%m-%d %H:%M:%S", &split_time);
        }
        sprintf(time_str + 17, ",%03d", (int)(it->get_t() % 1000));
        output(s, *it, time_str);
    }
    queue_.clear();
    s_ << s.str();
    s_.flush();
    last_flush_ = now;
}

bool LogAppender::should_flush(time_t now)
{
    return now - last_flush_ >= flush_interval_;
}

LogAppender::LogAppender(std::ostream &s, int flush_interval)
    : s_(s)
    , last_flush_(time(NULL))
    , flush_interval_(flush_interval)
{}

LogAppender::~LogAppender()
{
    flush();
}

void LogAppender::append(const LogRecord &rec)
{
    ScopedLock lk(queue_mutex_);
    int target_level = ll_ALL;
    LogLevelMap::iterator it = log_levels_.find(rec.component());
    if (log_levels_.end() != it)
        target_level = it->second;
    if (rec.level() <= target_level) {
        queue_.push_back(rec);
        time_t now = time(NULL);
        if (should_flush(now))
            do_flush(now);
    }
}

int LogAppender::get_level(const Yb::String &name)
{
    ScopedLock lk(queue_mutex_);
    LogLevelMap::iterator it = log_levels_.find(name);
    if (log_levels_.end() == it)
        return ll_ALL;
    return it->second;
}

void LogAppender::set_level(const Yb::String &name, int level)
{
    ScopedLock lk(queue_mutex_);
    size_t name_len = str_length(name);
    if (name_len >= 2 && str_substr(name, name_len - 2) == _T(".*"))
    {
        size_t prefix_len = name_len - 1;
        Yb::String prefix = str_substr(name, 0, prefix_len);
        LogLevelMap::iterator it = log_levels_.begin(),
                              it_end = log_levels_.end();
        for (; it != it_end; ++it)
            if (str_substr(it->first, 0, prefix_len) == prefix)
                it->second = level;
        Yb::String prefix0 = str_substr(name, 0, name_len - 2);
        log_levels_[prefix0] = level;
    }
    else {
        LogLevelMap::iterator it = log_levels_.find(name);
        if (log_levels_.end() == it)
            log_levels_[name] = level;
        else
            it->second = level;
    }
}

void LogAppender::flush()
{
    ScopedLock lk(queue_mutex_);
    time_t now = time(NULL);
    do_flush(now);
}

} // namespace Yb

#if 0
int main()
{
    using namespace std;
    using namespace Yb;
    LogAppender appender(cerr);
    Logger logger(&appender);
    logger.info("test main logger with default component\n");
    {
        ILogger::Ptr xlogger = logger.new_logger("test-component");
        xlogger->debug("test component logger");
        sleep(2);
        for (int i = 0; i < 1000000; ++i) {
            xlogger->debug("test component logger: " +
                Yb::to_stdstring(i));
        }
    }
    ILogger::Ptr ylogger = logger.new_logger("yyy");
    ylogger->error("test logger 1");
    ILogger::Ptr zlogger = ylogger->new_logger("zzz");
    zlogger->error("test logger 2");
    return 0;
}
#endif

// vim:ts=4:sts=4:sw=4:et:
