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

EXPORT uint8_t* reduce_frame_channel(uint8_t* data, int width, int height, int dst_channel, int src_channel);

EXPORT struct bounding_box* find_bounding_box(uint8_t* data, int threshold, int width, int height, struct bounding_box *pre_bounding_box);
EXPORT uint8_t* find_motion_mask(struct darray *framelist, int width, int height, int channel, int motion_level);
//EXPORT uint8_t* find_roi(uint8_t* data, int width, int height);
EXPORT void average_smooth(uint8_t* data, int width, int height, int channel);
EXPORT void median_smooth(uint8_t* data, int width, int height);
