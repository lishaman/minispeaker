// System headers.
#include <stdio.h>
#include <stdlib.h>

#include "saveBmp.h"



/* ================================================================================ */
static int write_long(FILE *fp,int  l);
static int write_dword(FILE *fp, unsigned int dw);
static int write_word(FILE *fp, unsigned short w);

#define BF_TYPE 0x4D42             

typedef struct BMPINFOHEADER {                     
	unsigned int   biSize;           
	int            biWidth;          
	int            biHeight;         
	unsigned short biPlanes;         
	unsigned short biBitCount;       
	unsigned int   biCompression;    
	unsigned int   biSizeImage;      
	int            biXPelsPerMeter;  
	int            biYPelsPerMeter;  
	unsigned int   biClrUsed;        
	unsigned int   biClrImportant;   
} BMPINFOHEADER;

typedef struct BITMAPFILEHEADER {                     
	unsigned int   bfType;           
	int            bfSize;          
	int            bfOffBits;         
	unsigned short bfReserved1;         
	unsigned short bfReserved2;       

} BITMAPFILEHEADER;

typedef union RGB {
	struct{             
		unsigned char   rgbBlue;          
		unsigned char   rgbGreen;         
		unsigned char   rgbRed;           
		unsigned char   rgbReserved;
	};
	unsigned long u;
} RGB;

typedef struct _BMPINFO {                      
	BMPINFOHEADER   bmiHeader;      
	RGB             bmiColors[256]; 
} BMPINFO;

static inline int write_word(FILE *fp, unsigned short w)
{
	putc(w, fp);
	return (putc(w >> 8, fp));
}

/*
 * 'write_dword()' - Write a 32-bit unsigned integer.
 */
static inline int write_dword(FILE *fp, unsigned int dw)
{
	putc(dw, fp);
	putc(dw >> 8, fp);
	putc(dw >> 16, fp);
	return (putc(dw >> 24, fp));
}


/*
 * 'write_long()' - Write a 32-bit signed integer.
 */
static inline int write_long(FILE *fp,int  l)
{
	putc(l, fp);
	putc(l >> 8, fp);
	putc(l >> 16, fp);
	return (putc(l >> 24, fp));
}

/*
 * rgb_order: 0, BGRA, BGRX. 1, RGBA, RGBX
 */
int SaveBufferToBmp32(const char *filename, unsigned char *buffer, int alignedWidth, int alignedHeight, int wStride, int rgb_order)
{

	FILE *fp;
	BMPINFO bitmap;
	BMPINFO *info = &bitmap;                      
	unsigned int    size, infosize, bitsize;                 
	int i, srcStride, dstStride;

	printf("-1-SaveBufferToBmp32, savebmp \"%s\"\n", filename);
	if ((fp = fopen(filename, "wb")) == NULL)
		return (-1);

	bitmap.bmiHeader.biSize = 40;
	bitmap.bmiHeader.biWidth = alignedWidth;
	bitmap.bmiHeader.biHeight = -alignedHeight;
	bitmap.bmiHeader.biPlanes = 1;
	bitmap.bmiHeader.biBitCount = 32;
	bitmap.bmiHeader.biCompression = 3;
	bitmap.bmiHeader.biSizeImage = bitsize =  info->bmiHeader.biWidth *
		((info->bmiHeader.biBitCount + 7) / 8) *
		(-info->bmiHeader.biHeight);;
	bitmap.bmiHeader.biXPelsPerMeter = 0;
	bitmap.bmiHeader.biYPelsPerMeter = 0;
	bitmap.bmiHeader.biClrUsed = 0;
	bitmap.bmiHeader.biClrImportant = 0;

        if( rgb_order == 1 ) {
            bitmap.bmiColors[2].u           =  0x00FF0000;
            bitmap.bmiColors[1].u           = 0x0000FF00;
            bitmap.bmiColors[0].u           = 0x000000FF;
        }
        else {
            bitmap.bmiColors[0].u           =  0x00FF0000;
            bitmap.bmiColors[1].u           = 0x0000FF00;
            bitmap.bmiColors[2].u           = 0x000000FF;
        }


	srcStride  =  info->bmiHeader.biWidth * ((info->bmiHeader.biBitCount + 7) / 8);
	if ( wStride == 0 ) {
		dstStride  =  alignedWidth * ((info->bmiHeader.biBitCount + 7) / 8);
	}
	else {
		dstStride  =  wStride;
	}

	infosize = 40+12;

	size = 14 + infosize + bitsize;

	write_word(fp, BF_TYPE);        
	write_dword(fp, size);          
	write_word(fp, 0);              
	write_word(fp, 0);              
	write_dword(fp, 14 + infosize); 

	write_dword(fp, info->bmiHeader.biSize);
	write_long(fp, info->bmiHeader.biWidth);
	write_long(fp, abs(info->bmiHeader.biHeight));
	write_word(fp, info->bmiHeader.biPlanes);
	write_word(fp, info->bmiHeader.biBitCount);
	write_dword(fp, info->bmiHeader.biCompression);
	write_dword(fp, info->bmiHeader.biSizeImage);
	write_long(fp, info->bmiHeader.biXPelsPerMeter);
	write_long(fp, info->bmiHeader.biYPelsPerMeter);
	write_dword(fp, info->bmiHeader.biClrUsed);
	write_dword(fp, info->bmiHeader.biClrImportant);
	write_long(fp, bitmap.bmiColors[0].u);
	write_long(fp, bitmap.bmiColors[1].u);
	write_long(fp, bitmap.bmiColors[2].u);

	for (i = 0; i < abs(info->bmiHeader.biHeight); i++){
		if (info->bmiHeader.biHeight > 0) {
			fwrite(buffer + dstStride * i, 1, srcStride, fp);
		}
		else {
			fwrite(buffer + dstStride * (abs(info->bmiHeader.biHeight) - 1 - i), 1, srcStride, fp);
		}
	}

	fclose(fp);

	return (0);

}
int SaveBuffer(const char *filename, unsigned char *buffer, int alignedWidth, int alignedHeight, int wStride)
{
	FILE *fp;
	int i, srcStride, dstStride;

	printf("-+-SaveBuffer, savebmp \"%s\"\n", filename);

	if ((fp = fopen(filename, "wb")) == NULL)
		return (-1);

#if 0
	dstStride = srcStride = 1;
	for (i = 0; i < alignedHeight * alignedWidth * 2; i++){
			fwrite(buffer + dstStride * i, 1, srcStride, fp);
	}
#endif
	dstStride = srcStride = wStride;
	dstStride = srcStride = wStride;
	for (i = 0; i < alignedHeight; i++){
			fwrite(buffer + dstStride * i, 1, srcStride, fp);
	}
	fclose(fp);
	return (0);
}

