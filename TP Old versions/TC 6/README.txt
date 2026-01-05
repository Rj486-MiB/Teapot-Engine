=========================================
 TEAPOT COMPILER PACK (TCP) - Build 2.1
=========================================

1. MAP COMPILER (v2.1)
Parses Map.txt into binary Map.bs2.

Commands:
	BOX [X] [Y] [Z] [Size] [TextureName]
	SPRITE [X] [Y] [Z] [TextureName]

Example: BOX 0 20 500 40 Wall1
	(Creates a 40-unit cube at Z=500 using Wall1 texture)

2. WAD COMPILER (v1.0)
	Creates Texture.wad from .BMP files.
Requirements:
	Must be 24-bit Bitmap (BMP)
	Must be 64x64 resolution
	Filename = Texture Name in Map.txt

OUTPUT DIRECTORY:
	Map.bs2 will be created in bin\maps\
	Texture.wad will be created in the local folder.