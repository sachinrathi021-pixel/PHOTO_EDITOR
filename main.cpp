// ImageLab - Data Structures Assignment 2 (Phase 2)
// Custom data structures only: stackType, queueType, priorityQueueType.
// No standard containers, no enums, no recursion for bucket fill.
// stb_image/stb_image_write used ONLY for PNG/JPG file decoding/encoding.

#include "ImageLab.h"

// ============================================================
//  Console menu
// ============================================================
int askInt(const char* msg)
{
    int v;
    cout << msg;
    while (!(cin >> v))
    {
        if (cin.eof()) exit(0);
        cin.clear(); cin.ignore(10000, '\n');
        cout << "Enter a number: ";
    }
    return v;
}
string askStr(const char* msg)
{
    string s;
    cout << msg;
    // consume any leftover newline or whitespace before reading
    while (cin.peek() == '\n' || cin.peek() == '\r' || cin.peek() == ' ' || cin.peek() == '\t')
        cin.get();
    if (!getline(cin, s)) exit(0);
    // remove surrounding quotes if user typed them
    if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\'')))
        s = s.substr(1, s.size() - 2);
    return s;
}

void printMenu()
{
    cout << "\n------------- ImageLab (Phase 2) -------------\n"
         << " 1 Load image (PNG/JPG/PPM)  2 Image info\n"
         << " 3 Crop     4 Rotate     5 Flip     6 Brightness     7 Grayscale\n"
         << " 8 Undo     9 Redo\n"
         << "10 Queue Resize   11 Queue Blur   12 Queue Bucket fill\n"
         << "13 Queue Save     14 Queue Export\n"
         << "15 List jobs  16 Process next  17 Process all  18 Cancel job  19 Clear jobs\n"
         << "20 Inspector\n"
         << "--- Phase 2 ---\n"
         << "21 Set scheduling mode (FIFO/Priority)\n"
         << "22 Set memory budget\n"
         << "23 Run all experiments\n"
         << " 0 Exit\n> ";
}

int main()
{
    int cap = askInt("Maximum pending-job queue capacity (e.g. 10): ");
    long budget = (long)askInt("Memory budget in MB (0 = unlimited): ") * 1024 * 1024;
    ImageLab lab(cap, budget);

    while (true)
    {
        printMenu();
        int ch;
        if (!(cin >> ch)) { if (cin.eof()) break; cin.clear(); cin.ignore(10000, '\n'); continue; }
        if (ch == 0) break;

        switch (ch)
        {
        case 1: lab.load(askStr("File name: ")); break;
        case 2: lab.info(); break;
        case 3:
        {
            int x = askInt("x: "), y = askInt("y: "), w = askInt("width: "), h = askInt("height: ");
            lab.crop(x, y, w, h); break;
        }
        case 4: lab.rotate(askInt("Angle (90/180/270): ")); break;
        case 5: lab.flip(askInt("1 = horizontal, 2 = vertical: ") == 1); break;
        case 6: lab.brightness(askInt("Adjustment (+/-): ")); break;
        case 7: lab.grayscale(); break;
        case 8: lab.undo(); break;
        case 9: lab.redo(); break;
        case 10: { Job j; j.type = J_RESIZE; j.a = askInt("new width: "); j.b = askInt("new height: "); lab.submit(j); break; }
        case 11: { Job j; j.type = J_BLUR; j.a = askInt("blur radius: "); lab.submit(j); break; }
        case 12:
        {
            Job j; j.type = J_FILL;
            j.a = askInt("start x: "); j.b = askInt("start y: ");
            int r = askInt("R: "), g = askInt("G: "), b = askInt("B: ");
            j.color = Pixel(Image::clamp255(r), Image::clamp255(g), Image::clamp255(b));
            j.useStack = (askInt("1 = stack fill, 2 = queue fill: ") == 1);
            lab.submit(j); break;
        }
        case 13: { Job j; j.type = J_SAVE; lab.submit(j); break; }
        case 14: { Job j; j.type = J_EXPORT; j.file = askStr("Export file name (e.g. out.png): "); lab.submit(j); break; }
        case 15: lab.listJobs(); break;
        case 16: lab.processNext(); break;
        case 17: lab.processAll(); break;
        case 18: lab.cancel(askInt("Job id to cancel: ")); break;
        case 19: lab.clearJobs(); break;
        case 20: lab.inspector(); break;
        case 21: lab.setSchedulingMode(askInt("1 = FIFO, 2 = Priority: ") == 2); break;
        case 22: lab.setMemoryBudget((long)askInt("Memory budget in MB (0 = unlimited): ") * 1024 * 1024); break;
        case 23: lab.runAllExperiments(); break;
        default: cout << "Unknown option\n";
        }
    }
    cout << "Goodbye\n";
    return 0;
}
