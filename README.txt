ImageLab - Data Structures Assignment 2 (Phase 2)
FAST University, CS-3H
Student: Sachin Kumar (25i-0813)

==========================================================
  BUILD INSTRUCTIONS
==========================================================

Compiler:  g++ (MinGW GCC 6.3.0+) with C++11 support
Platform:  Windows

Dependencies:
  - stb_image.h and stb_image_write.h (included in src/)
    Single-header libraries for PNG/JPG decoding/encoding.
    Source: https://github.com/nothings/stb
    These libraries are used ONLY for file I/O (read/write),
    not for any image-processing operations.

Build the application:
  g++ -std=c++11 -O2 -o imagelab.exe src/main.cpp src/stb_impl.cpp

Build the test program:
  g++ -std=c++11 -O2 -o tests_p2.exe tests/test_main.cpp src/stb_impl.cpp

==========================================================
  RUN INSTRUCTIONS
==========================================================

Run the application:
  .\imagelab.exe
  - You will be asked for queue capacity and memory budget.
  - Then a menu with 23 options appears.
  - Load images using option 1 (supports PNG, JPG, PPM).

Run all tests:
  .\tests_p2.exe
  - Runs 123 automated tests covering all data structures,
    image operations, undo/redo, memory budget, coalescing,
    priority scheduling, batch operations, and inspector.
  - Exit code 0 means all tests pass.

Run experiments (from inside the application):
  - Option 23 runs all Phase 2 experiments:
    A) Stack vs Queue bucket fill at multiple sizes
    B) Memory-aware history under budget
    C) FIFO vs Priority scheduling with 30+ jobs
    D) Single vs Batched operations (1K, 10K, 100K)

==========================================================
  SUPPORTED IMAGE FORMATS
==========================================================

Loading: PNG, JPG/JPEG, PPM (P3/P6)
Saving:  PNG (.png), JPG (.jpg/.jpeg), PPM (.ppm)
Format is auto-detected by file extension.

==========================================================
  PROJECT STRUCTURE
==========================================================

25i-0813_H_A2_P2/
  src/
    common.h            - Pixel, Point structs, validSize()
    stackType.h         - Custom stack (dynamic array, batch ops)
    queueType.h         - Custom circular queue (batch ops)
    priorityQueueType.h - Custom priority queue (ordered array)
    Image.h             - Image class, all operations, bucket fill
    ImageLab.h          - Application controller, Phase 2 features
    main.cpp            - Console menu entry point
    stb_image.h         - stb image decoder (3rd party, read-only)
    stb_image_write.h   - stb image writer (3rd party, read-only)
    stb_impl.cpp        - stb implementation translation unit
  tests/
    test_main.cpp       - Automated test suite (123 tests)
  images/
    (test images)
  report/
    (Phase 2 PDF report)
  README.txt            - This file

==========================================================
  PHASE 2 FEATURES
==========================================================

1. Memory-Aware History
   - Compact operation-specific undo records
   - Rotate/Flip: store only parameters (no pixel data)
   - Bucket Fill: store only changed pixel coords + originals
   - Other ops: store full pre-operation image
   - Configurable memory budget with oldest-record eviction

2. Brightness Coalescing
   - Consecutive brightness operations merge into one record
   - Non-brightness ops break the chain
   - Clipping handled correctly via stored original pixels

3. Priority Scheduling
   - Toggle between FIFO (queueType) and Priority (priorityQueueType)
   - Priority levels: Undo/Redo=1, Blur/Fill=2, Resize=3, Save/Export=4
   - FIFO tie-breaking for same-priority jobs
   - Finalization barriers for Save/Export jobs

4. Batch Operations
   - pushBatch/popBatch on stackType
   - addQueueBatch/deleteQueueBatch on queueType
   - Single capacity check, block insert

5. Experimental Evaluation
   - Experiment A: Stack vs Queue bucket fill performance
   - Experiment B: Memory-aware history under budget
   - Experiment C: FIFO vs Priority scheduler comparison
   - Experiment D: Single vs Batched operation benchmarks

==========================================================
  RESTRICTIONS FOLLOWED
==========================================================

- No std::vector, std::list, std::stack, std::queue, etc.
- No enums (const int used instead)
- No recursion for bucket fill
- No image library functions for processing operations
- No threads or concurrency libraries
- stb used ONLY for PNG/JPG file decoding/encoding
