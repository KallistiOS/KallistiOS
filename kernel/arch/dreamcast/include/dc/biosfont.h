/* KallistiOS ##version##

   dc/biosfont.h
   Copyright (C) 2000-2001 Megan Potter
   Japanese Functions Copyright (C) 2002 Kazuaki Matsumoto
   Copyright (C) 2017 Donald Haase
   Copyright (C) 2024 Falco Girgis
   Copyright (C) 2024 Andress Barajas

*/

/** \file    dc/biosfont.h
    \brief   BIOS font drawing functions.
    \ingroup bfont

    This file provides support for utilizing the font built into the Dreamcast's
    BIOS. These functions allow access to both the western character set and
    Japanese characters.

    \author Megan Potter
    \author Kazuaki Matsumoto
    \author Donald Haase
    \author Falco Girgis

    \todo
        - More user-friendly way to fetch/print DC-specific icons.
*/

#ifndef __DC_BIOSFONT_H
#define __DC_BIOSFONT_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

/** \defgroup bfont     BIOS
    \brief              API for the Dreamcast's built-in BIOS font
    \ingroup            video_fonts
    @{
*/

/** \defgroup bfont_size  Dimensions
    \brief    Sizes for of the BIOS font's dimensions
    @{
*/
#define BFONT_THIN_WIDTH                        12  /**< \brief Width of Thin Font (ISO8859_1, half-JP) */
#define BFONT_WIDE_WIDTH    (BFONT_THIN_WIDTH * 2)  /**< \brief Width of Wide Font (full-JP) */
#define BFONT_HEIGHT                            24  /**< \brief Height of All Fonts */
/** @} */

/** \brief Number of bytes to represent a single character within the BIOS font. */
#define BFONT_BYTES_PER_CHAR        (BFONT_THIN_WIDTH * BFONT_HEIGHT / 8)
#define BFONT_BYTES_PER_WIDE_CHAR   (BFONT_WIDE_WIDTH * BFONT_HEIGHT / 8)

/** \defgroup bfont_indicies Structure
    \brief                   Structure of the Bios Font
    @{
*/
/** \brief Start of Narrow Characters in Font Block */
#define BFONT_NARROW_START          0   
#define BFONT_OVERBAR               BFONT_NARROW_START
#define BFONT_ISO_8859_1_33_126     (BFONT_NARROW_START + ( 1 * BFONT_BYTES_PER_CHAR))
#define BFONT_YEN                   (BFONT_NARROW_START + (95 * BFONT_BYTES_PER_CHAR))
#define BFONT_ISO_8859_1_160_255    (BFONT_NARROW_START + (96 * BFONT_BYTES_PER_CHAR))

/* JISX-0208 Rows 1-7 and 16-84 are encoded between BFONT_WIDE_START and BFONT_DREAMCAST_SPECIFIC.
    Only the box-drawing characters (row 8) are missing. */
/** \brief Size of a row for JISX-0208 characters */
#define JISX_0208_ROW_SIZE          94
/** \brief Start of Wide Characters in Font Block */
#define BFONT_WIDE_START            (288 * BFONT_BYTES_PER_CHAR)
/** \brief Start of JISX-0208 Rows 1-7 in Font Block */   
#define BFONT_JISX_0208_ROW1        BFONT_WIDE_START
/** \brief Start of JISX-0208 Row 16-47 (Start of Level 1) in Font Block */   
#define BFONT_JISX_0208_ROW16       (BFONT_WIDE_START + (658 * BFONT_BYTES_PER_WIDE_CHAR))
/** \brief JISX-0208 Row 48-84 (Start of Level 2) in Font Block */
#define BFONT_JISX_0208_ROW48       (BFONT_JISX_0208_ROW16 + ((32 * JISX_0208_ROW_SIZE) * BFONT_BYTES_PER_WIDE_CHAR))

/** \brief Start of DC Specific Characters in Font Block */
#define BFONT_DREAMCAST_SPECIFIC    (BFONT_WIDE_START + (7056 * BFONT_BYTES_PER_WIDE_CHAR))

/** \brief Takes a DC-specific icon index and returns a character offset. */
#define BFONT_DC_ICON(offset)       (BFONT_DREAMCAST_SPECIFIC + ((offset) * BFONT_BYTES_PER_WIDE_CHAR))

