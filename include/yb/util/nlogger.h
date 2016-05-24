// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef YB__UTIL__NLOGGER__INCLUDED
#define YB__UTIL__NLOGGER__INCLUDED

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <stdexcept>
#include <time.h>
#include "util_config.h"
#include "exception.h"
#include "thread.h"
#include "string_type.h"

namespace Yb {

class YBUTIL_DECL InvalidLogLevel: public ValueError
{
public:
    InvalidLogLevel();
};

class YBUTIL_DECL InvalidLoggerName: public ValueError
{
public:
    InvalidLoggerName();
};

typedef LongInt MilliSec;

YBUTIL_DECL unsigned long get_process_id();
YBUTIL_DECL unsigned long get_thread_id();
YBUTIL_DECL MilliSec get_cur_time_millisec();
YBUTIL_DECL struct tm *localtime_safe(const time_t *clock, struct tm *result);


enum {
    ll_NONE = 0,
    ll_CRITICAL,
    ll_ERROR,
    ll_WARNING,
    ll_INFO,
    ll_DEBUG,
    ll_TRACE,
    ll_ALL
};

class YBUTIL_DECL LogRecord
{
    MilliSec t_;
    unsigned long pid_;
    unsigned long tid_;
    int level_;
    Yb::String component_;
    Yb::String msg_;

    static int check_level(int level);

public:
    static const Yb::String &get_level_name(int level);

    LogRecord(int level, const Yb::String &component, const Yb::String &msg);

    MilliSec get_t() const { return t_; }
    time_t get_sec() const { return t_ / 1000; }
    unsigned long pid() const { return pid_; }
    unsigned long tid() const { return tid_; }
    int level() const { return level_; }
    const Yb::String &component() const { return component_; }
    const Yb::String &msg() const { return msg_; }
    const Yb::String &level_name() const { return get_level_name(level_); }
};

class YBUTIL_DECL ILogAppender
{
public:
    virtual void append(const LogRecord &rec) = 0;
    virtual int get_level(const Yb::String &name) = 0;
    virtual void set_level(const Yb::String &name, int level) = 0;
    virtual ~ILogAppender();
};

class YBUTIL_DECL ILogger
{
public:
    typedef std::auto_ptr<ILogger> Ptr;
    virtual ILogger::Ptr new_logger(const Yb::String &name) = 0;
    virtual ILogger::Ptr get_logger(const Yb::String &name) = 0;
    virtual int get_level() = 0;
    virtual void set_level(int level) = 0;
    virtual void log(int level, const Yb::String &msg) = 0;
    virtual const Yb::String get_name() const = 0;
    virtual ~ILogger();
    void trace    (const Yb::String &msg) { log(ll_TRACE,    msg); }
    void debug    (const Yb::String &msg) { log(ll_DEBUG,    msg); }
    void info     (const Yb::String &msg) { log(ll_INFO,     msg); }
    void warning  (const Yb::String &msg) { log(ll_WARNING,  msg); }
    void error    (const Yb::String &msg) { log(ll_ERROR,    msg); }
    void critical (const Yb::String &msg) { log(ll_CRITICAL, msg); }
};

class YBUTIL_DECL Logger: public ILogger
{
    ILogAppender *appender_;
    const Yb::String name_;
public:
    Logger(ILogAppender *appender, const Yb::String &name = _T(""));
    ILogger::Ptr new_logger(const Yb::String &name);
    ILogger::Ptr get_logger(const Yb::String &name);
    int get_level();
    void set_level(int level);
    void log(int level, const Yb::String &msg);
    const Yb::String get_name() const;
    static bool valid_name(const Yb::String &name, bool allow_dots = false);
};

class YBUTIL_DECL LogAppender: public ILogAppender
{
    std::ostream &s_;
    typedef std::vector<LogRecord> Queue;
    Queue queue_;
    Mutex queue_mutex_;
    time_t last_flush_;
    const int flush_interval_;
    typedef std::map<Yb::String, int> LogLevelMap;
    LogLevelMap log_levels_;

    static void output(std::ostream &s, const LogRecord &rec, const char *time_str);
    void do_flush(time_t now);
    bool should_flush(time_t now);
public:
    LogAppender(std::ostream &s, int flush_interval = 0);
    ~LogAppender();
    void append(const LogRecord &rec);
    int get_level(const Yb::String &name);
    void set_level(const Yb::String &name, int level);
    void flush();
};

} // end of namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__UTIL__NLOGGER__INCLUDED
