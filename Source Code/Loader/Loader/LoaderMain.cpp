#include <windows.h>

extern "C" __declspec(dllexport) int LoadGameData(const char* m1, const char* m2) {
    if (GetFileAttributes("bin\\Materials\\Texture.wad") == INVALID_FILE_ATTRIBUTES) {
        return 1;
    }
    if (m1 && m1[0] && GetFileAttributes(m1) == INVALID_FILE_ATTRIBUTES) return 2;
    if (m2 && m2[0] && GetFileAttributes(m2) == INVALID_FILE_ATTRIBUTES) return 3;
    
    return 0;
}