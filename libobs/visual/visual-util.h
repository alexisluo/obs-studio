#include <obs-internal.h>

/*
void check_video_format(enum video_format format);
LPWSTR charToW(const char* str);
LPWSTR intToW(int n, int len);
int str2Int(char* str);
 */

//改变图像大小，图像中每个像素占4位
uint8_t* resize4Image(uint8_t* data, int w, int h, int newW, int newH);

//图像空间转换
void RGB2YCbCr(uint8_t r, uint8_t g, uint8_t b, uint8_t* y, uint8_t* Cb, uint8_t* Cr);
void yCbCr2RGB(uint8_t* r, uint8_t* g, uint8_t* b, uint8_t y, uint8_t Cb, uint8_t Cr);

void YUV2RGB(uint8_t* r, uint8_t* g, uint8_t* b, uint8_t y, uint8_t u, uint8_t v);

//void RGB2HSV(uint8_t r, uint8_t g, uint8_t b, uint8_t* h, uint8_t* s, uint8_t* v);
//void HSV2RGB(uint8_t* r, uint8_t* g, uint8_t* b, uint8_t h, uint8_t s, uint8_t v);

//2d点 位置矩阵
void mat4_trans(const struct matrix4 *trans_mat, int* x, int* y, int *res_x, int *res_y);
void mat4_invtrans(const struct matrix4 *trans_mat, int* x, int* y, int *res_x, int *res_y);