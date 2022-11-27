#include<iostream>
#include<pthread.h>
#include<sys/time.h>
#include<stdio.h>

#include"ypipe.hpp"

static ypipe_t<int,10000> ypipe;
static int s_queue_item_num = 2000000;
static int s_count_push = 0;
static int s_count_pop = 0;

//主要用做同步
pthread_mutex_t mutex;
pthread_cond_t cond;

//生产者线程
void* ypipe_producer_thread_cond(void* arg){
    int count = 0;
    int item_num = s_queue_item_num / 10;
    for(int i = 0;i < item_num;){
        //批量写入，依次写10个数据
        ypipe.write(count,true);
        count = __sync_fetch_and_add(&s_count_push,1) + 1;
        ypipe.write(count,true);
        count = __sync_fetch_and_add(&s_count_push,1) + 1;
        ypipe.write(count,true);
        count = __sync_fetch_and_add(&s_count_push,1) + 1;
        ypipe.write(count,true);
        count = __sync_fetch_and_add(&s_count_push,1) + 1;
        ypipe.write(count,true);
        count = __sync_fetch_and_add(&s_count_push,1) + 1;
        ypipe.write(count,true);
        count = __sync_fetch_and_add(&s_count_push,1) + 1;
        ypipe.write(count,true);
        count = __sync_fetch_and_add(&s_count_push,1) + 1;
        ypipe.write(count,true);
        count = __sync_fetch_and_add(&s_count_push,1) + 1;
        ypipe.write(count,true);
        count = __sync_fetch_and_add(&s_count_push,1) + 1;
        ypipe.write(count,false); //更新f_
        count = __sync_fetch_and_add(&s_count_push,1) + 1;
        i++;
        if(!ypipe.flush()){
            //代表读线程陷入睡眠，于是这里会将它给唤醒
            pthread_mutex_lock(&mutex);
            pthread_cond_signal(&cond);
            pthread_mutex_unlock(&mutex);
        }
    }
    return NULL;
}

//消费者线程
void* ypipe_consumer_thread_cond(void* arg){
    int last_value = 0;
    while(true){
        int value = 0;
        if(ypipe.read(&value)){
            __sync_fetch_and_add(&s_count_pop,1);
            last_value = value;
        }else{
            //读取数据失败
            //表示当前队列中没有元素可以读了
            pthread_mutex_lock(&mutex);
            pthread_cond_wait(&cond,&mutex);
            pthread_mutex_unlock(&mutex);
        }
        if(s_count_pop >= s_queue_item_num){
            break;
        }
    }
    return NULL;
}

int main(int argc,char** argv){
    pthread_t tid_push;
    pthread_t tid_pop;
    pthread_mutex_init(&mutex,NULL);
    pthread_cond_init(&cond,NULL);
    struct timeval tv;
    gettimeofday(&tv,NULL);
    int start_sec = tv.tv_sec;
    int start_usec = tv.tv_usec;

    int ret = pthread_create(&tid_push,NULL,ypipe_producer_thread_cond,NULL);
    if(0 != ret){
        printf("create producer thread failed\n");
        exit(0);
    }
    
    ret = pthread_create(&tid_pop,NULL,ypipe_consumer_thread_cond,NULL);
    if(0 != ret){
        printf("create consumer thread failed\n");
        exit(0);
    }

    pthread_join(tid_push,NULL);
    pthread_join(tid_pop,NULL);

    gettimeofday(&tv,NULL);
    int end_sec = tv.tv_sec;
    int end_usec = tv.tv_usec;
    
    double time_diff = (end_sec - start_sec) * 1000000 + (end_usec - start_usec);
    printf("spend time:%f ms,push:%ld, pop:%ld\n",time_diff / 1000,s_count_push,s_count_pop);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    return 0;
}
