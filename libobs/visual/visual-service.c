#include "visual-service.h"
#include <stdlib.h>

struct visual_service *visualService = NULL;

struct visual_service* get_visual_service() {
	if (visualService != NULL)
		return visualService;

	visualService = bzalloc(sizeof(struct visual_service));
	visualService->cached_source = cached_source;
	visualService->visual_render = visual_render;
	visualService->cur_stage = VISUAL_STAGE_SHOW;

	struct obs_video_info ovi;
	obs_get_video_info(&ovi);
	visualService->width = ovi.base_width;
	visualService->height = ovi.base_height;
	visualService->cached_data = NULL;
    
    visualService->cached_camera_data = NULL;
    visualService->camera_width = 0;
    visualService->camera_height = 0;
    visualService->background_handler = NULL;

	if (pthread_mutex_init(&visualService->mutex, NULL) != 0) {
		free(visualService);
		return NULL;
	}

	return visualService;
}

uint8_t* get_yuv_y_channel(struct obs_source_frame *frame) {
    if (!frame)
        return NULL;
    
    enum video_format format = frame->format;
    unsigned int width, height;
    width = frame->width;
    height = frame->height;
    
    uint8_t* data = malloc(sizeof(uint8_t)*width*height);
    unsigned int ypos, pos;
    unsigned int r, g, b, y, Cb, Cr;
    for (uint32_t h = 0; h < height; ++h) {
        for (uint32_t w = 0; w < width; ++w) {
            if (frame->format == VIDEO_FORMAT_BGRA) {
                r = frame->data[0][4 * (h*width + w)];
                g= frame->data[0][4 * (h*width + w) + 1];
                b = frame->data[0][4 * (h*width + w) + 2];
                RGB2YCbCr(r, g, b, &y, &Cb, &Cr);
                data[h*width+w] = y;
            }
            else if (frame->format == VIDEO_FORMAT_YUY2) {
                pos = h*width + w;
                ypos = 2 * pos;
                data[h*width+w] =frame->data[0][ypos];
            }
            else if (frame->format == VIDEO_FORMAT_UYVY) {
                pos = h*width +w;
                ypos = 2 * pos+1;
                data[h*width+w] =frame->data[0][ypos];
                
            }
            else if (frame->format == VIDEO_FORMAT_I420) {
                data[h*width+w] = frame->data[0][h*width + w];
            }
        }
    }
    return data;
}

uint8_t* get_source_frame_data(struct obs_source_frame *frame) {
	if (!frame)
		return NULL;

	unsigned int width, height;
	width = frame->width;
	height = frame->height;
    
	uint8_t* data = bmalloc(sizeof(uint8_t)*width*height * 4);
	uint8_t y, Cb, Cr, u, v, r, g, b;
	unsigned int ypos, upos, vpos, pos;
	for (uint32_t h = 0; h < height; ++h) {
		for (uint32_t w = 0; w < width; ++w) {
			if (frame->format == VIDEO_FORMAT_BGRA) {
				data[4 * (h*width + w)] = frame->data[0][4 * (h*width + w)];
				data[4 * (h*width + w) + 1] = frame->data[0][4 * (h*width + w) + 1];
				data[4 * (h*width + w) + 2] = frame->data[0][4 * (h*width + w) + 2];
				data[4 * (h*width + w) + 3] = frame->data[0][4 * (h*width + w) + 3];
			}
			else if (frame->format == VIDEO_FORMAT_YUY2) {
                pos = h*width + w;
				ypos = 2 * pos;
				upos = (pos % 2 == 0) ? ypos + 1 : ypos - 1;
				vpos = (pos/2 % 2 == 0) ? ypos + 3 : ypos + 1;

				y = frame->data[0][ypos];
				u = frame->data[0][upos];
				v = frame->data[0][vpos];
				YUV2RGB(&r, &g, &b, y, u, v);

				data[4 * (h*width + w)] = b;
				data[4 * (h*width + w) + 1] = g;
				data[4 * (h*width + w) + 2] = r;
				data[4 * (h*width + w) + 3] = 255;
			}
            else if (frame->format == VIDEO_FORMAT_UYVY) {
                pos = h*width + w;
                ypos = 2 * pos+1;
                if (pos%2==0) {
                    upos = ypos-1;
                    vpos = ypos+1;
                } else {
                    upos = ypos-3;
                    vpos = ypos-1;
                }
                //upos = (pos % 2 == 0) ? ypos - 1 : ypos - 3;
                //vpos = (pos % 2 == 0) ? ypos + 1 : ypos - 1;
                
                y = frame->data[0][ypos];
                u = frame->data[0][upos];
                v = frame->data[0][vpos];
                //YUV2RGB(&r, &g, &b, y, u, v);
                yCbCr2RGB(&r, &g, &b, y, u, v);
                
                data[4 * (h*width + w)] = b;
                data[4 * (h*width + w) + 1] = g;
                data[4 * (h*width + w) + 2] = r;
                data[4 * (h*width + w) + 3] = 255;
                
            }
			else if (frame->format == VIDEO_FORMAT_I420) {
				y = frame->data[0][h*width + w];
				Cb = frame->data[1][(h / 2)*(width / 2) + w / 2];
				Cr = frame->data[2][(h / 2)*(width / 2) + w / 2];
				yCbCr2RGB(&r, &g, &b, y, Cb, Cr);

				data[4 * (h*width + w)] = b;
				data[4 * (h*width + w) + 1] = g;
				data[4 * (h*width + w) + 2] = r;
				data[4 * (h*width + w) + 3] = 255;
			}
			else {
				data[4 * (h*width + w)] = 0;
				data[4 * (h*width + w) + 1] = 0;
				data[4 * (h*width + w) + 2] = 0;
				data[4 * (h*width + w) + 3] = 0;
			}
		}
	}
	return data;
}

