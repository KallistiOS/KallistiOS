pvrtex converts images to Dreamcast PowerVR textures.

Created by TapamN, 2023


It is designed to work similarly to tvspelsfreak's texconv, so it can be used in place of texconv will minimal changes. It might be helpful to read the readme for texconv for additional information not covered here. In particular, there are explainations of the types of textures supported by the Dreamcast.

Compared to texconv, pvrtex has the following enhancements:

	* Faster texture compression and palette generation
	* Can generate small codebook VQ textures
	* No Qt dependency
	* Support for compressed stride textures
	* Better mipmap generation
	* Dithering
	* Support for additional output file types (adds .PVR and .DT)

--------------------------------------------------------------------------

Usage Examples:

pvrtex -i source.png -o texture.dt
	Converts a PNG file to a DT file that is twiddled, uncompressed texture, without mipmaps. The format is automatically chosen depending on alpha content of source.png (See description for AUTO texture format in command option listing).

pvrtex -i source.png -o texture.dt -f argb4444 -d -c 64 -m quality -r -R
	Converts a PNG file to a DT file that is twiddled, compressed texture, with mipmaps. The texture will use the ARGB4444 color format, and dithered. If the source image is not already a square power-of-two, it will be resized to be the nearest square power-of-two. The codebook used by the compression will be limited to 64 entries out of the potential 256; this reduces quality, but reduces the size of the texture by 1.5 KB, and improves fillrate.

pvrtex -i source.png -o texture.dt -f normal -m
	Converts a PNG file containing a normal map to a DT file, with mipmaps.

pvrtex -i source.png -o texture.dt -s
	Converts a PNG file to a nontwiddled texture. source.png is not required to be a power-of-two width, and can also be any multiple of 32 that is <= 1024.

pvrtex -i source.png -o texture.dt -f pal8bpp -C 64 -d -p preview.png
	Converts a PNG file to a DT file with 8-bit color. The resulting image will not use more than 64 colors out of the potential 256, and will be dithered. The pallete for the texture will be written to texture.dt.pal. A preview of the resulting texture will be written to preview.png.

pvrtex -i mip256.png -i mip128.png -i mip64.png -i mip32.png -i mip16.png -o texture.dt -m
	Generates a mipmapped texture, using the different input images as user defined mipmap levels instead of automatically generating all of them. If a mipmap level is not defined by the user, it will be generated from a higher level. By default, the higher level will not be the level above, but three levels above; if you want to use the level above, use fast mipmaps (-m fast) instead.

--------------------------------------------------------------------------

Building:

	Run "make install" to install pvrtex to ${KOS_HOME}/../bin/

	To generate the proper README with linebreaks, from readme_unformatted.txt, run "make README" or "make all". Requires "fmt".

--------------------------------------------------------------------------

Command Line Options:

--help, -h
	Displays command line options

--examples -E
	Displays usage examples

--version, -V
	Displays version

--in [filename], -i [filename]
	Input image file. This option is required.

	If multiple input images are specified, they are currently assumed to be different mipmap levels for a single texture. Resize options can not be used for custom mipmaps, so all images must be a square power-of-two.

	Uses stb_image library for reading the image. The supported formats are:
		JPEG, PNG, TGA, BMP, PSD, GIF, HDR, PIC, PMN

--out [filename], -o [filename]
	Sets the file name of the converted texture. The extension of this filename controls the file format.

	The supported formats are:

	.PVR
		Official PowerVR texture. This is incomplete as was created to help test the output of the converter, by using PC PVR viewing programs to check the resulting texture. There are likely incompatiblities with .PVR handling of official games (for example, pvrtex does not add a GBIX chunk).
	.TEX
		Format used by tvspelsfreak's texconv. pvrtex will generate certain formats not supported by texconv but representable in the file format (like compressed stride textures).
	.DT
		New file format used by this program. Supports small codebook VQ, and texture data is aligned to a 32-byte boundry to make DMA easier.

	It's possible to specify no output file if only a preview image is desired.

