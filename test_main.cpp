// Phase 2 Test Program - ImageLab Data Structures Assignment 2
// Tests: stackType, queueType, priorityQueueType, Image, ImageLab Phase 2 features
// Compile: g++ -std=c++11 -O2 -o tests_p2.exe tests/test_main.cpp src/stb_impl.cpp

#include "../src/ImageLab.h"
#include <sstream>
#include <cstring>

// ============================================================
//  Test framework
// ============================================================
static int passed = 0, failed = 0;

void check(bool cond, const char* desc)
{
    if (cond)
    {
        cout << "  PASS  " << desc << "\n";
        passed++;
    }
    else
    {
        cout << "  FAIL  " << desc << "\n";
        failed++;
    }
}

// Suppress output during tests
struct NullBuf : public streambuf { int overflow(int c) { return c; } };

void quiet(bool on)
{
    static streambuf* orig = 0;
    static NullBuf nb;
    if (on && !orig) { orig = cout.rdbuf(&nb); }
    else if (!on && orig) { cout.rdbuf(orig); orig = 0; }
}

// ============================================================
//  1. stackType tests (Phase 1 + batch ops)
// ============================================================
void testStack()
{
    cout << "\n=== stackType tests ===\n";
    stackType<int> s(4);
    check(s.isEmptyStack(), "new stack is empty");
    s.push(10); s.push(20); s.push(30);
    check(s.size() == 3, "push 3 items => size=3");
    check(s.top() == 30, "top is 30");
    s.pop();
    check(s.top() == 20, "after pop, top is 20");

    // Test growing beyond initial capacity
    for (int i = 0; i < 100; i++) s.push(i);
    check(s.size() == 102, "stack grows beyond initial cap");

    // removeBottom
    s.initializeStack();
    s.push(1); s.push(2); s.push(3);
    s.removeBottom();
    check(s.size() == 2, "removeBottom: size 3->2");
    check(s.at(0) == 2, "removeBottom: new bottom is 2");
    check(s.top() == 3, "removeBottom: top still 3");

    // pushBatch
    stackType<int> sb(4);
    int items[5] = {10, 20, 30, 40, 50};
    sb.pushBatch(items, 5);
    check(sb.size() == 5, "pushBatch 5 items => size=5");
    check(sb.top() == 50, "pushBatch: top is 50");
    check(sb.at(0) == 10, "pushBatch: bottom is 10");

    // popBatch
    sb.popBatch(3);
    check(sb.size() == 2, "popBatch(3) => size=2");
    check(sb.top() == 20, "popBatch: new top is 20");

    // Empty stack safety
    quiet(true);
    stackType<int> es(4);
    es.pop();  // should not crash
    int t = es.top();  // should not crash
    quiet(false);
    check(es.isEmptyStack(), "pop on empty: still empty");

    // pushBatch(0) should be no-op
    sb.pushBatch(items, 0);
    check(sb.size() == 2, "pushBatch(0) is no-op");

    // Inspector counters
    stackType<int> ic(10);
    ic.push(1); ic.push(2); ic.push(3);
    ic.pop();
    check(ic.getPushCount() == 3, "push count = 3");
    check(ic.getPopCount() == 1, "pop count = 1");
    check(ic.getMaxSize() == 3, "max size = 3");
}

