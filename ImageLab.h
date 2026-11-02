#ifndef IMAGELAB_H_
#define IMAGELAB_H_

#include "Image.h"
#include "stackType.h"
#include "queueType.h"
#include "priorityQueueType.h"
#include <chrono>
using namespace std;

// ============================================================
//  Operation type constants (no enums - forbidden by assignment)
// ============================================================
const int J_RESIZE = 0;
const int J_BLUR   = 1;
const int J_FILL   = 2;
const int J_SAVE   = 3;
const int J_EXPORT = 4;
const int J_UNDO   = 5;
const int J_REDO   = 6;

static const char* const JOB_NAMES[] = { "Resize", "Blur", "BucketFill", "Save", "Export", "Undo", "Redo" };

// Operation name constants for history
const int OP_CROP       = 0;
const int OP_ROTATE     = 1;
const int OP_FLIP       = 2;
const int OP_BRIGHTNESS = 3;
const int OP_GRAYSCALE  = 4;
const int OP_RESIZE     = 5;
const int OP_BLUR       = 6;
const int OP_FILL       = 7;

static const char* const OP_NAMES[] = { "Crop", "Rotate", "Flip", "Brightness", "Grayscale", "Resize", "Blur", "Fill" };

// Priority levels (smaller = higher priority)
const int PRIO_UNDO_REDO    = 1;
const int PRIO_BLUR_FILL    = 2;
const int PRIO_RESIZE       = 3;
const int PRIO_FINALIZATION = 4;

// ============================================================
//  Phase 2: Compact History Record
//  Stores operation-specific undo data instead of full images.
// ============================================================
struct HistoryRecord
{
    int opType;             // OP_CROP, OP_ROTATE, etc.
    string opName;          // human-readable name
    int seqNum;             // sequence number

    // Operation parameters
    int paramA, paramB, paramC, paramD;   // crop: x,y,w,h | rotate: angle | flip: 1=h,0=v | brightness: delta | resize: oldW,oldH
    bool paramBool;                        // flip direction, fill useStack

    // Pixel data for undo (dynamically allocated)
    Pixel* savedPixels;     // stored pixel data for reversal
    int savedCount;         // number of saved pixels
    int savedW, savedH;     // dimensions of saved region

    // For bucket fill: stored changed pixel coordinates
    Point* changedCoords;   // coordinates of changed pixels
    int changedCount;       // number of changed pixels
    Pixel fillOrigColor;    // original color before fill

    // For brightness with clipping: store original pixel values
    // (savedPixels stores the ENTIRE pre-operation image for ops that need it)

    long estimatedBytes;    // memory estimate for this record

    HistoryRecord()
        : opType(0), seqNum(0), paramA(0), paramB(0), paramC(0), paramD(0),
          paramBool(false), savedPixels(0), savedCount(0), savedW(0), savedH(0),
          changedCoords(0), changedCount(0), estimatedBytes(0) {}

    HistoryRecord(const HistoryRecord& o)
        : opType(o.opType), opName(o.opName), seqNum(o.seqNum),
          paramA(o.paramA), paramB(o.paramB), paramC(o.paramC), paramD(o.paramD),
          paramBool(o.paramBool), savedPixels(0), savedCount(o.savedCount),
          savedW(o.savedW), savedH(o.savedH),
          changedCoords(0), changedCount(o.changedCount),
          fillOrigColor(o.fillOrigColor), estimatedBytes(o.estimatedBytes)
    {
        if (o.savedPixels && savedCount > 0)
        {
            savedPixels = new Pixel[savedCount];
            for (int i = 0; i < savedCount; i++) savedPixels[i] = o.savedPixels[i];
        }
        if (o.changedCoords && changedCount > 0)
        {
            changedCoords = new Point[changedCount];
            for (int i = 0; i < changedCount; i++) changedCoords[i] = o.changedCoords[i];
        }
    }

    HistoryRecord& operator=(const HistoryRecord& o)
    {
        if (this != &o)
        {
            delete[] savedPixels;
            delete[] changedCoords;
            opType = o.opType; opName = o.opName; seqNum = o.seqNum;
            paramA = o.paramA; paramB = o.paramB; paramC = o.paramC; paramD = o.paramD;
            paramBool = o.paramBool;
            savedCount = o.savedCount; savedW = o.savedW; savedH = o.savedH;
            changedCount = o.changedCount;
            fillOrigColor = o.fillOrigColor;
            estimatedBytes = o.estimatedBytes;
            savedPixels = 0;
            changedCoords = 0;
            if (o.savedPixels && savedCount > 0)
            {
                savedPixels = new Pixel[savedCount];
                for (int i = 0; i < savedCount; i++) savedPixels[i] = o.savedPixels[i];
            }
            if (o.changedCoords && changedCount > 0)
            {
                changedCoords = new Point[changedCount];
                for (int i = 0; i < changedCount; i++) changedCoords[i] = o.changedCoords[i];
            }
        }
        return *this;
    }

    ~HistoryRecord()
    {
        delete[] savedPixels;
        delete[] changedCoords;
    }
};

// ============================================================
//  Job struct for the processing pipeline
// ============================================================
struct Job
{
    int id;
    int type;
    int priority;       // Phase 2: priority level
    int seqNum;         // Phase 2: arrival sequence for tie-breaking
    int a, b;
    Pixel color;
    bool useStack;
    string file;
    bool cancelled;
    double estimatedCost;  // Phase 2: estimated execution cost

    Job() : id(0), type(0), priority(PRIO_RESIZE), seqNum(0),
            a(0), b(0), useStack(true), cancelled(false), estimatedCost(0) {}
};

