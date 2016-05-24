// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef _AUTH__APP_H_
#define _AUTH__APP_H_

#include <memory>
#include <string>
#include <fstream>
#include <util/nlogger.h>
#include <util/singleton.h>
#include <orm/data_object.h>

class FileLogAppender: public Yb::LogAppender
{
public:
    FileLogAppender(std::ostream &out);
    void append(const Yb::LogRecord &rec);
};

#if !defined(YBUTIL_WINDOWS)
const std::string get_process_name();
const std::string fmt_string_escape(const std::string &s);

class SyslogAppender: public Yb::ILogAppender
{
    static char process_name[100];

    Yb::Mutex appender_mutex_;
    typedef std::map<Yb::String, int> LogLevelMap;
    LogLevelMap log_levels_;

    static int log_level_to_syslog(int log_level);
    void really_append(const Yb::LogRecord &rec);

public:
    SyslogAppender();
    ~SyslogAppender();
    void append(const Yb::LogRecord &rec);
    int get_level(const Yb::String &name);
    void set_level(const Yb::String &name, int level);
};
#endif // #if !defined(YBUTIL_WINDOWS)

int decode_log_level(const Yb::String &log_level);
const Yb::String encode_log_level(int level);

class App: public Yb::ILogger
{
    std::auto_ptr<std::ofstream> file_stream_;
    std::auto_ptr<Yb::ILogAppender> appender_;
    Yb::ILogger::Ptr log_;
    bool use_db_;
    std::auto_ptr<Yb::Engine> engine_;

    void init_log(const Yb::String &log_name,
                  const Yb::String &log_level);
    void init_engine(const Yb::String &db_name);
    const Yb::String get_db_url();

public:
    App(): use_db_(false) {}
    void init(const Yb::String &log_name = _T("log.txt"),
              const Yb::String &log_level = _T("DEBUG"),
              const Yb::String &db_name = _T(""));
    virtual ~App();
    Yb::Engine &get_engine();
    bool uses_db() const { return use_db_; }
    std::auto_ptr<Yb::Session> new_session();

    // implement ILogger
    Yb::ILogger::Ptr new_logger(const Yb::String &name);
    Yb::ILogger::Ptr get_logger(const Yb::String &name);
    int get_level();
    void set_level(int level);
    void log(int level, const Yb::String &msg);
    const Yb::String get_name() const;
};

typedef Yb::SingletonHolder<App> theApp;

#endif // _AUTH__APP_H_
// vim:ts=4:sts=4:sw=4:et:
