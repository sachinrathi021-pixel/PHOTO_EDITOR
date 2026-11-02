#ifndef STACKTYPE_H_
#define STACKTYPE_H_

#include <iostream>
#include <cstring>
using namespace std;

// ============================================================
//  stackType  (growing dynamic array)
//  push/pop/top are O(1) amortized; the array doubles when full.
// ============================================================
template <class Type>
class stackType
{
private:
    Type* data;
    int count;      // number of elements (also index of next free slot)
    int cap;        // allocated size
    long pushCount, popCount;
    int maxSize;    // largest size ever reached

    void grow()
    {
        int newCap = cap * 2;
        Type* nd = new Type[newCap];
        for (int i = 0; i < count; i++) nd[i] = data[i];
        delete[] data;
        data = nd;
        cap = newCap;
    }

    void growTo(int needed)
    {
        int newCap = cap;
        while (newCap < needed) newCap *= 2;
        if (newCap == cap) return;
        Type* nd = new Type[newCap];
        for (int i = 0; i < count; i++) nd[i] = data[i];
        delete[] data;
        data = nd;
        cap = newCap;
    }

    void copyFrom(const stackType& o)
    {
        cap = o.cap;
        count = o.count;
        pushCount = o.pushCount;
        popCount = o.popCount;
        maxSize = o.maxSize;
        data = new Type[cap];
        for (int i = 0; i < count; i++) data[i] = o.data[i];
    }

public:
    stackType(int stackSize = 100)
    {
        cap = (stackSize > 0) ? stackSize : 100;
        data = new Type[cap];
        count = 0;
        pushCount = popCount = 0;
        maxSize = 0;
    }
    stackType(const stackType& o) { copyFrom(o); }
    stackType& operator=(const stackType& o)
    {
        if (this != &o) { delete[] data; copyFrom(o); }
        return *this;
    }
    ~stackType() { delete[] data; }

    void initializeStack() { count = 0; }
    bool isEmptyStack() const { return count == 0; }
    bool isFullStack() const { return false; }     // grows, so never full

    void push(const Type& newItem)
    {
        if (count == cap) grow();
        data[count++] = newItem;
        pushCount++;
        if (count > maxSize) maxSize = count;
    }
    Type top() const
    {
        if (count == 0)
        {
            cout << "[stack] top() on empty stack - returning default value\n";
            return Type();
        }
        return data[count - 1];
    }
    void pop()
    {
        if (count == 0) { cout << "[stack] pop() on empty stack - ignored\n"; return; }
        count--;
        popCount++;
    }
    int size() const { return count; }

    // --- Phase 2 batch operations ---
    // Pushes n items in one operation: one capacity check, one possible growth, block copy.
    void pushBatch(const Type items[], int n)
    {
        if (n <= 0) return;
        int needed = count + n;
        if (needed > cap) growTo(needed);
        for (int i = 0; i < n; i++) data[count + i] = items[i];
        count += n;
        pushCount += n;
        if (count > maxSize) maxSize = count;
    }

    // Pops n items in one operation.
    void popBatch(int n)
    {
        if (n <= 0) return;
        if (n > count) n = count;
        count -= n;
        popCount += n;
    }

    // Phase 2: drop the oldest record (bottom of stack)
    void removeBottom()
    {
        if (count == 0) return;
        for (int i = 1; i < count; i++) data[i - 1] = data[i];   // O(n) shift
        count--;
    }

    // --- extras (Inspector / traversal) ---
    const Type& at(int i) const { return data[i]; }   // i = 0 is the BOTTOM
    Type& at(int i) { return data[i]; }
    long getPushCount() const { return pushCount; }
    long getPopCount() const { return popCount; }
    int getMaxSize() const { return maxSize; }
};

#endif
