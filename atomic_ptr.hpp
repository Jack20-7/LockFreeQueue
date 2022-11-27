#ifndef _ATOMIC_PTR_H_
#define _ATOMIC_PTR_H_

#include<atomic>

//用来对共享资源进行保护,一般那些可能会被多个线程同时操作的指针就可以使用该类来对他进行管理

template<class T>
class atomic_ptr_t{
public:
    atomic_ptr_t(){
        ptr_ = NULL;
    }
    //不是线程安全的，在使用时需要用户自己保证没有其他线程同时也在操作ptr
    void set(T* ptr){
        ptr_ = ptr;
    }

    //该函数是线程安全的，它的作用就是将ptr_ 的值设置为ptr,并且将之前的那个ptr的值进行返回
    T* xchg(T* ptr){
        return ptr_.exchange(ptr,std::memory_order_acq_rel);
    }

    //该函数的就是就是比较cmp和ptr_是否相同，如果相同，就将ptr_的值设置为val，然后返回以前ptr_的值.如果不相同的话，那么就直接返回ptr_的值就ok了
    T* cas(T* cmp,T* val){
        ptr_.compare_exchange_strong(cmp,val,std::memory_order_acq_rel);
        return cmp;
    }
private:
    std::atomic<T*> ptr_;
};

#endif