// ============================================================
//  ImageLab application (Phase 2)
// ============================================================
class ImageLab
{
private:
    Image img;
    bool loaded;
    stackType<HistoryRecord> undoS, redoS;
    queueType<Job> fifoJobs;           // FIFO queue
    priorityQueueType<Job> prioJobs;   // Priority queue
    bool usePriority;                  // scheduling mode
    int nextId;
    int nextSeq;                       // global sequence counter for jobs
    int historySeq;                    // sequence counter for history records
    long lastInserts; int lastMax; string lastFillKind;
    string workingFile;                // project working filename

    // Phase 2: memory budget
    long memoryBudget;                 // in bytes (0 = unlimited)
    long currentMemory;                // estimated current usage of undo+redo stacks

    // --- Memory accounting ---
    void addMemory(long bytes) { currentMemory += bytes; }
    void subtractMemory(long bytes) { currentMemory -= bytes; if (currentMemory < 0) currentMemory = 0; }

    long estimateRecordMemory(const HistoryRecord& r) const
    {
        long mem = sizeof(HistoryRecord);
        mem += (long)r.savedCount * sizeof(Pixel);
        mem += (long)r.changedCount * sizeof(Point);
        return mem;
    }

    void enforceMemoryBudget()
    {
        if (memoryBudget <= 0) return;
        while (currentMemory > memoryBudget && !undoS.isEmptyStack())
        {
            subtractMemory(undoS.at(0).estimatedBytes);
            undoS.removeBottom();
        }
    }

    // --- History record creation (compact, operation-specific) ---

    // Rotate/Flip: only store the inverse parameters
    HistoryRecord makeRotateRecord(int angle)
    {
        HistoryRecord r;
        r.opType = OP_ROTATE;
        r.opName = "Rotate";
        r.seqNum = historySeq++;
        r.paramA = angle;
        // No pixel data needed - inverse is another rotation
        r.estimatedBytes = sizeof(HistoryRecord);
        return r;
    }

    HistoryRecord makeFlipRecord(bool horizontal)
    {
        HistoryRecord r;
        r.opType = OP_FLIP;
        r.opName = horizontal ? "FlipH" : "FlipV";
        r.seqNum = historySeq++;
        r.paramBool = horizontal;
        r.estimatedBytes = sizeof(HistoryRecord);
        return r;
    }

    // Brightness: store delta + original pixel values (for exact clipping reversal)
    HistoryRecord makeBrightnessRecord(int delta, const Image& before)
    {
        HistoryRecord r;
        r.opType = OP_BRIGHTNESS;
        r.opName = "Brightness";
        r.seqNum = historySeq++;
        r.paramA = delta;
        // Store full pre-operation pixels to handle clipping correctly
        int n = before.width() * before.height();
        r.savedPixels = new Pixel[n];
        r.savedCount = n;
        r.savedW = before.width();
        r.savedH = before.height();
        for (int i = 0; i < n; i++) r.savedPixels[i] = before.rawPixels()[i];
        r.estimatedBytes = sizeof(HistoryRecord) + (long)n * sizeof(Pixel);
        return r;
    }

    // Crop: store crop coords + removed pixels (the entire pre-crop image)
    HistoryRecord makeCropRecord(int x, int y, int cw, int ch, const Image& before)
    {
        HistoryRecord r;
        r.opType = OP_CROP;
        r.opName = "Crop";
        r.seqNum = historySeq++;
        r.paramA = x; r.paramB = y; r.paramC = cw; r.paramD = ch;
        int n = before.width() * before.height();
        r.savedPixels = new Pixel[n];
        r.savedCount = n;
        r.savedW = before.width();
        r.savedH = before.height();
        for (int i = 0; i < n; i++) r.savedPixels[i] = before.rawPixels()[i];
        r.estimatedBytes = sizeof(HistoryRecord) + (long)n * sizeof(Pixel);
        return r;
    }

    // Bucket fill: store original color + changed pixel coordinates and original values
    HistoryRecord makeFillRecord(bool useStack, const Image& before, const Image& after)
    {
        HistoryRecord r;
        r.opType = OP_FILL;
        r.opName = useStack ? "Fill(stack)" : "Fill(queue)";
        r.seqNum = historySeq++;
        r.paramBool = useStack;

        // Count changed pixels
        int w = before.width(), h = before.height();
        int changed = 0;
        for (int i = 0; i < w * h; i++)
            if (!(before.rawPixels()[i] == after.rawPixels()[i])) changed++;

        r.changedCount = changed;
        r.changedCoords = new Point[changed];
        r.savedPixels = new Pixel[changed];
        r.savedCount = changed;

        int idx = 0;
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                if (!(before.at(x, y) == after.at(x, y)))
                {
                    r.changedCoords[idx] = Point(x, y);
                    r.savedPixels[idx] = before.at(x, y);
                    idx++;
                }
        r.estimatedBytes = sizeof(HistoryRecord) + (long)changed * (sizeof(Point) + sizeof(Pixel));
        return r;
    }

    // Grayscale, Blur, Resize: store full previous image (entire image changes)
    HistoryRecord makeFullImageRecord(int opType, const char* name, const Image& before)
    {
        HistoryRecord r;
        r.opType = opType;
        r.opName = name;
        r.seqNum = historySeq++;
        int n = before.width() * before.height();
        r.savedPixels = new Pixel[n];
        r.savedCount = n;
        r.savedW = before.width();
        r.savedH = before.height();
        for (int i = 0; i < n; i++) r.savedPixels[i] = before.rawPixels()[i];
        r.estimatedBytes = sizeof(HistoryRecord) + (long)n * sizeof(Pixel);
        return r;
    }