/** \defgroup bfont_dc_indices Dreamcast-Specific 
    \brief    Dreamcast-specific BIOS icon offsets.
    @{
*/
#define BFONT_CIRCLECOPYRIGHT       BFONT_DC_ICON(0)    /**< \brief Circle copyright */
#define BFONT_CIRCLER               BFONT_DC_ICON(1)    /**< \brief Circle restricted */
#define BFONT_TRADEMARK             BFONT_DC_ICON(2)    /**< \brief Trademark */
#define BFONT_UPARROW               BFONT_DC_ICON(3)    /**< \brief Up arrow */
#define BFONT_DOWNARROW             BFONT_DC_ICON(4)    /**< \brief Down arrow */
#define BFONT_LEFTARROW             BFONT_DC_ICON(5)    /**< \brief Left arrow */
#define BFONT_RIGHTARROW            BFONT_DC_ICON(6)    /**< \brief Right arrow */
#define BFONT_UPRIGHTARROW          BFONT_DC_ICON(7)    /**< \brief Up right arrow */
#define BFONT_DOWNRIGHTARROW        BFONT_DC_ICON(8)    /**< \brief Down right arrow */
#define BFONT_DOWNLEFTARROW         BFONT_DC_ICON(9)    /**< \brief Down left arrow */
#define BFONT_UPLEFTARROW           BFONT_DC_ICON(10)   /**< \brief Up left arrow */
#define BFONT_ABUTTON               BFONT_DC_ICON(11)   /**< \brief A button */
#define BFONT_BBUTTON               BFONT_DC_ICON(12)   /**< \brief B button */
#define BFONT_CBUTTON               BFONT_DC_ICON(13)   /**< \brief C button */
#define BFONT_DBUTTON               BFONT_DC_ICON(14)   /**< \brief D button */
#define BFONT_XBUTTON               BFONT_DC_ICON(15)   /**< \brief X button */
#define BFONT_YBUTTON               BFONT_DC_ICON(16)   /**< \brief Y button */
#define BFONT_ZBUTTON               BFONT_DC_ICON(17)   /**< \brief Z button */
#define BFONT_LTRIGGER              BFONT_DC_ICON(18)   /**< \brief L trigger */
#define BFONT_RTRIGGER              BFONT_DC_ICON(19)   /**< \brief R trigger */
#define BFONT_STARTBUTTON           BFONT_DC_ICON(20)   /**< \brief Start button */
#define BFONT_VMUICON               BFONT_DC_ICON(21)   /**< \brief VMU icon */
/** @} */

#define BFONT_ICON_DIMEN                 32    /**< \brief Dimension of vmu icons */
#define BFONT_VMU_DREAMCAST_SPECIFIC     (BFONT_DREAMCAST_SPECIFIC+(22 * BFONT_BYTES_PER_WIDE_CHAR))
/** @} */

