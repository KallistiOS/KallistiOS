#!/usr/bin/env python

# GIMP Plug-in for the Sega Dreamcast MR file format
# Copyright (C) 2019 by BBHoodsta
#
# Encoding based on logotools(pngtomr.c) source code by Andrew Kieschnick (ADK)
# http://napalm-x.thegypsy.com/andrewk/dc/
#
# Decoding based on Selfboot Inducer (mrimage.pas) source code by SiZiOUS 
# http://sizious.com/

import os, sys, struct

from gimpfu import *
from array import array

def to_bytes(n, length, endianness='big'):
    if(sys.version_info[0] < 3):
        h = '%x' % n
        s = ('0'*(len(h) % 2) + h).zfill(length*2).decode('hex')
        return s if endianness == 'big' else s[::-1]
    else:
        return n.to_bytes(length, byteorder=endianness)

def mr_encode(input, output, size):
    length = 0
    position = 0
    run = 0
    
    while(position < size):
        run = 1

        while((run < 0x17f) and (position+run < size) and (input[position] == input[position+run])):
            run += 1

        if(run > 0xff):
            output[length] = 0x82
            length += 1
            output[length] = 0x80 | (run - 0x100)
            length += 1
            output[length] = input[position]
            length += 1
        elif(run > 0x7f):
            output[length] = 0x81
            length += 1
            output[length] = run
            length += 1
            output[length] = input[position]
            length += 1
        elif(run > 1):
            output[length] = 0x80 | run
            length += 1
            output[length] = input[position]
            length += 1
        else:
            output[length] = input[position]
            length += 1
        
        position += run

    return length
        
def save_mr(img, drawable, filename, raw_filename):
    src_width = img.width   # image width
    src_height = img.height # image height

    # Display error if image is bigger than 320x90
    if(src_width > 320 or src_height > 90):
        gimp.message("Your image should be 320x90 or smaller\n")
        return

    # Grab region of pixels
    src_rgn = drawable.get_pixel_rgn(0, 0, src_width, src_height, False, False)

    # Create a pixel array from the region
    src_pixels = array("B", src_rgn[0:src_width, 0:src_height])

    # Allocate raw and compressed outputs
    raw_output = array("B", "\x00" * (src_width * src_height )) 
    compressed_output = array("B", "\x00" * (src_width * src_height))

    psize = len(src_rgn[0,0]) # Should be 3
    palette_count = 0
    palette_colors = array("B", "\x00" * (128 * 3)) # 128 colors (RGB)

    # Generate raw_output and create the palette of the image
    for i in range(src_width*src_height):
        found = False
        palette_index = 0

        pixel_index = i * psize
        while(not found and palette_index < palette_count):
            if(src_pixels[pixel_index:pixel_index+psize] == palette_colors[palette_index*3:palette_index*3+psize]):
                found = True
            else:
                palette_index += 1

        # Display error message if image has more than 128 colors
        if(not found and palette_index == 128):
            gimp.message("Reduce the number of colors to <= 128 and try again.\n")
            return
        
        if(not found):
            palette_colors[palette_index*3:palette_index*3+psize] = src_pixels[pixel_index:pixel_index+psize]
            palette_count += 1
        
        raw_output[i] = palette_index

    compressed_size = mr_encode(raw_output, compressed_output, src_width*src_height)

    # Display warning if compressed image is bigger than 8192 bytes
    if(compressed_size > 8192):
        gimp.message("WARNING: This will NOT fit in a normal ip.bin - it is %d bytes too big!\n", compressed_size - 8192)
    
    crap = 0
    endianness = 'little'
    offset = 30 + palette_count*4 # 30 byte header
    size = offset + compressed_size

    with open(filename, 'wb') as output:
        output.write("MR")                                    # 
        output.write(to_bytes(size, 4, endianness))           # Filesize
        output.write(to_bytes(crap, 4, endianness))           # 
        output.write(to_bytes(offset, 4, endianness))         # Data offset
        output.write(to_bytes(src_width, 4, endianness))      # Image width
        output.write(to_bytes(src_height, 4, endianness))     # Image height
        output.write(to_bytes(crap, 4, endianness))           # 
        output.write(to_bytes(palette_count, 4, endianness))  # Amount of colors in palette
        #output.write(struct.pack("<iiiiiii", size, crap, offset, src_width, src_height, crap, palette_count))

        for i in range(palette_count):
            palette_color = palette_colors[i*3:i*3+3]
            # Write RGB => BGR
            for x in reversed(range(3)):
                output.write(to_bytes(palette_color[x], 1, endianness))
            output.write(to_bytes(crap, 1, endianness)) # Unused alpha

        for i in range(compressed_size):
            output.write(to_bytes(compressed_output[i], 1, endianness))

