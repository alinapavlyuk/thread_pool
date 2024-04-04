#ifndef LAB2_THREADPOOL_H
#define LAB2_THREADPOOL_H

#include "TaskQueue.h"
#include <vector>
#include <functional>
#include <condition_variable>
#include <algorithm>

const size_t QUEUE_SIZE = 20;

using std::chrono::nanoseconds;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using time_point = std::chrono::time_point<high_resolution_clock>;

class ThreadPool {
public:
    inline ThreadPool() = default;
    inline ~ThreadPool() { terminate(); }

public:
    void initialize(size_t worker_count);
    void terminate();
    void urgent_terminate();

    void routine(int id);

    void pause();
    void unpause();

    bool working() const;
    bool working_unsafe() const;

public:
    void print_queue();

    int get_result(int id);
    int get_status(int id);
    void explain_status(int status);

    void compare_full_time();
    void show_statistics();

    void add_task(Task& task);
    inline void delete_task(int id);

public:
    ThreadPool(const ThreadPool& other) = delete;
    ThreadPool(ThreadPool&& other) = delete;
    ThreadPool& operator=(const ThreadPool& rhs) = delete;
    ThreadPool& operator=(ThreadPool&& rhs) = delete;

private:
    mutable read_write_lock tp_rw_lock; //thread pool lock
    mutable read_write_lock stat_rw_lock; //statistics lock
    mutable std::condition_variable_any task_waiter;
    mutable std::condition_variable_any unpause_waiter;

    std::vector<std::thread> workers;
    TaskQueue tasks{QUEUE_SIZE};
    std::unordered_map<int, int> results;
    std::unordered_map<int, double> wait_time_statistics;
    std::unordered_map<int, int> finished_tasks_statistics;

    int rejected_num = 0;
    volatile bool queue_full = false;
    time_point full_begin;
    time_point full_end;
    nanoseconds max_full_time{0};
    nanoseconds min_full_time{0};

    bool initialized = false;
    bool paused = false;
    bool terminated = false;
};

#endif //LAB2_THREADPOOL_H