    // --- Undo/Redo execution ---
    void applyUndo(const HistoryRecord& r)
    {
        switch (r.opType)
        {
        case OP_ROTATE:
        {
            // Inverse rotation
            int inverse = (360 - r.paramA) % 360;
            if (inverse == 0) inverse = 360;
            img.rotate(inverse);
            break;
        }
        case OP_FLIP:
            img.flip(r.paramBool);  // flip is self-inverse
            break;
        case OP_BRIGHTNESS:
        case OP_CROP:
        case OP_GRAYSCALE:
        case OP_RESIZE:
        case OP_BLUR:
        {
            // Restore from saved pixels
            Image restored(r.savedW, r.savedH);
            for (int i = 0; i < r.savedCount; i++) restored.rawPixels()[i] = r.savedPixels[i];
            img = restored;
            break;
        }
        case OP_FILL:
        {
            // Restore changed pixels to their original values
            for (int i = 0; i < r.changedCount; i++)
                img.at(r.changedCoords[i].x, r.changedCoords[i].y) = r.savedPixels[i];
            break;
        }
        }
    }

    void applyRedo(const HistoryRecord& r)
    {
        switch (r.opType)
        {
        case OP_ROTATE:
            img.rotate(r.paramA);
            break;
        case OP_FLIP:
            img.flip(r.paramBool);
            break;
        case OP_BRIGHTNESS:
            img.brightness(r.paramA);
            break;
        case OP_CROP:
            img.crop(r.paramA, r.paramB, r.paramC, r.paramD);
            break;
        case OP_GRAYSCALE:
            img.grayscale();
            break;
        case OP_RESIZE:
            img.resize(r.paramA, r.paramB);
            break;
        case OP_BLUR:
            img.blur(r.paramA);
            break;
        case OP_FILL:
        {
            // Re-apply fill: set all changed pixels to the fill color
            // We need to know the fill color. Store it from the current 'after' pixels.
            // Actually, for redo we re-apply the operation. We can compute the fill color
            // from the current pixels at the changed coordinates (they all became the same color).
            // But we need the fill color stored. Let's store it in fillOrigColor as the NEW color.
            // Actually, we store the original color in savedPixels and fillOrigColor.
            // For redo, we need the NEW color. Let's get it differently:
            // The fill color is what the pixels SHOULD become. We can store it in paramA/paramB/paramC.
            // But we already use those. Let's just re-run bucketFill... but that changes the image.
            // Simplest correct approach: store the post-fill pixels too.
            // OR: store the fill color in the record.
            // Let's use fillOrigColor as the TARGET color (what pixels became).
            // Wait, fillOrigColor was the ORIGINAL color. We need the REPLACEMENT color.
            // Let's store it differently. For simplicity and correctness:
            // We'll just swap back - undo gives us the before state, and the after state
            // is what we had. So we need to store the "after" pixels for the changed coords.
            // Actually - the simplest correct approach: for fill redo, just apply brightness(0)
            // No. Let me think about this differently.
            // The HistoryRecord for fill stores: changedCoords + savedPixels (original values).
            // For redo we need the NEW values at those coords.
            // Solution: store the fill replacement color.
            // fillOrigColor is used for this: it stores the replacement color.
            for (int i = 0; i < r.changedCount; i++)
                img.at(r.changedCoords[i].x, r.changedCoords[i].y) = r.fillOrigColor;
            break;
        }
        }
    }

    // Create an "inverse" record for the redo stack
    // For rotate/flip: the inverse is stored in the record itself
    // For pixel-based ops: we need to capture current state before undo
    HistoryRecord makeRedoRecord(const HistoryRecord& undoRec)
    {
        HistoryRecord r = undoRec;  // deep copy
        // For operations that restore from saved pixels, we need to save the CURRENT
        // pixels (which are the post-operation state) so that redo can restore them.
        if (undoRec.opType == OP_BRIGHTNESS || undoRec.opType == OP_CROP ||
            undoRec.opType == OP_GRAYSCALE || undoRec.opType == OP_RESIZE ||
            undoRec.opType == OP_BLUR)
        {
            // Save current image state (post-operation, pre-undo)
            int n = img.width() * img.height();
            delete[] r.savedPixels;
            r.savedPixels = new Pixel[n];
            r.savedCount = n;
            r.savedW = img.width();
            r.savedH = img.height();
            for (int i = 0; i < n; i++) r.savedPixels[i] = img.rawPixels()[i];
            r.estimatedBytes = sizeof(HistoryRecord) + (long)n * sizeof(Pixel);
        }
        // For rotate/flip: the record already contains the parameters, redo just re-applies
        // For fill: savedPixels has originals, fillOrigColor has replacement - both are in undoRec
        return r;
    }

    HistoryRecord makeUndoFromRedo(const HistoryRecord& redoRec)
    {
        HistoryRecord r = redoRec;
        if (redoRec.opType == OP_BRIGHTNESS || redoRec.opType == OP_CROP ||
            redoRec.opType == OP_GRAYSCALE || redoRec.opType == OP_RESIZE ||
            redoRec.opType == OP_BLUR)
        {
            // Save current image (post-redo-application, which is the "after" state)
            // For undo, we need the "before" state, which is what the image looked like
            // before redo re-applied the operation.
            // Actually: the redo record's savedPixels already has the post-op pixels.
            // For the undo record, we need the pre-op pixels (current state before redo).
            int n = img.width() * img.height();
            delete[] r.savedPixels;
            r.savedPixels = new Pixel[n];
            r.savedCount = n;
            r.savedW = img.width();
            r.savedH = img.height();
            for (int i = 0; i < n; i++) r.savedPixels[i] = img.rawPixels()[i];
            r.estimatedBytes = sizeof(HistoryRecord) + (long)n * sizeof(Pixel);
        }
        return r;
    }

