#ifndef QUEUETYPE_H_
#define QUEUETYPE_H_

#include <iostream>
using namespace std;

// ============================================================
//  queueType  (CIRCULAR array, no shifting)
//  front = index of first element, rear = index of last element.
//  Empty vs full is told apart with the 'count' variable.
//  If canGrow is true, the array doubles and keeps the order.
// ============================================================
template <class Type>
class queueType
{
private:
    Type* data;
    int cap, count, frontIdx, rearIdx;
    bool canGrow;
    long addCount, delCount;
    int maxSize;

    void grow()
    {
        int newCap = cap * 2;
        Type* nd = new Type[newCap];
        for (int i = 0; i < count; i++)
            nd[i] = data[(frontIdx + i) % cap];       // unwrap in correct order
        delete[] data;
        data = nd;
        cap = newCap;
        frontIdx = 0;
        rearIdx = count - 1;
    }

    void growTo(int needed)
    {
        int newCap = cap;
        while (newCap < needed) newCap *= 2;
        if (newCap == cap) return;
        Type* nd = new Type[newCap];
        for (int i = 0; i < count; i++)
            nd[i] = data[(frontIdx + i) % cap];
        delete[] data;
        data = nd;
        cap = newCap;
        frontIdx = 0;
        rearIdx = (count > 0) ? count - 1 : newCap - 1;
    }

    void copyFrom(const queueType& o)
    {
        cap = o.cap; count = o.count; canGrow = o.canGrow;
        addCount = o.addCount; delCount = o.delCount; maxSize = o.maxSize;
        data = new Type[cap];
        for (int i = 0; i < count; i++)
            data[i] = o.data[(o.frontIdx + i) % o.cap];
        frontIdx = 0;
        rearIdx = (count > 0) ? count - 1 : cap - 1;
    }

public:
    queueType(int queueSize = 100, bool growable = false)
    {
        cap = (queueSize > 0) ? queueSize : 100;
        data = new Type[cap];
        canGrow = growable;
        addCount = delCount = 0;
        maxSize = 0;
        initializeQueue();
    }
    queueType(const queueType& o) { copyFrom(o); }
    queueType& operator=(const queueType& o)
    {
        if (this != &o) { delete[] data; copyFrom(o); }
        return *this;
    }
    ~queueType() { delete[] data; }

    void initializeQueue() { count = 0; frontIdx = 0; rearIdx = cap - 1; }
    bool isEmptyQueue() const { return count == 0; }
    bool isFullQueue() const { return !canGrow && count == cap; }

    Type front() const
    {
        if (count == 0) { cout << "[queue] front() on empty queue\n"; return Type(); }
        return data[frontIdx];
    }
    Type back() const
    {
        if (count == 0) { cout << "[queue] back() on empty queue\n"; return Type(); }
        return data[rearIdx];
    }
    void addQueue(const Type& e)
    {
        if (count == cap)
        {
            if (canGrow) grow();
            else { cout << "[queue] FULL - element rejected\n"; return; }
        }
        rearIdx = (rearIdx + 1) % cap;
        data[rearIdx] = e;
        count++;
        addCount++;
        if (count > maxSize) maxSize = count;
    }
    void deleteQueue()
    {
        if (count == 0) { cout << "[queue] deleteQueue() on empty queue - ignored\n"; return; }
        frontIdx = (frontIdx + 1) % cap;
        count--;
        delCount++;
    }
    int size() const { return count; }
    int capacity() const { return cap; }

    // --- Phase 2 batch operations ---
    // Adds n items in one operation: one capacity check, one possible growth,
    // then copies across the circular boundary efficiently.
    void addQueueBatch(const Type items[], int n)
    {
        if (n <= 0) return;
        int needed = count + n;
        if (needed > cap)
        {
            if (canGrow) growTo(needed);
            else
            {
                // Add as many as fit
                int avail = cap - count;
                if (avail <= 0) { cout << "[queue] FULL - batch rejected\n"; return; }
                n = avail;
            }
        }
        // Copy in two segments if wrapping around
        for (int i = 0; i < n; i++)
        {
            rearIdx = (rearIdx + 1) % cap;
            data[rearIdx] = items[i];
        }
        count += n;
        addCount += n;
        if (count > maxSize) maxSize = count;
    }

    // Deletes n items from the front in one operation.
    void deleteQueueBatch(int n)
    {
        if (n <= 0) return;
        if (n > count) n = count;
        frontIdx = (frontIdx + n) % cap;
        count -= n;
        delCount += n;
    }

    // --- extras ---
    Type& peekAt(int i) { return data[(frontIdx + i) % cap]; }
    const Type& peekAt(int i) const { return data[(frontIdx + i) % cap]; }
    long getAddCount() const { return addCount; }
    long getDelCount() const { return delCount; }
    int getMaxSize() const { return maxSize; }
};

#endif
