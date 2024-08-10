# font2txf

**font2txf** is an utility used for generating texture mapped font files,
also known as the `TXF` format, from a **TrueType** (`TTF`) or **OpenType**
(`OTF`) font file.

`TXF` is a texture font which can be used in the context of **GLUT** (OpenGL
Utility Toolkit), created by [Mark J. Kilgard](https://en.wikipedia.org/wiki/Mark_Kilgard).
For the **Sega Dreamcast**, the `TXF` format is supported using the `libdcplib`
KallistiOS Port, using the `PLIB FNT` component. An example of source code,
including sample `TXF` fonts, is available here:

	$KOS_BASE/examples/dreamcast/cpp/dcplib

For generating `TXF` files, you can use the provided `gentexfont` (originally
written by Mark himself), `ttf2txf` (from the [Celestia](https://celestiaproject.space/) 
project) or this `font2txf` utility.

Below you'll find an example of a `TXF` generated font, from the `Arial`
TrueType font:

![Example](img/arial.png "Sample Arial TXF image")

## Building

This program is a standard C program which may be compiled with **GNU Make**.
It requires `FreeType` installed.
[Learn more about FreeType here](http://freetype.org/).

1. Edit the `Makefile.cfg` and check if everything is OK for you;
2. Enter `make` (`gmake` on BSD systems).

If you want to enable the **Preview** feature (as displayed in this 
`README` file), you will have to install `freeglut`. 
[Learn more about freeglut here](https://freeglut.sourceforge.net/).

Please note that you can indeed make a static binary by setting up
the `STANDALONE_BINARY` flag to `1`. This was created with Microsoft 
Windows in mind, but could work on other OS as well.

## Usage

To use this tool, the usage is nearly identical as the original `ttf2txf`
utility. The minimal command-line is:

    ./font2txf <fontfile.ttf>

This will convert `fontfile.ttf` to the corresponding `fontfile.txf`,
using the defaults, which are a `256x256` texture size, using a `20pt` font
size and the default charset, which is:

    (space)(A..Z)1234567890(a..z)?.;,!*:"/+-|'@#$%^&<>()[]{}_

Of course, `(space)` means a blank space character (` `) and `(A..Z)` means
every characters, from `A` to `Z` (same for `(a..z)`).

Example of a more complete command-line:

	./ttf2txf -o revenant.txf -s 22 -w 256 -h 256 -c "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()-_=+[]{}|;:,.<>~ ?/" revenant.ttf

The `revenant.txf` file will be generated from the `revenant.ttf` font,
in a `256x256` texture, using the charset passed with the `-c` switch, 
and using the `22pt` size.

### Available Options

Available options are (displayed with the `-h` switch):

    -w <width>         Texture width (default: 256)
    -e <height>        Texture height (default: 256); also `-h` for compatibility
    -c <string>        Override charset to convert; read from command-line
                        Cannot be mixed with `-f`
    -f <filename.txt>  Override charset to convert; read from a text file
                        Cannot be mixed with `-c`
    -g <gap>           Space between glyphs (default: 1)
    -s <size>          Font point size (default: 20)
    -o <filename.txf>  Output file for textured font (default: <fontfile>.txf)
    -q                 Quiet; except error messages, cannot be mixed with `-v`
    -v                 Verbose; display more info, cannot be mixed with `-q`
    -p                 Preview; display the txf output at the end of the process
    -h                 Usage information (you're looking at it); if `-w` not set

### Altering the generated texture

The `-w` (width) and `-e` (height) options are used for altering the size of the
generated texture. For compatibility reasons, `-e` can be replaced by `-h`, if
used with `-w`. Indeed in the original `ttf2txf` tool, `-h` was mapped to texture
height, but now it means displaying help.

### Altering the default charset

By default, a character set is already defined (see above), but you can alter it
by using 2 different ways:

1. Directly from the command-line; using the `-c` switch
2. From a text file; using the `-f` switch

Of course, you can't mix the 2 options at the same time.

If using the `-c` switch, you can of course use quotes in your command-line, in
that case, if you need to include the `"` character, you will need to escape it
(usually with `\`).

If you are using the `-f` switch, you have the possibility to create a text file
containing a character per line, using the unicode value, in hex format, like:

    0020
    0021
    0022
    0023
    0024
    0025
    0026
    0027
    0028
    0029
    002A
    002B
    002C
    002D
    002E
    002F
    ...

A good example is the [wgl4.txt](https://web.archive.org/web/20050908024112/http://www.shatters.net/~claurel/celestia/fonts/tt2txf/wgl4.txt)
file used for the [Celestia](https://celestiaproject.space/) project.

### Preview the texture after generation

If you enable the `PREVIEW` flag in `Makefile.cfg`, you will have the
`-p` switch available. Before doing so, you will have to install 
[freeglut](https://freeglut.sourceforge.net/).

This option will allow you to see the converted texture, at the end of
the conversion process.

### Verbose or quiet

You have the possibility to output more info while running this tool,
using the `-v` option. In the other side, if you want to hide everything,
you can use the `-q` option. In that case, only warning, error and fatal
messages will be displayed.

Of couse, you can't mix `-v` and `-q` options at the same time.

## Useful links

* [A Simple OpenGL-based API for Texture Mapped Text](http://sgifiles.irixnet.org/sgi/opengl/contrib/mjk/tips/TexFont/TexFont.html)
  (the original Mark J. Kilgard article while he was working at SGI)
* [Building .txf font texture files for the Celestia project](https://en.wikibooks.org/wiki/Celestia/Internationalization#Building_.txf_font_texture_files)
* [Online Celestia Font Texture Generator](http://web.archive.org/web/20080413101857/http://celestia.teyssier.org/ttf2txf/index.html)
  (not working anymore but some useful info is displayed)
* [OpenGL Utility Toolkit (GLUT) TXF examples](https://www.opengl.org/archives/resources/code/samples/glut_examples/texfont/texfont.html)

## Acknowledgments

* [Mark J. Kilgard](https://github.com/markkilgard): Creator of
  the Texture Mapped Font (`TXF`) format.
* [Chris Laurel](https://www.fifthstarlabs.com/chris) and the
  [Celestia project team](https://celestiaproject.space/): 
  The initial authors of the original 
  [ttf2txf](https://web.archive.org/web/20051104023112/http://www.shatters.net/~claurel/celestia/fonts/tt2txf/)
  utility.
* [Mickaël Cardoso](http://www.mickael-cardoso.fr/) ([SiZiOUS](http://sizious.com/)): Complete refactoring of the project.

## License

This project is licensed under the **GNU GPL 2 License** - see
the [LICENSE](LICENSE) file for details.
