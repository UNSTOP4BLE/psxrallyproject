#!/usr/bin/env python3
# -*- coding: utf-8 -*-

__version__ = "0.2.0"
__author__  = "spicyjpeg"

from argparse import ArgumentParser, FileType, Namespace

import numpy
from numpy import ndarray
from PIL   import Image
from enum import IntEnum
from common import TexHeader, TextureInfo

class GP0BlendMode(IntEnum):
    GP0_BLEND_SEMITRANS = 0
    GP0_BLEND_ADD       = 1
    GP0_BLEND_SUBTRACT  = 2
    GP0_BLEND_DIV4_ADD  = 3

class GP0ColorDepth(IntEnum):
    GP0_COLOR_4BPP  = 0
    GP0_COLOR_8BPP  = 1
    GP0_COLOR_16BPP = 2

def gp0_page(x: int, y: int, blend_mode: GP0BlendMode, color_depth: GP0ColorDepth) -> int:
    return (
        ((x          & 0xF) << 0)  |
        ((y          & 0x1) << 4)  |
        ((blend_mode & 0x3) << 5)  |
        ((color_depth & 0x3) << 7) |
        ((y          & 0x2) << 10)
    ) & 0xFFFF

def gp0_clut(x: int, y: int) -> int:
    return (
        ((x & 0x03F) << 0) |
        ((y & 0x3FF) << 6)
    ) & 0xFFFF


## Input image handling

# Pillow's built-in quantize() method will use different algorithms, some of
# which are broken, depending on whether the input image has an alpha channel.
# As a workaround, all images are "quantized" (with no dithering - images with
# more colors than allowed are rejected) manually instead. This conversion is
# performed on indexed color images as well in order to normalize their
# palettes.
def quantizeImage(imageObj: Image.Image, numColors: int) -> Image.Image:
	#if imageObj.mode == "P":
		#return imageObj
	if imageObj.mode not in ( "RGB", "RGBA" ):
		imageObj = imageObj.convert("RGBA")

	image: ndarray = numpy.asarray(imageObj, "B")
	image          = image.reshape((
		imageObj.width * imageObj.height,
		image.shape[2]
	))
	clut, image    = numpy.unique(
		image,
		return_inverse = True,
		axis           = 0
	)

	if clut.shape[0] > numColors:
		raise RuntimeError(
			f"source image contains {clut.shape[0]} unique colors (must be "
			f"{numColors} or less)"
		)

	image               = image.astype("B").reshape((
		imageObj.height,
		imageObj.width
	))
	newObj: Image.Image = Image.fromarray(image)
	newObj = newObj.convert("P")
	newObj.putpalette(clut.tobytes(), imageObj.mode)
	return newObj

