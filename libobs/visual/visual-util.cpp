#include "visual-util.h"


void check_video_format(enum video_format format) {
	if (format == VIDEO_FORMAT_NONE) {
		MessageBox(GetActiveWindow(), TEXT("NONE"), TEXT("c"), MB_OK);
	}
	else if (format == VIDEO_FORMAT_I420) {
		MessageBox(GetActiveWindow(), TEXT("I420"), TEXT("c"), MB_OK);
	}
	else if (format == VIDEO_FORMAT_NV12) {
		MessageBox(GetActiveWindow(), TEXT("NV12"), TEXT("c"), MB_OK);
	}
	else if (format == VIDEO_FORMAT_YVYU) {
		MessageBox(GetActiveWindow(), TEXT("YVYU"), TEXT("c"), MB_OK);
	}
	else if (format == VIDEO_FORMAT_YUY2) {
		MessageBox(GetActiveWindow(), TEXT("YUY2"), TEXT("c"), MB_OK);
	}
	else if (format == VIDEO_FORMAT_UYVY) {
		MessageBox(GetActiveWindow(), TEXT("UYVY"), TEXT("c"), MB_OK);
	}
	else if (format == VIDEO_FORMAT_RGBA) {
		MessageBox(GetActiveWindow(), TEXT("RGBA"), TEXT("c"), MB_OK);
	}
	else if (format == VIDEO_FORMAT_BGRA) {
		MessageBox(GetActiveWindow(), TEXT("BGRA"), TEXT("c"), MB_OK);
	}
	else if (format == VIDEO_FORMAT_BGRX) {
		MessageBox(GetActiveWindow(), TEXT("BGRX"), TEXT("c"), MB_OK);
	}
	else if (format == VIDEO_FORMAT_I444) {
		MessageBox(GetActiveWindow(), TEXT("I444"), TEXT("c"), MB_OK);
	}
	else {
		MessageBox(GetActiveWindow(), TEXT("unknow format"), TEXT("c"), MB_OK);
	}
}

LPWSTR intToW(int n, int len) {
	wchar_t *wc = malloc(sizeof(wchar_t)*len);
	_itow_s(n, wc, len, len);

	return wc;
}

LPWSTR charToW(const char* str) {
	LPWSTR wchar = malloc(sizeof(wchar_t)*(strlen(str) + 1));
	MultiByteToWideChar(CP_UTF8, 0, str, strlen(str), wchar, strlen(str));
	wchar[strlen(wchar)] = '\0';
	return wchar;
}

int str2Int(char* str) {
	unsigned int res, i;
	res = i = 0;
	while (str[i]!=NULL && str[i] >= '0' && str[i] <= '9') {
		res *= 10;
		res += str[i] - '0';
		++i;
	}
	return res;
}

uint8_t* resize4Image(uint8_t* data, int w, int h, int newW, int newH) {
	uint8_t* newData = malloc(sizeof(uint8_t) * 4 * newW*newH);
	float wDiv = (float)w / newW;
	float hDiv = (float)h / newH;

	int pos, newPos;
	for (int j = 0; j < newH; ++j) {
		for (int i = 0; i < newW; ++i) {
			pos = 4 * ((int)(j*hDiv)*w + (int)(i*wDiv));
			newPos = 4*(j*newW + i);

			newData[newPos] = data[pos];
			newData[newPos+1] = data[pos+1];
			newData[newPos+2] = data[pos+2];
			newData[newPos+3] = data[pos+3];
		}
	}

	return newData;
}

void RGB2YCbCr(uint8_t r, uint8_t g, uint8_t b, uint8_t* y, uint8_t* Cb, uint8_t* Cr) {
	int y0 = 0.257*r + 0.504*g + 0.098*b + 16;
	int Cb0 = 0.257*r + 0.504*g + 0.098*b + 16;
	int Cr0 = 0.439*r - 0.368*g - 0.0714*b + 128;

	*y = y0 > 255 ? 255 : y0;
	*Cb = Cb0 > 255 ? 255 : Cb0;
	*Cr = Cr0 > 255 ? 255 : Cr0;
}

void yCbCr2RGB(uint8_t* r, uint8_t* g, uint8_t* b, uint8_t y, uint8_t Cb, uint8_t Cr) {
	int r0 = 1.164*(y - 16) + 1.596*(Cr - 128);
	int g0 = 1.164*(y - 16) - 0.392*(Cb - 128) - 0.813*(Cr - 128);
	int b0 = 1.164*(y - 16) + 2.017*(Cb - 128);

	*r = r0 > 255 ? 255 : r0;
	*g = g0 > 255 ? 255 : g0;
	*b = b0 > 255 ? 255 : b0;
}

void YUV2RGB(uint8_t* r, uint8_t* g, uint8_t* b, uint8_t y, uint8_t u, uint8_t v) {
	int r0 = y + 1.140*v;
	int g0 = y - 0.394*u - 0.581*v;
	int b0 = y + 2.032*u;

	*r = r0 > 255 ? 255 : r0;
	*g = g0 > 255 ? 255 : g0;
	*g = g0 < 0 ? 0 : g0;
	*b = b0 > 255 ? 255 : b0;
}

/*
void RGB2HSV(uint8_t r, uint8_t g, uint8_t b, uint8_t* h, uint8_t* s, uint8_t* v) {
	*r = y + 1.140*v;
	*g = y - 0.394*u - 0.581*v;
	*b = y + 2.032*u;
}

void HSV2RGB(uint8_t* r, uint8_t* g, uint8_t* b, uint8_t h, uint8_t s, uint8_t v) {
	*r = y + 1.140*v;
	*g = y - 0.394*u - 0.581*v;
	*b = y + 2.032*u;
}
*/