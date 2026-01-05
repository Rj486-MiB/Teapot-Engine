#include <windows.h>
#include <math.h>
#include <stdio.h>

//STRUCTURES (It defines objects n stuff)

// A 3D point in space
struct Vec3 { float x, y, z; };

// A single wall or box side made of 4 vertices
struct Face { 
    int v[4];           // Indices: Which 4 points from the vertex array make this face
    char tex[16];       // The name of the texture ("brick" smt like that)
    unsigned int* pix;  // The actual pixel data (RAM cache) loaded from the WAD
};

// A Billboard sprite (2D image that always rotates to face the player)
struct Sprite { 
    float x, y, z;      // Current position
    float x1, z1, x2, z2; // Path start and end for MOVABLE types
    float speed, timer; // How fast it moves and its animation progress
    char tex[16]; 
    unsigned int* pix; 
    bool isMovable;     // 1 if it moves, 0 if it stays still
};

// GLOBALS
Vec3* gV = NULL;   Face* gF = NULL;   Sprite* gS = NULL; // Arrays to hold world
int gVC = 0, gFC = 0, gSC = 0;                           // Counters for how many objects exist
unsigned int* gErr = NULL;                               // Checkerboard pattern for missing textures
char gMapName[64] = "Teapot Level";
bool gInit = false;                                      // Stops the engine from re-loading every frame

// Cross Product: Used to determine if a wall is facing the player or facing away (Backface Culling)
inline float Cross(int x1, int y1, int x2, int y2, int x3, int y3) { 
    return (float)((x2 - x1) * (y3 - y1) - (y2 - y1) * (x3 - x1)); 
}

// Distance Fog: Gradually turns pixels black the further away they are from the player
unsigned int Fog(unsigned int c, float d) {
    float f = 1.0f - (d / 2200.0f); // 2200 units is the "Max View Distance"
    if (f < 0) f = 0; if (f > 1) f = 1;
    unsigned int r = ((c >> 16) & 255) * f;
    unsigned int g = ((c >> 8) & 255) * f;
    unsigned int b = (c & 255) * f;
    return (r << 16) | (g << 8) | b;
}

//INITIALIZATION (Loading Stuff)
void Init() {
    if (gInit) return;
    
    // Create the pink/black "Error" texture (Classic :3)
    gErr = new unsigned int[4096];
    for (int i = 0; i < 4096; i++) gErr[i] = ((i / 64 / 8 + i % 64 / 8) % 2 == 0) ? 0xFF00FF : 0x000000;

    // LOAD MAP (.bs2)
    FILE* f = fopen("bin/maps/Map.bs2", "rb");
    if (f) {
        int nLen; fread(&nLen, 4, 1, f);
        fread(gMapName, 1, nLen, f); gMapName[nLen] = '\0';
        
        fread(&gVC, 4, 1, f); gV = new Vec3[gVC]; fread(gV, sizeof(Vec3), gVC, f);
        fread(&gFC, 4, 1, f); gF = new Face[gFC];
        for (int i = 0; i < gFC; i++) {
            fread(gF[i].v, 4, 4, f); 
            fread(gF[i].tex, 1, 16, f); 
            gF[i].pix = NULL;
        }

        // Load Sprites/Objects written at the end of the file
        if (!feof(f)) {
            fread(&gSC, 4, 1, f);
            gS = new Sprite[gSC];
            for (int i = 0; i < gSC; i++) {
                fread(&gS[i].isMovable, sizeof(bool), 1, f);
                if (gS[i].isMovable) {
                    fread(&gS[i].x1, 4, 1, f); fread(&gS[i].z1, 4, 1, f);
                    fread(&gS[i].x2, 4, 1, f); fread(&gS[i].z2, 4, 1, f);
                    fread(&gS[i].y, 4, 1, f);  fread(&gS[i].speed, 4, 1, f);
                    gS[i].x = gS[i].x1; gS[i].z = gS[i].z1; // Start at point 1
                } else {
                    fread(&gS[i].x, 4, 1, f); fread(&gS[i].y, 4, 1, f); fread(&gS[i].z, 4, 1, f);
                }
                fread(gS[i].tex, 1, 16, f);
                gS[i].pix = NULL; gS[i].timer = 0;
            }
        }
        fclose(f);
    }

    // LOAD WAD (.wad)
    FILE* wad = fopen("bin/materials/Texture.wad", "rb");
    if (wad) {
        int texCount; 
        if(fread(&texCount, 4, 1, wad)) {
            struct WadEntry { char name[16]; int offset; };
            WadEntry* dir = new WadEntry[texCount];
            for (int i = 0; i < texCount; i++) {
                fread(dir[i].name, 1, 16, wad);
                fread(&dir[i].offset, 4, 1, wad);
            }
            // Match the pixels to the objects in map
            for (int i = 0; i < texCount; i++) {
                fseek(wad, dir[i].offset, SEEK_SET);
                unsigned int* rawPixels = new unsigned int[4096]; // 64x64 textures
                fread(rawPixels, 4, 4096, wad);
                for (int j = 0; j < gFC; j++) if (!_stricmp(dir[i].name, gF[j].tex)) {
                    gF[j].pix = new unsigned int[4096]; memcpy(gF[j].pix, rawPixels, 16384);
                }
                for (int j = 0; j < gSC; j++) if (!_stricmp(dir[i].name, gS[j].tex)) {
                    gS[j].pix = new unsigned int[4096]; memcpy(gS[j].pix, rawPixels, 16384);
                }
                delete[] rawPixels;
            }
            delete[] dir;
        }
        fclose(wad);
    }
    gInit = true;
}

