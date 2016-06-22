#ifndef SAVE_BMP_H
#define SAVE_BMP_H


#ifdef __cplusplus
extern "C" {
#endif


int SaveBufferToBmp16(const char *filename, unsigned char *buffer, int alignedWidth, int alignedHeight, int stride);

/*
 * rgb_order: 0, BGRA, BGRX. 1, RGBA, RGBX
 */
int SaveBufferToBmp32(const char *filename, unsigned char *buffer, int alignedWidth, int alignedHeight, int stride, int rgb_order);
int SaveBuffer(const char *filename, unsigned char *buffer, int alignedWidth, int alignedHeight, int wStride);

#ifdef __cplusplus
}
#endif



#endif	/* SAVE_BMP_H */
