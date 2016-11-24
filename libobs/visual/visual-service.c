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
    
    da_init(visualService->cached_camera_data);
    visualService->camera_width = 0;
    visualService->camera_height = 0;
    visualService->background_handler = NULL;
    memset(&visualService->bd_box, 0, sizeof(int)*6);

	if (pthread_mutex_init(&visualService->mutex, NULL) != 0) {
		free(visualService);
		return NULL;
	}

	return visualService;
}

uint8_t* get_source_frame_data(struct obs_source_frame *frame) {
	if (!frame)
		return NULL;
    
    //blog(LOG_INFO, "video format:%d", frame->format);

	unsigned int width, height;
	width = frame->width;
	height = frame->height;
    
	uint8_t* data = malloc(sizeof(uint8_t)*width*height * 4);
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

void change_frame_focus(enum visual_frame_focus_type focus_type, struct obs_scene_item* item, int frame_width, int frame_height) {
    const struct matrix4 *trans_mat = &item->draw_transform;
    int height=0, width=0, left=0, top=0;
    float scalex=0, scaley=0;
    struct bounding_box *bd_box = &visualService->bd_box;
    struct uint8_t* motion_mask = visualService->cached_motion_mask;
    struct matrix4 temp;
    
    switch (focus_type) {
        case BOUNDING_BOX_FOCUS:
            if (bd_box == NULL) return;
            width = bd_box->right_bot.x - bd_box->left_top.x + 1;
            height = bd_box->right_bot.y - bd_box->left_top.y + 1;
            left = bd_box->left_top.x;
            top = bd_box->left_top.y;
            
            if (width<=1 || height <=1 || frame_width<=0 || frame_height<=0) return;
            
            int offset_w = left + (frame_width-width)/2;
            int offset_h = top + (frame_height-height)/2;
            
            vec4_set(&temp.x, trans_mat->x.x, 0.0f, 0.0f, 0.0f);
            vec4_set(&temp.y, 0.0f, trans_mat->y.y, 0.0f, 0.0f);
            vec4_set(&temp.z, 0.0f, 0.0f, 1.0, 0.0f);
            vec4_set(&temp.t, offset_w, offset_h, 0.0f, 1.0f);
            matrix4_copy(trans_mat, &temp);
            break;
            
        case BOUNDING_BOX_RESIZE_FOCUS:
            if (bd_box == NULL) return;
            
            width = bd_box->right_bot.x - bd_box->left_top.x + 1;
            height = bd_box->right_bot.y - bd_box->left_top.y + 1;
            left = bd_box->left_top.x;
            top = bd_box->left_top.y;
            
            if (width<=1 || height <=1 || frame_width<=0 || frame_height<=0) return;
            
            scalex = (float)width/frame_width;
            scaley = (float)height/frame_height;
            
            vec4_set(&temp.x, scalex, 0.0f, 0.0f, 0.0f);
            vec4_set(&temp.y, 0.0f, scaley, 0.0f, 0.0f);
            vec4_set(&temp.z, 0.0f, 0.0f, 1.0, 0.0f);
            vec4_set(&temp.t, left, top, 0.0f, 1.0f);
            matrix4_copy(trans_mat, &temp);
            
            break;
        
        case MOTION_MASK_FOCUS:
            //if (motion_mask == NULL) return;
            break;
        
        default:
            break;
    }
}

void cache_camera_data(uint8_t* frame_data, int frame_width, int frame_height) {
    //pop out data
    if (visualService->cached_camera_data.num > 1) {
        uint8_t *top = visualService->cached_camera_data.array[0];
        da_erase(visualService->cached_camera_data, 0);
        if (top != NULL) free(top);
    }
    //if width or height is not matched , empty the cache
    if (visualService->camera_width!=frame_width || visualService->camera_height!=frame_height) {
        while (visualService->cached_camera_data.num > 1) {
            uint8_t *top = visualService->cached_camera_data.array[0];
            da_erase(visualService->cached_camera_data, 0);
            if (top != NULL) free(top);
        }
    }
    
    visualService->camera_width = frame_width;
    visualService->camera_height = frame_height;
    
    uint8_t* rgbdata = reduce_frame_channel(frame_data, frame_width, frame_height, 3, 4);
    da_push_back(visualService->cached_camera_data, &rgbdata);
}

void blend_frame_color(uint8_t* dst, enum visual_frame_type visual_frame_type, uint8_t color_a, uint8_t color_b, uint8_t alpha) {
    int tmp_color;
    if (visual_frame_type == SCREEN_FRAME) {
        tmp_color = (int)color_a + color_b + color_a*color_b/255;
        *dst = tmp_color > 255 ? 255 : tmp_color;
        *dst = (((int)255 - alpha)*color_a + alpha*(*dst)) / 255;
    }
    else {
        *dst = (((int)255 - alpha)*color_a + alpha*color_b) / 255;
    }
}

void cached_source(struct obs_scene_item* item) {
//    blog(LOG_INFO, "s1");
    
	struct obs_source* source = item->source;
	struct obs_source_frame *frame = obs_source_get_frame(source);
	if (NULL == frame)
		return;

	unsigned int width = visualService->width;
	unsigned int height = visualService->height;
	unsigned int frame_width = frame->width;
	unsigned int frame_height = frame->height;

	uint8_t *framedata = get_source_frame_data(frame);
	obs_source_release_frame(source, frame);
    pthread_mutex_lock(&source->async_mutex);

	if (NULL == framedata) return;
	struct obs_source_frame *new_frame = obs_source_frame_create(VIDEO_FORMAT_BGRA, frame_width, frame_height);
    if (NULL == new_frame) return;
	memcpy(new_frame->data[0], framedata, 4 * frame_width*frame_height);
    
	obs_data_t * settings = obs_source_get_settings(source);
	enum visual_frame_type visual_frame_type = obs_data_get_int(settings, VISUAL_FRAME_TYPE);
    enum visual_frame_focus_type frame_focus_type = obs_data_get_int(settings, VISUAL_FRAME_FOCUS_TYPE);
    const struct matrix4 *trans_mat = &item->draw_transform;
    
	pthread_mutex_lock(&visualService->mutex);
    
    // init cache data
	if (visualService->cached_data == NULL) {
		visualService->cached_data = malloc(4 * width*height);
        memset(visualService->cached_data, 0, width*height * 4);
	}
    
    // cache camera data
    if (visual_frame_type == CAMERA_FRAME) {
        cache_camera_data(framedata, frame_width, frame_height);
    }
    
    //get motion mask
    uint8_t *motion_mask = NULL;
    int motion_level = 255; //control the motion level!
    int bdbox_threshold = motion_level/5;
    struct bounding_box *bd_box = NULL; //visualService->bd_box;
    if (visual_frame_type == CAMERA_FRAME) {
        motion_mask = find_motion_mask(&visualService->cached_camera_data, frame_width, frame_height, 3, motion_level);
        bd_box = find_bounding_box(motion_mask, bdbox_threshold, frame_width, frame_height, &visualService->bd_box);
        if (bd_box != NULL) {
            struct point* left_top = &bd_box->left_top;
            struct point* right_bot = &bd_box->right_bot;
            mat4_trans(trans_mat, &left_top->x, &left_top->y, &visualService->bd_box.left_top.x, &visualService->bd_box.left_top.y);
            mat4_trans(trans_mat, &right_bot->x, &right_bot->y, &visualService->bd_box.right_bot.x, &visualService->bd_box.right_bot.y);
            free(bd_box);
        }
    }
    
    //follow the mask or bounding box
    if (visual_frame_type == CAMERA_FRAME)
        visualService->camera_trans_mat = &item->draw_transform;
    if (visual_frame_type != CAMERA_FRAME && frame_focus_type != NORMAL_FOCUS) {
        change_frame_focus(frame_focus_type, item, frame_width, frame_height);
    }
    
    unsigned int motion_width = visualService->camera_width;
    unsigned int motion_height = visualService->camera_height;
    unsigned int h, w, frame_w, frame_h;
	uint8_t color, frame_color, alpha;
//	uint8_t diff, bg_configdence;
    
    //cache motion mask
    if (visual_frame_type == CAMERA_FRAME && motion_mask!=NULL && frame_focus_type == MOTION_MASK_SHOW) {
        for (h = 0; h < frame_height; ++h) {
            for (w = 0; w < frame_width; ++w) {
                color_bgr_pixel(&framedata[4 * (h*frame_width + w)], motion_mask[h*frame_width + w], motion_level);
            }
        }
    }
    
    // merge and cached frame
	for (h = 0; h < height; ++h) {
		for (w = 0; w < width; ++w) {
			mat4_invtrans(trans_mat, &w, &h, &frame_w, &frame_h);
            if (frame_w < 0 || frame_w >= frame_width || frame_h < 0 || frame_h >= frame_height)
				continue;
            
            alpha = framedata[4 * (frame_h*frame_width + frame_w) + 3];
            
            //merge with motion
            if (visual_frame_type != CAMERA_FRAME && frame_focus_type == MOTION_MASK_FOCUS && visualService->cached_motion_mask!=NULL) {
                int motion_w, motion_h;
                mat4_invtrans(visualService->camera_trans_mat, &w, &h, &motion_w, &motion_h);
                if (motion_w >= 0 && motion_w < motion_width && motion_h >= 0 && motion_h < motion_height) {
                    uint8_t* mask = visualService->cached_motion_mask;
                    alpha = 255* mask[motion_h*motion_width + motion_w] / motion_level;
                }
            }
             
			for (uint8_t c = 0; c < 3; ++c) {
				color = visualService->cached_data[4 * (h*width + w) + c];
				frame_color = framedata[4 * (frame_h*frame_width + frame_w) + c];
                
                blend_frame_color(&visualService->cached_data[4 * (h*width + w) + c], visual_frame_type, color, frame_color, alpha);
				//new_frame->data[0][4 * (frame_h*frame_width + frame_w) + c] = visualService->cached_data[4 * (h*width + w) + c];
			}
			//new_frame->data[0][4 * (frame_h*frame_width + frame_w) + 3] = 255;
			visualService->cached_data[4 * (h*width + w) + 3] = 255;
		}
	}
    
    // fill the new frame with cached frame value
    for (frame_h = 0; frame_h < frame_height; ++frame_h) {
        for (frame_w = 0; frame_w < frame_width; ++frame_w) {
            mat4_trans(trans_mat, &frame_w, &frame_h, &w, &h);
            if (w < 0 || w >= width || h < 0 || h >= height)
                continue;
            
            memcpy(&new_frame->data[0][4 * (frame_h*frame_width + frame_w)], &visualService->cached_data[4 * (h*width + w)], 4);
        }
    }
    
    if (motion_mask!=NULL) {
        if (visualService->cached_motion_mask != NULL) free(visualService->cached_motion_mask);
        visualService->cached_motion_mask = motion_mask;
    }
    
    pthread_mutex_unlock(&visualService->mutex);
    
//	pthread_mutex_lock(&source->async_mutex);
	source->visual_cur_async_frame = new_frame;
	pthread_mutex_unlock(&source->async_mutex);
	free(framedata);
}

void visual_render() {
	pthread_mutex_lock(&visualService->mutex);
	if (NULL != visualService->cached_data) {
		free(visualService->cached_data);
		visualService->cached_data = NULL;
	}
	pthread_mutex_unlock(&visualService->mutex);
}

void viusal_set_item_visible(obs_sceneitem_t *item) {
    //obs_sceneitem_set_visible(item, true);
}