/** \brief Builtin VMU Icons
    \ingroup  bfont_indicies

    Builtin VMU volume user icons. The Dreamcast's
    BIOS allows the user to set these when formatting the VMU.
*/
typedef enum bfont_vmu_icon {
    BFONT_ICON_INVALID_VMU     = 0x00, /**< \brief Invalid */
    BFONT_ICON_HOURGLASS_ONE   = 0x01, /**< \brief Hourglass 1 */
    BFONT_ICON_HOURGLASS_TWO   = 0x02, /**< \brief Hourglass 2 */
    BFONT_ICON_HOURGLASS_THREE = 0x03, /**< \brief Hourglass 3 */
    BFONT_ICON_HOURGLASS_FOUR  = 0x04, /**< \brief Hourglass 4 */
    BFONT_ICON_VMUICON         = 0x05, /**< \brief VMU */
    BFONT_ICON_EARTH           = 0x06, /**< \brief Earth */
    BFONT_ICON_SATURN          = 0x07, /**< \brief Saturn */
    BFONT_ICON_QUARTER_MOON    = 0x08, /**< \brief Quarter moon */
    BFONT_ICON_LAUGHING_FACE   = 0x09, /**< \brief Laughing face */
    BFONT_ICON_SMILING_FACE    = 0x0A, /**< \brief Smiling face */
    BFONT_ICON_CASUAL_FACE     = 0x0B, /**< \brief Casual face */
    BFONT_ICON_ANGRY_FACE      = 0x0C, /**< \brief Angry face */
    BFONT_ICON_COW             = 0x0D, /**< \brief Cow */
    BFONT_ICON_HORSE           = 0x0E, /**< \brief Horse */
    BFONT_ICON_RABBIT          = 0x0F, /**< \brief Rabbit */
    BFONT_ICON_CAT             = 0x10, /**< \brief Cat */
    BFONT_ICON_CHICK           = 0x11, /**< \brief Chick */
    BFONT_ICON_LION            = 0x12, /**< \brief Lion */
    BFONT_ICON_MONKEY          = 0x13, /**< \brief Monkye */
    BFONT_ICON_PANDA           = 0x14, /**< \brief Panda */
    BFONT_ICON_BEAR            = 0x15, /**< \brief Bear */
    BFONT_ICON_PIG             = 0x16, /**< \brief Pig */
    BFONT_ICON_DOG             = 0x17, /**< \brief Dog */
    BFONT_ICON_FISH            = 0x18, /**< \brief Fish */
    BFONT_ICON_OCTOPUS         = 0x19, /**< \brief Octopus */
    BFONT_ICON_SQUID           = 0x1A, /**< \brief Squid */
    BFONT_ICON_WHALE           = 0x1B, /**< \brief Whale */
    BFONT_ICON_CRAB            = 0x1C, /**< \brief Crab */
    BFONT_ICON_BUTTERFLY       = 0x1D, /**< \brief Butterfly */
    BFONT_ICON_LADYBUG         = 0x1E, /**< \brief Ladybug */
    BFONT_ICON_ANGLER_FISH     = 0x1F, /**< \brief Angler fish */
    BFONT_ICON_PENGUIN         = 0x20, /**< \brief Penguin */
    BFONT_ICON_CHERRIES        = 0x21, /**< \brief Cherries */
    BFONT_ICON_TULIP           = 0x22, /**< \brief Tulip */
    BFONT_ICON_LEAF            = 0x23, /**< \brief Leaf */
    BFONT_ICON_SAKURA          = 0x24, /**< \brief Sakura */
    BFONT_ICON_APPLE           = 0x25, /**< \brief Apple */
    BFONT_ICON_ICECREAM        = 0x26, /**< \brief Ice cream */
    BFONT_ICON_CACTUS          = 0x27, /**< \brief Cactus */
    BFONT_ICON_PIANO           = 0x28, /**< \brief Piano */
    BFONT_ICON_GUITAR          = 0x29, /**< \brief Guitar */
    BFONT_ICON_EIGHTH_NOTE     = 0x2A, /**< \brief Eighth note */
    BFONT_ICON_TREBLE_CLEF     = 0x2B, /**< \brief Treble clef */
    BFONT_ICON_BOAT            = 0x2C, /**< \brief Boat */
    BFONT_ICON_CAR             = 0x2D, /**< \brief Car */
    BFONT_ICON_HELMET          = 0x2E, /**< \brief Helmet */
    BFONT_ICON_MOTORCYCLE      = 0x2F, /**< \brief Motorcycle */
    BFONT_ICON_VAN             = 0x30, /**< \brief Van */
    BFONT_ICON_TRUCK           = 0x31, /**< \brief Truck */
    BFONT_ICON_CLOCK           = 0x32, /**< \brief Clock */
    BFONT_ICON_TELEPHONE       = 0x33, /**< \brief Telephone */
    BFONT_ICON_PENCIL          = 0x34, /**< \brief Pencil */
    BFONT_ICON_CUP             = 0x35, /**< \brief Cup */
    BFONT_ICON_SILVERWARE      = 0x36, /**< \brief Silverware */
    BFONT_ICON_HOUSE           = 0x37, /**< \brief House */
    BFONT_ICON_BELL            = 0x38, /**< \brief Bell */
    BFONT_ICON_CROWN           = 0x39, /**< \brief Crown */
    BFONT_ICON_SOCK            = 0x3A, /**< \brief Sock */
    BFONT_ICON_CAKE            = 0x3B, /**< \brief cake */
    BFONT_ICON_KEY             = 0x3C, /**< \brief Key */
    BFONT_ICON_BOOK            = 0x3D, /**< \brief Book */
    BFONT_ICON_BASEBALL        = 0x3E, /**< \brief Baseball */
    BFONT_ICON_SOCCER          = 0x3F, /**< \brief Soccer */
    BFONT_ICON_BULB            = 0x40, /**< \brief Bulb */
    BFONT_ICON_TEDDY_BEAR      = 0x41, /**< \brief Teddy bear */
    BFONT_ICON_BOW_TIE         = 0x42, /**< \brief Bow tie */
    BFONT_ICON_BOW_ARROW       = 0x43, /**< \brief Bow and arrow */
    BFONT_ICON_SNOWMAN         = 0x44, /**< \brief Snowman */
    BFONT_ICON_LIGHTNING       = 0x45, /**< \brief Lightning */
    BFONT_ICON_SUN             = 0x46, /**< \brief Sun */
    BFONT_ICON_CLOUD           = 0x47, /**< \brief Cloud */
    BFONT_ICON_UMBRELLA        = 0x48, /**< \brief Umbrella */
    BFONT_ICON_ONE_STAR        = 0x49, /**< \brief One star */
    BFONT_ICON_TWO_STARS       = 0x4A, /**< \brief Two stars */
    BFONT_ICON_THREE_STARS     = 0x4B, /**< \brief Three stars */
    BFONT_ICON_FOUR_STARS      = 0x4C, /**< \brief Four stars */
    BFONT_ICON_HEART           = 0x4D, /**< \brief Heart */
    BFONT_ICON_DIAMOND         = 0x4E, /**< \brief Diamond */
    BFONT_ICON_SPADE           = 0x4F, /**< \brief Spade */
    BFONT_ICON_CLUB            = 0x50, /**< \brief Club */
    BFONT_ICON_JACK            = 0x51, /**< \brief Jack */
    BFONT_ICON_QUEEN           = 0x52, /**< \brief Queen */
    BFONT_ICON_KING            = 0x53, /**< \brief King */
    BFONT_ICON_JOKER           = 0x54, /**< \brief Joker */
    BFONT_ICON_ISLAND          = 0x55, /**< \brief Island */
    BFONT_ICON_0               = 0x56, /**< \brief `0` digit */
    BFONT_ICON_1               = 0x57, /**< \brief `1` digit */
    BFONT_ICON_2               = 0x58, /**< \brief `2` digit */
    BFONT_ICON_3               = 0x59, /**< \brief `3` digit */
    BFONT_ICON_4               = 0x5A, /**< \brief `4` digit */
    BFONT_ICON_5               = 0x5B, /**< \brief `5` digit */
    BFONT_ICON_6               = 0x5C, /**< \brief `6` digit */
    BFONT_ICON_7               = 0x5D, /**< \brief `7` digit */
    BFONT_ICON_8               = 0x5E, /**< \brief `8` digit */
    BFONT_ICON_9               = 0x5F, /**< \brief `9` digit */
    BFONT_ICON_A               = 0x60, /**< \brief `A` letter */
    BFONT_ICON_B               = 0x61, /**< \brief `B` letter */   
    BFONT_ICON_C               = 0x62, /**< \brief `C` letter */
    BFONT_ICON_D               = 0x63, /**< \brief `D` letter */
    BFONT_ICON_E               = 0x64, /**< \brief `E` letter */
    BFONT_ICON_F               = 0x65, /**< \brief `F` letter */
    BFONT_ICON_G               = 0x66, /**< \brief `G` letter */
    BFONT_ICON_H               = 0x67, /**< \brief `H` letter */
    BFONT_ICON_I               = 0x68, /**< \brief `I` letter */
    BFONT_ICON_J               = 0x69, /**< \brief `J` letter */
    BFONT_ICON_K               = 0x6A, /**< \brief `K` letter */
    BFONT_ICON_L               = 0x6B, /**< \brief `L` letter */
    BFONT_ICON_M               = 0x6C, /**< \brief `M` letter */
    BFONT_ICON_N               = 0x6D, /**< \brief `N` letter */
    BFONT_ICON_O               = 0x6E, /**< \brief `O` letter */
    BFONT_ICON_P               = 0x6F, /**< \brief `P` letter */
    BFONT_ICON_Q               = 0x70, /**< \brief `Q` letter */
    BFONT_ICON_R               = 0x71, /**< \brief `R` letter */
    BFONT_ICON_S               = 0x72, /**< \brief `S` letter */
    BFONT_ICON_T               = 0x73, /**< \brief `T` letter */
    BFONT_ICON_U               = 0x74, /**< \brief `U` letter */
    BFONT_ICON_V               = 0x75, /**< \brief `V` letter */
    BFONT_ICON_W               = 0x76, /**< \brief `W` letter */
    BFONT_ICON_X               = 0x77, /**< \brief `X` letter */
    BFONT_ICON_Y               = 0x78, /**< \brief `Y` letter */
    BFONT_ICON_Z               = 0x79, /**< \brief `Z` letter */
    BFONT_ICON_CHECKER_BOARD   = 0x7A, /**< \brief Checker board */
    BFONT_ICON_GRID            = 0x7B, /**< \brief Grid */
    BFONT_ICON_LIGHT_GRAY      = 0x7C, /**< \brief Light gray */
    BFONT_ICON_DIAG_GRID       = 0x7D, /**< \brief Diagonal grid */
    BFONT_ICON_PACMAN_GRID     = 0x7E, /**< \brief Pacman grid */
    BFONT_ICON_DARK_GRAY       = 0x7F, /**< \brief Dark gray */
    BFONT_ICON_EMBROIDERY      = 0x80  /**< \brief Embroidery */
} bfont_vmu_icon_t;
/** @} */

