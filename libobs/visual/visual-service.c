#include "visual-service.h"

struct visual_service *visualService = NULL;

struct visual_service* get_visual_service() {
	if (visualService != NULL)
		return visualService;

	visualService = bzalloc(sizeof(struct visual_service));
	visualService->cached_source = cached_source;
	visualService->visual_render = visual_render;

	struct obs_video_info ovi;
	obs_get_video_info(&ovi);
	visualService->width = ovi.base_width;
	visualService->height = ovi.base_height;
	visualService->cached_data = NULL;

	return visualService;
}

uint8_t* get_source_frame_data(struct obs_source* source, const struct matrix4 *trans_mat) {
	struct obs_source_frame *frame = obs_source_get_frame(source);
	source->cur_async_frame = frame;
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
	if (frame->format == VIDEO_FORMAT_I420) {
		uint8_t y, Cb, Cr, r, g, b;
		unsigned int trans_w, trans_h;
		for (uint32_t h = 0; h < frame_height; ++h) {
			for (uint32_t w = 0; w < frame_width; ++w) {
				y = frame->data[0][h*frame_width + w];
				Cb = frame->data[1][(h / 2)*(frame_width / 2) + w / 2];
				Cr = frame->data[2][(h / 2)*(frame_width / 2) + w / 2];
				yCbCr2RGB(&r, &g, &b, y, Cb, Cr);
				
				trans_w = trans_mat->x.x * w + trans_mat->y.x * h + trans_mat->t.x;
				trans_h = trans_mat->x.y * w + trans_mat->y.y * h + trans_mat->t.y;

				data[4 * (trans_h*width + trans_w)] = b;
				data[4 * (trans_h*width + trans_w)+1] = g;
				data[4 * (trans_h*width + trans_w)+2] = r;
				data[4 * (trans_h*width + trans_w)+3] = 255;
			}
		}
	}
	else if (frame->format == VIDEO_FORMAT_BGRA) {
		unsigned int trans_w, trans_h;
		for (uint32_t h = 0; h < frame_height; ++h) {
			for (uint32_t w = 0; w < frame_width; ++w) {
				trans_w = trans_mat->x.x * w + trans_mat->y.x * h + trans_mat->t.x;
				trans_h = trans_mat->x.y * w + trans_mat->y.y * h + trans_mat->t.y;

				if (trans_w<0 || trans_w>=width || trans_h<0 || trans_h>=height)
					continue;

				data[4 * (trans_h*width + trans_w)] = frame->data[0][4 * (h*frame_width + w)];
				data[4 * (trans_h*width + trans_w) + 1] = frame->data[0][4 * (h*frame_width + w)+1];
				data[4 * (trans_h*width + trans_w) + 2] = frame->data[0][4 * (h*frame_width + w)+2];
				data[4 * (trans_h*width + trans_w) + 3] = frame->data[0][4 * (h*frame_width + w)+3];
			}
		}
	}

	obs_source_release_frame(source, frame);
	return data;
}

void cached_source(struct obs_scene_item* item) {
	struct obs_source* source = item->source;
	uint8_t *framedata = get_source_frame_data(source, &item->draw_transform);
	if (NULL == framedata)
		return;

	unsigned int width = visualService->width;
	unsigned int height = visualService->height;

	if (visualService->cached_data == NULL) {
		visualService->cached_data = framedata;
		return;
	}

	for (uint32_t h = 0; h < height; ++h) {
		for (uint32_t w = 0; w < width; ++w) {

			for (uint8_t i = 0; i < 4; ++i) {
				if (framedata[4 * (h*width + w) + i] > 0) {
					MessageBox(GetActiveWindow(), intToW(framedata[4 * (h*width + w) + i], 10), TEXT("c"), MB_OK);
					visualService->cached_data[4 * (h*width + w) + i] = framedata[4 * (h*width + w) + i];
				}
			}
			/*
			visualService->cached_data[4 * (h*width + w)] = framedata[4 * (h*width + w)];
			visualService->cached_data[4 * (h*width + w)+1] = framedata[4 * (h*width + w)+1];
			visualService->cached_data[4 * (h*width + w)+2] = framedata[4 * (h*width + w)+2];
			visualService->cached_data[4 * (h*width + w)+3] = framedata[4 * (h*width + w)+3];
			*/
		}
	}

	free(framedata);
}

void visual_render() {
	if (NULL == visualService->cached_data)
		return;

	obs_enter_graphics();
	gs_texture_t *texture = gs_texture_create(visualService->width, visualService->height, GS_BGRA, 1,
		(const uint8_t**)&(visualService->cached_data), 0);

	if (!texture)
		return;

	gs_effect_t *effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
	gs_eparam_t *param = gs_effect_get_param_by_name(effect, "image");
	gs_effect_set_texture(param, texture);

	gs_technique_t *tech = gs_effect_get_technique(effect, "Draw");
	size_t passes = gs_technique_begin(tech);
	for (size_t i = 0; i < passes; ++i) {
		if (gs_technique_begin_pass(tech, i)) {
			gs_draw_sprite(texture, 0, visualService->width, visualService->height);
			gs_technique_end_pass(tech);
		}
	}
	gs_technique_end(tech);
	gs_texture_destroy(texture);

	obs_leave_graphics();

	free(visualService->cached_data);
	visualService->cached_data = NULL;

}