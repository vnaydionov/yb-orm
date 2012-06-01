// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#include <util/Thread.h>

namespace Yb {

Mutex::Mutex()
{}

void Mutex::lock()
{
#if defined(YB_USE_WX)
    wxMutexError r = mutex_.Lock();
    if (r != wxMUTEX_NO_ERROR)
        throw RunTimeError(_T("Can't lock mutex"));
#elif defined(YB_USE_QT)
    mutex_.lock();
#else
    lock_.reset(new boost::mutex::scoped_lock(mutex_));
#endif
}

void Mutex::unlock()
{
#if defined(YB_USE_WX)
    wxMutexError r = mutex_.Unlock();
    if (r != wxMUTEX_NO_ERROR)
        throw RunTimeError(_T("Can't unlock mutex"));
#elif defined(YB_USE_QT)
    mutex_.unlock();
#else
    lock_.reset(NULL);
#endif
}

ScopedLock::ScopedLock(Mutex &mutex)
    : mutex_(mutex)
#if !defined(YB_USE_WX) && !defined(YB_USE_QT)
    , lock_(mutex_.mutex_)
#endif
{
#if defined(YB_USE_WX) || defined(YB_USE_QT)
    mutex_.lock();
#endif
}

ScopedLock::~ScopedLock()
{
#if defined(YB_USE_WX) || defined(YB_USE_QT)
    mutex_.unlock();
#endif
}

Condition::Condition(Mutex &mutex)
    : mutex_(mutex)
#if defined(YB_USE_WX)
    , cond_(mutex_.mutex_)
#endif
{
#if defined(YB_USE_WX)
    if (!cond_.IsOk())
        throw RunTimeError(_T("Can't init condition"));
#endif
}

bool Condition::wait(ScopedLock &lock, long milliSec)
{
    if (&mutex_ != &lock.mutex_)
        throw RunTimeError(_T("Mixing condition's mutexes"));
#if defined(YB_USE_WX)
    wxCondError r = milliSec == -1?
        cond_.Wait():
        cond_.WaitTimeout((unsigned long )milliSec);
    if (r == wxCOND_TIMEOUT)
        return false;
    if (r != wxCOND_NO_ERROR)
        throw RunTimeError(_T("Can't wait on condition"));
    return true;
#elif defined(YB_USE_QT)
    return cond_.wait(&mutex_.mutex_,
            milliSec == -1? ULONG_MAX: (unsigned long )milliSec);
#else
    if (milliSec == -1) {
        cond_.wait(lock.lock_);
        return true;
    }
    else {
        boost::xtime xt;
        boost::xtime_get(&xt, boost::TIME_UTC);
        xt.sec += ((unsigned long )milliSec)/1000;
        xt.nsec += (((unsigned long )milliSec)%1000) * 1000000;
        return cond_.timed_wait(lock.lock_, xt);
    }
#endif
}

void Condition::notify_one()
{
#if defined(YB_USE_WX)
    cond_.Signal();
#elif defined(YB_USE_QT)
    cond_.wakeOne();
#else
    cond_.notify_one();
#endif
}

void Condition::notify_all()
{
#if defined(YB_USE_WX)
    cond_.Broadcast();
#elif defined(YB_USE_QT)
    cond_.wakeAll();
#else
    cond_.notify_all();
#endif
}

#if !defined(YB_USE_WX) && !defined(YB_USE_QT)
ThreadCallable::ThreadCallable(Thread *thread_obj)
    : thread_obj_(thread_obj)
{}

void ThreadCallable::operator()()
{
    thread_obj_->on_run();
    thread_obj_->finished_ = true;
}
#endif

#if defined(YB_USE_WX)
void *Thread::Entry()
{
    on_run();
    finished_ = true;
    return 0;
}
#elif defined(YB_USE_QT)
void Thread::run()
{
    on_run();
    finished_ = true;
}
#endif

Thread::Thread() :
#if defined(YB_USE_WX)
    wxThread(wxTHREAD_JOINABLE),
#endif
    should_terminate_(false), finished_(false)
{
#if defined(YB_USE_WX)
    if (Create() != wxTHREAD_NO_ERROR)
        throw RunTimeError(_T("Error while creating thread"));
#endif
}

void Thread::start()
{
#if defined(YB_USE_WX)
    if (Run() != wxTHREAD_NO_ERROR)
        throw RunTimeError(_T("Error while starting thread"));
#elif defined(YB_USE_QT)
    QThread::start();
#else
    thread_.reset(new boost::thread(ThreadCallable(this)));
#endif
}

void Thread::terminate()
{
    should_terminate_ = true;
#if defined(YB_USE_WX)
    if (Delete() != wxTHREAD_NO_ERROR)
        throw RunTimeError(_T("Error while deleting thread"));
#endif
}

bool Thread::is_terminating()
{
#if defined(YB_USE_WX)
    return TestDestroy();
#else
    return should_terminate_;
#endif
}

void Thread::wait()
{
#if defined(YB_USE_WX)
    if ((long)Wait() == -1L)
        throw RunTimeError(_T("Error while waiting for thread"));
#elif defined(YB_USE_QT)
    QThread::wait();
#else
    thread_->join();
#endif
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
