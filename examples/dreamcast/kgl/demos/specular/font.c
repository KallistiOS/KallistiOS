/* 
   KallistiOS 2.0.0

   font.c
   (C) 2013 Josh Pearson

   DC Font Render Routine using GL and KOS (C) Josh PH3NOM Pearson 2013
*/

#include <stdio.h>
#include <stdlib.h>

#include <KGL/gl.h>

#include "font.h"

Font *FontInit(float TexW, float TexH,
               unsigned char RowStride, unsigned char ColStride,
               uint32 Color) {
    Font *font = malloc(sizeof(Font));

    font->TexW      = TexW;
    font->TexH      = TexH;
    font->RowStride = RowStride;
    font->ColStride = ColStride;
    font->CharW = (float)TexW / RowStride;
    font->CharH = (float)TexH / ColStride;
    font->Color = Color;

    unsigned short i;

    for(i = 0; i < ASCI_TOTAL_CHAR; i++)
        font->TexUV[i][0] = INVALID_UV;

    return font;
}

unsigned char FontUvNotCached(Font *font, int index) {
    return font->TexUV[index][0] == INVALID_UV;
}

void FontDrawChar(Font *font, int index, float x, float y, float w, float h) {
    int col = index / font->RowStride;
    int row = index % font->RowStride;

    if(FontUvNotCached(font, index)) {
        font->TexUV[index][0] = ((row * font->CharW)) / font->TexW;
        font->TexUV[index][1] = ((row * font->CharW) + font->CharW) / font->TexW;
        font->TexUV[index][2] = ((col * font->CharH)) / font->TexH;
        font->TexUV[index][3] = ((col * font->CharH) + font->CharH) / font->TexH;
    }

    glColor1ui(font->Color);

    glTexCoord2f(font->TexUV[index][0], font->TexUV[index][3]);
    glKosVertex2f(x, y + h);

    glTexCoord2f(font->TexUV[index][1], font->TexUV[index][3]);
    glKosVertex2f(x + w, y + h);

    glTexCoord2f(font->TexUV[index][1], font->TexUV[index][2]);
    glKosVertex2f(x + w, y);

    glTexCoord2f(font->TexUV[index][0], font->TexUV[index][2]);
    glKosVertex2f(x, y);
}

void FontPrintString(Font *font, char *str, float xpos, float ypos,
                     float width, float height) {
    float x = xpos, y = ypos, w = width, h = height;
    int i = 0;

    while(str[i] != '\n' && str[i] != '\0') {
        FontDrawChar(font, str[i] - 32, x, y, w, h);

        x += w;
        i++;
    }
}
