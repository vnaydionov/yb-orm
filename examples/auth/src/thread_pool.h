#ifndef _AUTH__THREAD_POOL_H_
#define _AUTH__THREAD_POOL_H_

#include <vector>
#include <deque>
#include <boost/tuple/tuple.hpp>
#include <util/thread.h>
#include <util/nlogger.h>

template <class WTask>
class WorkerTaskQueue {
public:
    WorkerTaskQueue()
        : queue_cond_(queue_mux_)
    {}

    boost::tuple<size_t, Yb::MilliSec> put(const WTask &task) {
        Yb::ScopedLock lock(queue_mux_);
        queue_.push_back(task);
        size_t new_size = queue_.size();
        queue_cond_.notify_one();
        Yb::MilliSec time_since_get = Yb::get_cur_time_millisec() - last_get_;
        return boost::make_tuple(new_size, time_since_get);
    }

    const WTask get() {
        Yb::ScopedLock lock(queue_mux_);
        while (!queue_.size())
            queue_cond_.wait(lock, 250);
        const WTask result = queue_.front();
        queue_.pop_front();
        last_get_ = Yb::get_cur_time_millisec();
        return result;
    }

private:
    Yb::Mutex queue_mux_;
    Yb::Condition queue_cond_;
    std::deque<WTask> queue_;
    Yb::MilliSec last_get_;
};

template <class WTask>
class WorkerThread: public Yb::Thread {
    void on_run() {
        //std::ostringstream out1;
        //out1 << "Worker " << (long)this << " started\n";
        //std::cerr << out1.str();
        while (true) {
            const WTask task = task_queue_.get();
            if (task.is_empty())
                break;
            //std::ostringstream out3;
            //out3 << "Worker " << (long)this << " processing...\n";
            //std::cerr << out3.str();
            process_task(task);
        }
        //std::ostringstream out2;
        //out2 << "Worker " << (long)this << " finished\n";
        //std::cerr << out2.str();
    }

public:
    typedef WTask WorkerTask;

    WorkerThread(WorkerTaskQueue<WTask> &task_queue):
        task_queue_(task_queue)
    {}

    virtual void process_task(const WTask &task) = 0;

private:
    WorkerTaskQueue<WTask> &task_queue_;
};

template <class WThread>
class WorkerThreadPool {
public:
    typedef typename WThread::WorkerTask WTask;

    WorkerThreadPool(size_t size) {
        spawn(size);
    }

    ~WorkerThreadPool() {
        stop();
    }

    void process_task(const WTask &task) {
        size_t new_size;
        Yb::MilliSec time_since_get;
        boost::tie(new_size, time_since_get) = task_queue_.put(task);
    }

    void spawn(size_t count) {
        Yb::ScopedLock lock(workers_mux_);
        for (size_t i = 0; i < count; ++i) {
            workers_.push_back(NULL);
            workers_.back() = new WThread(task_queue_);
            workers_.back()->start();
        }
    }

    void stop() {
        Yb::ScopedLock lock(workers_mux_);
        for (size_t i = 0; i < workers_.size(); ++i)
            task_queue_.put(WTask::empty_task());
        for (size_t i = 0; i < workers_.size(); ++i) {
            workers_[i]->wait();
            delete workers_[i];
        }
    }

    size_t workers_count() const { return workers_.size(); }
private:
    WorkerTaskQueue<WTask> task_queue_;
    Yb::Mutex workers_mux_;
    std::vector<WThread *> workers_;
};

#endif // _AUTH__THREAD_POOL_H_
// vim:ts=4:sts=4:sw=4:et:
