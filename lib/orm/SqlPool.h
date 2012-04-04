// -*- C++ -*-
#ifndef YB__ORM__SQL_POOL__INCLUDED
#define YB__ORM__SQL_POOL__INCLUDED

#include <map>
#include <deque>
#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>
#include <orm/SqlDriver.h>

namespace Yb {

#define YB_POOL_SIZE 5 // max pool size
#define YB_POOL_IDLE_TIME 15 // sec.
#define YB_POOL_MGR_SLEEP 5  // sec.

class SqlPool
{
public:
    typedef SqlConnect *SqlConnectPtr;
    SqlPool(int pool_size = YB_POOL_SIZE,
            int idle_time = YB_POOL_IDLE_TIME,
            int mgr_sleep = YB_POOL_MGR_SLEEP,
            ILogger *logger = NULL);
    ~SqlPool();
    void add_source(const SqlSource &source);
    SqlConnectPtr get(const String &id, int timeout = 0);
    void put(SqlConnectPtr handle, bool close_now = false);

private:
    std::map<String, SqlSource> sources_;
    std::map<String, int> counts_;
    typedef std::deque<SqlConnectPtr> Pool;
    std::map<String, Pool> pools_;
    boost::mutex pool_mux_, stop_mux_;
    boost::condition pool_cond_, stop_cond_;
    int pool_size_, idle_time_, mgr_sleep_;
    bool stop_mgr_flag_;
    boost::thread manager_;
    ILogger::Ptr logger_;

    void *manager_thread();
    void close_all();
    void stop_manager_thread();
    void conn_manager();
    bool sleep_not_stop();
};

class SqlPoolDescriptor
{
    SqlPool &pool_;
    String source_id_;
    int timeout_;
public:
    SqlPoolDescriptor(SqlPool &pool, const String &source_id,
                      int timeout)
        : pool_(pool)
        , source_id_(source_id)
        , timeout_(timeout)
    {}
    SqlPool &get_pool() const { return pool_; }
    const String &get_source_id() const { return source_id_; }
    int get_timeout() const { return timeout_; }
};

class SqlHandleVar: public boost::noncopyable
{
    SqlPool &pool_;
    SqlConnect *sqlh_;
public:
    explicit SqlHandleVar(const SqlPoolDescriptor &d)
        : pool_(d.get_pool())
        , sqlh_(pool_.get(d.get_source_id(), d.get_timeout()))
    {
        if (!sqlh_)
            throw GenericDBError(_T("Can't get connection"));
    }
    SqlHandleVar(SqlPool &pool, const String &source_id,
                 int timeout)
        : pool_(pool)
        , sqlh_(pool_.get(source_id, timeout))
    {
        if (!sqlh_)
            throw GenericDBError(_T("Can't get connection"));
    }
    ~SqlHandleVar()
    {
        pool_.put(sqlh_);
    }
    SqlConnect *operator-> () const { return sqlh_; }
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__SQL_POOL__INCLUDED
