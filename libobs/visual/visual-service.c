#include "visual-service.h"

struct visual_service *visualService = NULL;

enum visual_current_stage {
	VISUAL_STAGE_SHOW,
	VISUAL_STAGE_ANALYSE_BACKGROUND
};

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

	if (pthread_mutex_init(&visualService->mutex, NULL) != 0) {
		free(visualService);
		return NULL;
	}

	return visualService;
}
/*
uint8_t* get_source_frame_data(struct obs_source* source, const struct matrix4 *trans_mat) {
	struct obs_source_frame *frame = obs_source_get_frame(source);
	if (!frame)
		return NULL;
	
	unsigned int width, height, frame_width, frame_height;
	width = visualService->width;
	height = visualService->height;
	frame_width = frame->width;
	frame_height = frame->height;

	uint8_t* data = malloc(sizeof(uint8_t)*width*height*4);
	for (uint32_t i = 0; i < sizeof(uint8_t)*width*height * 4; ++i)
		data[i] = 0;

	//MessageBox(GetActiveWindow(), intToW(frame->format, 10), TEXT("c"), MB_OK);

	uint8_t y, Cb, Cr, u, v, r, g, b;
	unsigned int trans_w, trans_h;
	for (uint32_t h = 0; h < height; ++h) {
		for (uint32_t w = 0; w < width; ++w) {
			mat4_invtrans(trans_mat, &w, &h, &trans_w, &trans_h);
			if (trans_w < 0 || trans_w >= frame_width || trans_h < 0 || trans_h >= frame_height)
				continue;

			if (frame->format == VIDEO_FORMAT_BGRA) {
				data[4 * (h*width + w)] = frame->data[0][4 * (trans_h*frame_width + trans_w)];
				data[4 * (h*width + w) + 1] = frame->data[0][4 * (trans_h*frame_width + trans_w) + 1];
				data[4 * (h*width + w) + 2] = frame->data[0][4 * (trans_h*frame_width + trans_w) + 2];
				data[4 * (h*width + w) + 3] = frame->data[0][4 * (trans_h*frame_width + trans_w) + 3];
			}
			else if (frame->format == VIDEO_FORMAT_YUY2) {
				y = frame->data[0][trans_h*frame_width + trans_w];
				u = frame->data[1][(trans_h*frame_width + trans_w)/2];
				v = frame->data[2][(trans_h*frame_width + trans_w)/2];
				YUV2RGB(&r, &g, &b, y, u, v);

				data[4 * (h*width + w)] = b;
				data[4 * (h*width + w) + 1] = g;
				data[4 * (h*width + w) + 2] = r;
				data[4 * (h*width + w) + 3] = 255;
			}
			else if (frame->format == VIDEO_FORMAT_I420) {
				y = frame->data[0][trans_h*frame_width + trans_w];
				Cb = frame->data[1][(trans_h / 2)*(frame_width / 2) + trans_w / 2];
				Cr = frame->data[2][(trans_h / 2)*(frame_width / 2) + trans_w / 2];
				yCbCr2RGB(&r, &g, &b, y, Cb, Cr);

				data[4 * (h*width + w)] = b;
				data[4 * (h*width + w) + 1] = g;
				data[4 * (h*width + w) + 2] = r;
				data[4 * (h*width + w) + 3] = 255;
			}
		}
	}
	obs_source_release_frame(source, frame);
	return data;
}
*/