--format [type], -f [type]
	Sets the pixel format of the resulting texture.

	[type] can be one of the following:

	RGB565
		Best color out of standard formats without sacrificing speed, but can have rainbowing on grayscale images.
	ARGB1555
		Allows for fully transparent texels. Better choice for grayscale than RGB565, which can have rainbowing.
	ARGB4444
		Allows for alpha gradients, but poorest color depth with noticable banding.
	YUV422 / YUV
		Better than RGB565 or ARGB1555 for gradients, but bi/trilinear filtering has worse performance.
	PAL8BPP
		Maximum of 256 colors. Palette is generated as a seperate file. Must be twiddled. If compression, mipmapping, and bi/trilinear are used, a hardware bug causes some texels to be filtered incorrectly on the top left/bottom right corners of a 4x2 block.
	PAL4BPP
		Maximum of 16 colors. Palette is generated as a seperate file. Must be twiddled. If compression, mipmapping, and bi/trilinear are used, a hardware bug causes some texels to be filtered incorrectly on the top left/bottom right corners of a 4x4 block
	BUMPMAP
		Generates PVR normal map. Source image is treated as a height map.
	NORMAL
		Generates PVR normal map. Source image is treated as a DOT3 normal map, with RGB channels corresponding to the normal's XYZ.
	AUTO
		Selects RGB565, ARGB1555, or ARGB4444 depending on alpha content of input image. If fully opaque, RGB565 is used, all pixels have either fully opaque or fully transparent alpha, ARGB1555 is used, and if some pixels have non-fully opaque or transparent pixels, ARGB4444 is used.
	AUTOYUV
		Same as AUTO, but usage of RGB565 is replaced with YUV422.

--preview [filename], -p [filename]
	Generates a preview of the resulting texture file. You can see the results of bit depth reduction, dithering, mipmaps, and compression.

	The preview is can be a PNG, JPG, BMP, or TGA file. Preview JPGs are generally not a good choice do to the lack of alpha, and possiblility of compression artifacts.

--compress [codebook_size / "small"], -c [codebook_size / "small"]
	Generates a VQ compressed texture.

	codebook_size is an optional parameter adjusts the size of the codebook generated for the texture. Reducing the codebook_size can improve fillrate, and, with .PVR and .DT files, improve the compression ratio of small textures. By default, a full codebook is used to generate the best quality texture. codebook_size can be a number from 1 to 256, or the string "small".

	For .PVR files, using a number will never reduce the size of the texture, but can improve fillrate. Specifying "small" as the codebook size will reduce the texture size for certain textures smaller than 64x64 without mipmaps, or 32x32 with mipmaps.

	For .TEX files, codebook_size will never reduce the size of the texture, but can still improve performance.

	For .DT files, reducing the codebook_size will reduce the size of the texture.  Specifying "small" as the codebook size will select a smaller codebook automatically for textures that are 128x128 or smaller, with a size of 192 for a 128x128 mipmapped texture, down to 10 for an 8x8 non-mipped texture.

--max-color [colors], -C [colors]
	This limits the number of colors used for a PAL8BPP or PAL4BPP format texture. This option could be used to generate an 8BPP texture that only uses 64 colors, and the unused colors could be used for other textures.

--mipmap ["fast"], -m ["fast"]
	With this option, the resulting file will have mipmaps.

	By default, a Mitchell-Netravalli filter will be used on a level 3 steps above. (e.g. 64x64 will be generated from 512x512)

	Adding the parameter "fast" to this option, each mipmap level is generated by downsampling the level above. (e.g. 64x64 level will be generated from 128x128) This speeds up mipmap generation for large textures.

	When generating high quality mipmaps for a resized image, the largest mips will be generated directly from the source image. (i.e. with a 1600x500 source image, converted with "-m -r near -R x2", pvrtex will create a 1024x1024 texture, with mipmap levels 512x512 and 256x256 generated directly from the 1600x500 source, and not the 1024x1024 top most level).

