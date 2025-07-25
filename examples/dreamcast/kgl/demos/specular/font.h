/*
   KallistiOS 2.0.0

   font.h
   (C) 2013 Josh Pearson
*/

#ifndef FONT_H
#define FONT_H

#define ASCI_SYMBOL_OFT 32
#define ASCI_TOTAL_CHAR 128-ASCI_SYMBOL_OFT
#define INVALID_UV -1.0f

typedef struct {
    float TexW, TexH;
    float CharW, CharH;
    unsigned char RowStride, ColStride;
    unsigned int TexFmt;
    unsigned int Color;
    unsigned int TexId;
    float  TexUV[ASCI_TOTAL_CHAR][4];
    void  *TexAddr;
} Font;

Font *FontInit(float TexW, float TexH,
               unsigned char RowStride, unsigned char ColStride,
               uint32_t Color);

void FontPrintString(Font *font, char *str, float xpos, float ypos,
                     float width, float height);

void FontSetColor(Font *font, uint32_t color);

#endif