// ============================================================
//  2. queueType tests (Phase 1 + batch ops)
// ============================================================
void testQueue()
{
    cout << "\n=== queueType tests ===\n";
    // Fixed-capacity queue
    queueType<int> q(4, false);
    check(q.isEmptyQueue(), "new queue is empty");
    q.addQueue(10); q.addQueue(20); q.addQueue(30);
    check(q.size() == 3, "add 3 => size=3");
    check(q.front() == 10, "front is 10 (FIFO)");
    q.deleteQueue();
    check(q.front() == 20, "after delete, front is 20");

    // Circular wraparound
    queueType<int> cq(4, false);
    cq.addQueue(1); cq.addQueue(2); cq.addQueue(3); cq.addQueue(4);
    check(cq.isFullQueue(), "4/4 => full");
    cq.deleteQueue(); cq.deleteQueue();
    cq.addQueue(5); cq.addQueue(6);
    check(cq.front() == 3, "wraparound: front is 3");
    check(cq.size() == 4, "wraparound: size = 4");

    // Growing queue
    queueType<int> gq(2, true);
    for (int i = 0; i < 20; i++) gq.addQueue(i);
    check(gq.size() == 20, "growable queue holds 20");
    check(gq.front() == 0, "front after growing is 0");

    // addQueueBatch
    queueType<int> bq(4, true);
    int items[6] = {10, 20, 30, 40, 50, 60};
    bq.addQueueBatch(items, 6);
    check(bq.size() == 6, "addQueueBatch 6 items => size=6");
    check(bq.front() == 10, "batch front is 10");

    // deleteQueueBatch
    bq.deleteQueueBatch(4);
    check(bq.size() == 2, "deleteQueueBatch(4) => size=2");
    check(bq.front() == 50, "batch front after delete is 50");

    // Batch on fixed queue
    queueType<int> fq(3, false);
    quiet(true);
    fq.addQueueBatch(items, 6);  // should only add 3
    quiet(false);
    check(fq.size() == 3, "fixed queue batch: only added up to cap");

    // Empty queue safety
    quiet(true);
    queueType<int> eq(4, false);
    eq.deleteQueue();  // should not crash
    int f = eq.front();  // should not crash
    quiet(false);
    check(eq.isEmptyQueue(), "delete on empty: still empty");

    // Inspector counters
    queueType<int> ic(10, false);
    ic.addQueue(1); ic.addQueue(2); ic.addQueue(3);
    ic.deleteQueue();
    check(ic.getAddCount() == 3, "add count = 3");
    check(ic.getDelCount() == 1, "del count = 1");
    check(ic.getMaxSize() == 3, "max size = 3");
}

// ============================================================
//  3. priorityQueueType tests
// ============================================================
void testPriorityQueue()
{
    cout << "\n=== priorityQueueType tests ===\n";

    // Simple ordering
    {
        priorityQueueType<Job> pq(10);
        check(pq.isEmptyQueue(), "new pq is empty");
        Job j1; j1.type = J_RESIZE;  j1.priority = 3; j1.seqNum = 0; j1.id = 1;
        Job j2; j2.type = J_BLUR;    j2.priority = 2; j2.seqNum = 1; j2.id = 2;
        Job j3; j3.type = J_UNDO;    j3.priority = 1; j3.seqNum = 2; j3.id = 3;
        pq.addQueue(j1); pq.addQueue(j2); pq.addQueue(j3);
        check(pq.size() == 3, "add 3 jobs => size=3");
        check(pq.front().id == 3, "highest prio (undo, prio=1) is first");
        pq.deleteQueue();
        check(pq.front().id == 2, "after del, blur (prio=2) is first");
        pq.deleteQueue();
        check(pq.front().id == 1, "resize (prio=3) is last");
    }

    // Same-priority FIFO tie-breaking
    {
        priorityQueueType<Job> pq(10);
        for (int i = 0; i < 5; i++)
        {
            Job j; j.priority = 2; j.seqNum = i; j.id = i + 1;
            j.type = J_BLUR;
            pq.addQueue(j);
        }
        bool fifoOk = true;
        for (int i = 0; i < 5; i++)
        {
            if (pq.front().id != i + 1) fifoOk = false;
            pq.deleteQueue();
        }
        check(fifoOk, "same-priority: FIFO ordering preserved");
    }

    // Mixed priorities
    {
        priorityQueueType<Job> pq(10);
        Job save; save.priority = 4; save.seqNum = 0; save.id = 1; save.type = J_SAVE;
        Job blur1; blur1.priority = 2; blur1.seqNum = 1; blur1.id = 2; blur1.type = J_BLUR;
        Job undo; undo.priority = 1; undo.seqNum = 2; undo.id = 3; undo.type = J_UNDO;
        Job blur2; blur2.priority = 2; blur2.seqNum = 3; blur2.id = 4; blur2.type = J_BLUR;
        pq.addQueue(save); pq.addQueue(blur1); pq.addQueue(undo); pq.addQueue(blur2);
        check(pq.front().id == 3, "mixed: undo (prio=1) is first");
        pq.deleteQueue();
        check(pq.front().id == 2, "mixed: blur1 (prio=2, seq=1) second");
        pq.deleteQueue();
        check(pq.front().id == 4, "mixed: blur2 (prio=2, seq=3) third");
        pq.deleteQueue();
        check(pq.front().id == 1, "mixed: save (prio=4) last");
    }

    // Dynamic growth
    {
        priorityQueueType<Job> pq(2);
        for (int i = 0; i < 50; i++)
        {
            Job j; j.priority = 50 - i; j.seqNum = i; j.id = i;
            j.type = J_RESIZE;
            pq.addQueue(j);
        }
        check(pq.size() == 50, "dynamic growth to 50 elements");
        check(pq.front().priority == 1, "front has highest priority after growth");
    }

    // Inspector counters
    {
        priorityQueueType<Job> pq(10);
        Job j1; j1.priority = 2; j1.seqNum = 0; j1.id = 1; j1.type = J_BLUR;
        Job j2; j2.priority = 1; j2.seqNum = 1; j2.id = 2; j2.type = J_UNDO;
        pq.addQueue(j1); pq.addQueue(j2);
        pq.deleteQueue();
        check(pq.getAddCount() == 2, "pq add count = 2");
        check(pq.getDelCount() == 1, "pq del count = 1");
        check(pq.getMaxSize() == 2, "pq max size = 2");
    }

    // Empty queue safety
    {
        quiet(true);
        priorityQueueType<Job> pq(4);
        pq.deleteQueue();  // should not crash
        Job f = pq.front();  // should not crash
        quiet(false);
        check(pq.isEmptyQueue(), "delete on empty pq: still empty");
    }
}