// RENDERING
extern "C" __declspec(dllexport) void RenderFrame(HWND hwnd, float px, float py, float pz, float ang, float lY, int fps) {
    if (!gInit) Init();
    RECT r; GetClientRect(hwnd, &r); int w = r.right, h = r.bottom;
    if (w <= 0 || h <= 0) return;

    unsigned int* scr = new unsigned int[w * h]; // Pixel buffer
    float* zb = new float[w * h];                 // Z-Buffer: Prevents walls from drawing over sprites
    int hor = h / 2 + (int)lY;                   // The Horizon: Shifts up/down when looking

    // --- STEP 1: DRAW BACKGROUND (Sky and Floor) ---
    for (int i = 0; i < w * h; i++) {
        zb[i] = 10000.0f; // Reset Z-Buffer to "infinite" distance
        if ((i / w) < hor) scr[i] = 0x050505; // SKY: Top half (Black)
        else               scr[i] = 0x151520; // FLOOR: Bottom half (Dark Gray)
    }

    float cA = cos(-ang), sA = sin(-ang);

    //RENDER WALLS & BOXES
    for (int i = 0; i < gFC; i++) {
        int sx[4], sy[4]; float rz[4], iz[4], uz[4], vz[4]; bool ok = 0;
        for (int j = 0; j < 4; j++) {
            // Translate 3D position relative to Player
            float tx = gV[gF[i].v[j]].x - px, ty = gV[gF[i].v[j]].y - py, tz = gV[gF[i].v[j]].z - pz;
            // Rotate based on Camera Angle
            float rx = tx * cA - tz * sA; rz[j] = tx * sA + tz * cA; if (rz[j] > 1.0f) ok = 1;
            iz[j] = 1.0f / max(1.0f, rz[j]); // 1/Z: Used for perspective projection
            
            // Texture coordinates (UV mapping)
            uz[j] = (gV[gF[i].v[j]].x + gV[gF[i].v[j]].z) * (1.0f / 64.0f) * iz[j];
            vz[j] = gV[gF[i].v[j]].y * (1.0f / 64.0f) * iz[j];
            
            // Screen Projection: Turn 3D into 2D (X, Y) pixels
            sx[j] = (int)(rx * 550.0f * iz[j]) + w / 2; 
            sy[j] = hor - (int)(ty * 550.0f * iz[j]);
        }
        
        // Don't draw if face is behind player or facing away
        if (!ok || Cross(sx[0], sy[0], sx[1], sy[1], sx[2], sy[2]) >= 0) continue;
        
        // Scanline Rasterization: Fill the face with pixels
        int xn = max(0, min(sx[0], min(sx[1], min(sx[2], sx[3])))), xx = min(w - 1, max(sx[0], max(sx[1], max(sx[2], sx[3]))));
        int yn = max(0, min(sy[0], min(sy[1], min(sy[2], sy[3])))), yx = min(h - 1, max(sy[0], max(sy[1], max(sy[2], sy[3]))));
        float a1 = Cross(sx[0], sy[0], sx[1], sy[1], sx[2], sy[2]), a2 = Cross(sx[0], sy[0], sx[2], sy[2], sx[3], sy[3]);
        
        for (int y = yn; y <= yx; y++) {
            for (int x = xn; x <= xx; x++) {
                float w0 = Cross(sx[1], sy[1], sx[2], sy[2], x, y) / a1, w1 = Cross(sx[2], sy[2], sx[0], sy[0], x, y) / a1;
                float w02 = Cross(sx[2], sy[2], sx[3], sy[3], x, y) / a2, w12 = Cross(sx[3], sy[3], sx[0], sy[0], x, y) / a2;
                if ((w0 >= 0 && w1 >= 0 && 1 - w0 - w1 >= 0) || (w02 >= 0 && w12 >= 0 && 1 - w02 - w12 >= 0)) {
                    float z = 1.0f / ((w0 >= 0) ? (w0 * iz[0] + w1 * iz[1] + (1 - w0 - w1) * iz[2]) : (w02 * iz[0] + w12 * iz[2] + (1 - w02 - w12) * iz[3]));
                    if (z < zb[y * w + x]) {
                        unsigned int* p = gF[i].pix ? gF[i].pix : gErr;
                        float iU = (w0 >= 0) ? (w0 * uz[0] + w1 * uz[1] + (1 - w0 - w1) * uz[2]) : (w02 * uz[0] + w12 * uz[2] + (1 - w02 - w12) * uz[3]);
                        float iV = (w0 >= 0) ? (w0 * vz[0] + w1 * vz[1] + (1 - w0 - w1) * vz[2]) : (w02 * vz[0] + w12 * vz[2] + (1 - w02 - w12) * vz[3]);
                        scr[y * w + x] = Fog(p[((int)(iV * z * 63) & 63) * 64 + ((int)(iU * z * 63) & 63)], z);
                        zb[y * w + x] = z;
                    }
                }
            }
        }
    }

    // RENDER SPRITES
    for (int i = 0; i < gSC; i++) {
        // Handle Sprite_Movable logic
        if (gS[i].isMovable) {
            gS[i].timer += gS[i].speed;
            float lerp = (float)sin(gS[i].timer) * 0.5f + 0.5f; // Oscillates between 0 and 1
            gS[i].x = gS[i].x1 + (gS[i].x2 - gS[i].x1) * lerp;
            gS[i].z = gS[i].z1 + (gS[i].z2 - gS[i].z1) * lerp;
        }

        float tx = gS[i].x - px, ty = gS[i].y - py, tz = gS[i].z - pz;
        float rx = tx * cA - tz * sA, rz = tx * sA + tz * cA;
        if (rz < 10.0f) continue; // Behind player clipping

        int sx = (int)(rx * 550.0f / rz) + w / 2, sy = hor - (int)(ty * 550.0f / rz);
        int size = (int)(550.0f * 64.0f / rz); // Shrinks as distance (rz) grows

        // Simple Sprite Drawing
        for (int y = max(0, sy - size); y < min(h, sy); y++) {
            for (int x = max(0, sx - size / 2); x < min(w, sx + size / 2); x++) {
                if (rz < zb[y * w + x]) {
                    unsigned int* p = gS[i].pix ? gS[i].pix : gErr;
                    int tX = (x - (sx - size / 2)) * 64 / size;
                    int tY = (y - (sy - size)) * 64 / size;
                    unsigned int col = p[(tY & 63) * 64 + (tX & 63)];
                    if (col != 0xFF00FF) { // If pixel isn't PINK, draw it (transparency)
                        scr[y * w + x] = Fog(col, rz); zb[y * w + x] = rz; 
                    }
                }
            }
        }
    }

    // OUTPUT TO WINDOW
    HDC hdc = GetDC(hwnd);
    BITMAPINFO bmi = {sizeof(BITMAPINFOHEADER), w, -h, 1, 32, BI_RGB};
    StretchDIBits(hdc, 0, 0, w, h, 0, 0, w, h, scr, &bmi, DIB_RGB_COLORS, SRCCOPY);

    // DRAW OSD (HUD)
    SetTextColor(hdc, 0x00FF00); SetBkMode(hdc, TRANSPARENT); SetTextAlign(hdc, TA_RIGHT);
    char b[128];
    sprintf(b, "FPS: %d | Map: %s", fps, gMapName); TextOut(hdc, w - 10, 10, b, strlen(b));
    sprintf(b, "Pos: %.1f, %.1f, %.1f | Ang: %.2f", px, py, pz, ang); TextOut(hdc, w - 10, 30, b, strlen(b));

    ReleaseDC(hwnd, hdc);
    delete[] scr; delete[] zb;
}
/* Ignore this or idc whatever

RUYB//

When the demon heart is crying
And the blood is gushing bright
Raise up your bat for the burning fight
When your hope is slowly dying
And your future's lost its rights
Raise up your bat and face the fright
Let's knock 'em dead into the night

[Bridge]
Come follow me into the dark
With your heart as the ark
Which shall shine you the way
Because I'm with you in the dark
With your hеart as my mark
Which shall guide you the way, through the wavеs
Come follow me into the dark
With your heart as the ark
Which shall shine you the way
Because I'm with you in the dark
With your heart as my mark
Which shall guide you the way, through the waves

[Chorus]
When the demon heart is crying
And the blood is gushing bright
Raise up your bat for the burning fight
When your hope is slowly dying
And your future's lost its rights
Raise up your bat and face the fright
Let's knock 'em dead into the night

*/