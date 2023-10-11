Using texconv command line utility to convert image formats to DDS files:

Options
-------
-l		force output filename to lower case
-pmalpha 	premultiplied alpha pixel format
-m		generate mipmaps (1 for no mipmap generation)
-f		block compression format
-srgbi|o	add _SRGB chunk for rendering to gamma-corrected render targets
-y		overwrite existing output file

Examples	Command line									Purpose
--------	------------									-------
2D sprite	texconv cat.png -pmalpha -m 1 -f BC1_UNORM -srgbi -y 			Rendering to fp16 rt with tonemapping postprocess
2D sprite	texconv cat.png -pmalpha -m 1 -f BC1_UNORM_SRGB -srgbi -y 			Rendering to fp10 rt after fp16 resolve and tonemapping
Normal map	texconv rocks_NM_height.png -nmap l -nmapamp 4 -f BC7_UNORM -y				Normal map generation
Cube map	texconv grasscube1024.dds -l -m 1 -f BC1_UNORM_SRGB -srgbi -y			Rendering to fp16 rt with tonemapping postprocess
3D texture	texconv baketest.png -l -pmalpha -m 11 -f BC3_UNORM_SRGB -srgbi -srgbo -y 	Rendering to fp16 rt with tonemapping postprocess
DXR raytracing	texconv texture.png -m 11 -f BC1_UNORM_SRGB -y

Notes
-----
Important! Textures for Blender OBJ model exports MUST BE ROTATED 180 DEGREES before DDS conversion and loading to DirectXTK game!