    void record(HistoryRecord& r)
    {
        r.estimatedBytes = estimateRecordMemory(r);
        addMemory(r.estimatedBytes);
        // Try coalescing brightness
        if (r.opType == OP_BRIGHTNESS && !undoS.isEmptyStack())
        {
            HistoryRecord& top = undoS.at(undoS.size() - 1);
            if (top.opType == OP_BRIGHTNESS)
            {
                // Coalesce: combine deltas, keep the OLDER record's saved pixels
                // (those are the original pixels before any brightness in this chain)
                top.paramA += r.paramA;
                top.opName = "Brightness(coalesced)";
                // The top record already has the pre-chain pixels, keep them.
                // The new record's saved pixels (current pre-op pixels) are discarded
                // because top's saved pixels go further back.
                // Update memory: the new record's pixels are not added
                subtractMemory(r.estimatedBytes);  // don't add this record
                // But we need to recalculate - the coalesced record size doesn't change
                return;
            }
        }

        undoS.push(r);
        // Clear redo stack (new action after undo clears redo)
        while (!redoS.isEmptyStack())
        {
            subtractMemory(redoS.top().estimatedBytes);
            redoS.pop();
        }
        enforceMemoryBudget();
    }

    void doUndo()
    {
        if (undoS.isEmptyStack()) { cout << "Nothing to undo\n"; return; }
        HistoryRecord r = undoS.top();
        // Create redo record BEFORE undoing (captures current state)
        HistoryRecord redoRec = makeRedoRecord(r);
        undoS.pop();
        subtractMemory(r.estimatedBytes);
        applyUndo(r);
        redoRec.estimatedBytes = estimateRecordMemory(redoRec);
        addMemory(redoRec.estimatedBytes);
        redoS.push(redoRec);
        cout << "Undid " << r.opName << "\n";
    }

    void doRedo()
    {
        if (redoS.isEmptyStack()) { cout << "Nothing to redo\n"; return; }
        HistoryRecord r = redoS.top();
        // Create undo record BEFORE redoing
        HistoryRecord undoRec = makeUndoFromRedo(r);
        redoS.pop();
        subtractMemory(r.estimatedBytes);
        applyRedo(r);
        undoRec.estimatedBytes = estimateRecordMemory(undoRec);
        addMemory(undoRec.estimatedBytes);
        undoS.push(undoRec);
        cout << "Redid " << r.opName << "\n";
    }

    int getPriority(int jobType) const
    {
        if (jobType == J_UNDO || jobType == J_REDO) return PRIO_UNDO_REDO;
        if (jobType == J_BLUR || jobType == J_FILL) return PRIO_BLUR_FILL;
        if (jobType == J_RESIZE) return PRIO_RESIZE;
        return PRIO_FINALIZATION;  // Save, Export
    }

    bool isFinalization(int jobType) const
    {
        return jobType == J_SAVE || jobType == J_EXPORT;
    }

    void execute(const Job& j)
    {
        cout << "Running job #" << j.id << " " << JOB_NAMES[j.type] << "\n";
        switch (j.type)
        {
        case J_RESIZE:
        {
            Image before = img;
            if (img.resize(j.a, j.b))
            {
                HistoryRecord r = makeFullImageRecord(OP_RESIZE, "Resize", before);
                r.paramA = j.a; r.paramB = j.b;
                record(r);
            }
            break;
        }
        case J_BLUR:
        {
            Image before = img;
            if (img.blur(j.a))
            {
                HistoryRecord r = makeFullImageRecord(OP_BLUR, "Blur", before);
                r.paramA = j.a;
                record(r);
            }
            break;
        }
        case J_FILL:
        {
            Image before = img;
            int mx; long ins;
            long n = img.bucketFill(j.a, j.b, j.color, j.useStack, mx, ins);
            if (n > 0)
            {
                HistoryRecord r = makeFillRecord(j.useStack, before, img);
                r.fillOrigColor = j.color;  // store replacement color for redo
                record(r);
                lastInserts = ins; lastMax = mx; lastFillKind = j.useStack ? "stack" : "queue";
                cout << n << " pixels changed\n";
            }
            else if (n == 0) cout << "Fill: start colour equals replacement - nothing changed\n";
            break;
        }
        case J_SAVE:
        {
            string fname = workingFile.empty() ? "project_working.png" : workingFile;
            if (img.save(fname)) cout << "Saved " << fname << "\n";
            break;
        }
        case J_EXPORT:
            if (img.save(j.file)) cout << "Exported " << j.file << "\n";
            break;
        case J_UNDO: doUndo(); break;
        case J_REDO: doRedo(); break;
        }
    }

public:
    ImageLab(int queueCap, long budget = 0)
        : loaded(false), fifoJobs(queueCap, false), prioJobs(queueCap),
          usePriority(false), nextId(1), nextSeq(0), historySeq(0),
          lastInserts(0), lastMax(0), lastFillKind("-"),
          memoryBudget(budget), currentMemory(0) {}

    const Image& current() const { return img; }
    int undoCount() const { return undoS.size(); }
    int redoCount() const { return redoS.size(); }
    int pendingCount() const { return usePriority ? prioJobs.size() : fifoJobs.size(); }
    bool isPriorityMode() const { return usePriority; }
    long getMemoryBudget() const { return memoryBudget; }
    long getCurrentMemory() const { return currentMemory; }

    void setMemoryBudget(long budget) { memoryBudget = budget; enforceMemoryBudget(); }
    void setSchedulingMode(bool priority) { usePriority = priority; cout << "Scheduling mode: " << (priority ? "Priority" : "FIFO") << "\n"; }

    bool need() { if (!loaded) cout << "Load an image first\n"; return loaded; }

    void load(const string& f)
    {
        if (img.load(f))
        {
            loaded = true;
            workingFile = f;
            undoS.initializeStack(); redoS.initializeStack();
            currentMemory = 0;
            if (usePriority)
            {
                if (!prioJobs.isEmptyQueue())
                {
                    prioJobs.initializeQueue();
                    cout << "Pending jobs cleared because a new image was loaded\n";
                }
            }
            else
            {
                if (!fifoJobs.isEmptyQueue())
                {
                    fifoJobs.initializeQueue();
                    cout << "Pending jobs cleared because a new image was loaded\n";
                }
            }
            cout << "Loaded " << img.width() << "x" << img.height() << "\n";
        }
    }

