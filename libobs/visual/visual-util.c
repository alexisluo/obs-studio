#include "visual-util.h"

void check_video_format(enum video_format format) {
    /*
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
     */
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

int int_fit_in_range(int *value, int min, int max) {
    *value = *value<min?min:*value;
    *value = *value>max?max:*value;
}

void RGB2YCbCr(uint8_t r, uint8_t g, uint8_t b, uint8_t* y, uint8_t* Cb, uint8_t* Cr) {
	int y0 = 0.257*r + 0.504*g + 0.098*b + 16;
	int Cb0 = 0.257*r + 0.504*g + 0.098*b + 16;
	int Cr0 = 0.439*r - 0.368*g - 0.0714*b + 128;
    
    int_fit_in_range(&y0, 0, 255);
    int_fit_in_range(&Cb0, 0, 255);
    int_fit_in_range(&Cr0, 0, 255);

	*y = y0;
	*Cb = Cb0;
	*Cr = Cr0;
}

void yCbCr2RGB(uint8_t* r, uint8_t* g, uint8_t* b, uint8_t y, uint8_t Cb, uint8_t Cr) {
    //hdtv half range
    //int r0 = 1.164*(y - 16) + 1.793*(Cr - 128);
    //int g0 = 1.164*(y - 16) - 0.213*(Cb - 128) - 0.533*(Cr - 128);
    //int b0 = 1.164*(y - 16) + 2.112*(Cb - 128);
    
    //hdtv full range turn red
    int r0 = y + 1.402*(Cr - 128);
    int g0 = y - 0.344*(Cb - 128) - 0.714*(Cr - 128);
    int b0 = y + 1.772*(Cb - 128);
    
    /*
	int r0 = 1.164*(y - 16) + 1.596*(Cr - 128);
	int g0 = 1.164*(y - 16) - 0.392*(Cb - 128) - 0.813*(Cr - 128);
	int b0 = 1.164*(y - 16) + 2.017*(Cb - 128);
     */

    int_fit_in_range(&r0, 0, 255);
    int_fit_in_range(&g0, 0, 255);
    int_fit_in_range(&b0, 0, 255);
    
    *r = r0;
    *g = b0;
    *b = b0;
}

void YUV2RGB(uint8_t* r, uint8_t* g, uint8_t* b, int y, int u, int v) {
    /*
	//int r0 = y + 1.13983*(v-128);
	//int g0 = y - 0.39465*(u-128) - 0.58060*(v-128);
	//int b0 = y + 2.03211*(u-128); //wrong formula
    */
    int r0 = y + 1.4075*(v-128);
    int g0 = y - 0.3455*(u-128) - 0.7169*(v-128);
    int b0 = y + 1.779*(u-128);

    int_fit_in_range(&r0, 0, 255);
    int_fit_in_range(&g0, 0, 255);
    int_fit_in_range(&b0, 0, 255);
    
    *r = r0;
    *g = b0;
    *b = b0;
}

//2d point, m_4 always 0001
void mat4_trans(const struct matrix4 *trans_mat, int* x, int* y, int *res_x, int *res_y) {
	float mat11, mat12, mat14, mat21, mat22, mat24;
	mat11 = trans_mat->x.x;
	mat12 = trans_mat->y.x;
	mat14 = trans_mat->t.x;
	mat21 = trans_mat->x.y;
	mat22 = trans_mat->y.y;
	mat24 = trans_mat->t.y;

	*res_x = (float)mat11 * *x + mat12 * *y + mat14;
	*res_y = (float)mat21 * *x + mat22 * *y + mat24;
}

//2d point, m_4 always 0001
void mat4_invtrans(const struct matrix4 *trans_mat, int* x, int* y, int *res_x, int *res_y) {
	float mat11, mat12, mat14, mat21, mat22, mat24;
	mat11 = trans_mat->x.x;
	mat12 = trans_mat->y.x;
	mat14 = trans_mat->t.x;
	mat21 = trans_mat->x.y;
	mat22 = trans_mat->y.y;
	mat24 = trans_mat->t.y;

	float divsior = mat11*mat22 - mat21*mat12;
	*res_x = round( (mat12* *y - mat22* *x + mat14*mat22 - mat24*mat12) / (-divsior) );
	*res_y = round( (mat11* *y - mat21* *x + mat14*mat21 - mat24*mat11) / divsior );
}


void setColor(uint8_t* dst, uint8_t b, uint8_t g, uint8_t r) {
    *(dst) = b;
    *(dst+1) = g;
    *(dst+2) = r;
}

uint8_t* get_palette() {
    int color_num = 7;
    uint8_t* palette = malloc(sizeof(uint8_t)*3*color_num);
    
    setColor(palette, 0, 0, 0);
    setColor(&palette[3], 0, 255, 191);
    setColor(&palette[6], 0, 255, 255);
    setColor(&palette[9], 0, 191, 255);
    setColor(&palette[12], 0, 128, 255);
    setColor(&palette[15], 0, 64, 255);
    setColor(&palette[18], 0, 0, 255);
    
    return palette;
}

/* color pixel
 * dst: the piexel being colored
 * level: the which level the pixel belong to
 * max_level: how many level in all
 */
void color_bgr_pixel(uint8_t* dst, int level, int max_level) {
    float alpha = (float)level/max_level;
    
    //using default color
    if (max_level < 7) {
        uint8_t* palette = get_palette();
        uint8_t* color = &palette[level*3];
        
        *(dst) = *(dst)*(1-alpha) + *(color)*alpha;
        *(dst+1) = *(dst+1)*(1-alpha) + *(color+1)*alpha;
        *(dst+2) = *(dst+2)*(1-alpha) + *(color+2)*alpha;
        //memcpy(dst, &palette[level*3], 3);
        free(palette);
    } else {
        uint8_t color[3];
        color[0] = 0;
        color[1] = round((float)(max_level-level) * 255/max_level);
        color[2] = round((float)level * 255/max_level);
        
        *(dst) = *(dst)*(1-alpha) + *(color)*alpha;
        *(dst+1) = *(dst+1)*(1-alpha) + *(color+1)*alpha;
        *(dst+2) = *(dst+2)*(1-alpha) + *(color+2)*alpha;
    }
}