/** \name  Coloring
    \brief Methods for modifying the text color.
    @{
*/

/** \brief   Set the font foreground color.

    This function selects the foreground color to draw when a pixel is opaque in
    the font. The value passed in for the color should be in whatever pixel
    format that you intend to use for the image produced.

    \param  c               The color to use.
    \return                 The old foreground color.

    \sa bfont_set_background_color()
*/
uint32_t bfont_set_foreground_color(uint32_t c);

/** \brief   Set the font background color.

    This function selects the background color to draw when a pixel is drawn in
    the font. This color is only used for pixels not covered by the font when
    you have selected to have the font be opaque.

    \param  c               The color to use.
    \return                 The old background color.

    \sa bfont_set_foreground_color()
*/
uint32_t bfont_set_background_color(uint32_t c);

/** @} */

/* Constants for the function below */
typedef enum bfont_code {
    BFONT_CODE_ISO8859_1 = 0,   /**< \brief ISO-8859-1 (western) charset */
    BFONT_CODE_EUC       = 1,   /**< \brief EUC-JP charset */
    BFONT_CODE_SJIS      = 2,   /**< \brief Shift-JIS charset */
    BFONT_CODE_RAW       = 3   /**< \brief Raw indexing to the BFONT */
} bfont_code_t;

/** \brief   Set the font encoding.

    This function selects the font encoding that is used for the font. This
    allows you to select between the various character sets available.

    \param  enc             The character encoding in use
*/
void bfont_set_encoding(bfont_code_t enc);

