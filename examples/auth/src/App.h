#ifndef _AUTH__APP_H_
#define _AUTH__APP_H_

#include "logger.h"
#include <orm/MetaDataSingleton.h>

class App: public Yb::ILogger
{
    Yb::ILogger::Ptr log_;
    std::auto_ptr<Yb::Engine> engine_;

    void init_log()
    {
        if (!log_.get())
            log_.reset(new Log("log.txt"));
    }

    void init_engine(Yb::ILogger *root_logger)
    {
        if (!engine_.get()) {
            Yb::init_default_meta();
            std::auto_ptr<Yb::SqlPool> pool(
                    new Yb::SqlPool(YB_POOL_MAX_SIZE, YB_POOL_IDLE_TIME,
                        YB_POOL_MONITOR_SLEEP, root_logger));
            pool->add_source(Yb::Engine::sql_source_from_env(_T("auth_db")));
            engine_.reset(new Yb::Engine(Yb::Engine::MANUAL, pool, _T("auth_db")));
            engine_->set_echo(true);
            engine_->set_logger(root_logger->new_logger("orm"));
        }
    }

public:
    Yb::Engine *get_engine() { return engine_.get(); }

    std::auto_ptr<Yb::Session> new_session()
    {
        return std::auto_ptr<Yb::Session>(
                new Yb::Session(Yb::theMetaData::instance(), get_engine()));
    }

    App()
    {
        init_log();
        init_engine(log_.get());
    }

    ~App()
    {
        engine_.reset(NULL);
        log_.reset(NULL);
    }

    Yb::ILogger::Ptr new_logger(const std::string &name)
    {
        return log_->new_logger(name);
    }

    void log(int level, const std::string &msg)
    {
        return log_->log(level, msg);
    }

    const std::string get_name() const
    {
        return log_->get_name();
    }
};

typedef Yb::SingletonHolder<App> theApp;

#endif // _AUTH__APP_H_
// vim:ts=4:sts=4:sw=4:et:
