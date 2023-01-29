#ifndef _YPIPE_H_
#define _YPIPE_H_

#include"atomic_ptr.hpp"
#include"yqueue.hpp"

template<class T>
class ypipe_base_t{
public:
    virtual ~ypipe_base_t() {};
    virtual void write(const T& value,bool incomplete) = 0;
    virtual bool unwrite(T* value) = 0;
    virtual bool flush() = 0;
    virtual bool check_read() = 0;
    virtual bool read(T* value) = 0;
};


template<class T,int N>
class ypipe_t : public ypipe_base_t<T>{
public:
    ypipe_t(){
        //yqueue在初始化之后，确实需要先调用push来对back_chunk 和 back_pos来进行初始化
        queue_.push();

        r_ = w_ = f_ = &queue_.back();
        c_.set(&queue_.back());
    }
    
    //incomplele = true表示当前写入的是一个数据分段，并不是完整的数据
    //只有当incomplete = false的时候，才表示写入了完成的数据,然后对f_的值进行更新.这样从w_ -> f_ 这一段之间的数据就代表一段完整的数据
    inline void write(const T& value,bool incomplete){
        queue_.back() = value;
        queue_.push();//更新back_pos 和 end_pos

        if(!incomplete){
            f_ = &queue_.back();
        }
    }

    //返回的就是回退的元素
    bool unwrite(T* value){
        //表示当前已经刷新过了，不能够在回退了
        if(f_ == &queue_.back()){
            return false;
        }
        queue_.unpush();
        *value = queue_.back();
        return true;
    }

    //用来刷新数据
    //这里是通过将c_的值设置为 f_来通知读线程，也就是让读线程看到本次写入的数据
    bool flush(){
      if(w_ == f_){
          //如果相同就代表当前根本没有新的数据可以操作
          return true;
      }
      //执行到这里就表示写入有新数据
      
      if(c_.cas(w_,f_) != w_){
         //在写线程中,c_ 的值 和w_的值一般是相同的.只有一种请求会导致c_的值和w_的值不相同，那就是读线程没有数据可以读，这个时候读线程就会将c_的值 = NULL，然后陷入睡眠
         //所以如果执行到这里，就表示读线程陷入了睡眠
         c_.set(f_);
         w_ = f_;
         return false;  //这里return false是用来唤醒读线程的
      }
      w_ = f_;
      return true;
    }

    //该函数的职责就是检查是否有数据可以读 并且 进行预读
    //判断是否有数据可以读就是通过判断r_ 和 front() 来进行判断的
    bool check_read(){
        //从[&queue_.front(),r_]  这一段的元素就是可以进行预读的元素
        //随着不断的读出(pop)数据，front最后一定等于 r_.表示暂时没有数据可以读，下面就需要进预读
        if(&queue_.front() != r_ && r_){
            return true;
        }
        //接下来进行进行预读操作
        //要进行的预读操作其实就是将r_ 的值设置为 c_.并且一步会判断如果c_ = &queue_.front()的话，那么就代表队列中没有数据可以进行读取的，于是会将c_的值设置为NULL,r 设置为front
        //因为每一次write false的时候，会对f_进行更新,之后调用flush的话，会将 c_ 、w_ 和 f_的值都更新.这样 c_ 就不等于 front.
        r_ = c_.cas(&queue_.front(),NULL);

        if(r_ == &queue_.front() || !r_){
            return false;
        }
        return true;
    }

    //读出队列中的第一个元素
    bool read(T* value){
        if(!check_read()){
            return false;
        }

        *value = queue_.front();
        queue_.pop();

        return true;
    }
private:
    yqueue_t<T,N> queue_;  //底层维护的队列

    //无锁队列机制的实现主要就是依靠以下的成员

    T* w_; //指向第一个未被刷新的元素，只能够被写线程使用
    T* r_; //指向第一个未被读取的元素,只能够被读线程使用. 
    T* f_; //指向下一轮要被刷新的元素中的第一个元素
    
    atomic_ptr_t<T> c_;  //读写线程共享的指针，指向每一轮刷新的起点
};


#endif