/** \name Character Lookups
    \brief Methods for finding various font characters and icons.
    @{
*/

/** \brief   Find an ISO-8859-1 character in the font.

    This function retrieves a pointer to the font data for the specified
    character in the font, if its available. Generally, you will not have to
    use this function, use one of the bfont_draw_* functions instead.

    \param  ch              The character to look up
    \return                 A pointer to the raw character data
*/
uint8_t *bfont_find_char(uint32_t ch);

/** \brief   Find an full-width Japanese character in the font.

    This function retrieves a pointer to the font data for the specified
    character in the font, if its available. Generally, you will not have to
    use this function, use one of the bfont_draw_* functions instead.

    This function deals with full-width kana and kanji.

    \param  ch              The character to look up
    \return                 A pointer to the raw character data
*/
uint8_t *bfont_find_char_jp(uint32_t ch);

/** \brief   Find an half-width Japanese character in the font.

    This function retrieves a pointer to the font data for the specified
    character in the font, if its available. Generally, you will not have to
    use this function, use one of the bfont_draw_* functions instead.

    This function deals with half-width kana only.

    \param  ch              The character to look up
    \return                 A pointer to the raw character data
*/
uint8_t *bfont_find_char_jp_half(uint32_t ch);

/** \brief   Find a VMU icon.

    This function retrieves a pointer to the icon data for the specified VMU
    icon in the bios, if its available. The icon data is flipped both vertically
    and horizontally. Each vmu icon has dimensions 32x32 pixels and is 128 bytes
    long.

    \param  icon            The VMU icon index to look up.
    \return                 A pointer to the raw icon data or NULL if icon value
                            is incorrect.
*/
uint8_t *bfont_find_icon(bfont_vmu_icon_t icon);

