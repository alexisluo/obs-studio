#include "visual/visual-util.h"

typedef DARRAY(struct point*) points;

struct point {
    int x;
    int y;
};

struct edge {
    struct point* p1;
    struct point* p2;
    
    int weight;
};

struct bounding_box {
    struct point left_top;
    struct point right_bot;
    struct point center;
};

EXPORT struct bounding_box* find_bounding_box(uint8_t* data, int width, int height, struct bounding_box *pre_bounding_box);
EXPORT uint8_t* find_motion_mask(uint8_t* old_framedata, uint8_t *framedata, int width, int height, int channel);
//EXPORT uint8_t* find_roi(uint8_t* data, int width, int height);
EXPORT void average_smooth(uint8_t* data, int width, int height);
EXPORT void median_smooth(uint8_t* data, int width, int height);