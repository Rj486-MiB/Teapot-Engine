#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

struct Vec3 { float x, y, z; };
struct Face { 
    int v[4]; 
    char tex[16]; 
    unsigned int* pix;
};

Vec3* gV = NULL; 
Face* gF = NULL; 
unsigned int* gErrTex = NULL;
int gVC = 0, gFC = 0; 

char gMapName[64] = "Unknown";
bool gInit = false;
bool gUseExtConsole = false;
float gConfigFOV = 550.0f;
time_t gLastLogTime = 0;


inline float Cross(int x1, int y1, int x2, int y2, int x3, int y3) { 
    return (float)((x2 - x1) * (y3 - y1) - (y2 - y1) * (x3 - x1)); 
}

unsigned int ApplyFog(unsigned int color, float dist) {
    float factor = 1.0f - (dist / 2200.0f);
    if (factor < 0.0f) factor = 0.0f;
    if (factor > 1.0f) factor = 1.0f;
    unsigned char r = (unsigned char)(((color >> 16) & 0xFF) * factor);
    unsigned char g = (unsigned char)(((color >> 8) & 0xFF) * factor);
    unsigned char b = (unsigned char)((color & 0xFF) * factor);
    return (r << 16) | (g << 8) | b;
}

void InitRenderer() {
    if (gInit) return;

    FILE* cf = fopen("Config.ini", "r");
    if (cf) {
        int consoleInt = 0;
        fscanf(cf, "Width=%*d\nHeight=%*d\nFOV=%f\nConsole=%d", &gConfigFOV, &consoleInt);
        gUseExtConsole = (consoleInt == 1);
        fclose(cf);
    }

    if (gUseExtConsole) {
        AllocConsole();
        freopen("CONOUT$", "w", stdout);
    }

    gErrTex = new unsigned int[4096];
    for (int i = 0; i < 4096; i++) {
        gErrTex[i] = ((i / 64 / 8 + i % 64 / 8) % 2 == 0) ? 0xFF00FF : 0x000000;
    }

    FILE* f = fopen("bin/maps/Map.bs2", "rb");
    if (f) {
        int nameLen = 0;
        fread(&nameLen, 4, 1, f);
        if (nameLen > 0 && nameLen < 64) {
            fread(gMapName, 1, nameLen, f);
            gMapName[nameLen] = '\0';
        }

        fread(&gVC, 4, 1, f);
        gV = new Vec3[gVC];
        fread(gV, sizeof(Vec3), gVC, f);

        fread(&gFC, 4, 1, f);
        gF = new Face[gFC];
        for (int i = 0; i < gFC; i++) {
            fread(gF[i].v, 4, 4, f);
            fread(gF[i].tex, 1, 16, f);
            gF[i].pix = NULL;
        }
        fclose(f);
    }

    FILE* wad = fopen("bin/materials/Texture.wad", "rb");
    if (wad) {
        int texCount = 0;
        fread(&texCount, 4, 1, wad);
        for (int t = 0; t < texCount; t++) {
            char tName[16];
            int tOffset;
            fread(tName, 1, 16, wad);
            fread(&tOffset, 4, 1, wad);
            
            long currentPos = ftell(wad);
            fseek(wad, tOffset, SEEK_SET);
            
            unsigned int* buffer = new unsigned int[4096];
            fread(buffer, 4, 4096, wad);

            for (int i = 0; i < gFC; i++) {
                if (_stricmp(tName, gF[i].tex) == 0) {
                    gF[i].pix = new unsigned int[4096];
                    memcpy(gF[i].pix, buffer, 16384);
                }
            }
            delete[] buffer;
            fseek(wad, currentPos + 16384, SEEK_SET);
        }
        fclose(wad);
    }

    gInit = true;
}