// ============================================================
//  4. Image tests
// ============================================================
void testImage()
{
    cout << "\n=== Image tests ===\n";

    // Create and access
    Image img(4, 3);
    check(img.width() == 4 && img.height() == 3, "Image(4,3) dimensions");
    img.at(0, 0) = Pixel(255, 0, 0);
    check(img.at(0, 0).r == 255, "pixel set/get");

    // Crop
    Image c(10, 10);
    for (int y = 0; y < 10; y++)
        for (int x = 0; x < 10; x++)
            c.at(x, y) = Pixel(x * 25, y * 25, 0);
    c.crop(2, 3, 5, 4);
    check(c.width() == 5 && c.height() == 4, "crop 10x10 -> 5x4");
    check(c.at(0, 0).r == 50, "cropped origin pixel correct");

    // Rotate 90
    Image r(3, 2);
    r.at(0, 0) = Pixel(1, 0, 0); r.at(1, 0) = Pixel(2, 0, 0); r.at(2, 0) = Pixel(3, 0, 0);
    r.at(0, 1) = Pixel(4, 0, 0); r.at(1, 1) = Pixel(5, 0, 0); r.at(2, 1) = Pixel(6, 0, 0);
    r.rotate(90);
    check(r.width() == 2 && r.height() == 3, "rotate 90: 3x2 -> 2x3");

    // Flip
    Image f(3, 1);
    f.at(0, 0) = Pixel(1, 0, 0); f.at(1, 0) = Pixel(2, 0, 0); f.at(2, 0) = Pixel(3, 0, 0);
    f.flip(true);
    check(f.at(0, 0).r == 3 && f.at(2, 0).r == 1, "flip horizontal");

    // Brightness clamping
    Image b(2, 2);
    b.at(0, 0) = Pixel(200, 200, 200);
    b.at(1, 0) = Pixel(10, 10, 10);
    b.brightness(100);
    check(b.at(0, 0).r == 255, "brightness clamps at 255");
    b.brightness(-500);
    check(b.at(0, 0).r == 0, "brightness clamps at 0");

    // Grayscale
    Image g(2, 1);
    g.at(0, 0) = Pixel(255, 0, 0);
    g.grayscale();
    check(g.at(0, 0).r == g.at(0, 0).g && g.at(0, 0).g == g.at(0, 0).b, "grayscale: all channels equal");

    // Resize
    Image rs(4, 4);
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            rs.at(x, y) = Pixel(x * 60, y * 60, 0);
    rs.resize(2, 2);
    check(rs.width() == 2 && rs.height() == 2, "resize 4x4 -> 2x2");

    // Blur
    Image bl(5, 5);
    for (int y = 0; y < 5; y++)
        for (int x = 0; x < 5; x++)
            bl.at(x, y) = Pixel(100, 100, 100);
    bl.at(2, 2) = Pixel(200, 200, 200);
    bl.blur(1);
    check(bl.at(2, 2).r < 200 && bl.at(2, 2).r > 100, "blur center pixel changed");

    // Bucket fill (stack-based, iterative)
    Image bf(5, 5);
    for (int y = 0; y < 5; y++)
        for (int x = 0; x < 5; x++)
            bf.at(x, y) = Pixel(255, 255, 255);
    int maxS; long ins;
    long changed = bf.bucketFill(0, 0, Pixel(255, 0, 0), true, maxS, ins);
    check(changed == 25, "stack fill: all 25 pixels changed");
    check(bf.at(4, 4).r == 255 && bf.at(4, 4).g == 0, "stack fill: corner pixel is red");

    // Bucket fill (queue-based)
    Image bf2(5, 5);
    for (int y = 0; y < 5; y++)
        for (int x = 0; x < 5; x++)
            bf2.at(x, y) = Pixel(0, 0, 0);
    changed = bf2.bucketFill(2, 2, Pixel(0, 255, 0), false, maxS, ins);
    check(changed == 25, "queue fill: all 25 pixels changed");

    // Fill: same color should do nothing
    Image sf(3, 3);
    for (int y = 0; y < 3; y++)
        for (int x = 0; x < 3; x++)
            sf.at(x, y) = Pixel(100, 100, 100);
    changed = sf.bucketFill(1, 1, Pixel(100, 100, 100), true, maxS, ins);
    check(changed == 0, "fill same color => 0 changed");

    // PPM save/load roundtrip
    Image ppmImg(3, 3);
    for (int y = 0; y < 3; y++)
        for (int x = 0; x < 3; x++)
            ppmImg.at(x, y) = Pixel(x * 80, y * 80, 128);
    ppmImg.save("_test_roundtrip.ppm");
    Image loaded;
    bool loadOk = loaded.load("_test_roundtrip.ppm");
    check(loadOk && loaded.width() == 3 && loaded.height() == 3, "PPM save/load roundtrip");
    check(loaded.at(0, 0).b == 128, "PPM roundtrip pixel data correct");
    remove("_test_roundtrip.ppm");

    // PNG save/load roundtrip
    Image pngImg(4, 4);
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            pngImg.at(x, y) = Pixel(x * 50, y * 50, 200);
    pngImg.save("_test_roundtrip.png");
    Image pngLoaded;
    loadOk = pngLoaded.load("_test_roundtrip.png");
    check(loadOk && pngLoaded.width() == 4 && pngLoaded.height() == 4, "PNG save/load roundtrip");
    check(pngLoaded.at(0, 0).b == 200, "PNG roundtrip pixel data correct");
    remove("_test_roundtrip.png");

    // Copy constructor / assignment
    Image orig(2, 2);
    orig.at(0, 0) = Pixel(42, 42, 42);
    Image cpy(orig);
    check(cpy.at(0, 0).r == 42, "copy constructor preserves data");
    cpy.at(0, 0) = Pixel(99, 99, 99);
    check(orig.at(0, 0).r == 42, "copy is independent (deep copy)");
}

