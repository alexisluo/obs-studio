#include "visual/visual-util.h"

enum bg_model_status {
    MODEL_NOT_INIT,
    MODEL_PROCESS
};

struct bg_handler {
    uint8_t* bg_samples;
    unsigned int MIN_BG_COUNT; // a bg point at least equals to min bg samples
    unsigned int* fg_count;
    unsigned int MAX_FG_COUNT; //if fgcount>max_fg_count, then it's bg
    
    unsigned int width;
    unsigned int height;
    
    unsigned int radius; //max neigbors distance
    unsigned int threshold; //if |color1-color2|<threshold, then color1==color2
    unsigned int sample_num; //how many bg samples per pixel
    float sample_rate; //how many frame update once
    
    enum bg_model_status status;
};

EXPORT struct bg_handler* create_background_handler(unsigned int width, unsigned int height);
EXPORT void release_background_handler(struct bg_handler* handler);
EXPORT void background_substraction(struct bg_handler* handler, uint8_t *framedata);
