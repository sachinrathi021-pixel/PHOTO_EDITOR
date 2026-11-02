#ifndef PRIORITYQUEUETYPE_H_
#define PRIORITYQUEUETYPE_H_

#include <iostream>
using namespace std;

// ============================================================
//  priorityQueueType  (ORDERED dynamic array)
//  Elements are kept sorted by priority (ascending) so that
//  front() and deleteQueue() are O(1).  addQueue() is O(n)
//  because it shifts elements to maintain sorted order.
//  Ties in priority are broken by arrival order (FIFO):
//  a new element with the same priority is placed AFTER all
//  existing elements of that priority.
//  A smaller priority number = higher priority.
// ============================================================
template <class Type>
class priorityQueueType
{
private:
    Type* data;
    int cap;
    int count;
    long addCnt, delCnt;
    int maxSize;

    void grow()
    {
        int newCap = cap * 2;
        Type* nd = new Type[newCap];
        for (int i = 0; i < count; i++) nd[i] = data[i];
        delete[] data;
        data = nd;
        cap = newCap;
    }

    void copyFrom(const priorityQueueType& o)
    {
        cap = o.cap;
        count = o.count;
        addCnt = o.addCnt;
        delCnt = o.delCnt;
        maxSize = o.maxSize;
        data = new Type[cap];
        for (int i = 0; i < count; i++) data[i] = o.data[i];
    }

public:
    priorityQueueType(int queueSize = 100)
    {
        cap = (queueSize > 0) ? queueSize : 100;
        data = new Type[cap];
        count = 0;
        addCnt = delCnt = 0;
        maxSize = 0;
    }
    priorityQueueType(const priorityQueueType& o) { copyFrom(o); }
    priorityQueueType& operator=(const priorityQueueType& o)
    {
        if (this != &o) { delete[] data; copyFrom(o); }
        return *this;
    }
    ~priorityQueueType() { delete[] data; }

    void initializeQueue() { count = 0; }
    bool isEmptyQueue() const { return count == 0; }
    bool isFullQueue() const { return false; }  // grows dynamically

    // Insert in sorted order.  The Type must have a .priority member (int)
    // and a .seqNum member (int) for tie-breaking.
    // data[0] = highest priority (smallest number), data[count-1] = lowest.
    void addQueue(const Type& e)
    {
        if (count == cap) grow();
        // Find insertion position: place AFTER all elements with <= priority
        // This preserves FIFO order among elements of equal priority.
        int pos = count;
        for (int i = count - 1; i >= 0; i--)
        {
            if (data[i].priority > e.priority)
            {
                data[i + 1] = data[i];
                pos = i;
            }
            else break;
        }
        data[pos] = e;
        count++;
        addCnt++;
        if (count > maxSize) maxSize = count;
    }

    Type front() const
    {
        if (count == 0) { cout << "[pqueue] front() on empty queue\n"; return Type(); }
        return data[0];
    }

    void deleteQueue()
    {
        if (count == 0) { cout << "[pqueue] deleteQueue() on empty queue - ignored\n"; return; }
        for (int i = 1; i < count; i++) data[i - 1] = data[i];
        count--;
        delCnt++;
    }

    int size() const { return count; }
    int capacity() const { return cap; }

    const Type& peekAt(int i) const { return data[i]; }
    Type& peekAt(int i) { return data[i]; }
    long getAddCount() const { return addCnt; }
    long getDelCount() const { return delCnt; }
    int getMaxSize() const { return maxSize; }
};

#endif
