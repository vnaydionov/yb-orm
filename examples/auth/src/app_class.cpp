#include "app_class.h"
#include <fstream>
#include <stdexcept>
#include <util/string_utils.h>
#include <orm/domain_object.h>
#if !defined(YBUTIL_WINDOWS)
#include <syslog.h>
#endif

using namespace std;
typedef std::runtime_error RunTimeError;

FileLogAppender::FileLogAppender(std::ostream &out)
    : LogAppender(out)
{}

void FileLogAppender::append(const Yb::LogRecord &rec)
{
    Yb::LogAppender::append(rec);
}

#if !defined(YBUTIL_WINDOWS)
const std::string get_process_name()
{
    const std::string file_name = "/proc/self/cmdline";
    std::ifstream inp(file_name.c_str());
    if (!inp)
        throw RunTimeError("can't open file: " + file_name);
    std::string cmdline;
    std::copy(std::istream_iterator<char>(inp),
              std::istream_iterator<char>(),
              std::back_inserter(cmdline));
    size_t pos = cmdline.find('\0');
    if (std::string::npos != pos)
        cmdline = cmdline.substr(0, pos);
    pos = cmdline.rfind('/');
    if (std::string::npos != pos)
        cmdline = cmdline.substr(pos + 1);
    return cmdline;
}

const std::string fmt_string_escape(const std::string &s)
{
    std::string r;
    r.reserve(s.size() * 2);
    for (size_t pos = 0; pos < s.size(); ++pos) {
        if ('%' == s[pos])
            r += '%';
        r += s[pos];
    }
    return r;
}

char SyslogAppender::process_name[100];

int SyslogAppender::log_level_to_syslog(int log_level)
{
    switch (log_level) {
    case Yb::ll_CRITICAL:
        return LOG_CRIT;
    case Yb::ll_ERROR:
        return LOG_ERR;
    case Yb::ll_WARNING:
        return LOG_WARNING;
    case Yb::ll_INFO:
        return LOG_INFO;
    case Yb::ll_DEBUG:
    case Yb::ll_TRACE:
        return LOG_DEBUG;
    }
    return LOG_DEBUG;
}

SyslogAppender::SyslogAppender()
{
    std::string process = get_process_name();
    strncpy(process_name, process.c_str(), sizeof(process_name));
    process_name[sizeof(process_name) - 1] = 0;
    ::openlog(process_name, LOG_NDELAY | LOG_PID, LOG_USER);
}

SyslogAppender::~SyslogAppender()
{
    ::closelog();
}

void SyslogAppender::really_append(const Yb::LogRecord &rec)
{
    int priority = log_level_to_syslog(rec.get_level());
    std::ostringstream msg;
    msg << "T" << rec.get_tid() << " "
        << rec.get_component() << ": "
        << rec.get_msg();
    ::syslog(priority, fmt_string_escape(msg.str()).c_str());
}

void SyslogAppender::append(const Yb::LogRecord &rec)
{
    Yb::ScopedLock lk(appender_mutex_);
    int target_level = Yb::ll_ALL;
    LogLevelMap::iterator it = log_levels_.find(rec.get_component());
    if (log_levels_.end() != it)
        target_level = it->second;
    if (rec.get_level() <= target_level) {
        really_append(rec);
    }
}

int SyslogAppender::get_level(const std::string &name)
{
    Yb::ScopedLock lk(appender_mutex_);
    LogLevelMap::iterator it = log_levels_.find(name);
    if (log_levels_.end() == it)
        return Yb::ll_ALL;
    return it->second;
}