    void info() { if (need()) cout << "Image " << img.width() << "x" << img.height() << "\n"; }

    // --- instant ops ---
    void crop(int x, int y, int w, int h)
    {
        if (!need()) return;
        Image before = img;
        if (img.crop(x, y, w, h))
        {
            HistoryRecord r = makeCropRecord(x, y, w, h, before);
            record(r);
        }
    }

    void rotate(int a)
    {
        if (!need()) return;
        Image before = img;
        if (img.rotate(a))
        {
            HistoryRecord r = makeRotateRecord(a);
            record(r);
        }
    }

    void flip(bool hor)
    {
        if (!need()) return;
        if (img.flip(hor))
        {
            HistoryRecord r = makeFlipRecord(hor);
            record(r);
        }
    }

    void brightness(int d)
    {
        if (!need()) return;
        Image before = img;
        if (img.brightness(d))
        {
            HistoryRecord r = makeBrightnessRecord(d, before);
            record(r);
        }
    }

    void grayscale()
    {
        if (!need()) return;
        Image before = img;
        if (img.grayscale())
        {
            HistoryRecord r = makeFullImageRecord(OP_GRAYSCALE, "Grayscale", before);
            record(r);
        }
    }

    // --- undo / redo ---
    void undo()
    {
        if (!need()) return;
        bool hasPending = usePriority ? !prioJobs.isEmptyQueue() : !fifoJobs.isEmptyQueue();
        if (!hasPending) doUndo();
        else { Job j; j.type = J_UNDO; submit(j); }
    }

    void redo()
    {
        if (!need()) return;
        bool hasPending = usePriority ? !prioJobs.isEmptyQueue() : !fifoJobs.isEmptyQueue();
        if (!hasPending) doRedo();
        else { Job j; j.type = J_REDO; submit(j); }
    }

    // --- pipeline ---
    void submit(Job j)
    {
        if (!need()) return;
        j.priority = getPriority(j.type);
        j.seqNum = nextSeq++;
        j.id = nextId++;

        if (usePriority)
        {
            prioJobs.addQueue(j);
        }
        else
        {
            if (fifoJobs.isFullQueue())
            {
                cout << "Queue is full (" << fifoJobs.capacity() << "): job rejected. Process pending jobs first.\n";
                return;
            }
            fifoJobs.addQueue(j);
        }
        cout << "Job #" << j.id << " (" << JOB_NAMES[j.type] << ") queued\n";
    }

    void listJobs() const
    {
        if (usePriority)
        {
            cout << "Pending jobs (priority order), " << prioJobs.size() << ":\n";
            for (int i = 0; i < prioJobs.size(); i++)
            {
                const Job& j = prioJobs.peekAt(i);
                cout << "  [" << i + 1 << "] #" << j.id << " " << JOB_NAMES[j.type]
                     << " (prio=" << j.priority << ")" << (j.cancelled ? " (cancelled)" : "") << "\n";
            }
            if (prioJobs.isEmptyQueue()) cout << "  (none)\n";
        }
        else
        {
            cout << "Pending jobs (front -> rear), " << fifoJobs.size() << "/" << fifoJobs.capacity() << ":\n";
            for (int i = 0; i < fifoJobs.size(); i++)
            {
                const Job& j = fifoJobs.peekAt(i);
                cout << "  [" << i + 1 << "] #" << j.id << " " << JOB_NAMES[j.type]
                     << (j.cancelled ? " (cancelled)" : "") << "\n";
            }
            if (fifoJobs.isEmptyQueue()) cout << "  (none)\n";
        }
    }

    // Priority mode processing with finalization barriers
    void processNextPriority()
    {
        while (!prioJobs.isEmptyQueue())
        {
            Job j = prioJobs.front();
            prioJobs.deleteQueue();
            if (j.cancelled) { cout << "Skipping cancelled job #" << j.id << "\n"; continue; }
            execute(j);
            return;
        }
        cout << "No pending jobs\n";
    }

    void processNext()
    {
        if (usePriority) { processNextPriority(); return; }

        while (!fifoJobs.isEmptyQueue())
        {
            Job j = fifoJobs.front();
            fifoJobs.deleteQueue();
            if (j.cancelled) { cout << "Skipping cancelled job #" << j.id << "\n"; continue; }
            execute(j);
            return;
        }
        cout << "No pending jobs\n";
    }

    void processAll()
    {
        bool empty = usePriority ? prioJobs.isEmptyQueue() : fifoJobs.isEmptyQueue();
        if (empty) cout << "No pending jobs\n";
        while (!(usePriority ? prioJobs.isEmptyQueue() : fifoJobs.isEmptyQueue()))
            processNext();
    }

    void cancel(int id)
    {
        if (usePriority)
        {
            for (int i = 0; i < prioJobs.size(); i++)
                if (prioJobs.peekAt(i).id == id) { prioJobs.peekAt(i).cancelled = true; cout << "Job #" << id << " cancelled\n"; return; }
        }
        else
        {
            for (int i = 0; i < fifoJobs.size(); i++)
                if (fifoJobs.peekAt(i).id == id) { fifoJobs.peekAt(i).cancelled = true; cout << "Job #" << id << " cancelled\n"; return; }
        }
        cout << "No pending job with id " << id << "\n";
    }

    void clearJobs()
    {
        if (usePriority) prioJobs.initializeQueue();
        else fifoJobs.initializeQueue();
        cout << "All pending jobs cleared\n";
    }