def mr_decode(input, cdata_size, idata_size):
    position = 0
    idx_position = 0
    run = 0
    indexed_data = array("B", "\x00" * idata_size)

    while(position < cdata_size):
        first_byte = input[position]
        if((position+1) < cdata_size):
            second_byte = input[position+1]

        # The bytes lower than 0x80 are recopied just as they are in the Bitmap
        if(first_byte < 0x80):
            run = 1
            position += 1
        # The tag 0x81 is followed by a byte giving directly the count of points
        elif(first_byte == 0x81):
            run = second_byte
            first_byte = input[position+2]
            position += 3
        # The tag 0x82 is followed by the number of the points decoded in Run
        # By retaining only the first byte for each point
        elif(first_byte == 0x82 and second_byte >= 0x80):
            run = second_byte - 0x80 + 0x100
            first_byte = input[position+2]
            position += 3
        else:
            run = first_byte - 0x80
            first_byte = second_byte
            position += 2

        # Writing decompressed bytes
        for i in range(run):
            # The additional byte (+ 1) is useless, but it always present in MR files.
            if(idx_position+i < idata_size):
                indexed_data[idx_position+i] = first_byte

        idx_position += run

    return indexed_data

def load_mr(filename, raw_filename):
    opacity = 100
    file_content = ""

    # Get content of file
    with open(filename, 'rb') as input:
        file_content = input.read()

    # Parse header
    header_blob = struct.unpack("<iiiiiii", file_content[2:30]) # Grab header ignoring 'MR'
    filesize = header_blob[0]
    dataoffset = header_blob[2]
    img_width = header_blob[3]
    img_height = header_blob[4]
    num_colors = header_blob[6]

    # Parse Palette
    rgb_palette = array("B", "\x00" * (num_colors*3))
    bgra_palette = struct.unpack("<" + ("B"*(num_colors*4)), file_content[30:30+num_colors*4])
    for i in range(num_colors):
        # Convert BGRA => RGB
        for x in reversed(range(3)):
            rgb_palette[i*3+(2-x)] = bgra_palette[i*3+x+i]

    # Decode indexed data
    cdata_size = filesize - dataoffset
    compressed_data = struct.unpack("<" + ("B" * cdata_size), file_content[dataoffset:dataoffset + cdata_size])
    indexed_data = mr_decode(compressed_data, cdata_size, img_width * img_height)

    # Indexed data => RGB data
    rgb_data = array("B", "\x00" * (img_width * img_height * 3))
    for i in range(img_width * img_height):
        index = indexed_data[i]
        rgb_data[i*3:i*3+3] = rgb_palette[index*3:index*3+3]

    # Create image
    img = gimp.Image(img_width, img_height, RGB)
    img.filename = filename
    img_layer = gimp.Layer(img, filename, img_width, img_height, RGB_IMAGE, opacity, NORMAL_MODE)
    img_layer_region = img_layer.get_pixel_rgn(0, 0, img_width, img_height, True)
    img_layer_region[0:img_width, 0:img_height] = rgb_data.tostring()
    img.add_layer(img_layer, 0)
    gimp.displays_flush()

    return img

def register_load_handlers():
    gimp.register_load_handler('file-mr-load', 'mr', '')
    pdb['gimp-register-file-handler-mime']('file-mr-load', 'image/mr')

def register_save_handlers():
    gimp.register_save_handler('file-mr-save', 'mr', '')

register(
    'file-mr-save', #name
    'Save an MR (.mr) file', #description
    'Save an MR (.mr) file',
    'BBHoodsta', #author
    'BBHoodsta', #copyright
    '2019', #year
    'Sega Dreamcast MR image',
    '*',
    [   #input args. Format (type, name, description, default [, extra])
    (PF_IMAGE, "image", "Input image", None),
    (PF_DRAWABLE, "drawable", "Input drawable", None),
    (PF_STRING, "filename", "The name of the file", None),
    (PF_STRING, "raw-filename", "The name of the file", None),
    ],
    [], #results. Format (type, name, description)
    save_mr, #callback
    on_query = register_save_handlers,
    menu = '<Save>'
    )

register(
    'file-mr-load', #name
    'Load an MR (.mr) file', #description
    'Load an MR (.mr) file',
    'BBHoodsta', #author
    'BBHoodsta', #copyright
    '2019', #year
    'Sega Dreamcast MR image',
    None, #image type
    [   #input args. Format (type, name, description, default [, extra])
    (PF_STRING, 'filename', 'The name of the file to load', None),
    (PF_STRING, 'raw-filename', 'The name entered', None),
    ],
    [(PF_IMAGE, 'image', 'Output image')], #results. Format (type, name, description)
    load_mr, #callback
    on_query = register_load_handlers,
    menu = "<Load>",
    )

main()