/** @} */

/** \name  Character Drawing 
    \brief Methods for rendering characters.
    @{
*/

/** \brief   Draw a single character of any sort to the buffer.

    This function draws a single character in the set encoding to the given
    buffer. This function sits under draw, draw_thin, and draw_wide, while
    exposing the colors and bitdepths desired. This will allow the writing
    of bfont characters to paletted textures.

    \param buffer       The buffer to draw to.
    \param bufwidth     The width of the buffer in pixels.
    \param fg           The foreground color to use.
    \param bg           The background color to use.
    \param bpp          The number of bits per pixel in the buffer.
    \param opaque       If true, overwrite background areas with black,
                            otherwise do not change them from what they are.
    \param c            The character to draw.
    \param wide         Draw a wide character.
    \param iskana       Draw a half-width kana character.
    \return             Amount of width covered in bytes.
*/
size_t bfont_draw_ex(void *buffer, uint32_t bufwidth, uint32_t fg,
                     uint32_t bg, uint8_t bpp, bool opaque, uint32_t c,
                     bool wide, bool iskana);

/** \brief   Draw a single character to a buffer.

    This function draws a single character in the set encoding to the given
    buffer. Calling this is equivalent to calling bfont_draw_thin() with 0 for
    the final parameter.

    \param  buffer          The buffer to draw to (at least 12 x 24 pixels)
    \param  bufwidth        The width of the buffer in pixels
    \param  opaque          If true, overwrite blank areas with black,
                            otherwise do not change them from what they are
    \param  c               The character to draw
    \return                 Amount of width covered in bytes.
*/
size_t bfont_draw(void *buffer, uint32_t bufwidth, bool opaque, uint32_t c);

/** \brief   Draw a single thin character to a buffer.

    This function draws a single character in the set encoding to the given
    buffer. This only works with ISO-8859-1 characters and half-width kana.

    \param  buffer          The buffer to draw to (at least 12 x 24 pixels)
    \param  bufwidth        The width of the buffer in pixels
    \param  opaque          If true, overwrite blank areas with black,
                            otherwise do not change them from what they are
    \param  c               The character to draw
    \param  iskana          Set to 1 if the character is a kana, 0 if ISO-8859-1
    \return                 Amount of width covered in bytes.
*/
size_t bfont_draw_thin(void *buffer, uint32_t bufwidth, bool opaque,
                       uint32_t c, bool iskana);

/** \brief   Draw a single wide character to a buffer.

    This function draws a single character in the set encoding to the given
    buffer. This only works with full-width kana and kanji.

    \param  buffer          The buffer to draw to (at least 24 x 24 pixels)
    \param  bufwidth        The width of the buffer in pixels
    \param  opaque          If true, overwrite blank areas with black,
                            otherwise do not change them from what they are
    \param  c               The character to draw
    \return                 Amount of width covered in bytes.
*/
size_t bfont_draw_wide(void *buffer, uint32_t bufwidth, bool opaque, 
                       uint32_t c);

/** @} */

/** \name  String Drawing 
    \brief Methods for rendering formatted text.

    @{
*/

/** \brief   Draw a full string of any sort to any sort of buffer.

    This function draws a NUL-terminated string in the set encoding to the given
    buffer. This will automatically handle mixed half and full-width characters
    if the encoding is set to one of the Japanese encodings. Colors and bitdepth
    can be set.

    \param  b               The buffer to draw to.
    \param  width           The width of the buffer in pixels.
    \param  fg              The foreground color to use.
    \param  bg              The background color to use.
    \param  bpp             The number of bits per pixel in the buffer.
    \param  opaque          If true, overwrite background areas with black,
                            otherwise do not change them from what they are.
    \param str              The string to draw.

    \sa bfont_draw_str_ex_fmt(), bfont_draw_str_ex_va()
*/
void bfont_draw_str_ex(void *b, uint32_t width, uint32_t fg, uint32_t bg,
                       uint8_t bpp, bool opaque, const char *str);

