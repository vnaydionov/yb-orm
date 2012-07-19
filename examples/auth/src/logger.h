#ifndef _AUTH__LOGGER_H_
#define _AUTH__LOGGER_H_

#include <memory>
#include <string>
#include <fstream>
#include <util/nlogger.h>

class Log: public Yb::ILogger {
    bool should_close;
    std::ofstream file_stream;
    Yb::LogAppender appender;
    Yb::Logger logger;
    // non-copyable
    Log(const Log &);
    Log &operator=(const Log &);
public:
    Log(const std::string&);
    void close();
    // implement virtual functions
    Yb::ILogger::Ptr new_logger(const std::string &name);
    void log(int level, const std::string &msg);
    const std::string get_name() const;
    ~Log();
};

#endif // _AUTH__LOGGER_H_
// vim:ts=4:sts=4:sw=4:et:
