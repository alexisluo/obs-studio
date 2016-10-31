#include "background-handler.h"
#include <time.h>
#include <stdlib.h>

struct bg_handler* create_background_handler(unsigned int width, unsigned int height) {
    struct bg_handler* handler = bzalloc(sizeof(struct bg_handler));
    handler->height = height;
    handler->width = width;
    
    handler->threshold = 15;
    handler->sample_num = 12;
    handler->sample_rate = 0.2;
    handler->radius = 1;
    
    handler->MIN_BG_COUNT = 5;
    handler->MAX_FG_COUNT = 100;
    handler->bg_samples = bmalloc(sizeof(uint8_t)*width*height * handler->sample_num);
    handler->fg_count = bmalloc(sizeof(unsigned int)*width*height);
    for (int i=0; i<width*height; ++i) {
        handler->fg_count[i] = 0;
    }
    
    handler->status = MODEL_NOT_INIT;
    
    return handler;
}

void release_background_handler(struct bg_handler* handler) {
    bfree(handler->bg_samples);
    bfree(handler->fg_count);
    bfree(handler);
}


static unsigned int find_random_neighbor_pos(int w, int h, unsigned int radius, int *r_w, int *r_h) {
    srand(time(NULL));
    int wr = rand()%(2*radius)-radius;
    int hr = rand()%(2*radius)-radius;
    
    *r_w = w+wr;
    *r_h = h+hr;
}

uint8_t* background_substraction(struct bg_handler* handler, uint8_t *framedata) {
    if (handler == NULL) {
        return NULL;
    }
    
    unsigned int pos;
    unsigned int width = handler->width;
    unsigned int height = handler->height;
    int sample_rate = handler->sample_rate;
    int sample_num = handler->sample_num;
    int rw,rh;
    
    uint8_t* bg_mask = malloc(sizeof(uint8_t)*width*height);
    if (handler->status == MODEL_NOT_INIT) {
        for (uint32_t h = 0; h < height; ++h) {
            for (uint32_t w = 0; w < width; ++w) {
                for (uint32_t s = 0; s < sample_num; ++s) {
                    find_random_neighbor_pos(w,h,handler->radius,&rw,&rh);
                    int_fit_in_range(&rw, 0, width-1);
                    int_fit_in_range(&rh, 0, height-1);
                    pos = rh*width+rw;
                    handler->bg_samples[pos*sample_num+s] = framedata[pos];
                }
            }
        }
        handler->status = MODEL_PROCESS;
        return NULL;
    }
    
    srand(time(NULL));
    int diff, ra, fg_count, bg_count, totol_bg_count;
    totol_bg_count = 0;
    for (uint32_t h = 0; h < height; ++h) {
        for (uint32_t w = 0; w < width; ++w) {
            pos = h*width+w;
            bg_count = 0;
            for (uint32_t s = 0; s < sample_num; ++s) {
                diff = abs( (int)framedata[pos] - (int)handler->bg_samples[pos*sample_num+s]);
                if (diff < handler->threshold) {
                    bg_count++;
                }
            }
            //blog(LOG_INFO, "Background found diff: %d", diff);

            if (bg_count > handler->MIN_BG_COUNT || handler->fg_count[pos] > handler->MAX_FG_COUNT) {
                bg_mask[pos] = 0;
                totol_bg_count++;
                //blog(LOG_INFO, "bgcount: %d", bg_count);
                
                //update sample pixel
                ra = rand()%100;
                if (ra < sample_rate*100) {
                    ra = rand()%sample_num;
                    handler->bg_samples[pos*sample_num+ra] = framedata[pos];
                }
                //update pixel neighbors
                for (int n=0; n<handler->radius * handler->radius; ++n) {
                    ra = rand()%100;
                    if (ra < sample_rate*100) {
                        find_random_neighbor_pos(w,h,handler->radius,&rw,&rh);
                        int_fit_in_range(&rw, 0, width-1);
                        int_fit_in_range(&rh, 0, height-1);
                        pos = rh*width+rw;
                        ra = rand()%sample_num;
                        handler->bg_samples[pos*sample_num+ra] = framedata[pos];
                    }
                }
                
                handler->fg_count[pos] = 0;
            } else {
                handler->fg_count[pos]++;
            }
        }
    }
    
    //if bg section is too small, reinit
    if (totol_bg_count < width*height/4)
        handler->status = MODEL_NOT_INIT;
    
    return bg_mask;
}