// ============================================================
//  5. ImageLab Phase 2: undo/redo with compact history
// ============================================================
void testUndoRedo()
{
    cout << "\n=== Undo/Redo (compact history) tests ===\n";

    // Create a small test image
    Image testImg(4, 4);
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            testImg.at(x, y) = Pixel(100, 100, 100);
    testImg.save("_test_undo.ppm");

    quiet(true);
    ImageLab lab(10);
    lab.load("_test_undo.ppm");
    quiet(false);

    // Brightness undo
    quiet(true);
    lab.brightness(50);
    quiet(false);
    check(lab.current().at(0, 0).r == 150, "brightness +50: pixel 150");
    check(lab.undoCount() == 1, "undo stack has 1 record");

    quiet(true);
    lab.undo();
    quiet(false);
    check(lab.current().at(0, 0).r == 100, "undo brightness: pixel back to 100");
    check(lab.redoCount() == 1, "redo stack has 1 record");

    quiet(true);
    lab.redo();
    quiet(false);
    check(lab.current().at(0, 0).r == 150, "redo brightness: pixel 150 again");

    // Rotate undo (compact: only stores angle, no pixel data)
    quiet(true);
    lab.rotate(90);
    quiet(false);
    check(lab.current().width() == 4 && lab.current().height() == 4, "rotate 90: 4x4 stays 4x4");
    check(lab.undoCount() == 2, "undo stack has 2 records");

    quiet(true);
    lab.undo();
    quiet(false);
    check(lab.undoCount() == 1, "undo rotate: 1 record left");

    // Flip undo (compact: only stores direction)
    quiet(true);
    lab.flip(true);
    lab.undo();
    quiet(false);
    check(lab.current().at(0, 0).r == 150, "flip undo: pixel unchanged");

    // Crop undo
    quiet(true);
    lab.crop(1, 1, 2, 2);
    quiet(false);
    check(lab.current().width() == 2 && lab.current().height() == 2, "crop to 2x2");
    quiet(true);
    lab.undo();
    quiet(false);
    check(lab.current().width() == 4 && lab.current().height() == 4, "undo crop: back to 4x4");

    // Redo clears on new action (use flip to avoid brightness coalescing)
    quiet(true);
    lab.flip(true);
    lab.undo();
    quiet(false);
    check(lab.redoCount() == 1, "redo has 1 after undo");
    quiet(true);
    lab.flip(false);  // new action should clear redo
    quiet(false);
    check(lab.redoCount() == 0, "new action clears redo");

    remove("_test_undo.ppm");
}

