#include "ThreadPool.h"

void ThreadPool::initialize(const size_t worker_count) {
    write_lock _(tp_rw_lock);

    if (initialized || terminated) {
        return;
    }
    workers.reserve(worker_count);

    for (size_t id = 1; id <= worker_count; id++) {
        workers.emplace_back(&ThreadPool::routine, this, id);
    }
    initialized = !workers.empty();
}

void ThreadPool::terminate()
{
    {
        write_lock _(tp_rw_lock);

        if (working_unsafe()) {
            terminated = true;
        } else {
            std::printf("Already terminated.\n");
            workers.clear();
            terminated = false;
            initialized = false;
            return;
        }
    }

    task_waiter.notify_all();

    for (std::thread &worker: workers) {
        worker.join();
    }

    workers.clear();
    terminated = false;
    initialized = false;
    show_statistics();
}

void ThreadPool::urgent_terminate() {
    {
        write_lock _(tp_rw_lock);

        if (working_unsafe()) {
            terminated = true;
            tasks.clear();
        } else {
            workers.clear();
            terminated = false;
            initialized = false;
            return;
        }
    }

    task_waiter.notify_all();

    for (std::thread &worker: workers) {
        worker.join();
    }

    workers.clear();
    terminated = false;
    initialized = false;
    show_statistics();
}



void ThreadPool::routine(int id)
{
    nanoseconds total_wait{0};
    int wait_times = 0;

    while (true) {
        bool task_acquired = false;

        {
            write_lock _(tp_rw_lock);
            auto wait_condition = [this] {
                return !paused;
            };
            unpause_waiter.wait(_, wait_condition);
        }

        Task task;
        {
            write_lock _(tp_rw_lock);
            auto wait_condition = [this, &task_acquired, &task] {
                task_acquired = tasks.pop(task);

                write_lock _(stat_rw_lock);
                if(queue_full) {
                     full_end = std::chrono::high_resolution_clock::now();
                     queue_full = false;
                     compare_full_time();
                }

                return terminated || task_acquired;
            };

            auto wait_begin =std::chrono::high_resolution_clock::now();
            task_waiter.wait(_, wait_condition);
            auto wait_end = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<nanoseconds>(wait_end - wait_begin);
            total_wait += elapsed;
            wait_times++;
        }

        if (terminated && !task_acquired) {
            auto mean_wait = static_cast<double>(total_wait.count())/wait_times;
            wait_time_statistics[id] = mean_wait * 1e-9;
            return;
        }

        {
            write_lock _(stat_rw_lock);
            results[task.id] = -1;
        }

        int result = task.function();

        {
            write_lock _(stat_rw_lock);
            results[task.id] = result;
            finished_tasks_statistics[id]++;
        }
    }
}

void ThreadPool::pause() {
    write_lock _(tp_rw_lock);
    paused = true;
}

void ThreadPool::unpause() {
    write_lock _(tp_rw_lock);
    paused = false;
    unpause_waiter.notify_all();
}

bool ThreadPool::working() const {
    read_lock _(tp_rw_lock);
    return working_unsafe();
}

bool ThreadPool::working_unsafe() const {
    return initialized && !terminated;
}

void ThreadPool::print_queue() {
    tasks.print();
}

int ThreadPool::get_result(int id) {
    read_lock _(stat_rw_lock);
    return results[id];
}

int ThreadPool::get_status(int id) {
    read_lock _(stat_rw_lock);
    int result = results[id];
    if (result == 0) {
        return 1;
    } else if (result > 0) {
        return 2;
    } else {
        return 3;
    }
}

void ThreadPool::explain_status(int id) {
    int status = get_status(id);
    if (status == 1) {
        std::printf("\nTask #%d has not been started/added yet.\n\n", id);
    } else if (status == 2) {
        std::printf("\nTask #%d has been successfully completed. Result is %d\n\n", id, results[id]);
    } else {
        std::printf("\nTask #%d is executing now.\n\n", id);
    }
}

void ThreadPool::compare_full_time() {
    auto elapsed = std::chrono::duration_cast<nanoseconds>(full_end - full_begin);

    if(min_full_time.count() == 0) {
        min_full_time = elapsed;
    }

    max_full_time = max(max_full_time, elapsed);
    min_full_time = min(min_full_time, elapsed);
}

void ThreadPool::show_statistics() {
    std::printf("\n");
    for(int i = 1; i <= wait_time_statistics.size(); i++) {
        std::printf("Average waiting time for %d thread is %f. Number of finished tasks: %d.\n", i, wait_time_statistics[i], finished_tasks_statistics[i]);
    }

    std::printf("\nThere were %d rejected tasks.\n\n", rejected_num);

    double min_full_time_s = static_cast<double>(min_full_time.count()) * 1e-9;
    double max_full_time_s = static_cast<double>(max_full_time.count()) * 1e-9;
    std::printf("The minimal time of queue being full is %f seconds.\n", min_full_time_s);
    std::printf("The maximum time of queue being full is %f seconds.\n", max_full_time_s);
}

void ThreadPool::add_task(Task& task) {
    if (!working()) {
        return;
    }

    if(!tasks.emplace(task)){
        rejected_num++;
    };

    {
        write_lock _(stat_rw_lock);
        if (tasks.get_size() == QUEUE_SIZE && !queue_full) {
            full_begin = std::chrono::high_resolution_clock::now();
            queue_full = true;
        }
    }

    task_waiter.notify_one();
}

void ThreadPool::delete_task(int id) {
    if (!working()) {
        return;
    }

    tasks.remove(id);
}