#include "background-handler.h"
#include <time.h>
#include <stdlib.h>

struct bg_handler* create_background_handler(unsigned int width, unsigned int height) {
    struct bg_handler* handler = bzalloc(sizeof(struct bg_handler));
    handler->height = height;
    handler->width = width;
    
    handler->threshold = 60;
    handler->sample_num = 15;
    handler->sample_rate = 0.2;
    handler->radius = 3;
    
    handler->MIN_BG_COUNT = 3;
    handler->MAX_FG_COUNT = 200;
    handler->bg_samples = bmalloc(sizeof(uint8_t)*width*height * handler->sample_num*3);
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

void background_substraction(struct bg_handler* handler, uint8_t *framedata) {
    if (handler == NULL) {
        return;
    }
    unsigned int pos;
    unsigned int width = handler->width;
    unsigned int height = handler->height;
    int sample_rate = handler->sample_rate;
    int sample_num = handler->sample_num;
    int rw,rh;
    if (handler->status == MODEL_NOT_INIT) {
        for (uint32_t h = 0; h < height; ++h) {
            for (uint32_t w = 0; w < width; ++w) {
                for (uint32_t s = 0; s < sample_num; ++s) {
                    find_random_neighbor_pos(w,h,handler->radius,&rw,&rh);
                    int_fit_in_range(&rw, 0, width-1);
                    int_fit_in_range(&rh, 0, height-1);
                    pos = rh*width+rw;
                    memcpy(&handler->bg_samples[(pos*sample_num+s)*3], &framedata[pos*4], 3);
                }
            }
        }
        handler->status = MODEL_PROCESS;
        return;
    }
    
    srand(time(NULL));
    int diff, ra, fg_count, bg_count;
    for (uint32_t h = 0; h < height; ++h) {
        for (uint32_t w = 0; w < width; ++w) {
            
            bg_count = 0;
            for (uint32_t s = 0; s < sample_num; ++s) {
                pos = h*width+w;
                for (uint8_t c=0; c<3; ++c) {
                    diff = abs( (int)framedata[pos*4+c] - handler->bg_samples[(pos*sample_num+s)*3+c] );
                    if (diff>handler->threshold) {
                        bg_count--;
                        break;
                    }
                }
                bg_count++;
            }
            //blog(LOG_INFO, "Background found diff: %d", diff);
                
            if (bg_count > handler->MIN_BG_COUNT || handler->fg_count[pos] > handler->MAX_FG_COUNT) {
                framedata[pos*4] = 255;
                framedata[pos*4+1] = 0;
                framedata[pos*4+2] = 0;
                framedata[pos*4+3] = 255; //bg alpha = 0
                
                //update sample pixel
                ra = rand()%100;
                if (ra < sample_rate*100) {
                    ra = rand()%sample_num;
                    memcpy(&handler->bg_samples[(pos*sample_num+ra)*3], &framedata[pos*4], 3);
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
                        memcpy(&handler->bg_samples[(pos*sample_num+ra)*3], &framedata[pos*4], 3);
                    }
                }
                
                break;
                handler->fg_count[pos] = 0;
            } else {
                //handler->fg_count[pos]++;
            }
        }
    }
}