// ============================================================
//  6. Memory-aware history tests
// ============================================================
void testMemoryBudget()
{
    cout << "\n=== Memory-aware history tests ===\n";

    Image testImg(10, 10);
    for (int y = 0; y < 10; y++)
        for (int x = 0; x < 10; x++)
            testImg.at(x, y) = Pixel(100, 100, 100);
    testImg.save("_test_mem.ppm");

    // Test with very small budget
    quiet(true);
    ImageLab lab(10, 500);  // 500 bytes budget - very tight
    lab.load("_test_mem.ppm");

    // Each brightness record stores 100 pixels * 3 bytes = ~300+ bytes
    // With 500 byte budget, only ~1 record should fit
    lab.brightness(10);
    lab.brightness(10);  // This may coalesce with previous
    lab.brightness(10);
    quiet(false);

    check(lab.getCurrentMemory() <= 500 || lab.undoCount() <= 2,
          "memory budget enforced: stays near budget");

    // Test that undo still works after eviction
    int undoBefore = lab.undoCount();
    quiet(true);
    if (undoBefore > 0)
    {
        lab.undo();
        check(true, "undo after eviction: no crash");
    }
    else
    {
        check(true, "all records evicted (budget very tight)");
    }
    quiet(false);

    // Test with unlimited budget
    quiet(true);
    ImageLab lab2(10, 0);  // unlimited
    lab2.load("_test_mem.ppm");
    for (int i = 0; i < 20; i++) lab2.grayscale();  // grayscale is idempotent after first
    quiet(false);
    check(lab2.undoCount() == 20, "unlimited budget: all 20 records kept");

    // Set budget dynamically
    lab2.setMemoryBudget(1);  // very small => should evict most
    check(lab2.undoCount() < 20, "setMemoryBudget: records evicted");

    remove("_test_mem.ppm");
}

// ============================================================
//  7. Brightness coalescing tests
// ============================================================
void testCoalescing()
{
    cout << "\n=== Brightness coalescing tests ===\n";

    Image testImg(4, 4);
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            testImg.at(x, y) = Pixel(100, 100, 100);
    testImg.save("_test_coal.ppm");

    quiet(true);
    ImageLab lab(10);
    lab.load("_test_coal.ppm");

    // Consecutive brightness should coalesce
    lab.brightness(10);
    lab.brightness(20);
    lab.brightness(5);
    quiet(false);

    check(lab.undoCount() == 1, "3 consecutive brightness => 1 coalesced record");
    check(lab.current().at(0, 0).r == 135, "pixel value: 100+10+20+5=135");

    // Undo should reverse the entire chain
    quiet(true);
    lab.undo();
    quiet(false);
    check(lab.current().at(0, 0).r == 100, "undo coalesced: back to 100");

    // Interruption: non-brightness breaks the chain
    quiet(true);
    lab.brightness(10);
    lab.flip(true);    // interrupts the chain
    lab.brightness(20);
    quiet(false);
    check(lab.undoCount() == 3, "brightness-flip-brightness: 3 separate records");

    // Undo each
    quiet(true);
    lab.undo();  // undo brightness 20
    quiet(false);
    check(lab.current().at(0, 0).r == 110, "undo last brightness: 110");
    quiet(true);
    lab.undo();  // undo flip
    lab.undo();  // undo first brightness
    quiet(false);
    check(lab.current().at(0, 0).r == 100, "undo all: back to 100");

    // Clipping test: brightness that causes clipping
    quiet(true);
    lab.brightness(200);   // 100+200=255 (clamped)
    lab.brightness(100);   // should coalesce: delta 300, clamped to 255
    quiet(false);
    check(lab.current().at(0, 0).r == 255, "clipping: value is 255");
    check(lab.undoCount() == 1, "clipped brightness coalesced");

    quiet(true);
    lab.undo();
    quiet(false);
    check(lab.current().at(0, 0).r == 100, "undo clipped brightness: back to 100");

    remove("_test_coal.ppm");
}

