#ifndef _YQUEUE_H_
#define _YQUEUE_H_

#include"atomic_ptr.hpp"

#include<unistd.h>
#include<stdlib.h>

//高效的队列结构
//T代表存储的元素类型，N代表每一个chunk中包含多少个元素
template<class T,int N>
class yqueue_t{
public:
    inline yqueue_t(){
        begin_chunk = allocate_chunk();
        begin_pos = 0;
        //由于当前没有元素，所以就将back_chunk的值设置为空
        back_chunk = NULL;
        back_pos = 0;
        end_chunk = begin_chunk;
        end_pos = 0;
    }

    inline ~yqueue_t(){
        while(true){
            //如果只剩下一个chunk
            if(begin_chunk == end_chunk){
                free(begin_chunk);
                break;
            }
            chunk_t* o = begin_chunk;
            begin_chunk = begin_chunk->next;
            free(o);
        }
        chunk_t* sc = spare_chunk.xchg(NULL);
        free(sc);
    }

    inline T& front(){
        return begin_chunk->values[begin_pos];
    }
    inline T& back(){
        return back_chunk->values[back_pos];
    }

    inline void push(){
        back_chunk = end_chunk;
        back_pos = end_pos;

        if(++end_pos != N){
            return ;
        }
        //如果最后一个chunk存储满了的话，那么就需要分配新的chunk
        chunk_t* sc = spare_chunk.xchg(NULL);
        if(sc){
            end_chunk->next = sc;
            sc->prev = end_chunk;
        }else{
            end_chunk->next = allocate_chunk();
            end_chunk->next->prev = end_chunk;
        }
        end_chunk = end_chunk->next;
        end_pos = 0;
    }
    //回滚操作
    inline void unpush(){
        if(back_pos){
            --back_pos;
        }else{
            back_pos = N - 1;
            back_chunk = back_chunk->prev;
        }
        if(end_pos){
            --end_pos;
        }else{
            end_pos = N - 1;
            end_chunk = end_chunk->prev;
            free(end_chunk->next);
            end_chunk->next = NULL;
        }
    }
    
    inline void pop(){
        if(++begin_pos == N){
            chunk_t* o = begin_chunk;
            begin_chunk = begin_chunk->next;

            begin_chunk->prev = NULL;
            begin_pos = 0;

            chunk_t* sc = spare_chunk.xchg(o);
            free(sc);
        }
    }
private:
    struct chunk_t{
        T values[N];
        chunk_t* prev;
        chunk_t* next;
    };
    static inline chunk_t* allocate_chunk(){
        return static_cast<chunk_t*> (malloc(sizeof(chunk_t)));
    }

    //成员变量
    chunk_t* begin_chunk; //指向第一个元素所在的chunk
    int begin_pos;      //第一个元素在chunk中下标

    chunk_t* back_chunk;  //指向最后一个元素所在的chunk
    int back_pos;         //最后一个元素在chunk中的位置

    chunk_t* end_chunk;   //指向队列中最后一个chunk
    int end_pos;

    atomic_ptr_t<chunk_t> spare_chunk;
};

#endif
