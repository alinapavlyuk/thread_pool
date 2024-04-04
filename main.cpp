#include "ThreadPool.h"
#include <iostream>

const size_t EXECUTORS_NUM = 6;
const size_t TASK_ADDERS_NUM = 1;

void create_add_task(ThreadPool& thread_pool, int id) {
    for(int i = 1; i <= 20; i++) {
        Task new_task{id * 100000 + i};
        thread_pool.add_task(new_task);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
};

void test(int milliseconds) {
    ThreadPool thread_pool;
    thread_pool.initialize(EXECUTORS_NUM);

    std::vector<std::thread> threads;
    threads.reserve(TASK_ADDERS_NUM);
    for (int i = 0; i < TASK_ADDERS_NUM; i++) {
        threads[i] = std::thread(create_add_task, std::ref(thread_pool), i + 1);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds/4));

    std::printf("\nThread pool is paused.\n\n");
    thread_pool.pause();

    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds/4));

    std::printf("\nThread pool is unpaused.\n\n");
    thread_pool.unpause();

    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds/4));

    thread_pool.explain_status(30005);

    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds/4));

    std::printf("\nUrgent termination started.\n\n");
    thread_pool.urgent_terminate();
    std::printf("\nUrgent termination finished.\n\n");

    for (int i = 0; i < TASK_ADDERS_NUM; i++) {
        threads[i].join();
    }
}

int main() {
    srand(time(nullptr));

    test(20000);
    return 0;
}