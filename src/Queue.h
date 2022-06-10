#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

template <class T>
class Queue {
    std::queue<T> queue;
    
    std::mutex mutex;
    std::condition_variable cv;

public:
    template <typename T1>
    void push(T1 &&value) {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push(std::forward<T1>(value));
        cv.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(mutex);
        while (queue.empty()) cv.wait(lock);
        T value = std::move(queue.front());
        queue.pop();
        return value;
    }
};