void analyse_background(uint8_t *framedata) {
	unsigned int width = visualService->width;
	unsigned int height = visualService->height;

	if (visualService->background_var == NULL) {
		visualService->background_var = malloc(sizeof(uint8_t)*width*height * 3);
		for (uint32_t i = 0; i < height * width * 3; ++i)
			visualService->background_var[i] = 0;
	}

	if (visualService->background_mean == NULL) {
		visualService->background_mean = malloc(sizeof(uint8_t)*width*height*3);
		for (uint32_t h = 0; h < height; ++h) {
			for (uint32_t w = 0; w < width; ++w) {
				for (uint8_t c = 0; c < 3; ++c) {
					visualService->background_mean[3 * (h*width + w) + c] = framedata[4 * (h*width + w) + c];
				}
			}
		}
		return;
	}

	visualService->background_curr_a *= 0.2;
	float a = visualService->background_curr_a;

	unsigned int mean, data, var, data_var;
	for (uint32_t h = 0; h < height; ++h) {
		for (uint32_t w = 0; w < width; ++w) {
			for (uint8_t c = 0; c < 3; ++c) {
				mean = visualService->background_mean[3 * (h*width + w) + c];
				data = framedata[4 * (h*width + w) + c];
				visualService->background_mean[3 * (h*width + w) + c] = (1 - a)*mean + a*data;

				var = visualService->background_var[3 * (h*width + w) + c];
				data_var = (framedata[4 * (h*width + w) + c] - mean)*(framedata[4 * (h*width + w) + c] - mean);
				visualService->background_var[3 * (h*width + w) + c] = (1 - a)*var + a*data_var;
			}
		}
	}
}

uint8_t* get_source_frame_data(struct obs_source_frame *frame) {
	if (!frame)
		return NULL;

	unsigned int width, height;
	width = frame->width;
	height = frame->height;

	uint8_t* data = bmalloc(sizeof(uint8_t)*width*height * 4);
	uint8_t y, Cb, Cr, u, v, r, g, b;
	unsigned int ypos, upos, vpos;
	for (uint32_t h = 0; h < height; ++h) {
		for (uint32_t w = 0; w < width; ++w) {
			if (frame->format == VIDEO_FORMAT_BGRA) {
				data[4 * (h*width + w)] = frame->data[0][4 * (h*width + w)];
				data[4 * (h*width + w) + 1] = frame->data[0][4 * (h*width + w) + 1];
				data[4 * (h*width + w) + 2] = frame->data[0][4 * (h*width + w) + 2];
				data[4 * (h*width + w) + 3] = frame->data[0][4 * (h*width + w) + 3];
			}
			else if (frame->format == VIDEO_FORMAT_YUY2) {
				ypos = 2 * (h*width + w);
				upos = (ypos % 2 == 0) ? ypos + 1 : ypos - 1;
				vpos = (ypos % 2 == 0) ? ypos + 3 : ypos + 1;

				y = frame->data[0][ypos];
				u = frame->data[0][upos];
				v = frame->data[0][vpos];
				YUV2RGB(&r, &g, &b, y, u, v);

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
	/*
	if (visualService->cur_stage == VISUAL_STAGE_ANALYSE_BACKGROUND) {
		if (source->visual_frame_type == CAMERA_FRAME) {
			uint8_t *framedata = get_source_frame_data(source, &item->draw_transform);
			analyse_background(framedata);

			if (visualService->cached_data!=NULL)
				free(visualService->cached_data);

			visualService->cached_data = framedata;
		}
		return;
	}
	*/
	struct obs_source_frame *frame = obs_source_get_frame(source);
	if (NULL == frame)
		return;

	unsigned int width = visualService->width;
	unsigned int height = visualService->height;
	unsigned int frame_width = frame->width;
	unsigned int frame_height = frame->height;

	uint8_t *framedata = get_source_frame_data(frame);
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

	unsigned int h, w, frame_w, frame_h;
	uint8_t color, frame_color, alpha;
	uint8_t diff, bg_configdence;
	for (h = 0; h < height; ++h) {
		for (w = 0; w < width; ++w) {
			mat4_invtrans(trans_mat, &w, &h, &frame_w, &frame_h);
			if (frame_w < 0 || frame_w >= frame_width || frame_h < 0 || frame_h >= frame_height)
				continue;

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
				else if (visual_frame_type == CAMERA_FRAME) {
					if (visualService->background_mean != NULL && visualService->background_var != NULL) {
						diff = frame_color - visualService->background_mean[3 * (h*width + w) + c];
						bg_configdence = ((int)diff * diff) / visualService->background_var[3 * (h*width + w) + c];

						if (bg_configdence > 1)
							alpha = 0;
						else if (bg_configdence > 0.5)
							alpha = 255 * (2 - bg_configdence / 0.5);
						else
							alpha = 255;
					}
					else {
						visualService->cached_data[4 * (h*width + w) + c] = (((int)255 - alpha)*color + alpha*frame_color) / 255;
					}
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
}