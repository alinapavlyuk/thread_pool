#ifndef LAB2_TASKQUEUE_H
#define LAB2_TASKQUEUE_H

#include <queue>
#include <thread>
#include <shared_mutex>
#include <iostream>

using read_write_lock = std::shared_mutex;
using read_lock = std::shared_lock<read_write_lock>;
using write_lock = std::unique_lock<read_write_lock>;

struct Task {
    int id;
    std::function<int()> function;

    Task() : id(0), function(std::function<int()>()) {}
    Task(int id) : id(id), function([id](){
        std::printf("Started executing task #%d.\n", id);
        int seconds = rand() % 6 + 5;
        std::this_thread::sleep_for(std::chrono::seconds(seconds));
        std::printf("Finished task #%d.\n", id);
        return id;
    }) {}
};

class TaskQueue {
    using task_queue_implementation = std::vector<Task>;

public:
    inline TaskQueue(size_t size = 0) : size(size) {};
    inline ~TaskQueue() { clear(); }
    inline bool empty() const;
    size_t get_size() const;

public:
    void clear();
    bool pop(Task& task);
    bool emplace(Task& task);
    void remove(int id);
    void print();

public:
    TaskQueue(const TaskQueue& other) = delete;
    TaskQueue(TaskQueue&& other) = delete;
    TaskQueue& operator=(const TaskQueue& rhs) = delete;
    TaskQueue& operator=(TaskQueue&& rhs) = delete;

private:
    mutable read_write_lock tq_rw_lock; //task queue lock
    task_queue_implementation tasks;
    size_t size;
};

#endif //LAB2_TASKQUEUE_H
