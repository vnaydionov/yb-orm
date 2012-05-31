#ifndef NLOGGER__INCLUDED
#define NLOGGER__INCLUDED

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <stdexcept>
#include <time.h>
#include <util/Thread.h>

namespace Yb {

enum LogLevel { ll_NONE = 0, ll_CRITICAL, ll_ERROR, ll_WARNING, ll_INFO, ll_DEBUG };

class InvalidLogLevel : public std::logic_error
{
public:
    InvalidLogLevel();
};

class InvalidLoggerName : public std::logic_error
{
public:
    InvalidLoggerName();
};

typedef LongInt MilliSec;

unsigned long get_process_id();
unsigned long get_thread_id();
MilliSec get_cur_time_millisec();
struct tm *localtime_safe(const time_t *clock, struct tm *result);

class LogRecord
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

class ILogAppender
{
public:
    virtual void append(const LogRecord &rec) = 0;
    virtual ~ILogAppender();
};

class ILogger
{
public:
    typedef std::auto_ptr<ILogger> Ptr;
    virtual ILogger::Ptr new_logger(const std::string &name) = 0;
    virtual void log(int level, const std::string &msg) = 0;
    virtual const std::string get_name() const = 0;
    virtual ~ILogger();
    void debug    (const std::string &msg) { log(ll_DEBUG,    msg); }
    void info     (const std::string &msg) { log(ll_INFO,     msg); }
    void warning  (const std::string &msg) { log(ll_WARNING,  msg); }
    void error    (const std::string &msg) { log(ll_ERROR,    msg); }
    void critical (const std::string &msg) { log(ll_CRITICAL, msg); }
};

/*
 * A log target implements ILogAppender.
 * Each target has unique name and its own config.
 * Also there is a standard implementation of ILogAppender,
 *  which takes care of routing. The router walks through
 *  list of rounting rules for each target; if it finds
 *  a matching rule then LogRecord is sent to the target.
 * Routing rules:
 * - each target has list of rules for matching against LogRecords,
 *   distinct rules in a list are ORed;
 * - each rule has component part and level part, the rule is
 *   matched OK if both parts are matched;
 * - component part can be either a component name ('xx.yy.zz')
 *   or a name mask ending with an asterisk ('xx.yy.*'),
 *   also component part can be negated ('!aa.bb.*')
 * - level part consists of a level and match_mode
 *   ('=DEBUG', '!CRITICAL', '<=INFO'), if level part is empty
 *   the rule can match any level.
 * - whole rule set for couple of targets may look like this:
 *    'dblog(dbpool.*:INFO mycomp.sql:) syslog(*:<=INFO)'
 *    or something equivalent in e.g. XML..
 */
struct LogTargetRule
{
    std::string component;
    bool include_children;
    bool negate_component;

    int level;
    int level_match_mode;  // 0 ==, 1 !=, 2 <=

    bool match_component(const std::string &component) const;
    bool match_level(int level) const;
    bool match(const LogRecord &rec) const;
};

struct LogTargetRules
{
    typedef std::vector<LogTargetRule> Rules;
    Rules target_rules;

    bool match(const LogRecord &rec) const;
};

typedef std::map<std::string, LogTargetRules> LogRules;

class ILogRulesConfig
{
public:
    virtual LogRules &get(LogRules &rules) = 0;
    virtual ~ILogRulesConfig();
};


class Logger : public ILogger
{
    ILogAppender *appender_;
    const std::string name_;
public:
    Logger(ILogAppender *appender, const std::string &name = "");
    ILogger::Ptr new_logger(const std::string &name);
    void log(int level, const std::string &msg);
    const std::string get_name() const;
    static bool valid_name(const std::string &name);
};

class LogAppender : public ILogAppender
{
    std::ostream &s_;
    typedef std::vector<LogRecord> Queue;
    Queue queue_;
    Mutex queue_mutex_;
    time_t last_flush_;
    const int flush_interval_;

    static void output(std::ostream &s, const LogRecord &rec, const char *time_str);
    void do_flush(time_t now);
    bool should_flush(time_t now);
public:
    LogAppender(std::ostream &s, int flush_interval = 0);
    ~LogAppender();
    void append(const LogRecord &rec);
    void flush();
};

} // namespace Yb
#endif /*NLOGGER__INCLUDED*/
// vim:ts=4:sts=4:sw=4:et