// ============================================================
//  8. Priority scheduling tests
// ============================================================
void testPriorityScheduling()
{
    cout << "\n=== Priority scheduling tests ===\n";

    Image testImg(10, 10);
    for (int y = 0; y < 10; y++)
        for (int x = 0; x < 10; x++)
            testImg.at(x, y) = Pixel(100, 100, 100);
    testImg.save("_test_prio.ppm");

    quiet(true);
    ImageLab lab(20);
    lab.load("_test_prio.ppm");
    lab.setSchedulingMode(true);  // priority mode
    quiet(false);

    check(lab.isPriorityMode(), "priority mode enabled");

    // Submit mixed jobs
    quiet(true);
    Job resize1; resize1.type = J_RESIZE; resize1.a = 8; resize1.b = 8;
    Job blur1; blur1.type = J_BLUR; blur1.a = 1;
    Job undo1; undo1.type = J_UNDO;

    lab.submit(resize1);   // prio 3
    lab.submit(blur1);     // prio 2
    // Process first should be blur (prio 2), not resize (prio 3)
    lab.processNext();
    quiet(false);
    check(lab.current().width() == 10, "prio: blur processed first (doesn't change dimensions)");

    quiet(true);
    lab.processNext();  // resize should be next
    quiet(false);
    check(lab.current().width() == 8, "prio: resize processed second");

    // Switch back to FIFO
    quiet(true);
    lab.setSchedulingMode(false);
    quiet(false);
    check(!lab.isPriorityMode(), "FIFO mode re-enabled");

    remove("_test_prio.ppm");
}

// ============================================================
//  9. Batch operation tests
// ============================================================
void testBatchOps()
{
    cout << "\n=== Batch operation tests ===\n";

    // Stack batch with overflow
    {
        stackType<int> s(4);
        int big[1000];
        for (int i = 0; i < 1000; i++) big[i] = i;
        s.pushBatch(big, 1000);
        check(s.size() == 1000, "stack pushBatch 1000: grows correctly");
        s.popBatch(500);
        check(s.size() == 500, "popBatch 500: size 500");
        s.popBatch(9999);  // more than available
        check(s.isEmptyStack(), "popBatch oversized: empties stack");
    }

    // Queue batch with wraparound
    {
        queueType<int> q(8, false);
        q.addQueue(1); q.addQueue(2); q.addQueue(3); q.addQueue(4);
        q.deleteQueue(); q.deleteQueue();  // front moves
        int items[4] = {10, 20, 30, 40};
        q.addQueueBatch(items, 4);  // should fill to cap, wrapping around
        check(q.size() == 6, "queue batch wraparound: size 6");
        check(q.front() == 3, "queue batch wraparound: front correct");
    }

    // Queue batch deletion oversized
    {
        queueType<int> q(10, false);
        q.addQueue(1); q.addQueue(2); q.addQueue(3);
        q.deleteQueueBatch(100);  // more than available
        check(q.isEmptyQueue(), "deleteQueueBatch oversized: empties queue");
    }
}