int SaveBufferToBmp16(const char *filename, unsigned char *buffer, int alignedWidth, int alignedHeight, int wStride)
{
	FILE *fp;
	BMPINFO bitmap;
	BMPINFO *info = &bitmap;                      
	unsigned int    size, infosize, bitsize;                 
	int i, srcStride, dstStride;

	printf("-1-SaveBufferToBmp16, savebmp \"%s\"\n", filename);

	if ((fp = fopen(filename, "wb")) == NULL)
		return (-1);



	bitmap.bmiHeader.biSize = 40;
	bitmap.bmiHeader.biWidth = alignedWidth;
	bitmap.bmiHeader.biHeight = -alignedHeight;
	bitmap.bmiHeader.biPlanes = 1;
	bitmap.bmiHeader.biBitCount = 16;
	bitmap.bmiHeader.biCompression = 3;
	bitmap.bmiHeader.biSizeImage = bitsize =  info->bmiHeader.biWidth *
		((info->bmiHeader.biBitCount + 7) / 8) *
		(-info->bmiHeader.biHeight);;
	bitmap.bmiHeader.biXPelsPerMeter = 0;
	bitmap.bmiHeader.biYPelsPerMeter = 0;
	bitmap.bmiHeader.biClrUsed = 0;
	bitmap.bmiHeader.biClrImportant = 0;

	bitmap.bmiColors[0].u           =  0xF800;
	bitmap.bmiColors[1].u           = 0x07E0;
	bitmap.bmiColors[2].u           = 0x001F;



	srcStride  =  info->bmiHeader.biWidth * ((info->bmiHeader.biBitCount + 7) / 8);
	if ( wStride == 0 ) {
		dstStride  =  alignedWidth * ((info->bmiHeader.biBitCount + 7) / 8);
	}
	else {
		dstStride  =  wStride;
	}


	infosize = 40+12;

	size = 14 + infosize + bitsize;

	write_word(fp, BF_TYPE);        
	write_dword(fp, size);          
	write_word(fp, 0);              
	write_word(fp, 0);              
	write_dword(fp, 14 + infosize); 

	write_dword(fp, info->bmiHeader.biSize);
	write_long(fp, info->bmiHeader.biWidth);
	write_long(fp, abs(info->bmiHeader.biHeight));
	write_word(fp, info->bmiHeader.biPlanes);
	write_word(fp, info->bmiHeader.biBitCount);
	write_dword(fp, info->bmiHeader.biCompression);
	write_dword(fp, info->bmiHeader.biSizeImage);
	write_long(fp, info->bmiHeader.biXPelsPerMeter);
	write_long(fp, info->bmiHeader.biYPelsPerMeter);
	write_dword(fp, info->bmiHeader.biClrUsed);
	write_dword(fp, info->bmiHeader.biClrImportant);
	write_long(fp, bitmap.bmiColors[0].u);
	write_long(fp, bitmap.bmiColors[1].u);
	write_long(fp, bitmap.bmiColors[2].u);
	for (i = 0; i < abs(info->bmiHeader.biHeight); i++){
		if (info->bmiHeader.biHeight > 0)
			fwrite(buffer + dstStride * i, 1, srcStride, fp);
		else
			fwrite(buffer + dstStride * (abs(info->bmiHeader.biHeight) - 1 - i), 1, srcStride, fp);
	}

	fclose(fp);
	return (0);
}

/* ================================================================================ */
