#include "logger.h"
#include <sstream>
#include <stdexcept>

Log::Log(const std::string& path)
    : should_close(false)
    , file_stream(path.c_str(), std::ios::app)
    , appender(file_stream)
    , logger(&appender)
{
    if (file_stream.fail())
        throw std::runtime_error("can't open logfile: " + path);
    should_close = true;
    info("log started");
}

Log::~Log()
{
    close();
}

void
Log::close()
{
    if (should_close) {
        info("end of log");
        appender.flush();
        file_stream.close();
        should_close = false;
    }
}

Yb::ILogger::Ptr
Log::new_logger(const std::string &name)
{
    return logger.new_logger(name);
}

void
Log::log(int level, const std::string &msg)
{
    if (should_close)
        logger.log(level, msg);
}

const std::string
Log::get_name() const
{
    return logger.get_name();
}

// vim:ts=4:sts=4:sw=4:et:
