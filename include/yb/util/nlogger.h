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
    std::string component_;
    std::string msg_;

    static int check_level(int level);
public:
    LogRecord(int level, const std::string &component, const std::string &msg);

    MilliSec get_t() const { return t_; }
    time_t get_sec() const { return t_ / 1000; }
    unsigned long get_pid() const { return pid_; }
    unsigned long get_tid() const { return tid_; }
    int get_level() const { return level_; }
    const std::string &get_component() const { return component_; }
    const std::string &get_msg() const { return msg_; }

    const char *get_level_name() const;
};

class YBUTIL_DECL ILogAppender
{
public:
    virtual void append(const LogRecord &rec) = 0;
    virtual int get_level(const std::string &name) = 0;
    virtual void set_level(const std::string &name, int level) = 0;
    virtual ~ILogAppender();
};

class YBUTIL_DECL ILogger
{
public:
    typedef std::auto_ptr<ILogger> Ptr;
    virtual ILogger::Ptr new_logger(const std::string &name) = 0;
    virtual ILogger::Ptr get_logger(const std::string &name) = 0;
    virtual int get_level() = 0;
    virtual void set_level(int level) = 0;
    virtual void log(int level, const std::string &msg) = 0;
    virtual const std::string get_name() const = 0;
    virtual ~ILogger();
    void debug    (const std::string &msg) { log(ll_DEBUG,    msg); }
    void info     (const std::string &msg) { log(ll_INFO,     msg); }
    void warning  (const std::string &msg) { log(ll_WARNING,  msg); }
    void error    (const std::string &msg) { log(ll_ERROR,    msg); }
    void critical (const std::string &msg) { log(ll_CRITICAL, msg); }
};

class YBUTIL_DECL Logger: public ILogger
{
    ILogAppender *appender_;
    const std::string name_;
public:
    Logger(ILogAppender *appender, const std::string &name = "");
    ILogger::Ptr new_logger(const std::string &name);
    ILogger::Ptr get_logger(const std::string &name);
    int get_level();
    void set_level(int level);
    void log(int level, const std::string &msg);
    const std::string get_name() const;
    static bool valid_name(const std::string &name, bool allow_dots=false);
};

class YBUTIL_DECL LogAppender: public ILogAppender
{
    std::ostream &s_;
    typedef std::vector<LogRecord> Queue;
    Queue queue_;
    Mutex queue_mutex_;
    time_t last_flush_;
    const int flush_interval_;
    typedef std::map<std::string, int> LogLevelMap;
    LogLevelMap log_levels_;

    static void output(std::ostream &s, const LogRecord &rec, const char *time_str);
    void do_flush(time_t now);
    bool should_flush(time_t now);
public:
    LogAppender(std::ostream &s, int flush_interval = 0);
    ~LogAppender();
    void append(const LogRecord &rec);
    int get_level(const std::string &name);
    void set_level(const std::string &name, int level);
    void flush();
};

} // end of namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__UTIL__NLOGGER__INCLUDED