extern "C" __declspec(dllexport) void RenderFrame(HWND hwnd, float px, float py, float pz, float ang, float lY) {
    if (!gInit) InitRenderer();

    RECT r; GetClientRect(hwnd, &r);
    int w = r.right, h = r.bottom;
    if (w <= 0 || h <= 0) return;

    unsigned int* screen = new unsigned int[w * h];
    float* zBuffer = new float[w * h];
    int horizon = h / 2 + (int)lY;

    for (int i = 0; i < w * h; i++) {
        zBuffer[i] = 10000.0f;
        screen[i] = (i / w < horizon) ? 0x151520 : 0x050505;
    }

    float cosA = (float)cos(-ang);
    float sinA = (float)sin(-ang);
    float texScale = 1.0f / 64.0f;

    for (int i = 0; i < gFC; i++) {
        int sx[4], sy[4];
        float rz[4], iz[4], uz[4], vz[4];
        bool visible = false;

        for (int j = 0; j < 4; j++) {
            float tx = gV[gF[i].v[j]].x - px;
            float ty = gV[gF[i].v[j]].y - py;
            float tz = gV[gF[i].v[j]].z - pz;

            float rx = tx * cosA - tz * sinA;
            rz[j] = tx * sinA + tz * cosA;

            if (rz[j] > 1.0f) visible = true;

            iz[j] = 1.0f / max(1.0f, rz[j]);
            uz[j] = (gV[gF[i].v[j]].x + gV[gF[i].v[j]].z) * texScale * iz[j];
            vz[j] = gV[gF[i].v[j]].y * texScale * iz[j];

            sx[j] = (int)(rx * gConfigFOV * iz[j]) + w / 2;
            sy[j] = horizon - (int)(ty * gConfigFOV * iz[j]);
        }

        if (!visible || Cross(sx[0], sy[0], sx[1], sy[1], sx[2], sy[2]) >= 0) continue;

        int xMin = max(0, min(sx[0], min(sx[1], min(sx[2], sx[3]))));
        int xMax = min(w - 1, max(sx[0], max(sx[1], max(sx[2], sx[3]))));
        int yMin = max(0, min(sy[0], min(sy[1], min(sy[2], sy[3]))));
        int yMax = min(h - 1, max(sy[0], max(sy[1], max(sy[2], sy[3]))));

        float area1 = Cross(sx[0], sy[0], sx[1], sy[1], sx[2], sy[2]);
        float area2 = Cross(sx[0], sy[0], sx[2], sy[2], sx[3], sy[3]);

        for (int y = yMin; y <= yMax; y++) {
            for (int x = xMin; x <= xMax; x++) {
                float w0 = Cross(sx[1], sy[1], sx[2], sy[2], x, y) / area1;
                float w1 = Cross(sx[2], sy[2], sx[0], sy[0], x, y) / area1;
                float w02 = Cross(sx[2], sy[2], sx[3], sy[3], x, y) / area2;
                float w12 = Cross(sx[3], sy[3], sx[0], sy[0], x, y) / area2;

                bool inTri1 = (w0 >= 0 && w1 >= 0 && (1 - w0 - w1) >= 0);
                bool inTri2 = (w02 >= 0 && w12 >= 0 && (1 - w02 - w12) >= 0);

                if (inTri1 || inTri2) {
                    float zInverted = inTri1 ? (w0 * iz[0] + w1 * iz[1] + (1 - w0 - w1) * iz[2]) :
                                               (w02 * iz[0] + w12 * iz[2] + (1 - w02 - w12) * iz[3]);
                    float z = 1.0f / zInverted;

                    if (z < zBuffer[y * w + x]) {
                        unsigned int* texture = gF[i].pix ? gF[i].pix : gErrTex;
                        float interpU = inTri1 ? (w0 * uz[0] + w1 * uz[1] + (1 - w0 - w1) * uz[2]) :
                                                 (w02 * uz[0] + w12 * uz[2] + (1 - w02 - w12) * uz[3]);
                        float interpV = inTri1 ? (w0 * vz[0] + w1 * vz[1] + (1 - w0 - w1) * vz[2]) :
                                                 (w02 * vz[0] + w12 * vz[2] + (1 - w02 - w12) * vz[3]);
                        
                        int tx_coord = (int)(interpU * z * 63) & 63;
                        int ty_coord = (int)(interpV * z * 63) & 63;

                        screen[y * w + x] = ApplyFog(texture[ty_coord * 64 + tx_coord], z);
                        zBuffer[y * w + x] = z;
                    }
                }
            }
        }
    }

    HDC hdc = GetDC(hwnd);
    BITMAPINFO bmi = {sizeof(BITMAPINFOHEADER), w, -h, 1, 32, BI_RGB};
    StretchDIBits(hdc, 0, 0, w, h, 0, 0, w, h, screen, &bmi, DIB_RGB_COLORS, SRCCOPY);

    if (gUseExtConsole) {
        if (time(NULL) - gLastLogTime >= 3) {
            system("cls");
            printf("SYSTEM LOG\n");
            printf("MAP LOADED: %s\n", gMapName);
            printf("PLAYER POS: X:%.1f Y:%.1f Z:%.1f\n", px, py, pz);
            printf("FOV       : %.0f\n", gConfigFOV);
            printf("--------------------------------\n");
            gLastLogTime = time(NULL);
        }
    } else {
        char uiBuf[128];
        sprintf(uiBuf, "Map: %s | X:%.0f Z:%.0f", gMapName, px, pz);
        SetTextColor(hdc, 0x00FF00);
        SetBkMode(hdc, TRANSPARENT);
        TextOut(hdc, 10, 10, uiBuf, strlen(uiBuf));
    }

    ReleaseDC(hwnd, hdc);
    delete[] screen;
    delete[] zBuffer;
}