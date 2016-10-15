#pragma once

#include <obs-internal.h>
#include <obs-scene.h>
#include "obs-source.h"
#include <media-io/video-io.h>

#include "visual-util.h"
#include "background-handler.h"

#define TEXT_BG_ANALYSE	           "analyse background"
#define TEXT_STOP_BG_ANALYSE       "stop analysis"

#define VISUAL_FRAME_TYPE		   "visual_frame_type"
#define VISUAL_FRAME_TYPE_DESCRIPTION 		   "visual_frame_type_description"

enum visual_current_stage {
    VISUAL_STAGE_SHOW,
    VISUAL_STAGE_ANALYSE_BACKGROUND
};

struct visual_service {
	uint8_t* cached_data;
	unsigned int width;
	unsigned int height;
	enum visual_current_stage cur_stage;

	void(*cached_source)(struct obs_scene_item* item);
	void(*visual_render)();
    
    struct bg_handler *background_handler;

	pthread_mutex_t mutex;
};

EXPORT struct visual_service* get_visual_service();
EXPORT void cached_source(struct obs_scene_item* item);
EXPORT void visual_render();

EXPORT bool visual_analyse_background_clicked(obs_properties_t * ppts, obs_property_t *p, void *data);