--no-mip-shift, -S
	When generating mipmaps, by default, pvrtex will preform a subpixel adjustment during downsampling to ensure the mipmaps line up correctly. This option will disable this.

--perfect-mip [levels], -M [levels]
	When using mipmaps and compression, small mipmap levels will be loselessly compressed.

	The levels parameter controls how many levels are lossless. 1 means only the 1x1 level will be losslessly compressed, 2 means 1x1 and 2x2 will be loselessly compressed, and so on.

	Generating lossless mipmaps use up VQ codebook slots. These are the total number of codebook entries used for a given number of lossless mipmaps:

	                 16-bit    8-bit    4-bit
	1 level   (1x1)    1         1        1
	2 levels  (2x2)    2         1        1
	3 levels  (4x4)    6         3        2
	4 levels  (8x8)   22        11        6
	5 levels (16x16)  86        43       22
	6 levels (32x32)  --       171       86

--high-weight [levels], -H [levels]
	When using mipmaps and compression, this increases the weight the compressor gives smaller mipmap levels, to encourage the compressor to generate them at higher quality, at the cost of lower quality higher levels. Not currently supported for 4BPP textures.

	The levels parameter controls how many levels below the largest have extra weight. A value of 1 means every level besides the top has boosted weight, a value of 2 means the two largest levels have normal weight, while every smaller level is boosted.

--dither [amount], -d [amount]
	Enables dithering. Currently, Floyd-Steinberg is used.

	Amount is an optional parameter that adjusts the amount of dithering, and is a decimal value from 0 to 1. 0 will result in no dithering, while 1 results in full dithering. If dithering is enabled but an amount is not specified, full dithering is used.

	This option has no effect on YUV textures, but is valid on all others.

--stride, -s
	Output a non-twiddled texture. This also allows for non-power-of-two sized textures. Width must still 8, 16, or a multiple of 32 less than or equal to 1024. Any height can be used, from 1 to 1024.

	If a texture has a power-of-two dimensions and --stride is used, the resulting texture will be a nontwiddled texture that can be rendered without stride, for formats that support such textures. (.DT and .TEX support this, .PVR does not.)

	Stride textures do not wrap as normal if the width is not a power-of-two, and have worse rendering performance than twiddled textures (especially when filtered or rotated). It is not possible to generate palettized or normal textures with stride. .PVR files cannnot use stride.

	Valid widths for stride texture:
		8, 16, 32, 64, 96, 128, 160, 192, 224, 256, 320, 352, 384, 416, 480, 512, 544, 576, 608, 640, 672, 704, 736, 768, 800, 832, 864, 896, 928, 960, 992, 1024

--resize [method], -r [method]
	Resize a input image that is not a supported PVR texture size to a valid size.

	If the texture is not strided, the texture will be resized to a power-of-two on both dimensions. For stride textures, width will be adjusted to an appropriate stride size, and the height will always be resized to a power-of-two. 

	Method controls how the image will be resized.

	NONE
		Generates an error if input image is not a valid size. This is the default.
	NEAR
		Round size up or down to nearest valid size. If resize is enabled, but no method is specified, this is the default.
	UP
		Round size up to next valid size
	DOWN
		Round size down to next valid size
	
	Examples for non-stride textures:
	Source size      NONE     NEAR        UP       DOWN
	 256x256       256x256   256x256   256x256   256x256
	 260x260        Error    256x256   512x512   256x256
	 200x200        Error    256x256   256x256   128x128
	 200x260        Error    256x256   256x512   128x256
	2000x2000       Error   1024x1024 1024x1024 1024x1024
	   1x1          Error      8x8       8x8       8x8

