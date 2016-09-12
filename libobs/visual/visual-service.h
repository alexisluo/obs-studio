#pragma once

#include <obs-internal.h>
#include <obs-scene.h>
#include <media-io/video-io.h>

#include "visual-util.h"

struct visual_service {
	uint8_t* cached_data;
	unsigned int width;
	unsigned int height;

	void(*cached_source)(struct obs_scene_item* item);
	void(*visual_render)();
};

EXPORT struct visual_service* get_visual_service();
EXPORT void cached_source(struct obs_scene_item* item);
EXPORT void visual_render();