void SyslogAppender::set_level(const std::string &name, int level)
{
    Yb::ScopedLock lk(appender_mutex_);
    if (name.size() >= 2 && name.substr(name.size() - 2) == ".*")
    {
        std::string prefix = name.substr(0, name.size() - 1);
        LogLevelMap::iterator it = log_levels_.begin(),
                              it_end = log_levels_.end();
        for (; it != it_end; ++it)
            if (it->first.substr(0, prefix.size()) == prefix)
                it->second = level;
        std::string prefix0 = name.substr(0, name.size() - 2);
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
#endif // !defined(YBUTIL_WINDOWS)

int decode_log_level(const string &log_level0)
{
    using Yb::StrUtils::str_to_lower;
    const string log_level = NARROW(str_to_lower(WIDEN(log_level0)));
    if (log_level.empty())
        return Yb::ll_DEBUG;
    if (log_level == "critical" || log_level == "cri" || log_level == "crit")
        return Yb::ll_CRITICAL;
    if (log_level == "error"    || log_level == "err" || log_level == "erro")
        return Yb::ll_ERROR;
    if (log_level == "warning"  || log_level == "wrn" || log_level == "warn")
        return Yb::ll_WARNING;
    if (log_level == "info"     || log_level == "inf")
        return Yb::ll_INFO;
    if (log_level == "debug"    || log_level == "dbg" || log_level == "debg")
        return Yb::ll_DEBUG;
    if (log_level == "trace"    || log_level == "trc" || log_level == "trac")
        return Yb::ll_TRACE;
    throw RunTimeError("invalid log level: " + log_level);
}

const string encode_log_level(int level)
{
    Yb::LogRecord r(level, "__xxx__", "yyy");
    return string(r.get_level_name());
}

void App::init_log(const string &log_name, const string &log_level)
{
    if (!log_.get()) {
#if !defined(YBUTIL_WINDOWS)
        using Yb::StrUtils::str_to_lower;
        if (_T("syslog") == str_to_lower(WIDEN(log_name))) {
            appender_.reset(new SyslogAppender());
        }
        else {
#endif // !defined(YBUTIL_WINDOWS)
            file_stream_.reset(new ofstream(log_name.data(), ios::app));
            if (file_stream_->fail())
                throw RunTimeError("can't open logfile: " + log_name);
            appender_.reset(new FileLogAppender(*file_stream_));
#if !defined(YBUTIL_WINDOWS)
        }
#endif // !defined(YBUTIL_WINDOWS)
        log_.reset(new Yb::Logger(appender_.get()));
        info("Application started.");
        info("Setting level " + encode_log_level(decode_log_level(log_level))
                + " for root logger");
        log_->set_level(decode_log_level(log_level));
    }
}

const Yb::String App::get_db_url()
{
    return Yb::StrUtils::xgetenv(_T("YBORM_URL"));
}

void App::init_engine(const Yb::String &db_name)
{
    if (!engine_.get()) {
        Yb::ILogger::Ptr yb_logger(new_logger("yb").release());
        Yb::init_schema();
        auto_ptr<Yb::SqlPool> pool(
                new Yb::SqlPool(
                    YB_POOL_MAX_SIZE, YB_POOL_IDLE_TIME,
                    YB_POOL_MONITOR_SLEEP, yb_logger.get(), false));
        Yb::SqlSource src(get_db_url());
        src[_T("&id")] = db_name;
        pool->add_source(src);
        engine_.reset(new Yb::Engine(Yb::Engine::READ_WRITE, pool, db_name));
        engine_->set_echo(true);
        engine_->set_logger(yb_logger);

#if SCHEMA_DUMP
        Yb::theSchema().export_xml("schema_dump.xml");
        Yb::theSchema().export_ddl("schema_dump.sql", src.dialect());
#endif
#if SCHEMA_CREATE
        try {
            auto_ptr<Yb::EngineCloned> engine(engine_->clone());
            engine->create_schema(Yb::theSchema(), false);
            cerr << "Schema created\n";
        }
        catch (const Yb::DBError &e) {
            cerr << "Schema already exists\n";
        }
#endif
    }
}

void App::init(const string &log_name, const string &log_level, const Yb::String &db_name)
{
    init_log(log_name, log_level);
    if (!Yb::str_empty(db_name)) {
        init_engine(db_name);
        use_db_ = true;
    }
}

App::~App()
{
    engine_.reset(NULL);
    if (log_.get()) {
        info("log finished");
        FileLogAppender *appender = dynamic_cast<FileLogAppender *> (
                appender_.get());
        if (appender)
            appender->flush();
        if (file_stream_.get())
            file_stream_->close();
    }
    log_.reset(NULL);
    appender_.reset(NULL);
    file_stream_.reset(NULL);
}

Yb::Engine &App::get_engine()
{
    if (!engine_.get())
        throw RunTimeError("engine not created");
    return *engine_.get();
}

auto_ptr<Yb::Session> App::new_session()
{
    return auto_ptr<Yb::Session>(
            new Yb::Session(Yb::theSchema(), &get_engine()));
}

Yb::ILogger::Ptr App::new_logger(const string &name)
{
    if (!log_.get())
        throw RunTimeError("log not opened");
    return log_->new_logger(name);
}

Yb::ILogger::Ptr App::get_logger(const std::string &name)
{
    if (!log_.get())
        throw RunTimeError("log not opened");
    return log_->get_logger(name);
}

int App::get_level()
{
    if (!log_.get())
        throw RunTimeError("log not opened");
    return log_->get_level();
}

void App::set_level(int level)
{
    if (!log_.get())
        throw RunTimeError("log not opened");
    log_->set_level(level);
}

void App::log(int level, const string &msg)
{
    if (log_.get())
        log_->log(level, msg);
}

const string App::get_name() const
{
    if (!log_.get())
        return string();
    return log_->get_name();
}

// vim:ts=4:sts=4:sw=4:et:
