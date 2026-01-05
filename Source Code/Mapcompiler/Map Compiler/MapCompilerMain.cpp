#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdio.h>

struct Vec3 { float x,y,z; Vec3(float _x, float _y, float _z):x(_x),y(_y),z(_z){} Vec3(){} };
struct Face { int v[4]; char tex[16]; };

int main() {
    std::string mapName;
    std::cout << "Enter Map Name: ";
    std::getline(std::cin, mapName);
    if(mapName.empty()) mapName = "?";

    std::ifstream in("Map.txt"); 
    if(!in) { printf("Error: Map.txt not found!\n"); system("pause"); return 1; }

    std::vector<Vec3> verts; 
    std::vector<Face> faces;
    std::string cmd;

    while(in >> cmd) {
        if(cmd == "BOX") {
            float x,z,y,w,h,d; int tex; 
            in >> x >> z >> y >> w >> h >> d >> tex;
            int s = verts.size();
            verts.push_back(Vec3(x-w/2, y-h/2, z-d/2)); verts.push_back(Vec3(x+w/2, y-h/2, z-d/2));
            verts.push_back(Vec3(x+w/2, y+h/2, z-d/2)); verts.push_back(Vec3(x-w/2, y+h/2, z-d/2));
            verts.push_back(Vec3(x-w/2, y-h/2, z+d/2)); verts.push_back(Vec3(x+w/2, y-h/2, z+d/2));
            verts.push_back(Vec3(x+w/2, y+h/2, z+d/2)); verts.push_back(Vec3(x-w/2, y+h/2, z+d/2));
            int idx[] = {0,1,2,3, 5,4,7,6, 4,0,3,7, 1,5,6,2, 3,2,6,7, 4,5,1,0};
            for(int i=0; i<6; i++) {
                Face f; sprintf(f.tex, "%d", tex);
                for(int j=0; j<4; j++) f.v[j] = s + idx[i*4+j];
                faces.push_back(f);
            }
        }
    }

    std::ofstream out("bin/maps/Map.bs2", std::ios::binary);
    int nameLen = (int)mapName.length();
    out.write((char*)&nameLen, 4);
    out.write(mapName.c_str(), nameLen);

    int vCount = (int)verts.size();
    out.write((char*)&vCount, 4);
    out.write((char*)&verts[0], vCount * sizeof(Vec3));

    int fCount = (int)faces.size();
    out.write((char*)&fCount, 4);
    out.write((char*)&faces[0], fCount * sizeof(Face));

    printf("Successfully compiled Map: %s\n", mapName.c_str());
    return 0;
}