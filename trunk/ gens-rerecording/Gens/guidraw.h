#include "vdp_rend.h"

#ifndef _GUIDRAW_H
#define _GUIDRAW_H

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */
void Pixel (short x, short y, unsigned int color32, unsigned short color16, char wrap, unsigned char Opac = 0xFF);
void PutText (char *string, short x, short y, unsigned short xn, unsigned short yn, unsigned short xx, unsigned short yx, int outstyle, int style);
void DrawEccoOct (short x, short y, short r, unsigned int color32, unsigned short color16, char wrap, unsigned char Opac = 0xFF);
void DrawLine(short x1, short y1, short x2, short y2, unsigned int color32, unsigned short color16, char wrap, unsigned char Opac = 0xFF);
void DrawBoxPP (short x1, short y1, short x2, short y2, unsigned int color32, unsigned short color16, char wrap, unsigned char Opac = 0xFF);
void DrawBoxMWH (short x, short y, short w, short h, unsigned int color32, unsigned short color16, char wrap, unsigned char Opac = 0xFF);
void DrawBoxCWH (short x, short y, short w, short h, unsigned int color32, unsigned short color16, char wrap, unsigned char Opac = 0xFF);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */
#endif