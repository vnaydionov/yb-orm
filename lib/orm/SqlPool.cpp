#if !(defined(__WIN32__) || defined(_WIN32))
#include <signal.h>
#endif
#include <time.h>
#include <boost/thread/xtime.hpp>
#include <boost/bind.hpp>
#include <orm/SqlPool.h>

#define LOG(l, x) do{ if (logger_.get()) { OStringStream __log; \
    __log << x; logger_->log(l, NARROW(__log.str())); } }while(0)

namespace Yb {

static void
block_sigpipe()
{
#if !(defined(__WIN32__) || defined(_WIN32))
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = SA_INTERRUPT | SA_NODEFER;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGPIPE, &sa, 0);
#endif
}

void *
SqlPool::manager_thread()
{
    block_sigpipe();
    LOG(ll_INFO, "Starting manager thread...");
    while (sleep_not_stop()) {
        SqlConnectPtr to_del = NULL;
        String del_source_id;
        int busy_count = 0, free_count = 0;
        {
            boost::mutex::scoped_lock lock(pool_mux_);
            bool break_it = false;
            std::map<String, Pool>::iterator 
                i = pools_.begin(), iend = pools_.end();
            for (; !break_it && i != iend; ++i) {
                Pool::iterator j = i->second.begin(), jend = i->second.end();
                for (; j != jend; ++j) {
                    if (time(NULL) - (*j)->free_since_ >
                            YB_POOL_IDLE_TIME)
                    {
                        break_it = true;
                        to_del = *j;
                        i->second.erase(j);
                        del_source_id = i->first;
                        busy_count = counts_[del_source_id];
                        free_count = pools_[del_source_id].size();
                        break;
                    }
                }
            }
        }
        if (to_del) {
            LOG(ll_INFO, del_source_id << ": idle close "
                    << busy_count << " " << free_count);
            delete to_del;
        }
    }
    return NULL;
}

void
SqlPool::close_all()
{
    boost::mutex::scoped_lock lock(pool_mux_);
    std::map<String, Pool>::iterator 
        i = pools_.begin(), iend = pools_.end();
    for (; i != iend; ++i) {
        Pool::iterator j = i->second.begin(), jend = i->second.end();
        for (; j != jend; ++j)
            delete *j;
    }
}

void
SqlPool::stop_manager_thread()
{
    boost::mutex::scoped_lock lock(stop_mux_);
    stop_mgr_flag_ = true;
    stop_cond_.notify_all();
}

bool
SqlPool::sleep_not_stop(void)
{
    boost::mutex::scoped_lock lock(stop_mux_);
    while (!stop_mgr_flag_) {
        boost::xtime xt;
        boost::xtime_get(&xt, boost::TIME_UTC);
        xt.sec += YB_POOL_MGR_SLEEP;
        if (!stop_cond_.timed_wait(lock, xt))
            return true;
    }
    return false;
}

SqlPool::SqlPool(int pool_size, int idle_time,
                 int mgr_sleep, ILogger *logger)
    : pool_size_(pool_size)
    , idle_time_(idle_time)
    , mgr_sleep_(mgr_sleep)
    , stop_mgr_flag_(false)
    , manager_(boost::bind(&SqlPool::manager_thread, this))
{
    if (logger) {
        ILogger::Ptr pool_logger = logger->new_logger("cpool");
        logger_.reset(pool_logger.release());
    }
}

SqlPool::~SqlPool()
{
    stop_manager_thread();
    manager_.join();
    close_all();
}

void
SqlPool::add_source(const SqlSource &source)
{
    sources_[source.get_id()] = source;
    counts_[source.get_id()] = 0;
    pools_[source.get_id()] = Pool();
}

SqlPool::SqlConnectPtr
SqlPool::get(const String &id, int timeout)
{
    std::map<String, SqlSource>::iterator src = sources_.find(id);
    if (sources_.end() == src)
        throw GenericDBError(_T("Unknown source ID: ") + id);
    boost::mutex::scoped_lock lock(pool_mux_);
    while (counts_[id] >= pool_size_) {
        if (!timeout) {
            LOG(ll_INFO, id << ": start waiting " << counts_[id]
                    << " " << pools_[id].size());
            pool_cond_.wait(lock);
        }
        else {
            LOG(ll_INFO, id << ": waiting " << counts_[id]
                    << " " << pools_[id].size());
            boost::xtime xt;
            boost::xtime_get(&xt, boost::TIME_UTC);
            xt.sec += timeout;
            if (!pool_cond_.timed_wait(lock, xt))
                return NULL;
        }
    }
    SqlConnectPtr d = NULL;
    if (pools_[id].size()) {
        d = pools_[id].front();
        ++counts_[id];
        pools_[id].pop_front();
        LOG(ll_INFO, id << ": getting " << counts_[id]
                << " " << pools_[id].size());
    } 
    else {
        try {
            LOG(ll_INFO, id << ": opening " << counts_[id] + 1
                    << " " << pools_[id].size());
            d = new SqlConnect(src->second);
            ++counts_[id];
            // TODO: optional post-connect initialization
        } 
        catch (const std::exception &e) {
            LOG(ll_ERROR, id << ": open error: " << e.what());
        }
    }
    return d;
}

void
SqlPool::put(SqlConnectPtr handle, bool close_now)
{
    if (!handle)
        return;
    if (handle->bad())
        close_now = true;
    if (!close_now)
        handle->clear();
    if (handle->bad())
        close_now = true;
    boost::mutex::scoped_lock lock(pool_mux_);
    handle->free_ = true;
    const String &id = handle->get_source().get_id();
    --counts_[id];
    if (!close_now) {
        pools_[id].push_back(handle);
        handle->free_since_ = time(NULL);
        LOG(ll_INFO, id << _T(": putting ") << counts_[id]
                << _T(" ") << pools_[id].size());
    }
    else {
        LOG(ll_INFO, id << _T(": closing ") << counts_[id]
                << _T(" ") << pools_[id].size());
        delete handle;
    }
    pool_cond_.notify_one();
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
