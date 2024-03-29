#include <assert.h>

#include "ArrayLockFreeQueue.h"
#include "atom_opt.h"

template <typename ELEM_T, QUEUE_INT Q_SIZE>
ArrayLockFreeQueue<ELEM_T, Q_SIZE>::ArrayLockFreeQueue() :
	m_writeIndex(0),
	m_readIndex(0),
	m_maximumReadIndex(0)
{
	m_count = 0;
}

template <typename ELEM_T, QUEUE_INT Q_SIZE>
ArrayLockFreeQueue<ELEM_T, Q_SIZE>::~ArrayLockFreeQueue()
{

}

template <typename ELEM_T, QUEUE_INT Q_SIZE>
inline QUEUE_INT ArrayLockFreeQueue<ELEM_T, Q_SIZE>::countToIndex(QUEUE_INT a_count)
{
	return (a_count % Q_SIZE);
}

template <typename ELEM_T, QUEUE_INT Q_SIZE>
QUEUE_INT ArrayLockFreeQueue<ELEM_T, Q_SIZE>::size()
{
	QUEUE_INT currentWriteIndex = m_writeIndex;
	QUEUE_INT currentReadIndex = m_readIndex;

	if(currentWriteIndex >= currentReadIndex)
		return currentWriteIndex - currentReadIndex;
	else
		return Q_SIZE + currentWriteIndex - currentReadIndex;

}

template <typename ELEM_T, QUEUE_INT Q_SIZE>
bool ArrayLockFreeQueue<ELEM_T, Q_SIZE>::enqueue(const ELEM_T &a_data)
{
	QUEUE_INT currentWriteIndex;		// 获取写指针的位置
	QUEUE_INT currentReadIndex;
	do
	{
		currentWriteIndex = m_writeIndex;
		currentReadIndex = m_readIndex;
		if(countToIndex(currentWriteIndex + 1) ==
			countToIndex(currentReadIndex))
		{
			return false;	//队列已满
		}
	} while(!CAS(&m_writeIndex, currentWriteIndex, (currentWriteIndex+1)));
	// We know now that this index is reserved for us. Use it to save the data
	m_thequeue[countToIndex(currentWriteIndex)] = a_data;

 	// update the maximum read index after saving the data. It wouldn't fail if there is only one thread 
	// inserting in the queue. It might fail if there are more than 1 producer threads because this
	// operation has to be done in the same order as the previous CAS
	while(!CAS(&m_maximumReadIndex, currentWriteIndex, (currentWriteIndex + 1)))
	{
		 // this is a good place to yield the thread in case there are more
		// software threads than hardware processors and you have more
		// than 1 producer thread
		// have a look at sched_yield (POSIX.1b)
		sched_yield();		// 当线程超过cpu核数的时候如果不让出cpu导致一直循环在这里
	}

	AtomicAdd(&m_count, 1);

	return true;

}

template <typename ELEM_T, QUEUE_INT Q_SIZE>
bool ArrayLockFreeQueue<ELEM_T, Q_SIZE>::try_dequeue(ELEM_T &a_data)
{
    return dequeue(a_data);
}

template <typename ELEM_T, QUEUE_INT Q_SIZE>
bool ArrayLockFreeQueue<ELEM_T, Q_SIZE>::dequeue(ELEM_T &a_data)
{
	QUEUE_INT currentMaximumReadIndex;
	QUEUE_INT currentReadIndex;

	do
	{
		 // to ensure thread-safety when there is more than 1 producer thread
       	// a second index is defined (m_maximumReadIndex)
		currentReadIndex = m_readIndex;
		currentMaximumReadIndex = m_maximumReadIndex;

		if(countToIndex(currentReadIndex) ==
			countToIndex(currentMaximumReadIndex))
		{
			// the queue is empty or
			// a producer thread has allocate space in the queue but is 
			// waiting to commit the data into it
			return false;
		}
		// retrieve the data from the queue
		a_data = m_thequeue[countToIndex(currentReadIndex)];

		// try to perfrom now the CAS operation on the read index. If we succeed
		// a_data already contains what m_readIndex pointed to before we 
		// increased it
		if(CAS(&m_readIndex, currentReadIndex, (currentReadIndex + 1)))
		{
			AtomicSub(&m_count, 1);	// 真正读取到了数据
			return true;
		}
	} while(true);

	assert(0);
	 // Add this return statement to avoid compiler warnings
	return false;

}