def getImagePalette(imageObj: Image.Image) -> ndarray:
	clut: ndarray = numpy.array(imageObj.getpalette("RGBA"), "B")
	clut          = clut.reshape(( clut.shape[0] // 4, 4 ))

	# Pillow's PNG decoder does not handle indexed color images with alpha
	# correctly, so a workaround is needed here to manually integrate the
	# contents of the image's "tRNs" chunk into the palette.
	if "transparency" in imageObj.info:
		alpha: bytes = imageObj.info["transparency"]
		clut[:, 3]   = numpy.frombuffer(alpha.ljust(clut.shape[0], b"\xff"))

	return clut

## Image data conversion

LOWER_ALPHA_BOUND: int = 32
UPPER_ALPHA_BOUND: int = 224

# Color 0x0000 is interpreted by the PS1 GPU as fully transparent, so black
# pixels must be changed to dark gray to prevent them from becoming transparent.
TRANSPARENT_COLOR: int = 0x0000
BLACK_COLOR:       int = 0x0421

def to16bpp(inputData: ndarray, forceSTP: bool = False) -> ndarray:
	source: ndarray = inputData.astype("<H")
	r:      ndarray = ((source[:, :, 0] * 31) + 127) // 255
	g:      ndarray = ((source[:, :, 1] * 31) + 127) // 255
	b:      ndarray = ((source[:, :, 2] * 31) + 127) // 255

	solid:           ndarray = r | (g << 5) | (b << 10)
	semitransparent: ndarray = solid | (1 << 15)

	data: ndarray = numpy.full_like(solid, TRANSPARENT_COLOR)

	if source.shape[2] == 4:
		alpha: ndarray = source[:, :, 3]
	else:
		alpha: ndarray = numpy.full(source.shape[:-1], 0xff, "B")

	numpy.copyto(data, semitransparent, where = (alpha >= LOWER_ALPHA_BOUND))

	if not forceSTP:
		numpy.copyto(data, solid, where = (alpha >= UPPER_ALPHA_BOUND))
		numpy.copyto(
			data,
			BLACK_COLOR,
			where = (
				(alpha >= UPPER_ALPHA_BOUND) &
				(solid == TRANSPARENT_COLOR)
			)
		)

	return data

def convertIndexedImage(
	imageObj: Image.Image,
	forceSTP: bool = False
) -> tuple[ndarray, ndarray]:
	clut:      ndarray = getImagePalette(imageObj)
	numColors: int     = clut.shape[0]
	padAmount: int     = (16 if (numColors <= 16) else 256) - numColors

	# Pad the palette to 16 or 256 colors after converting it to 16bpp.
	clut = clut.reshape(( 1, numColors, 4 ))
	clut = to16bpp(clut, forceSTP)

	if padAmount:
		clut = numpy.c_[
			clut,
			numpy.zeros(( 1, padAmount ), "<H")
		]

	image: ndarray = numpy.asarray(imageObj, "B")

	if image.shape[1] % 2:
		image = numpy.c_[
			image,
			numpy.zeros(( imageObj.height, 1 ), "B")
		]

	# Pack two pixels into each byte for 4bpp images.
	if numColors <= 16:
		image = image[:, 0::2] | (image[:, 1::2] << 4)

		if image.shape[1] % 2:
			image = numpy.c_[
				image,
				numpy.zeros(( imageObj.height, 1 ), "B")
			]

	return image, clut

## Main

def createParser() -> ArgumentParser:
	parser = ArgumentParser(description = "Converts an image file into 4bpp or 8bpp", add_help = False)

	group = parser.add_argument_group("Tool options")
	group.add_argument(
		"-h", "--help",
		action = "help",
		help   = "Show this help message and exit"
	)

	group = parser.add_argument_group("Conversion options")
	group.add_argument(
		"-s", "--force-stp",
		action = "store_true",
		help   = \
			"Set the semitransparency/blending flag on all pixels in the "
			"output image (useful when using additive or subtractive blending)"
	)

	group = parser.add_argument_group("File paths")
	group.add_argument("input", type = Image.open, help = "Path to input image file")
	group.add_argument("imageOutput", type = FileType("wb"), help = "Path to raw image data file to generate")
	group.add_argument("vram", type = FileType("r"), help = "Vram data")

	return parser

def main():
	parser: ArgumentParser = createParser()
	args:   Namespace      = parser.parse_args()

	vram = args.vram.read().split()
	vram = [eval(value) for value in vram]
	bpp = vram[4]

	try:
		image: Image.Image = quantizeImage(args.input, 2 ** bpp)
	except RuntimeError as err:
		parser.error(err.args[0])

	imageData, clutData = convertIndexedImage(image, args.force_stp)

	header = TexHeader()
	header.magic = int.from_bytes(b"XTEX", byteorder="little")
	info = TextureInfo()

	header.vrampos[:] = [vram[0], vram[1]]
	header.clutpos[:] = [vram[2], vram[3]]

	width_divider = 2 if bpp == 8 else 4
	colordepth = GP0ColorDepth.GP0_COLOR_8BPP if bpp == 8 else GP0ColorDepth.GP0_COLOR_4BPP
	#vram = [x,y, palx, paly, bpp]
	if (vram[2] % 16) != 0:
		raise ValueError("CLUT alignment invalid")
	if (vram[2] + clutData.size) > 1024:
		raise ValueError("Pallete clipping vram")
	
	info.page   = gp0_page(vram[0] // 64, vram[1] // 256, 0, colordepth)
	info.clut   = gp0_clut(vram[2] // 16, vram[3])
	info.u      = (vram[0] %  64) * width_divider
	info.v      = (vram[1] % 256)
	info.w  = image.size[0]
	info.h = image.size[1] 
	info.bpp    = bpp

	header.texinfo = info 
	header.clutsize = clutData.nbytes
	header.texsize = imageData.nbytes

	file = args.imageOutput
	file.write(header)
	file.write(clutData)
	file.write(imageData)

	file.close()

if __name__ == "__main__":
	main()
