#pragma once 

#include <vector>
#include <memory>
#include <mutex>

namespace mini_reids{

template<typename T>
class ObjectPool{
public:

    struct Deleter {
        ObjectPool* pool;
        void operator()(T *obj){
            if(pool){
                pool->release(obj);
            }else{
                delete obj;
            }
        }
    };


    // 使用std::function自定义删除器
    using Ptr=std::unique_ptr<T,Deleter>;
    
    explicit ObjectPool(size_t initial_size=16,size_t maxSize=1024):_maxSize(maxSize),_initial_size(initial_size){
        if(_initial_size>_maxSize)_initial_size=_maxSize;
        _pool.reserve(_maxSize);
        // 预分配
        for(size_t i=0;i<_initial_size;i++){
            _pool.emplace_back(std::make_unique<T>());
        }   
    }

    // 从池中获取一个对象，池为空时默认构造一个新对象
    Ptr acquire(){
        T* raw=nullptr;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if(!_pool.empty()){
                raw=_pool.back().release();
                _pool.pop_back();
            }
        }

        if(!raw){
            raw=new T();
        }
       
        //删除器自定义为调用对象池的release方法回收对象
        return Ptr(raw,Deleter{this});
    }

    // 从池中获取并重置一个对象
    template<typename ResetFunction>
    Ptr acquireAndReset(ResetFunction&& resetFunction){
        auto ptr=acquire();
        // 从std::unique_ptr从取出原始指针传递给重置函数
        resetFunction(*ptr);
        return ptr;
    }

    // 当前池中空闲对象数量
    size_t size() const{
        std::lock_guard<std::mutex> lock(_mutex);
        return _pool.size();
    }

    size_t maxSize() const{
        return _maxSize;
    }


private:
    void release(T* obj){
        if(obj==nullptr)return;
        std::lock_guard<std::mutex> lock(_mutex);
        if(_pool.size()<_maxSize){
            _pool.emplace_back(std::unique_ptr<T>(obj));
        }else{
            delete obj; // 超过阈值直接销毁
        }
    }

    std::vector<std::unique_ptr<T>> _pool;
    mutable std::mutex _mutex;
    size_t _maxSize;
    size_t _initial_size;
};

}