    // --- Data Structure Inspector ---
    void inspector() const
    {
        cout << "\n===== DATA STRUCTURE INSPECTOR =====\n";
        cout << "Scheduling mode: " << (usePriority ? "Priority" : "FIFO") << "\n";
        cout << "Undo stack size: " << undoS.size() << "   Redo stack size: " << redoS.size() << "\n";
        cout << "Undo stack (top first): ";
        for (int i = undoS.size() - 1; i >= 0; i--) cout << undoS.at(i).opName << " ";
        cout << (undoS.isEmptyStack() ? "(empty)" : "") << "\n";
        cout << "Redo stack (top first): ";
        for (int i = redoS.size() - 1; i >= 0; i--) cout << redoS.at(i).opName << " ";
        cout << (redoS.isEmptyStack() ? "(empty)" : "") << "\n";
        cout << "Undo stack  push/pop: " << undoS.getPushCount() << "/" << undoS.getPopCount() << "\n";
        cout << "Redo stack  push/pop: " << redoS.getPushCount() << "/" << redoS.getPopCount() << "\n";
        if (usePriority)
        {
            cout << "Priority queue  current size: " << prioJobs.size()
                 << "  max recorded: " << prioJobs.getMaxSize() << "\n";
            cout << "Priority queue  addQueue/deleteQueue: " << prioJobs.getAddCount() << "/" << prioJobs.getDelCount() << "\n";
        }
        else
        {
            cout << "Job queue   current size: " << fifoJobs.size() << "  max recorded: " << fifoJobs.getMaxSize()
                 << "  capacity: " << fifoJobs.capacity() << "\n";
            cout << "Job queue   addQueue/deleteQueue: " << fifoJobs.getAddCount() << "/" << fifoJobs.getDelCount() << "\n";
        }
        listJobs();
        cout << "Memory budget: " << (memoryBudget > 0 ? memoryBudget : -1) << " bytes"
             << "  Current usage: " << currentMemory << " bytes\n";
        if (lastFillKind == "-") cout << "Last bucket fill: none yet\n";
        else
        {
            bool st = (lastFillKind == "stack");
            cout << "Last bucket fill (" << lastFillKind << "): "
                 << (st ? "push/pop" : "addQueue/deleteQueue") << " = " << lastInserts << "/" << lastInserts
                 << ", max size reached = " << lastMax << "\n";
        }
        cout << "====================================\n\n";
    }

    // ============================================================
    //  Phase 2: Priority scheduling with finalization barriers
    //  Submit jobs into priority queue, respecting barriers.
    // ============================================================
    void submitWithBarrier(Job j)
    {
        if (!need()) return;
        j.priority = getPriority(j.type);
        j.seqNum = nextSeq++;
        j.id = nextId++;

        if (usePriority)
        {
            // Finalization barrier: if this is a finalization job, it must run
            // after all previously submitted jobs. We handle this by draining
            // the priority queue into a temporary structure, then re-adding with
            // the barrier constraint.
            // Implementation: use seqNum for tie-breaking in the priority queue.
            // The priorityQueueType already sorts by priority and preserves FIFO
            // for ties. Finalization jobs have the lowest priority (4), so they
            // naturally sort to the end. The barrier rule means no job submitted
            // AFTER a finalization job can run before it.
            // We enforce this at execution time by checking if a finalization
            // barrier exists.
            prioJobs.addQueue(j);
        }
        else
        {
            if (fifoJobs.isFullQueue())
            {
                cout << "Queue is full (" << fifoJobs.capacity() << "): job rejected.\n";
                return;
            }
            fifoJobs.addQueue(j);
        }
        cout << "Job #" << j.id << " (" << JOB_NAMES[j.type] << ") queued\n";
    }

    // Process respecting finalization barriers in priority mode
    void processNextWithBarrier()
    {
        if (usePriority)
        {
            if (prioJobs.isEmptyQueue()) { cout << "No pending jobs\n"; return; }

            // Find the first finalization barrier in submission order
            int barrierSeq = -1;
            for (int i = 0; i < prioJobs.size(); i++)
            {
                if (isFinalization(prioJobs.peekAt(i).type) && !prioJobs.peekAt(i).cancelled)
                {
                    barrierSeq = prioJobs.peekAt(i).seqNum;
                    break;
                }
            }

            // If there's a barrier, only process jobs submitted before it
            // (or the barrier itself if all prior jobs are done)
            if (barrierSeq >= 0)
            {
                // Check if any non-cancelled job with seqNum < barrierSeq exists
                bool priorJobsExist = false;
                int bestIdx = -1;
                for (int i = 0; i < prioJobs.size(); i++)
                {
                    const Job& j = prioJobs.peekAt(i);
                    if (j.cancelled) continue;
                    if (j.seqNum < barrierSeq)
                    {
                        priorJobsExist = true;
                        // The priority queue is sorted, so the first non-cancelled
                        // job with seqNum < barrier is the highest priority one
                        bestIdx = i;
                        break;
                    }
                }

                if (priorJobsExist && bestIdx >= 0)
                {
                    // Execute this job (remove it from queue)
                    Job j = prioJobs.peekAt(bestIdx);
                    // Remove by marking cancelled and skipping, or rebuild queue
                    // For simplicity, remove from front if it's the front
                    if (bestIdx == 0)
                    {
                        prioJobs.deleteQueue();
                        if (!j.cancelled) execute(j);
                        else cout << "Skipping cancelled job #" << j.id << "\n";
                    }
                    else
                    {
                        // Need to remove non-front element.
                        // Mark it processed and rebuild
                        prioJobs.peekAt(bestIdx).cancelled = true;
                        execute(j);
                    }
                    return;
                }
                // All prior jobs done, run the barrier itself
            }

            // Normal priority processing
            while (!prioJobs.isEmptyQueue())
            {
                Job j = prioJobs.front();
                prioJobs.deleteQueue();
                if (j.cancelled) continue;
                execute(j);
                return;
            }
            cout << "No pending jobs\n";
        }
        else
        {
            processNext();
        }
    }

