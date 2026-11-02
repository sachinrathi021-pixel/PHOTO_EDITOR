#ifndef IMAGE_H_
#define IMAGE_H_

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include "common.h"
#include "stackType.h"
#include "queueType.h"

// stb_image headers for PNG/JPG decoding/encoding ONLY (no image processing)
#include "stb_image.h"
#include "stb_image_write.h"

using namespace std;

// ============================================================
//  Image  (1-D pixel array, index = y * width + x)
//  Supports PPM (P3/P6), PNG, and JPG loading/saving.
// ============================================================
class Image
{
private:
    int w, h;
    Pixel* px;

    void copyFrom(const Image& o)
    {
        w = o.w; h = o.h;
        px = (w * h > 0) ? new Pixel[w * h] : 0;
        for (int i = 0; i < w * h; i++) px[i] = o.px[i];
    }

    static bool readToken(ifstream& f, string& t)
    {
        const int END = ifstream::traits_type::eof();
        t = "";
        while (true)
        {
            int c = f.peek();
            if (c == END) break;
            if (c == '#')
            {
                if (!t.empty()) break;
                while (f.peek() != END && f.peek() != '\n') f.get();
                continue;
            }
            if (c == ' ' || c == '\n' || c == '\r' || c == '\t')
            {
                if (!t.empty()) break;
                f.get();
                continue;
            }
            t += (char)f.get();
        }
        return !t.empty();
    }

    static unsigned char scaleChannel(int v, int maxval)
    {
        if (v < 0) v = 0;
        if (v > maxval) v = maxval;
        return (unsigned char)((v * 255 + maxval / 2) / maxval);
    }

    // Check file extension (case-insensitive)
    static bool endsWith(const string& s, const char* ext)
    {
        int slen = (int)s.size();
        int elen = 0;
        while (ext[elen]) elen++;
        if (slen < elen) return false;
        for (int i = 0; i < elen; i++)
        {
            char a = s[slen - elen + i];
            char b = ext[i];
            if (a >= 'A' && a <= 'Z') a += 32;
            if (b >= 'A' && b <= 'Z') b += 32;
            if (a != b) return false;
        }
        return true;
    }

    static bool isPPM(const string& name)
    {
        return endsWith(name, ".ppm");
    }
    static bool isPNG(const string& name)
    {
        return endsWith(name, ".png");
    }
    static bool isJPG(const string& name)
    {
        return endsWith(name, ".jpg") || endsWith(name, ".jpeg");
    }

public:
    Image() : w(0), h(0), px(0) {}
    Image(int W, int H) : w(0), h(0), px(0)
    {
        if (validSize(W, H)) { w = W; h = H; px = new Pixel[W * H]; }
    }
    Image(const Image& o) { copyFrom(o); }
    Image& operator=(const Image& o)
    {
        if (this != &o) { delete[] px; copyFrom(o); }
        return *this;
    }
    ~Image() { delete[] px; }

    int width() const { return w; }
    int height() const { return h; }
    bool empty() const { return w == 0 || h == 0; }
    Pixel& at(int x, int y) { return px[y * w + x]; }
    const Pixel& at(int x, int y) const { return px[y * w + x]; }
    bool inBounds(int x, int y) const { return x >= 0 && y >= 0 && x < w && y < h; }
    long bytes() const { return (long)w * h * 3; }

    // Raw pixel pointer for direct access (used in history records)
    Pixel* rawPixels() { return px; }
    const Pixel* rawPixels() const { return px; }

