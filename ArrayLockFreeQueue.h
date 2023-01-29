#ifndef _ARRAYLOCKFREEQUEUE_H___
#define _ARRAYLOCKFREEQUEUE_H___

#include <stdint.h>

#define QUEUE_INT unsigned long
#define ARRAY_LOCK_FREE_Q_DEFAULT_SIZE 65535 

//循环数组实现的无锁队列
template <typename ELEM_T, QUEUE_INT Q_SIZE = ARRAY_LOCK_FREE_Q_DEFAULT_SIZE>
class ArrayLockFreeQueue
{
public:

	ArrayLockFreeQueue();
	virtual ~ArrayLockFreeQueue();

	QUEUE_INT size();

	bool enqueue(const ELEM_T &a_data);

	bool dequeue(ELEM_T &a_data);

    bool try_dequeue(ELEM_T &a_data);

private:
	inline QUEUE_INT countToIndex(QUEUE_INT a_count);

	ELEM_T m_thequeue[Q_SIZE];          //存储元素的数组

	volatile QUEUE_INT m_count;         //当前数组中存储的元素个数
	volatile QUEUE_INT m_writeIndex;    //当前可以写入元素的下标 

	volatile QUEUE_INT m_readIndex;     //第一个可以读取的元素

	volatile QUEUE_INT m_maximumReadIndex;  //一般情况下是和m_writeIndex相同的

};

#endif