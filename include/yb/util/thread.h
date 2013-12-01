// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef YB__UTIL__THREAD__INCLUDED
#define YB__UTIL__THREAD__INCLUDED

#include <util/Utility.h>
#include <util/String.h>
#include <util/Exception.h>

#if defined(YB_USE_WX)
#include <wx/thread.h>
#elif defined(YB_USE_QT)
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#else
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/thread/condition.hpp>
#include <boost/version.hpp>
#endif

namespace Yb {

class Condition;
class ScopedLock;

class Mutex: NonCopyable {
    friend class Condition;
    friend class ScopedLock;
#if defined(YB_USE_WX)
    wxMutex mutex_;
#elif defined(YB_USE_QT)
    QMutex mutex_;
#else
    boost::mutex mutex_;
    std::auto_ptr<boost::mutex::scoped_lock> lock_;
#endif
public:
    Mutex();
    void lock();
    void unlock();
};

class ScopedLock: NonCopyable {
    friend class Condition;
    Mutex &mutex_;
#if !defined(YB_USE_WX) && !defined(YB_USE_QT)
    boost::mutex::scoped_lock lock_;
#endif
public:
    ScopedLock(Mutex &mutex);
    ~ScopedLock();
};

class Condition: NonCopyable {
    Mutex &mutex_;
#if defined(YB_USE_WX)
    wxCondition cond_;
#elif defined(YB_USE_QT)
    QWaitCondition cond_;
#else
    boost::condition cond_;
#endif
public:
    Condition(Mutex &mutex);
    bool wait(ScopedLock &lock, long milliSec = -1);
    void notify_one();
    void notify_all();
};

#if !defined(YB_USE_WX) && !defined(YB_USE_QT)
class Thread;
struct ThreadCallable
{
    Thread *thread_obj_;
    ThreadCallable(Thread *thread_obj);
    void operator()();
};
#endif

class Thread
#if defined(YB_USE_WX)
    : public wxThread
#elif defined(YB_USE_QT)
    : public QThread
#endif
{
    bool should_terminate_, finished_;
#if defined(YB_USE_WX)
    void *Entry();
#elif defined(YB_USE_QT)
    void run();
#else
    friend struct ThreadCallable;
    std::auto_ptr<boost::thread> thread_;
#endif
protected:
    bool is_terminating();
    virtual void on_run() = 0;
public:
    Thread();
#if !defined(YB_USE_WX) && !defined(YB_USE_QT)
    virtual ~Thread() {}
#endif
    void start();
    void terminate();
    void wait();
    bool finished() const { return finished_; }
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__UTIL__THREAD__INCLUDED
