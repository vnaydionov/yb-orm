#if !(defined(__WIN32__) || defined(_WIN32))
#include <signal.h>
#endif
#include <time.h>
#include <boost/thread/xtime.hpp>
#include <boost/bind.hpp>
#include <orm/SqlPool.h>

using namespace std;

#define LOG(l, x) do{ if (logger_.get()) { OStringStream __log; \
    __log << x; logger_->log(l, NARROW(__log.str())); } }while(0)

namespace Yb {

static
void
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

static
const String
format_stats(const String &source_id, int count = -1, int pool_size = -1)
{
    OStringStream stats;
    stats << _T(" [source: ") << source_id;
    if (count >= 0 && pool_size >= 0)
        stats << _T(", total open: ") << (count + pool_size)
            << _T(", in pool: ") << pool_size;
    stats << _T("]");
    return stats.str();
}

const String
SqlPool::get_stats(const String &source_id)
{
    map<String, int>::iterator c = counts_.find(source_id);
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
        //LOG(ll_DEBUG, _T("monitor wake"));
        String del_source_id;
        SqlConnectPtr del_handle = NULL;
        int del_count = 0, del_pool_sz = 0;
        // process all 'open' requests
        while (1) {
            //LOG(ll_DEBUG, _T("looking for connects_for_open_"));
            String id;
            {
                boost::mutex::scoped_lock lock(stop_mux_);
                if (!connects_for_open_.empty()) {
                    id = connects_for_open_.front();
                    connects_for_open_.pop_front();
                }
            }
            if (id.empty())
                break;
            map<String, SqlSource>::iterator src = sources_.find(id);
            if (sources_.end() == src) {
                LOG(ll_ERROR, _T("unknown source") << format_stats(id));
                continue;
            }
            try {
                LOG(ll_DEBUG, _T("opening new connection") << format_stats(id));
                SqlConnectPtr handle = new SqlConnect(src->second);
                handle->free_since_ = time(NULL);
                boost::mutex::scoped_lock lock(pool_mux_);
                ++counts_[id];
                LOG(ll_INFO, _T("opened new connection") << get_stats(id));
                --counts_[id];
                pools_[id].push_back(handle);
                pool_cond_.notify_one();
            }
            catch (const std::exception &e) {
                LOG(ll_ERROR, _T("connect error") << format_stats(id)
                        << _T(" ") << WIDEN(e.what()));
            }
        }
        // process all 'forced close' requests
        while (1) {
            //LOG(ll_DEBUG, _T("looking for connects_for_delete_"));
            del_handle = NULL;
            {
                boost::mutex::scoped_lock lock(stop_mux_);
                if (!connects_for_delete_.empty()) {
                    del_handle = connects_for_delete_.front();
                    connects_for_delete_.pop_front();
                }
            }
            if (!del_handle)
                break;
            delete del_handle;
            LOG(ll_INFO, _T("forced close connection"));
        }
        // processing 'idle close'
        {
            boost::mutex::scoped_lock lock(pool_mux_);
            map<String, Pool>::iterator i = pools_.begin(), iend = pools_.end();
            for (bool quit = false; !quit && i != iend; ++i) {
                Pool::iterator j = i->second.begin(), jend = i->second.end();
                for (; j != jend; ++j) {
                    if (time(NULL) - (*j)->free_since_ >= idle_time_) {
                        quit = true;
                        del_source_id = i->first;
                        del_handle = *j;
                        del_count = counts_[del_source_id];
                        i->second.erase(j);
                        del_pool_sz = i->second.size();
                        break;
                    }
                }
            }
        }
        if (del_handle) {
            LOG(ll_DEBUG, _T("closing idle connection")
                    << format_stats(del_source_id));
            delete del_handle;
            LOG(ll_INFO, _T("closed idle connection")
                    << format_stats(del_source_id, del_count, del_pool_sz));
        }
    }
    return NULL;
}

void
SqlPool::close_all()
{
    boost::mutex::scoped_lock lock(pool_mux_);
    for (map<String, Pool>::iterator i = pools_.begin(); i != pools_.end(); ++i) {
        LOG(ll_DEBUG, _T("closing all") << get_stats(i->first));
        for (Pool::iterator j = i->second.begin(); j != i->second.end(); ++j)
            delete *j;
        LOG(ll_INFO, _T("closed all") << get_stats(i->first));
    }
}

void
SqlPool::stop_monitor_thread()
{
    boost::mutex::scoped_lock lock(stop_mux_);
    stop_monitor_flag_ = true;
    stop_cond_.notify_all();
}

bool
SqlPool::sleep_not_stop(void)
{
    boost::mutex::scoped_lock lock(stop_mux_);
    while (!stop_monitor_flag_) {
        boost::xtime xt;
        boost::xtime_get(&xt, boost::TIME_UTC);
        xt.sec += monitor_sleep_;
        if (!stop_cond_.timed_wait(lock, xt) ||
                !connects_for_open_.empty() ||
                !connects_for_delete_.empty())
            return true;
    }
    return false;
}

SqlPool::SqlPool(int pool_max_size, int idle_time,
                 int monitor_sleep, ILogger *logger)
    : pool_max_size_(pool_max_size)
    , idle_time_(idle_time)
    , monitor_sleep_(monitor_sleep)
    , stop_monitor_flag_(false)
    , monitor_(boost::bind(&SqlPool::monitor_thread, this))
{
    if (logger) {
        ILogger::Ptr pool_logger = logger->new_logger("sql_pool");
        logger_.reset(pool_logger.release());
    }
}

SqlPool::~SqlPool()
{
    stop_monitor_thread();
    monitor_.join();
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
    //LOG(ll_DEBUG, _T("SqlPool::get()"));
    map<String, SqlSource>::iterator src = sources_.find(id);
    if (sources_.end() == src)
        throw GenericDBError(_T("Unknown source ID: ") + id);
    String stats;
    {
        boost::mutex::scoped_lock lock(pool_mux_);
        stats = get_stats(id); // can do this only when pool_mux_ is locked
    }
    boost::xtime xt;
    if (!pools_[id].size()) {
        if (counts_[id] >= pool_max_size_) {
            if (timeout > 0) {
                LOG(ll_INFO, _T("waiting for connection ")
                        << timeout << _T(" sec") << stats);
                boost::xtime_get(&xt, boost::TIME_UTC);
                xt.sec += timeout;
            }
            else
                LOG(ll_INFO, _T("waiting for connection forever") << stats);
        }
        else {
            timeout = 5; // new connection timeout
            LOG(ll_DEBUG, _T("waiting for new connection ")
                    << timeout << _T(" sec") << stats);
            boost::xtime_get(&xt, boost::TIME_UTC);
            xt.sec += timeout;
            boost::mutex::scoped_lock lock(stop_mux_);
            connects_for_open_.push_back(id);
            stop_cond_.notify_all();
        }
    }
    boost::mutex::scoped_lock lock(pool_mux_);
    while (!pools_[id].size()) {
        if (timeout > 0) {
            if (!pool_cond_.timed_wait(lock, xt))
                return NULL;
        }
        else
            pool_cond_.wait(lock);
    }
    SqlConnectPtr handle = NULL;
    handle = pools_[id].front();
    ++counts_[id];
    pools_[id].pop_front();
    LOG(ll_INFO, _T("got connection") << get_stats(id));
    return handle;
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
    SqlConnectPtr del_handle = NULL;
    int del_count = 0, del_pool_sz = 0;
    String id = handle->get_source().get_id();
    {
        boost::mutex::scoped_lock lock(pool_mux_);
        --counts_[id];
        if (!close_now) {
            handle->free_since_ = time(NULL);
            pools_[id].push_back(handle);
            LOG(ll_INFO, _T("put connection") << get_stats(id));
            pool_cond_.notify_one();
        }
        else {
            del_handle = handle;
            del_count = counts_[id];
            del_pool_sz = pools_[id].size();
        }
    }
    if (del_handle) {
        LOG(ll_DEBUG, _T("forced closing connection") << format_stats(id));
        boost::mutex::scoped_lock lock(stop_mux_);
        connects_for_delete_.push_back(del_handle);
        stop_cond_.notify_all();
    }
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