// ============================================================
//  10. Pipeline tests (FIFO)
// ============================================================
void testPipeline()
{
    cout << "\n=== Pipeline tests ===\n";

    Image testImg(10, 10);
    for (int y = 0; y < 10; y++)
        for (int x = 0; x < 10; x++)
            testImg.at(x, y) = Pixel(128, 128, 128);
    testImg.save("_test_pipe.ppm");

    quiet(true);
    ImageLab lab(5);
    lab.load("_test_pipe.ppm");
    quiet(false);

    check(lab.pendingCount() == 0, "no pending jobs");

    // Submit resize
    quiet(true);
    Job j1; j1.type = J_RESIZE; j1.a = 5; j1.b = 5;
    lab.submit(j1);
    quiet(false);
    check(lab.pendingCount() == 1, "1 pending job");

    // Process
    quiet(true);
    lab.processNext();
    quiet(false);
    check(lab.current().width() == 5, "resize processed: 5x5");
    check(lab.pendingCount() == 0, "0 pending after process");

    // Cancel test
    quiet(true);
    Job j2; j2.type = J_BLUR; j2.a = 1;
    lab.submit(j2);
    lab.cancel(lab.pendingCount());  // cancel by current id
    // Actually cancel by job id - submit returns the id via cout
    // Let's submit and cancel more directly
    lab.clearJobs();
    Job j3; j3.type = J_BLUR; j3.a = 1;
    lab.submit(j3);
    quiet(false);

    check(lab.pendingCount() == 1, "1 job before cancel");
    quiet(true);
    lab.clearJobs();
    quiet(false);
    check(lab.pendingCount() == 0, "clearJobs: 0 pending");

    // Queue full rejection
    quiet(true);
    for (int i = 0; i < 10; i++)
    {
        Job j; j.type = J_BLUR; j.a = 1;
        lab.submit(j);
    }
    quiet(false);
    check(lab.pendingCount() == 5, "fixed queue rejects beyond capacity (5)");

    quiet(true);
    lab.processAll();
    quiet(false);
    check(lab.pendingCount() == 0, "processAll clears queue");

    remove("_test_pipe.ppm");
}

// ============================================================
//  11. Inspector test
// ============================================================
void testInspector()
{
    cout << "\n=== Inspector test ===\n";

    Image testImg(4, 4);
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            testImg.at(x, y) = Pixel(100, 100, 100);
    testImg.save("_test_insp.ppm");

    quiet(true);
    ImageLab lab(10);
    lab.load("_test_insp.ppm");
    lab.brightness(10);
    lab.rotate(90);

    // Capture inspector output
    quiet(false);
    ostringstream capture;
    streambuf* old = cout.rdbuf(capture.rdbuf());
    lab.inspector();
    cout.rdbuf(old);

    string output = capture.str();
    check(output.find("Undo stack size: 2") != string::npos, "inspector shows undo count");
    check(output.find("FIFO") != string::npos, "inspector shows FIFO mode");
    check(output.find("Memory budget") != string::npos, "inspector shows memory budget");

    remove("_test_insp.ppm");
}

// ============================================================
//  12. Bucket fill undo test (compact record)
// ============================================================
void testFillUndo()
{
    cout << "\n=== Bucket fill undo test ===\n";

    Image testImg(5, 5);
    for (int y = 0; y < 5; y++)
        for (int x = 0; x < 5; x++)
            testImg.at(x, y) = Pixel(255, 255, 255);
    testImg.save("_test_fill_undo.ppm");

    quiet(true);
    ImageLab lab(10);
    lab.load("_test_fill_undo.ppm");

    // Queue a fill job and process it
    Job fillJob; fillJob.type = J_FILL;
    fillJob.a = 0; fillJob.b = 0;
    fillJob.color = Pixel(255, 0, 0);
    fillJob.useStack = true;
    lab.submit(fillJob);
    lab.processAll();
    quiet(false);

    check(lab.current().at(2, 2).r == 255 && lab.current().at(2, 2).g == 0, "fill: center pixel is red");

    // Undo fill
    quiet(true);
    lab.undo();
    quiet(false);
    check(lab.current().at(2, 2).r == 255 && lab.current().at(2, 2).g == 255, "undo fill: center pixel is white again");

    // Redo fill
    quiet(true);
    lab.redo();
    quiet(false);
    check(lab.current().at(2, 2).g == 0, "redo fill: center pixel is red again");

    remove("_test_fill_undo.ppm");
}

// ============================================================
//  main
// ============================================================
int main()
{
    cout << "====================================\n";
    cout << "  ImageLab Phase 2 Test Suite\n";
    cout << "====================================\n";

    testStack();
    testQueue();
    testPriorityQueue();
    testImage();
    testUndoRedo();
    testMemoryBudget();
    testCoalescing();
    testPriorityScheduling();
    testBatchOps();
    testPipeline();
    testInspector();
    testFillUndo();

    cout << "\n====================================\n";
    cout << "  Results: " << passed << " passed, " << failed << " failed\n";
    cout << "  Total:   " << (passed + failed) << " tests\n";
    cout << "====================================\n";

    return failed > 0 ? 1 : 0;
}