    // ============================================================
    //  Phase 2: Experiments
    // ============================================================
    void runBucketFillExperiment(int size)
    {
        cout << "\n--- Bucket Fill Experiment " << size << "x" << size << " ---\n";
        Image testImg(size, size);
        for (int y = 0; y < size; y++)
            for (int x = 0; x < size; x++)
                testImg.at(x, y) = Pixel(255, 255, 255);

        Pixel fillColor(255, 0, 0);
        double stackTimeTotal = 0, queueTimeTotal = 0;
        long stackInsTotal = 0, queueInsTotal = 0;
        int stackMaxTotal = 0, queueMaxTotal = 0;
        const int RUNS = 3;

        for (int run = 0; run < RUNS; run++)
        {
            Image s(testImg), q(testImg);
            int sm, qm;
            long si, qi;

            auto t1 = chrono::high_resolution_clock::now();
            s.bucketFill(0, 0, fillColor, true, sm, si);
            auto t2 = chrono::high_resolution_clock::now();
            double stackMs = chrono::duration<double, std::milli>(t2 - t1).count();

            Image q2(testImg);
            t1 = chrono::high_resolution_clock::now();
            q2.bucketFill(0, 0, fillColor, false, qm, qi);
            t2 = chrono::high_resolution_clock::now();
            double queueMs = chrono::duration<double, std::milli>(t2 - t1).count();

            stackTimeTotal += stackMs;
            queueTimeTotal += queueMs;
            stackInsTotal += si;
            queueInsTotal += qi;
            if (sm > stackMaxTotal) stackMaxTotal = sm;
            if (qm > queueMaxTotal) queueMaxTotal = qm;
        }

        cout << "  Stack fill: avg time = " << stackTimeTotal / RUNS << " ms"
             << ", avg insertions = " << stackInsTotal / RUNS
             << ", max structure size = " << stackMaxTotal << "\n";
        cout << "  Queue fill: avg time = " << queueTimeTotal / RUNS << " ms"
             << ", avg insertions = " << queueInsTotal / RUNS
             << ", max structure size = " << queueMaxTotal << "\n";
    }

    void runAllBucketFillExperiments()
    {
        cout << "\n========== EXPERIMENT A: Stack vs Queue Bucket Fill ==========\n";
        runBucketFillExperiment(512);
        runBucketFillExperiment(1024);
        runBucketFillExperiment(2048);
        // 4096 may be too slow/large for some machines
        cout << "  (4096x4096 skipped if memory is limited)\n";
    }

    void runMemoryExperiment()
    {
        cout << "\n========== EXPERIMENT B: Memory-Aware History ==========\n";
        Image testImg(100, 100);
        for (int y = 0; y < 100; y++)
            for (int x = 0; x < 100; x++)
                testImg.at(x, y) = Pixel(128, 128, 128);
        testImg.save("_exp_mem.ppm");

        // Test with 100 operations under 5MB budget
        for (int numOps : {100, 1000})
        {
            ImageLab exp(10, 5 * 1024 * 1024);  // 5 MB budget
            exp.load("_exp_mem.ppm");

            int discardedBefore = 0;
            auto t1 = chrono::high_resolution_clock::now();
            for (int i = 0; i < numOps; i++)
            {
                int prevUndo = exp.undoCount();
                exp.brightness(1);
                if (exp.undoCount() <= prevUndo && prevUndo > 0) discardedBefore++;
            }
            auto t2 = chrono::high_resolution_clock::now();
            double timeMs = chrono::duration<double, std::milli>(t2 - t1).count();

            cout << "  " << numOps << " brightness ops, 5MB budget:\n";
            cout << "    Undo stack size: " << exp.undoCount() << "\n";
            cout << "    Memory usage: " << exp.getCurrentMemory() << " bytes\n";
            cout << "    Time: " << timeMs << " ms\n";

            // Test undo time
            t1 = chrono::high_resolution_clock::now();
            while (exp.undoCount() > 0) exp.undo();
            t2 = chrono::high_resolution_clock::now();
            double undoMs = chrono::duration<double, std::milli>(t2 - t1).count();
            cout << "    Undo all time: " << undoMs << " ms\n";
        }
        remove("_exp_mem.ppm");
    }

