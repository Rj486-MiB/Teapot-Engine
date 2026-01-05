#include <windows.h>
#include <math.h>
#include <stdio.h>

typedef void (*REND)(HWND, float, float, float, float, float, int);
REND RenderFrame = NULL;
HINSTANCE hDLL = NULL;

// Globals
float px = 0, py = 60, pz = -400, ang = 0, lY = 0;
int gW = 800, gH = 600;
DWORD lastTime = 0;
int currentFPS = 0;

void LoadConfig() {
    FILE* f = fopen("Config.ini", "r");
    if(f) { fscanf(f, "Width=%d\nHeight=%d", &gW, &gH); fclose(f); }
}

LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if(m == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProc(h, m, w, l);
}

int WINAPI WinMain(HINSTANCE hI, HINSTANCE hP, LPSTR lp, int nS) {
    LoadConfig();
    hDLL = LoadLibrary("bin/Software.dll");
    if(!hDLL) return 1;
    RenderFrame = (REND)GetProcAddress(hDLL, "RenderFrame");

    WNDCLASS wc = {0}; wc.lpfnWndProc = WndProc; wc.hInstance = hI; wc.lpszClassName = "TeapotV74";
    wc.hCursor = LoadCursor(0, IDC_ARROW); RegisterClass(&wc);

    HWND hwnd = CreateWindow("TeapotV74", "Teapot Engine Build 7.4", 
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, gW, gH, 0, 0, hI, 0);

    MSG msg = {0};
    while(msg.message != WM_QUIT) {
        if(PeekMessage(&msg, 0, 0, 0, 1)) {
            TranslateMessage(&msg); DispatchMessage(&msg);
        } else {
            DWORD currentTime = GetTickCount();
            if (currentTime - lastTime > 0) currentFPS = 1000 / (currentTime - lastTime);
            lastTime = currentTime;

            if (GetActiveWindow() == hwnd) {
                float speed = GetAsyncKeyState(VK_SHIFT) ? 14.0f : 7.0f;
                
                // ANGLE ROTATION AND WRAPPING (360)
                if (GetAsyncKeyState(VK_LEFT))  ang += 0.07f;
                if (GetAsyncKeyState(VK_RIGHT)) ang -= 0.07f;
                
                // Keep angle between 0 and 2*PI (approx 6.28)
                if (ang > 6.2831f) ang -= 6.2831f;
                if (ang < 0.0f) ang += 6.2831f;

                if (GetAsyncKeyState(VK_PRIOR)) lY += 12.0f; 
                if (GetAsyncKeyState(VK_NEXT))  lY -= 12.0f;

                // MOVEMENT
                float fwdX = (float)sin(ang);
                float fwdZ = (float)cos(ang);
                float sideX = (float)cos(ang);
                float sideZ = (float)-sin(ang);

                // W/S - Move towards current angle
                if (GetAsyncKeyState('W')) { px -= fwdX * speed; pz += fwdZ * speed; }
                if (GetAsyncKeyState('S')) { px += fwdX * speed; pz -= fwdZ * speed; }
                
                // A/D - Strafe
                if (GetAsyncKeyState('A')) { px -= sideX * speed; pz += sideZ * speed; }
                if (GetAsyncKeyState('D')) { px += sideX * speed; pz -= sideZ * speed; }

                if (GetAsyncKeyState('R')) { px=0; py=60; pz=-400; ang=0; lY=0; }
            }
            if(RenderFrame) RenderFrame(hwnd, px, py, pz, ang, lY, currentFPS);
				Sleep(1); // To Denizyt9559 do not delete this if you delete this your CPU will hate you im not kidding if you remove this your CPU will get tortured whenever you run the program just trust me 
        }
    }
    return 0;
}
/*
 https://www.youtube.com/watch?v=xvFZjo5PgG0
 Version V7.4
 Probably the most painful version i had to suffer through
 Newer knew LAN networking could be such a pain
 thank god i didnt also deal with Port forwarding stuff
 or my head would be beating like a heart

 */


// OH MY GOD THESE STUPID C4996 WARNINGS I DONT CARE ABOUT FOPEN IT WORKS PERFECTLY FINE JUST SHUT UP