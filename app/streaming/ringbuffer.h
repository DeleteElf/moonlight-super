#pragma once

#include <mutex>
#include <deque>

/**
 * @brief The RingBuffer class 基于线程安全的环形缓存
 */
template<typename T>
class RingBuffer {
private:
    std::deque <T> buffer;
    std::mutex lockObject;
    int capacity = 20;//默认环大小为20
public:
    ~RingBuffer() {
        buffer.clear();
    }
    /**
     * @brief RingBuffer 构造默认大小的环形缓存
     */
    RingBuffer() {
    }
    /**
     * @brief RingBuffer 构造指定大小的环形缓存
     * @param capacity 大小
     */
    RingBuffer(const int &capacity) {
        this->capacity = capacity;
    }
    /**
     * @brief RingBuffer 构造指定大小的环形缓存
     * @param capacity 大小
     */
    RingBuffer(size_t &&capacity) {
        this->capacity = capacity;
    }
    /**
     * @brief push 向环形缓存的最后位置加入数据
     * @param t 数据
     */
    void push(T &t) {
        std::lock_guard <std::mutex> lock(lockObject);
        if (buffer.size() == capacity) { //队列满了
            qDebug() << "当前环数据已满，丢弃头个元素";
            buffer.pop_front();//移除最老的数据
        }
        buffer.push_back(t);//放入最新数据到尾部
    }
    /**
     * @brief pop 从环形缓存的头部取出数据
     * @return
     */
    T& pop() {
        std::lock_guard <std::mutex> lock(lockObject);
        T &result = buffer.front();
        buffer.pop_front();
        return result;
    }

    int Count(){
        std::lock_guard <std::mutex> lock(lockObject);
        return buffer.size();
    }

    /**
     * @brief pop 清空环形缓存
     * @return
     */
    bool empty() {
        std::lock_guard <std::mutex> lock(lockObject);
        return buffer.empty();
    }

    bool clear() {
        std::lock_guard <std::mutex> lock(lockObject);
        return buffer.clear();
    }

    bool isFull() {
        std::lock_guard <std::mutex> lock(lockObject);
        return buffer.size() == capacity;
    }

    bool isEmpty() {
        std::lock_guard <std::mutex> lock(lockObject);
        return buffer.size() == 0;
    }

    T& first() {
        std::lock_guard <std::mutex> lock(lockObject);
        return buffer.front();
    }

    T& last() {
        std::lock_guard <std::mutex> lock(lockObject);
        return buffer.back();
    }

    T& tryLast() {
        std::lock_guard <std::mutex> lock(lockObject);
        if(buffer.size() == 0){
            static T t;
            return t;
        }
        return buffer.back();
    }

};

