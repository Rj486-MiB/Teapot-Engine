#include <windows.h>
#include <stdio.h>

int main() {
    WIN32_FIND_DATA ffd;
    HANDLE hFind = FindFirstFile("*.bmp", &ffd);
    if (hFind == INVALID_HANDLE_VALUE) { printf("No BMP files found!\n"); return 1; }

    FILE* wad = fopen("Texture.wad", "wb");
    FILE* log = fopen("Texture_List.txt", "w");
    int count = 0;
    fwrite(&count, 4, 1, wad);

    fprintf(log, "TEAPOT ENGINE TEXTURE MAP\n=========================\n");

    do {
        count++;
        char texID[16];
        memset(texID, 0, 16); 
        sprintf(texID, "%d", count); 

        printf("ID [%s] -> %s\n", texID, ffd.cFileName);
        fprintf(log, "ID: %s -> File: %s\n", texID, ffd.cFileName);

        fwrite(texID, 1, 16, wad);
        
        int offset = ftell(wad) + 4; 
        fwrite(&offset, 4, 1, wad);
        
        FILE* bmp = fopen(ffd.cFileName, "rb");
        if(!bmp) {
            unsigned int errorPix[4096];
            for(int i=0; i<4096; i++) errorPix[i] = 0xFF00FF;
            fwrite(errorPix, 4, 4096, wad);
            continue;
        }

        fseek(bmp, 54, SEEK_SET); 
        unsigned char img[64 * 64 * 3];
        fread(img, 1, 64 * 64 * 3, bmp);
        fclose(bmp);

        for(int i=0; i<4096; i++) {
            unsigned int c = (img[i*3+2] << 16) | (img[i*3+1] << 8) | img[i*3];
            fwrite(&c, 4, 1, wad);
        }
    } while (FindNextFile(hFind, &ffd) != 0);

    fseek(wad, 0, SEEK_SET);
    fwrite(&count, 4, 1, wad);
    fclose(wad);
    fclose(log);

    printf("\nSuccess! Generated %d textures.\n", count);
    return 0;
}