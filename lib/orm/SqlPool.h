// -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
#ifndef YB__ORM__SQL_POOL__INCLUDED
#define YB__ORM__SQL_POOL__INCLUDED

#include <map>
#include <deque>
#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>
#include <orm/SqlDriver.h>

namespace Yb {

#define YB_POOL_MAX_SIZE 10
#define YB_POOL_IDLE_TIME 30 // sec.
#define YB_POOL_MONITOR_SLEEP 2 // sec.
#define YB_POOL_WAIT_TIME 20 // sec.

class SqlPool
{
public:
    typedef SqlConnect *SqlConnectPtr;
    SqlPool(int pool_max_size = YB_POOL_MAX_SIZE,
            int idle_time = YB_POOL_IDLE_TIME,
            int monitor_sleep = YB_POOL_MONITOR_SLEEP,
            ILogger *logger = NULL);
    ~SqlPool();
    void add_source(const SqlSource &source);
    SqlConnectPtr get(const String &id, int timeout = YB_POOL_WAIT_TIME);
    void put(SqlConnectPtr handle, bool close_now = false);

private:
    std::map<String, SqlSource> sources_;
    std::map<String, int> counts_;
    typedef std::deque<SqlConnectPtr> Pool;
    std::deque<String> connects_for_open_;
    std::deque<SqlConnectPtr> connects_for_delete_;
    std::map<String, Pool> pools_;
    boost::mutex pool_mux_, stop_mux_;
    boost::condition pool_cond_, stop_cond_;
    int pool_max_size_, idle_time_, monitor_sleep_;
    bool stop_monitor_flag_;
    boost::thread monitor_;
    ILogger::Ptr logger_;

    void *monitor_thread();
    void close_all();
    void stop_monitor_thread();
    bool sleep_not_stop();
    const String get_stats(const String &source_id);
};

class SqlPoolDescr
{
    SqlPool &pool_;
    String source_id_;
    int timeout_;
public:
    SqlPoolDescr(SqlPool &pool, const String &source_id, int timeout)
        : pool_(pool)
        , source_id_(source_id)
        , timeout_(timeout)
    {}
    SqlPool &get_pool() const { return pool_; }
    const String &get_source_id() const { return source_id_; }
    int get_timeout() const { return timeout_; }
};

class SqlConnectVar: public boost::noncopyable
{
    SqlPool &pool_;
    SqlConnect *handle_;
public:
    explicit SqlConnectVar(const SqlPoolDescr &d)
        : pool_(d.get_pool())
        , handle_(pool_.get(d.get_source_id(), d.get_timeout()))
    {
        if (!handle_)
            throw GenericDBError(_T("Can't get connection"));
    }
    SqlConnectVar(SqlPool &pool, const String &source_id,
                 int timeout = YB_POOL_WAIT_TIME)
        : pool_(pool)
        , handle_(pool_.get(source_id, timeout))
    {
        if (!handle_)
            throw GenericDBError(_T("Can't get connection"));
    }
    ~SqlConnectVar()
    {
        pool_.put(handle_);
    }
    SqlConnect *operator-> () const { return handle_; }
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__SQL_POOL__INCLUDED