/** \brief   Draw a full formatted string of any sort to any sort of buffer.

    This function is equivalent to bfont_draw_str_ex(), except that the string
    is formatted as with the `printf()` function.

    \param  b               The buffer to draw to.
    \param  width           The width of the buffer in pixels.
    \param  fg              The foreground color to use.
    \param  bg              The background color to use.
    \param  bpp             The number of bits per pixel in the buffer.
    \param  opaque          If true, overwrite background areas with black,
                            otherwise do not change them from what they are.
    \param  fmt             The printf-style format string to draw.
    \param  ...             Additional printf-style variadic arguments

    \sa bfont_draw_str_ex_vfmt()
*/
void bfont_draw_str_ex_fmt(void *b, uint32_t width, uint32_t fg, uint32_t bg,
                           uint8_t bpp, bool opaque, const char *fmt, ...)
                           __printflike(7, 8);

/** \brief   Draw formatted string of any sort to buffer (with va_args).

    This function is equivalent to bfont_draw_str_ex_fmt(), except that the
    variadic argument list is passed via a pointer to a va_list.

    \param  b               The buffer to draw to.
    \param  width           The width of the buffer in pixels.
    \param  fg              The foreground color to use.
    \param  bg              The background color to use.
    \param  bpp             The number of bits per pixel in the buffer.
    \param  opaque          If true, overwrite background areas with black,
                            otherwise do not change them from what they are.
    \param  fmt             The printf-style format string to draw.
    \param  var_args        Additional printf-style variadic arguments

    \sa bfont_draw_str_ex_fmt()
*/
void bfont_draw_str_ex_vfmt(void *b, uint32_t width, uint32_t fg, uint32_t bg,
                            uint8_t bpp, bool opaque, const char *fmt,
                            va_list *var_args);

/** \brief   Draw a full string to a buffer.

    This function draws a NUL-terminated string in the set encoding to the given
    buffer. This will automatically handle mixed half and full-width characters
    if the encoding is set to one of the Japanese encodings. Draws pre-set
    16-bit colors.

    \param  b               The buffer to draw to.
    \param  width           The width of the buffer in pixels.
    \param  opaque          If true, overwrite blank areas with bfont_bgcolor,
                            otherwise do not change them from what they are.
    \param  str             The string to draw.
*/
void bfont_draw_str(void *b, uint32_t width, bool opaque, const char *str);

/** \brief   Draw a full formatted string to a buffer.

    This function is equvalent to bfont_draw_str(), except that the string is
    formatted as with the `printf()` function.

    \param  b               The buffer to draw to.
    \param  width           The width of the buffer in pixels.
    \param  opaque          If true, overwrite blank areas with bfont_bgcolor,
                            otherwise do not change them from what they are.
    \param  fmt             The printf-style format string to draw.
    \param  ...             Additional printf-style variadic arguments.
*/
void bfont_draw_str_fmt(void *b, uint32_t width, bool opaque, const char *fmt,
                        ...) __printflike(4, 5);
                        
/** \brief   Draw a full formatted string to video ram (with va_args).
 
    This function is equivalent to bfont_draw_str_ex_vfmt(), except that 
    the variadic argument list is passed via a pointer to a va_list.

    \param  x               The x position to start drawing at.
    \param  y               The y position to start drawing at.
    \param  fg              The foreground color to use.
    \param  bg              The background color to use.
    \param  opaque          If true, overwrite background areas with black,
                            otherwise do not change them from what they are.
    \param  fmt             The printf-style format string to draw.
    \param  var_args        Additional printf-style variadic arguments

    \sa bfont_draw_str_ex()
*/
void bfont_draw_str_vram_vfmt(uint32_t x, uint32_t y, uint32_t fg, uint32_t bg, 
                              bool opaque, const char *fmt, 
                              va_list *var_args);

/** \brief   Draw a full string to video ram.

    This function draws a NUL-terminated string in the set encoding to video
    ram. This will automatically handle mixed half and full-width characters
    if the encoding is set to one of the Japanese encodings. Draws pre-set
    16-bit colors.

    \param  x               The x position to start drawing at.
    \param  y               The y position to start drawing at.
    \param  opaque          If true, overwrite blank areas with bfont_bgcolor,
                            otherwise do not change them from what they are.
    \param  fmt             The printf-style format string to draw.
    \param  ...             Additional printf-style variadic arguments.
*/
void bfont_draw_str_vram_fmt(uint32_t x, uint32_t y, bool opaque, const char *fmt, 
                             ...) __printflike(4, 5);

/** @} */

__END_DECLS

#endif  /* __DC_BIOSFONT_H */