    // ---------- file I/O: PPM, PNG, JPG ----------
    bool loadPPM(const string& name)
    {
        ifstream f(name.c_str(), ios::binary);
        if (!f) { cout << "Cannot open " << name << "\n"; return false; }
        string magic, sw, sh, sm;
        if (!readToken(f, magic) || !readToken(f, sw) || !readToken(f, sh) || !readToken(f, sm))
        { cout << "Bad PPM header\n"; return false; }
        long long W = strtoll(sw.c_str(), 0, 10), H = strtoll(sh.c_str(), 0, 10);
        long long M = strtoll(sm.c_str(), 0, 10);
        if (magic != "P3" && magic != "P6")
        { cout << "Unsupported PPM (need P3 or P6)\n"; return false; }
        if (M <= 0 || M > 255)
        { cout << "Unsupported PPM (maxval must be 1..255)\n"; return false; }
        if (!validSize(W, H))
        { cout << "Invalid or too large image size in header\n"; return false; }

        int n = (int)(W * H);
        Pixel* np = new Pixel[n];
        if (magic == "P3")
        {
            for (int i = 0; i < n; i++)
            {
                string a, b, c;
                if (!readToken(f, a) || !readToken(f, b) || !readToken(f, c))
                { delete[] np; cout << "PPM data truncated\n"; return false; }
                np[i] = Pixel(scaleChannel(atoi(a.c_str()), (int)M), scaleChannel(atoi(b.c_str()), (int)M),
                              scaleChannel(atoi(c.c_str()), (int)M));
            }
        }
        else
        {
            char d;
            if (!f.get(d)) { delete[] np; cout << "PPM data truncated\n"; return false; }
            if (d == '\r' && f.peek() == '\n') f.get();
            for (int i = 0; i < n; i++)
            {
                char c[3];
                if (!f.read(c, 3)) { delete[] np; cout << "PPM data truncated\n"; return false; }
                np[i] = Pixel(scaleChannel((unsigned char)c[0], (int)M), scaleChannel((unsigned char)c[1], (int)M),
                              scaleChannel((unsigned char)c[2], (int)M));
            }
        }
        delete[] px;
        px = np; w = (int)W; h = (int)H;
        return true;
    }

    bool loadStb(const string& name)
    {
        int W, H, channels;
        unsigned char* data = stbi_load(name.c_str(), &W, &H, &channels, 3);
        if (!data) { cout << "Cannot load " << name << ": " << stbi_failure_reason() << "\n"; return false; }
        if (!validSize(W, H)) { stbi_image_free(data); cout << "Image too large\n"; return false; }
        int n = W * H;
        Pixel* np = new Pixel[n];
        for (int i = 0; i < n; i++)
            np[i] = Pixel(data[i * 3], data[i * 3 + 1], data[i * 3 + 2]);
        stbi_image_free(data);
        delete[] px;
        px = np; w = W; h = H;
        return true;
    }

    bool load(const string& name)
    {
        if (isPPM(name)) return loadPPM(name);
        return loadStb(name);  // PNG, JPG, etc.
    }

    bool savePPM(const string& name) const
    {
        if (empty()) { cout << "No image to save\n"; return false; }
        ofstream f(name.c_str(), ios::binary);
        if (!f) { cout << "Cannot write " << name << "\n"; return false; }
        f << "P6\n" << w << " " << h << "\n255\n";
        for (int i = 0; i < w * h; i++)
        {
            f.put((char)px[i].r); f.put((char)px[i].g); f.put((char)px[i].b);
        }
        return true;
    }

    bool savePNG(const string& name) const
    {
        if (empty()) { cout << "No image to save\n"; return false; }
        int n = w * h;
        unsigned char* data = new unsigned char[n * 3];
        for (int i = 0; i < n; i++)
        {
            data[i * 3] = px[i].r;
            data[i * 3 + 1] = px[i].g;
            data[i * 3 + 2] = px[i].b;
        }
        int ok = stbi_write_png(name.c_str(), w, h, 3, data, w * 3);
        delete[] data;
        if (!ok) { cout << "Failed to write " << name << "\n"; return false; }
        return true;
    }

    bool saveJPG(const string& name) const
    {
        if (empty()) { cout << "No image to save\n"; return false; }
        int n = w * h;
        unsigned char* data = new unsigned char[n * 3];
        for (int i = 0; i < n; i++)
        {
            data[i * 3] = px[i].r;
            data[i * 3 + 1] = px[i].g;
            data[i * 3 + 2] = px[i].b;
        }
        int ok = stbi_write_jpg(name.c_str(), w, h, 3, data, 90);
        delete[] data;
        if (!ok) { cout << "Failed to write " << name << "\n"; return false; }
        return true;
    }

    bool save(const string& name) const
    {
        if (isPNG(name)) return savePNG(name);
        if (isJPG(name)) return saveJPG(name);
        return savePPM(name);  // default to PPM
    }

    // ---------- instant operations ----------
    bool crop(int x, int y, int cw, int ch)
    {
        if (cw <= 0 || ch <= 0 || x < 0 || y < 0 || x + cw > w || y + ch > h)
        { cout << "Crop region is outside the image\n"; return false; }
        Image out(cw, ch);
        for (int j = 0; j < ch; j++)
            for (int i = 0; i < cw; i++) out.at(i, j) = at(x + i, y + j);
        *this = out;
        return true;
    }

