// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#define YBORM_SOURCE

#if !defined(YBORM_WINDOWS)
#include <signal.h>
#endif
#include <time.h>
#include "orm/sql_pool.h"

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif // _MSC_VER

#define LOG(l, x) do { if (logger_.get()) { std::ostringstream __log; \
    __log << NARROW(x); logger_->log(l, __log.str()); } } while(0)

namespace Yb {

PoolError::PoolError(const String &err)
    : DBError(err)
{}

PoolMonThread::PoolMonThread(SqlPool *pool) : pool_(pool) {}

void PoolMonThread::on_run() { pool_->monitor_thread(); }

static
void
block_sigpipe()
{
#if !(defined(YBORM_WIN32) || defined(YBORM_WIN64))
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = SA_NODEFER;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGPIPE, &sa, 0);
#endif
}

static
const String
format_stats(const String &source_id, int count = -1, int pool_size = -1)
{
    std::ostringstream stats;
    stats << " [source: " << NARROW(source_id);
    if (count >= 0 && pool_size >= 0)
        stats << ", total open: " << (count + pool_size)
            << ", in pool: " << pool_size;
    stats << "]";
    return WIDEN(stats.str());
}

const String
SqlPool::get_stats(const String &source_id)
{
    std::map<String, int>::iterator c = counts_.find(source_id);
    if (counts_.end() == c)
        return _T(" [source: ") + source_id + _T(", unknown source]");
    return format_stats(source_id, c->second, pools_[source_id].size());
}

void *
SqlPool::monitor_thread()
{
    block_sigpipe();
    LOG(ll_INFO, _T("monitor thread started"));
    while (sleep_not_stop()) {
        // processing 'idle close'
        ScopedLock lock(pool_mux_);
        std::map<String, Pool>::iterator i = pools_.begin(), iend = pools_.end();
        for (bool quit = false; !quit && i != iend; ++i) {
            Pool::iterator j = i->second.begin(), jend = i->second.end();
            for (; j != jend; ++j) {
                if (time(NULL) - (*j)->free_since_ >= idle_time_) {
                    quit = true;
                    String del_source_id = i->first;
                    SqlConnectionPtr del_handle = *j;
                    int del_count = counts_[del_source_id];
                    i->second.erase(j);
                    int del_pool_sz = i->second.size();
                    LOG(ll_DEBUG, _T("closing idle connection")
                            + format_stats(del_source_id));
                    delete del_handle;
                    LOG(ll_INFO, _T("closed idle connection")
                            + format_stats(del_source_id, del_count, del_pool_sz));
                    break;
                }
            }
        }
    }
    return NULL;
}

void
SqlPool::close_all()
{
    ScopedLock lock(pool_mux_);
    for (std::map<String, Pool>::iterator i = pools_.begin(); i != pools_.end(); ++i) {
        LOG(ll_DEBUG, _T("closing all") + get_stats(i->first));
        for (Pool::iterator j = i->second.begin(); j != i->second.end(); ++j)
            delete *j;
        LOG(ll_INFO, _T("closed all") + get_stats(i->first));
    }
}

void
SqlPool::stop_monitor_thread()
{
    ScopedLock lock(stop_mux_);
    stop_monitor_flag_ = true;
    stop_cond_.notify_one();
}

bool
SqlPool::sleep_not_stop(void)
{
    ScopedLock lock(stop_mux_);
    while (!stop_monitor_flag_) {
        if (!stop_cond_.wait(lock, monitor_sleep_ * 1000))
            return true;
    }
    return false;
}

SqlPool::SqlPool(int pool_max_size, int idle_time,
                 int monitor_sleep, ILogger *logger, bool interlocked_open)
    : stop_cond_(stop_mux_)
    , pool_max_size_(pool_max_size)
    , idle_time_(idle_time)
    , monitor_sleep_(monitor_sleep)
    , stop_monitor_flag_(false)
    , interlocked_open_(interlocked_open)
    , monitor_(this)
{
    if (logger) {
        ILogger::Ptr pool_logger = logger->new_logger("pool");
        logger_.reset(pool_logger.release());
    }
    monitor_.start();
}

SqlPool::~SqlPool()
{
    stop_monitor_thread();
    monitor_.wait();
    close_all();
}

void
SqlPool::add_source(const SqlSource &source)
{
    sources_[source.id()] = source;
    counts_[source.id()] = 0;
    pools_[source.id()] = Pool();
}

SqlPool::SqlConnectionPtr
SqlPool::get(const String &source_id, int timeout)
{
    SqlSource src;
    {
        ScopedLock lock(pool_mux_);
        std::map<String, SqlSource>::iterator source_it = sources_.find(source_id);
        if (sources_.end() == source_it)
            throw PoolError(_T("Unknown source ID: ") + source_id);
        if (pools_[source_id].size()) {
            SqlConnectionPtr handle = pools_[source_id].front();
            ++counts_[source_id];
            pools_[source_id].pop_front();
            LOG(ll_INFO, _T("got connection") + get_stats(source_id));
            return handle;
        }
        src = source_it->second;
    }
    LOG(ll_DEBUG, _T("opening connection") + format_stats(source_id));
    SqlConnectionPtr handle;
    if (interlocked_open_) {
        ScopedLock lock(open_mux_);
        handle = new SqlConnection(src);
    }
    else
        handle = new SqlConnection(src);
    LOG(ll_INFO, _T("opened connection") + get_stats(source_id));
    {
        ScopedLock lock(pool_mux_);
        ++counts_[source_id];
    }
    return handle;
}

void
SqlPool::put(SqlConnectionPtr handle, bool close_now)
{
    if (!handle)
        return;
    if (handle->bad())
        close_now = true;
    if (!close_now)
        handle->clear();
    if (handle->bad())
        close_now = true;
    const String source_id = handle->get_source().id();
    ScopedLock lock(pool_mux_);
    --counts_[source_id];
    if (!close_now) {
        handle->free_since_ = time(NULL);
        pools_[source_id].push_back(handle);
        LOG(ll_INFO, _T("put connection") + get_stats(source_id));
    }
    else {
        LOG(ll_DEBUG, _T("forced closing connection") + format_stats(source_id));
        delete handle;
        LOG(ll_INFO, _T("forced closed connection") + get_stats(source_id));
    }
}

bool
SqlPool::reconnect(SqlConnectionPtr &conn)
{
    const SqlSource source = conn->get_source();
    const String &source_id = source.id();
    put(conn, true); // close now
    LOG(ll_DEBUG, _T("reopening connection") + format_stats(source_id));
    ScopedLock lock(pool_mux_);
    conn = new SqlConnection(source);
    ++counts_[source_id];
    LOG(ll_INFO, _T("reopened connection") + get_stats(source_id));
    return true;
}

SqlConnectionVar::SqlConnectionVar(const SqlPoolDescr &d)
    : pool_(d.get_pool())
    , handle_(pool_.get(d.get_source_id(), d.get_timeout()))
{
    if (!handle_)
        throw PoolError(_T("Can't get connection"));
}

SqlConnectionVar::SqlConnectionVar(SqlPool &pool,
        const String &source_id,
        int timeout)
    : pool_(pool)
    , handle_(pool_.get(source_id, timeout))
{
    if (!handle_)
        throw PoolError(_T("Can't get connection"));
}

SqlConnectionVar::~SqlConnectionVar()
{
    pool_.put(handle_);
}

SqlConnection *
SqlConnectionVar::operator-> () const
{
    return handle_;
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