--mip-resize [method], -R [method]
	When using mipmaps, resizes nonsquare images to be square. This option does nothing if not using mipmaps or the image is already square (after --resize). This new size calculation occurs after the standard --resize. Source images are only resized once. This option will not resize the image to a power-of-two size if it's not already (use --resize for that).

	Method controls how the image will be resized.

	NONE
		Generates an error if input image is not a valid mipmap size. This is the default.
	X2
		Doubles the narrower dimension. A 256x32 image will be resized to 64x64. If mip-resize is enabled, but no method is specified, this is the default.
	X4
		Quaduples or doubles the narrower dimension. A 256x32 image will be resized to 128x128.
	UP
		Resizes the narrower dimension to be the same size as the wider. A 256x32 image will be resized to 256x256.
	DOWN
		Resizes the wider dimension to be the same size as the narrower. A 256x32 image will be resized to 32x32.

	Examples:

	Source size      X2       X4        UP       DOWN
	 256x256      256x256   256x256   256x256   256x256
	 256x128      256x256   256x256   256x256   128x128
	 256x64       128x128   256x256   256x256    64x64
	 256x32        64x64    128x128   256x256    32x32
	1024x8         16x16     32x32   1024x1024    8x8

--edge [type], -e [type]
	Controls how the edges of the image are handled when resizing. This also affects height map to normal map conversion.

	Valid options:

	CLAMP
		Samples are clamped to edge of image, default if not mipmapped.
	WRAP
		Samples wrap around to other side of image, default if mipmaps are used. Is works well when the texture is used to that it repeats, but might cause noticble bleeding around the edge of the texture in certain situations. For example, a poster or sign that doesn't repeat across the polygon. In that cause, CLAMP should be used.
	REFLECT
		Samples reflect off edge of image back into valid area. If you use are planning on using UV mirroring instead of wrapping, use this instead of wrap.
	ZERO
		Outside of image is treated as transparent blackness. This is not currently supported for images with a --type of BUMPMAP.

	The default is CLAMP if not using mipmaps, or WRAP if mipmaps are used.

--bilinear, -b
	In texconv, this was used to generate mipmaps with a box filter. This option is ignored in pvrtex, which currently always uses a Mitchell-Netravalli filter.

--nearest, -n
	In texconv, this was used to generate mipmaps by point sampling. This option is not supported in pvrtex, and will cause pvrtex to abort.

--vqcodeusage (Not supported)
	This option from texconv is not recognized at all by pvrtex.

--------------------------------------------------------------------------

.DT File Format

	See file_dctex.h for documentation. file_dctex.h can also be used as a library to help access information from the file's header.

--------------------------------------------------------------------------

Known bugs

	High weight compressed mips (--high-weight) does not currently work with 4BPP textures; the parameter will be ignored if specified.

	There is a weird pathological performance issue with compression on certain textures. With a 1024x1024 texture with mips that is ff000000 on the left side, and ffffffff on the right side, performance drops by around 15x. It is apparently cache missing constantly, according to cachegrind, in a function that reads memory linearly (distance_limited in elbg.c). Doesn't make sense.

--------------------------------------------------------------------------

Future Ideas

	* Code clean up

	* Add Yliluoma dithering

	* Improve error checking

	* It might be possible to improve VQ quality by compressing 4/5/6 bit color instead of 8 bit (so the compressor won't waste codebook space on colors that are too similar to distinguish at given bit depth)

	* Speed up compression by not processing alpha for opaque textures

	* Add ability to generate a single palette to be shared for multiple textures

	* Allow specifying palette format

	* Allow specifiying custom external palette for texture

	* Add ability to generate animated VQ textures with shared codebook for all frames

	* Auto generate output name (e.g. -i image.png -o $.dt will output image.dt)

	* Add wildcard input files (e.g. -i *.png)

	* Add VQ index dithering

	* Add KIMG output support

	* Allow texture formats (PVR/TEX/DT/KMG) to be used as input formats, to transcode or recompress textures

	* Add per-axis edge sampling control

	* Rework code so that this can be used as a library

	* Allow sizing to arbitary size (i.e. --resize 256x256)

	* Add a filter to try to hide the palettized compressed mipmap bug

--------------------------------------------------------------------------

License:
	This uses code from FFmpeg, which is LGPL, so this project is also LGPL. Files not originating from FFmpeg can also be used as public domain code/BSD/MIT/whatever.
