#pragma once

#include <pthread.h>
#include <functional>

class ThreadCondition;

class Mutex {
public:
    friend class ThreadCondition;

    pthread_mutex_t mutex;
public:
    Mutex();

    void lock();

    void unlock();

    bool isLocked();

    ~Mutex();
};

class ThreadCondition {
    pthread_cond_t condition;
public:
    ThreadCondition();

    void wait(Mutex &m);

    void timedWait(Mutex &m, long millis);

    void signal();

    void broadcast();

    ~ThreadCondition();
};

class Thread {
    pthread_t thread;
    //it is !!!NOT!!! safe for the Thread to suicide in the onStop function
    std::function<void()> routine, onStop;
    volatile bool running, onStopCalledOnLastRun;

    void _onStop();

    static void *trick(void *_c);

    static bool setRunningCore(pthread_t, long core);

    void run();

    Thread(Thread const &) = delete;

    void operator=(Thread const &) = delete;
//add getter for the core
public:
    Thread(std::function<void()>, std::function<void()> = []() {});

    ~Thread();

    Thread & start();

    static void usleep(long micros);

    static long getNCores();

    static bool setThisThreadsRunningCore(long core_id);

    bool setRunningCore(long core_id);

    bool isRunning() const;

    void join() const;

    void cancel();
};