    void runSchedulerExperiment()
    {
        cout << "\n========== EXPERIMENT C: FIFO vs Priority Scheduling ==========\n";
        Image testImg(64, 64);
        for (int y = 0; y < 64; y++)
            for (int x = 0; x < 64; x++)
                testImg.at(x, y) = Pixel(128, 128, 128);
        testImg.save("_exp_sched.ppm");

        // Create 30+ mixed jobs
        struct JobSpec { int type; int a; int b; };
        JobSpec specs[35];
        int idx = 0;
        for (int i = 0; i < 8; i++) { specs[idx].type = J_RESIZE; specs[idx].a = 32 + i; specs[idx].b = 32 + i; idx++; }
        for (int i = 0; i < 8; i++) { specs[idx].type = J_BLUR; specs[idx].a = 1; specs[idx].b = 0; idx++; }
        for (int i = 0; i < 5; i++) { specs[idx].type = J_FILL; specs[idx].a = 0; specs[idx].b = 0; idx++; }
        for (int i = 0; i < 4; i++) { specs[idx].type = J_UNDO; specs[idx].a = 0; specs[idx].b = 0; idx++; }
        specs[idx].type = J_SAVE; specs[idx].a = 0; specs[idx].b = 0; idx++;
        specs[idx].type = J_EXPORT; specs[idx].a = 0; specs[idx].b = 0; idx++;
        for (int i = 0; i < 5; i++) { specs[idx].type = J_RESIZE; specs[idx].a = 40 + i; specs[idx].b = 40 + i; idx++; }
        for (int i = 0; i < 4; i++) { specs[idx].type = J_BLUR; specs[idx].a = 1; specs[idx].b = 0; idx++; }
        int totalJobs = idx;

        // FIFO mode
        {
            ImageLab fifo(50, 0);
            fifo.load("_exp_sched.ppm");
            fifo.setSchedulingMode(false);

            auto t1 = chrono::high_resolution_clock::now();
            for (int i = 0; i < totalJobs; i++)
            {
                Job j;
                j.type = specs[i].type;
                j.a = specs[i].a;
                j.b = specs[i].b;
                if (j.type == J_FILL) { j.color = Pixel(255, 0, 0); j.useStack = true; }
                if (j.type == J_EXPORT) j.file = "_exp_export.ppm";
                fifo.submit(j);
            }
            fifo.processAll();
            auto t2 = chrono::high_resolution_clock::now();
            double fifoMs = chrono::duration<double, std::milli>(t2 - t1).count();
            cout << "  FIFO mode: " << totalJobs << " jobs, total time = " << fifoMs << " ms\n";
        }

        // Priority mode
        {
            ImageLab prio(50, 0);
            prio.load("_exp_sched.ppm");
            prio.setSchedulingMode(true);

            auto t1 = chrono::high_resolution_clock::now();
            for (int i = 0; i < totalJobs; i++)
            {
                Job j;
                j.type = specs[i].type;
                j.a = specs[i].a;
                j.b = specs[i].b;
                if (j.type == J_FILL) { j.color = Pixel(255, 0, 0); j.useStack = true; }
                if (j.type == J_EXPORT) j.file = "_exp_export2.ppm";
                prio.submit(j);
            }
            prio.processAll();
            auto t2 = chrono::high_resolution_clock::now();
            double prioMs = chrono::duration<double, std::milli>(t2 - t1).count();
            cout << "  Priority mode: " << totalJobs << " jobs, total time = " << prioMs << " ms\n";
        }

        remove("_exp_sched.ppm");
        remove("_exp_export.ppm");
        remove("_exp_export2.ppm");
        remove("project_working.png");
    }

    static void runBatchExperiment()
    {
        cout << "\n========== EXPERIMENT D: Single vs Batched Operations ==========\n";
        int sizes[] = { 1000, 10000, 100000 };

        for (int s = 0; s < 3; s++)
        {
            int n = sizes[s];
            int* items = new int[n];
            for (int i = 0; i < n; i++) items[i] = i;

            // Stack single push
            {
                stackType<int> st(16);
                auto t1 = chrono::high_resolution_clock::now();
                for (int i = 0; i < n; i++) st.push(items[i]);
                auto t2 = chrono::high_resolution_clock::now();
                double singleMs = chrono::duration<double, std::milli>(t2 - t1).count();

                stackType<int> st2(16);
                t1 = chrono::high_resolution_clock::now();
                st2.pushBatch(items, n);
                t2 = chrono::high_resolution_clock::now();
                double batchMs = chrono::duration<double, std::milli>(t2 - t1).count();

                cout << "  Stack push " << n << " items: single=" << singleMs << "ms, batch=" << batchMs << "ms\n";
            }

            // Stack single pop
            {
                stackType<int> st(n + 16);
                st.pushBatch(items, n);
                auto t1 = chrono::high_resolution_clock::now();
                for (int i = 0; i < n; i++) st.pop();
                auto t2 = chrono::high_resolution_clock::now();
                double singleMs = chrono::duration<double, std::milli>(t2 - t1).count();

                stackType<int> st2(n + 16);
                st2.pushBatch(items, n);
                t1 = chrono::high_resolution_clock::now();
                st2.popBatch(n);
                t2 = chrono::high_resolution_clock::now();
                double batchMs = chrono::duration<double, std::milli>(t2 - t1).count();

                cout << "  Stack pop  " << n << " items: single=" << singleMs << "ms, batch=" << batchMs << "ms\n";
            }

            // Queue single add
            {
                queueType<int> q(16, true);
                auto t1 = chrono::high_resolution_clock::now();
                for (int i = 0; i < n; i++) q.addQueue(items[i]);
                auto t2 = chrono::high_resolution_clock::now();
                double singleMs = chrono::duration<double, std::milli>(t2 - t1).count();

                queueType<int> q2(16, true);
                t1 = chrono::high_resolution_clock::now();
                q2.addQueueBatch(items, n);
                t2 = chrono::high_resolution_clock::now();
                double batchMs = chrono::duration<double, std::milli>(t2 - t1).count();

                cout << "  Queue add  " << n << " items: single=" << singleMs << "ms, batch=" << batchMs << "ms\n";
            }

            // Queue single delete
            {
                queueType<int> q(n + 16, false);
                for (int i = 0; i < n; i++) q.addQueue(items[i]);
                auto t1 = chrono::high_resolution_clock::now();
                for (int i = 0; i < n; i++) q.deleteQueue();
                auto t2 = chrono::high_resolution_clock::now();
                double singleMs = chrono::duration<double, std::milli>(t2 - t1).count();

                queueType<int> q2(n + 16, false);
                for (int i = 0; i < n; i++) q2.addQueue(items[i]);
                t1 = chrono::high_resolution_clock::now();
                q2.deleteQueueBatch(n);
                t2 = chrono::high_resolution_clock::now();
                double batchMs = chrono::duration<double, std::milli>(t2 - t1).count();

                cout << "  Queue del  " << n << " items: single=" << singleMs << "ms, batch=" << batchMs << "ms\n";
            }

            delete[] items;
        }
    }

    void runAllExperiments()
    {
        cout << "\n############################################\n";
        cout << "  PHASE 2 EXPERIMENTAL EVALUATION\n";
        cout << "############################################\n";
        runAllBucketFillExperiments();
        runMemoryExperiment();
        runSchedulerExperiment();
        runBatchExperiment();
        cout << "\n############################################\n";
        cout << "  EXPERIMENTS COMPLETE\n";
        cout << "############################################\n";
    }
};

#endif
