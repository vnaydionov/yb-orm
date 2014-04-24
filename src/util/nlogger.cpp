// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#define YBUTIL_SOURCE

#if defined(__WIN32__) || defined(_WIN32) || defined(__CYGWIN__)
#include <windows.h>
#elif defined(__unix__)
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/time.h>
#endif
#include <time.h>
#include <stdio.h>
#include <sstream>
#include "util/nlogger.h"

namespace Yb {

unsigned long get_process_id()
{
#if defined(__WIN32__) || defined(_WIN32) || defined(__CYGWIN__)
    return GetCurrentProcessId();
#elif defined(__unix__)
    return getpid();
#else
    return -1;
#endif
}

unsigned long get_thread_id()
{
#if defined(__WIN32__) || defined(_WIN32) || defined(__CYGWIN__)
    return GetCurrentThreadId();
#elif defined(__unix__)
    return syscall(SYS_gettid);
#else
    return -1;
#endif
}

MilliSec get_cur_time_millisec()
{
#if defined(__WIN32__) || defined(_WIN32) || defined(__CYGWIN__)
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
#elif defined(__unix__)
    struct timeval tv;
    gettimeofday(&tv, NULL);
    MilliSec r = tv.tv_sec;
    r *= 1000;
    r += tv.tv_usec / 1000;
    return r;
#else
    MilliSec r = time(NULL);
    r *= 1000;
    return r;
#endif
}

struct tm *localtime_safe(const time_t *clock, struct tm *result)
{
    if (!clock || !result)
        return NULL;
#if defined(_MSC_VER)
    errno_t err = localtime_s(result, clock);
    if (err)
        return NULL;
#elif defined(__unix__)
    if (!localtime_r(clock, result))
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

InvalidLogLevel::InvalidLogLevel()
    : std::logic_error("Invalid log level")
{}

InvalidLoggerName::InvalidLoggerName()
    : std::logic_error("Invalid logger name")
{}

int LogRecord::check_level(int level)
{
    if (level <= ll_NONE || level > ll_DEBUG)
        throw InvalidLogLevel();
    return level;
}

LogRecord::LogRecord(int level, const std::string &component, const std::string &msg)
    : t_(get_cur_time_millisec())
    , pid_(get_process_id())
    , tid_(get_thread_id())
    , level_(check_level(level))
    , component_(component)
    , msg_(msg)
{}

const char *LogRecord::get_level_name() const
{
    static const char *ll_name[] = {
        "CRIT", "ERRO", "WARN", "INFO", "DEBG"
    };
    return ll_name[check_level(level_) - 1];
}

ILogAppender::~ILogAppender()
{}

ILogger::~ILogger()
{}

bool LogTargetRule::match_component(const std::string &component) const
{
    bool res;
    if (include_children)
        res = component.substr(0, this->component.size()) == this->component;
    else
        res = component == this->component;
    return negate_component ? !res : res;
}

bool LogTargetRule::match_level(int level) const
{
    switch (level_match_mode) {
    case 0:
        return level == this->level;
    case 1:
        return level != this->level;
    case 2:
        return level <= this->level;
    }
    return false;
}

bool LogTargetRule::match(const LogRecord &rec) const
{
    return match_component(rec.get_component())
        && match_level(rec.get_level());
}

bool LogTargetRules::match(const LogRecord &rec) const
{
    Rules::const_iterator it = target_rules.begin(),
        end = target_rules.end();
    for (; it != end; ++it)
        if (it->match(rec))
            return true;
    return false;
}

ILogRulesConfig::~ILogRulesConfig()
{}

Logger::Logger(ILogAppender *appender, const std::string &name)
    : appender_(appender)
    , name_(name)
{}

ILogger::Ptr Logger::new_logger(const std::string &name)
{
    if (!valid_name(name))
        throw InvalidLoggerName();
    std::string new_name = name;
    if (!name_.empty())
        new_name = name_ + "." + new_name;
    ILogger::Ptr p(new Logger(appender_, new_name));
    return p;
}

void Logger::log(int level, const std::string &msg)
{
    if (level <= ll_NONE || level > ll_DEBUG)
        throw InvalidLogLevel();
    LogRecord rec(level, get_name(), msg);
    appender_->append(rec);
}

const std::string Logger::get_name() const
{
    return name_.empty()? std::string("main"): name_;
}

bool Logger::valid_name(const std::string &name)
{
    if (name.empty())
        return false;
    for (size_t i = 0; i < name.size(); ++i) {
        char c = name[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '_' || c == '-'))
        {
            return false;
        }
    }
    return true;
}

void LogAppender::output(std::ostream &s, const LogRecord &rec, const char *time_str)
{
    s << time_str << " "
        << rec.get_pid() << "/" << rec.get_tid() << " "
        << rec.get_level_name() << " "
        << rec.get_component() << ": "
        << rec.get_msg();
    if (rec.get_msg()[rec.get_msg().size() - 1] != '\n')
        s << '\n';
}

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
        sprintf(time_str + 17, ".%03d", (int)(it->get_t() % 1000));
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
    queue_.push_back(rec);
    time_t now = time(NULL);
    if (should_flush(now))
        do_flush(now);
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
