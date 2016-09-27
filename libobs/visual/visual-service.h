﻿#pragma once

#include <obs-internal.h>
#include <obs-scene.h>
#include "obs-source.h"
#include <media-io/video-io.h>

#include "visual-util.h"

#define TEXT_BG_ANALYSE	           "analyse background"
#define TEXT_STOP_BG_ANALYSE       "stop analysis"

#define VISUAL_FRAME_TYPE		   "visual_frame_type"
#define VISUAL_FRAME_TYPE_DESCRIPTION 		   "visual_frame_type_description"

struct visual_service {
	uint8_t* cached_data;
	unsigned int width;
	unsigned int height;
	enum visual_current_stage cur_stage;

	uint8_t* background_mean;
	uint8_t* background_var;
	float background_curr_a;

	void(*cached_source)(struct obs_scene_item* item);
	void(*visual_render)();

	pthread_mutex_t mutex;
};

EXPORT struct visual_service* get_visual_service();
EXPORT void cached_source(struct obs_scene_item* item);
EXPORT void visual_render();

EXPORT bool visual_analyse_background_clicked(obs_properties_t * ppts, obs_property_t *p, void *data);