void cached_source(struct obs_scene_item* item) {
	struct obs_source* source = item->source;
	struct obs_source_frame *frame = obs_source_get_frame(source);
	if (NULL == frame)
		return;

	unsigned int width = visualService->width;
	unsigned int height = visualService->height;
	unsigned int frame_width = frame->width;
	unsigned int frame_height = frame->height;

	uint8_t *framedata = get_source_frame_data(frame);
    uint8_t *yframedata = get_yuv_y_channel(frame);
	obs_source_release_frame(source, frame);

	if (NULL == framedata)
		return;

	struct obs_source_frame *new_frame = obs_source_frame_create(VIDEO_FORMAT_BGRA, frame_width, frame_height);
	memcpy(new_frame->data[0], framedata, 4 * frame_width*frame_height);
	obs_data_t * settings = obs_source_get_settings(source);
	enum visual_frame_type visual_frame_type = obs_data_get_int(settings, VISUAL_FRAME_TYPE);
    
	pthread_mutex_lock(&visualService->mutex);
	if (visualService->cached_data == NULL) {
		visualService->cached_data = bmalloc(4 * width*height);
		for (int i = 0; i < width*height * 4; ++i)
			visualService->cached_data[i] = 0;
	}

	int tmp_color;
	const struct matrix4 *trans_mat = &item->draw_transform;
    
    int pos;
    if (visual_frame_type == CAMERA_FRAME) {
        if (visualService->cached_camera_data == NULL) {
            visualService->camera_width = frame_width;
            visualService->camera_height = frame_height;
            visualService->cached_camera_data = malloc(sizeof(uint8_t)*frame_width*frame_height*3);
        }
        
        if (visualService->camera_width!=frame_width || visualService->camera_height!=frame_height) {
            free(visualService->cached_camera_data);
            visualService->camera_width = frame_width;
            visualService->camera_height = frame_height;
            visualService->cached_camera_data = malloc(sizeof(uint8_t)*frame_width*frame_height*3);
        }
    }
    
    //get motion mask
    uint8_t *motion_mask = NULL;
    if (visual_frame_type == CAMERA_FRAME) {
        uint8_t* rgbdata = malloc(sizeof(uint8_t)*frame_width*frame_height*3);
        for (int h = 0; h < frame_height; ++h) {
            for (int w = 0; w < frame_width; ++w) {
                pos = h*frame_width+w;
                memcpy(&rgbdata[pos*3], &framedata[pos*4], 3);
            }
        }
        
        motion_mask = find_motion_mask(visualService->cached_camera_data, rgbdata, frame_width, frame_height, 3);
        if (visualService->bd_box!=NULL)
            free(visualService->bd_box);
        visualService->bd_box = find_bounding_box(motion_mask, frame_width, frame_height, visualService->bd_box);
        
        //cached camera data
        memcpy(visualService->cached_camera_data, rgbdata, 3*frame_width*frame_height);
        free(rgbdata);
    }
    
    unsigned int h, w, frame_w, frame_h;
    struct bounding_box *bd_box = visualService->bd_box;
    if (motion_mask != NULL && bd_box!=NULL) {
        //draw bounding box
        uint8_t blueColor[4];
        blueColor[0] = 255;
        blueColor[1] = 0;
        blueColor[2] = 0;
        blueColor[3] = 255;
        
        int h1 = bd_box->left_top.y, h2 = bd_box->right_bot.y;
        int w1 = bd_box->left_top.x, w2 = bd_box->right_bot.x;
        
        for (h=h1; h<h2; ++h) {
            memcpy(&framedata[4 * (h*frame_width + w1)], blueColor, 4);
            memcpy(&framedata[4 * (h*frame_width + w2)], blueColor, 4);
            motion_mask[h*frame_width + w1] = 1;
            motion_mask[h*frame_width + w2] = 1;
        }
        for (w=w1; w<w2; ++w) {
            memcpy(&framedata[4 * (h1*frame_width + w)], blueColor, 4);
            memcpy(&framedata[4 * (h2*frame_width + w)], blueColor, 4);
            motion_mask[h1*frame_width + w] = 1;
            motion_mask[h2*frame_width + w] = 1;
        }
    }

	uint8_t color, frame_color, alpha;
	uint8_t diff, bg_configdence;
	for (h = 0; h < height; ++h) {
		for (w = 0; w < width; ++w) {
			mat4_invtrans(trans_mat, &w, &h, &frame_w, &frame_h);
			if (frame_w < 0 || frame_w >= frame_width || frame_h < 0 || frame_h >= frame_height)
				continue;

            //find motion interest
            if (motion_mask!=NULL) {
                framedata[4 * (frame_h*frame_width + frame_w) + 3] *= motion_mask[frame_h*frame_width + frame_w];
            }
            
			alpha = framedata[4 * (frame_h*frame_width + frame_w) + 3];
			for (uint8_t c = 0; c < 3; ++c) {
				color = visualService->cached_data[4 * (h*width + w) + c];
				frame_color = framedata[4 * (frame_h*frame_width + frame_w) + c];

				if (visual_frame_type == NORMAL_FRAME) {
					visualService->cached_data[4 * (h*width + w) + c] = (((int)255 - alpha)*color + alpha*frame_color) / 255;
				}
				else if (visual_frame_type == SCREEN_FRAME) {
					tmp_color = (int)color + frame_color + color*frame_color / 255;
					visualService->cached_data[4 * (h*width + w) + c] = tmp_color>255 ? 255 : tmp_color;
				}
				else {
					visualService->cached_data[4 * (h*width + w) + c] = (((int)255 - alpha)*color + alpha*frame_color) / 255;
				}
				new_frame->data[0][4 * (frame_h*frame_width + frame_w) + c] = visualService->cached_data[4 * (h*width + w) + c];
			}
			new_frame->data[0][4 * (frame_h*frame_width + frame_w) + 3] = 255;
			visualService->cached_data[4 * (h*width + w) + 3] = 255;
		}
	}
    
	pthread_mutex_unlock(&visualService->mutex);

	pthread_mutex_lock(&source->async_mutex);
	source->visual_cur_async_frame = new_frame;
	pthread_mutex_unlock(&source->async_mutex);
	bfree(framedata);

    if (yframedata!=NULL)
        free(yframedata);
    //if (bg_mask!=NULL)
    //    free(bg_mask);
}

void visual_render() {
	pthread_mutex_lock(&visualService->mutex);
	if (NULL != visualService->cached_data) {
		bfree(visualService->cached_data);
		visualService->cached_data = NULL;
	}
	pthread_mutex_unlock(&visualService->mutex);
}


bool visual_analyse_background_clicked(obs_properties_t *ppts, obs_property_t *p, void *data) {
    /*
	if (visualService->cur_stage != VISUAL_STAGE_ANALYSE_BACKGROUND) {
		visualService->cur_stage = VISUAL_STAGE_ANALYSE_BACKGROUND;

		if (visualService->background_mean != NULL) {
			free(visualService->background_mean);
			visualService->background_mean = NULL;
		}
		if (visualService->background_var != NULL) {
			free(visualService->background_var);
			visualService->background_var = NULL;
		}
		visualService->background_curr_a = 1;
		obs_property_set_description(p, TEXT_STOP_BG_ANALYSE);
	}
	else {
		visualService->cur_stage = VISUAL_STAGE_SHOW;
		obs_property_set_description(p, TEXT_BG_ANALYSE);
	}

	return true;
     */
}