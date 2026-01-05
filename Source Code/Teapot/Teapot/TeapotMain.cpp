#include <windows.h>
#include <math.h>
#include <stdio.h>

typedef void (*REND)(HWND, float, float, float, float, float);
REND RenderFrame = NULL;
HINSTANCE hDLL = NULL;

int gW = 800, gH = 600, gConsoleMode = 1;
float gFOV = 550.0f;

void LoadConfig() {
    FILE* f = fopen("Config.ini", "r");
    if(f) {
        fscanf(f, "Width=%d\nHeight=%d\nFOV=%f\nConsole=%d", &gW, &gH, &gFOV, &gConsoleMode);
        fclose(f);
    }
}

LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if(m == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProc(h, m, w, l);
}

int WINAPI WinMain(HINSTANCE hI, HINSTANCE hP, LPSTR lp, int nS) {
    LoadConfig();

    if (GetFileAttributes("bin/maps/Map.bs2") == INVALID_FILE_ATTRIBUTES) {
        MessageBox(0, "CRITICAL ERROR: bin/maps/Map.bs2 is missing!", "Teapot Engine", MB_ICONERROR);
    }
    if (GetFileAttributes("bin/materials/Texture.wad") == INVALID_FILE_ATTRIBUTES) {
        MessageBox(0, "WARNING: Texture.wad missing.", "Teapot Engine", MB_ICONWARNING);
    }

    hDLL = LoadLibrary("bin/Software.dll");
    if(!hDLL) {
        MessageBox(0, "FATAL ERROR: Software.dll not found in bin/!", "Teapot Engine", MB_ICONERROR);
        return 1;
    }
    RenderFrame = (REND)GetProcAddress(hDLL, "RenderFrame");

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hI;
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.lpszClassName = "TeapotEngine";
    RegisterClass(&wc);

    HWND hwnd = CreateWindow("TeapotEngine", "Teapot Engine - Fart Edition", 
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, gW, gH, 0, 0, hI, 0);

    float px=0, py=60, pz=-400, ang=0, lY=0;
    MSG msg = {0};

    while(msg.message != WM_QUIT) {
        if(PeekMessage(&msg, 0, 0, 0, 1)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            if(GetAsyncKeyState('R')) { px=0; py=60; pz=-400; ang=0; lY=0; }

            static bool tKey = false;
            if(GetAsyncKeyState(VK_OEM_3)) {
                if(!tKey) { gConsoleMode = (gConsoleMode == 1 ? 0 : 1); tKey = true; }
            } else { tKey = false; }

            float speed = GetAsyncKeyState(VK_SHIFT) ? 15.0f : 7.0f;

            float dirX = (float)sin(ang);
            float dirZ = (float)cos(ang);

            if(GetAsyncKeyState('W')) { px += dirX * speed; pz += dirZ * speed; }
            if(GetAsyncKeyState('S')) { px -= dirX * speed; pz -= dirZ * speed; }
            
            if(GetAsyncKeyState('A')) { px -= dirZ * speed; pz += dirX * speed; }
            if(GetAsyncKeyState('D')) { px += dirZ * speed; pz -= dirX * speed; }

            if(GetAsyncKeyState(VK_LEFT))  ang -= 0.07f;
            if(GetAsyncKeyState(VK_RIGHT)) ang += 0.07f;
            if(GetAsyncKeyState(VK_PRIOR)) lY += 15.0f; 
            if(GetAsyncKeyState(VK_NEXT))  lY -= 15.0f;

            if(RenderFrame) RenderFrame(hwnd, px, py, pz, ang, lY);
            Sleep(1); 
        }
    }
    FreeLibrary(hDLL);
    return 0;
}