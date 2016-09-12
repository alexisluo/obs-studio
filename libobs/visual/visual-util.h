#include <obs.h>
#include <windows.h>

void check_video_format(enum video_format format);
LPWSTR charToW(const char* str);
LPWSTR intToW(int n, int len);
int str2Int(char* str);

//�ı�ͼ���С��ͼ����ÿ������ռ4λ
uint8_t* resize4Image(uint8_t* data, int w, int h, int newW, int newH);

//ͼ��ռ�ת��
void RGB2YCbCr(uint8_t r, uint8_t g, uint8_t b, uint8_t* y, uint8_t* Cb, uint8_t* Cr);
void yCbCr2RGB(uint8_t* r, uint8_t* g, uint8_t* b, uint8_t y, uint8_t Cb, uint8_t Cr);

void YUV2RGB(uint8_t* r, uint8_t* g, uint8_t* b, uint8_t y, uint8_t u, uint8_t v);

//void RGB2HSV(uint8_t r, uint8_t g, uint8_t b, uint8_t* h, uint8_t* s, uint8_t* v);
//void HSV2RGB(uint8_t* r, uint8_t* g, uint8_t* b, uint8_t h, uint8_t s, uint8_t v);