#pragma once

#include <queue>
#include <mutex>

template<class _Ty, size_t MaxSize>
class ConcurrentQueue : public std::queue<_Ty>
{
    //friend class std::queue<_Ty>;
private:
    std::mutex lockObject;
    // std::vector<_Ty> data2;
    // std::queue<_Ty> data;
public:
    ConcurrentQueue():std::queue<_Ty>(){

    }
    ConcurrentQueue(const size_t& _Cont):std::queue<_Ty>(_Cont){

    }
    ConcurrentQueue(size_t&& _Cont):std::queue<_Ty>(_Cont){
    }

    void push_safe(const _Ty& t){
        std::lock_guard<std::mutex> lock(lockObject);
        std::queue<_Ty>::push(t);
    }
    void push_safe(_Ty&& t){
        std::lock_guard<std::mutex> lock(lockObject);
        std::queue<_Ty>::push(t);
    }
    _Ty& pop_safe(){
        std::lock_guard<std::mutex> lock(lockObject);
        auto result=std::queue<_Ty>::front();
        std::queue<_Ty>::pop();
        return result;
    }

    bool empty_safe(){
        std::lock_guard<std::mutex> lock(lockObject);
        return std::queue<_Ty>::empty();
    }
};