    bool rotate(int angle)
    {
        if (angle == 90)
        {
            Image out(h, w);
            for (int y = 0; y < h; y++)
                for (int x = 0; x < w; x++) out.at(h - 1 - y, x) = at(x, y);
            *this = out;
        }
        else if (angle == 180)
        {
            Image out(w, h);
            for (int y = 0; y < h; y++)
                for (int x = 0; x < w; x++) out.at(w - 1 - x, h - 1 - y) = at(x, y);
            *this = out;
        }
        else if (angle == 270)
        {
            Image out(h, w);
            for (int y = 0; y < h; y++)
                for (int x = 0; x < w; x++) out.at(y, w - 1 - x) = at(x, y);
            *this = out;
        }
        else { cout << "Angle must be 90, 180 or 270\n"; return false; }
        return true;
    }

    bool flip(bool horizontal)
    {
        Image out(w, h);
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                out.at(x, y) = horizontal ? at(w - 1 - x, y) : at(x, h - 1 - y);
        *this = out;
        return true;
    }

    static unsigned char clamp255(int v) { return (unsigned char)(v < 0 ? 0 : (v > 255 ? 255 : v)); }

    bool brightness(int delta)
    {
        for (int i = 0; i < w * h; i++)
        {
            px[i].r = clamp255(px[i].r + delta);
            px[i].g = clamp255(px[i].g + delta);
            px[i].b = clamp255(px[i].b + delta);
        }
        return true;
    }

    bool grayscale()
    {
        for (int i = 0; i < w * h; i++)
        {
            int g = (px[i].r * 299 + px[i].g * 587 + px[i].b * 114) / 1000;
            px[i] = Pixel((unsigned char)g, (unsigned char)g, (unsigned char)g);
        }
        return true;
    }

    // ---------- queued operations ----------
    bool resize(int nw, int nh)
    {
        if (!validSize(nw, nh)) { cout << "Invalid or too large size\n"; return false; }
        Image out(nw, nh);
        for (int y = 0; y < nh; y++)
            for (int x = 0; x < nw; x++)
                out.at(x, y) = at(x * w / nw, y * h / nh);
        *this = out;
        return true;
    }

    bool blur(int radius)
    {
        if (radius <= 0) { cout << "Blur radius must be >= 1\n"; return false; }
        Image src(*this);
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                int sr = 0, sg = 0, sb = 0, n = 0;
                for (int dy = -radius; dy <= radius; dy++)
                    for (int dx = -radius; dx <= radius; dx++)
                    {
                        int nx = x + dx, ny = y + dy;
                        if (!src.inBounds(nx, ny)) continue;
                        const Pixel& p = src.at(nx, ny);
                        sr += p.r; sg += p.g; sb += p.b; n++;
                    }
                at(x, y) = Pixel((unsigned char)(sr / n), (unsigned char)(sg / n), (unsigned char)(sb / n));
            }
        return true;
    }

    // ---------- bucket fill (NO recursion) ----------
    long bucketFill(int sx, int sy, Pixel nc, bool useStack, int& maxStruct, long& inserts)
    {
        maxStruct = 0; inserts = 0;
        if (!inBounds(sx, sy)) { cout << "Start pixel is outside the image\n"; return -1; }
        Pixel orig = at(sx, sy);
        if (orig == nc) return 0;

        const int dx[4] = { 0, 0, -1, 1 };
        const int dy[4] = { -1, 1, 0, 0 };
        long changed = 0;

        if (useStack)
        {
            stackType<Point> s(1024);
            at(sx, sy) = nc; changed++;
            s.push(Point(sx, sy)); inserts++;
            while (!s.isEmptyStack())
            {
                Point c = s.top(); s.pop();
                for (int d = 0; d < 4; d++)
                {
                    int nx = c.x + dx[d], ny = c.y + dy[d];
                    if (inBounds(nx, ny) && at(nx, ny) == orig)
                    {
                        at(nx, ny) = nc; changed++;
                        s.push(Point(nx, ny)); inserts++;
                    }
                }
            }
            maxStruct = s.getMaxSize();
        }
        else
        {
            queueType<Point> q(1024, true);
            at(sx, sy) = nc; changed++;
            q.addQueue(Point(sx, sy)); inserts++;
            while (!q.isEmptyQueue())
            {
                Point c = q.front(); q.deleteQueue();
                for (int d = 0; d < 4; d++)
                {
                    int nx = c.x + dx[d], ny = c.y + dy[d];
                    if (inBounds(nx, ny) && at(nx, ny) == orig)
                    {
                        at(nx, ny) = nc; changed++;
                        q.addQueue(Point(nx, ny)); inserts++;
                    }
                }
            }
            maxStruct = q.getMaxSize();
        }
        return changed;
    }
};

#endif
