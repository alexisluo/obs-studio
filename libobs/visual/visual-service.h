#pragma once

#include <obs-internal.h>
#include <obs-scene.h>
#include "obs-source.h"
#include <media-io/video-io.h>

#include "visual-util.h"
#include "background-handler.h"
#include "frame-process.h"

#define TEXT_BG_ANALYSE	           "analyse background"
#define TEXT_STOP_BG_ANALYSE       "stop analysis"

#define VISUAL_FRAME_TYPE		   "visual_frame_type"
#define VISUAL_FRAME_TYPE_DESCRIPTION 		   "visual_frame_type_description"

#define VISUAL_FRAME_FOCUS_TYPE     "visual_frame_focus_type"
#define VISUAL_FRAME_FOCUS_DESCRIPTION      "visual_frame_focus_description"

enum visual_current_stage {
    VISUAL_STAGE_SHOW,
    VISUAL_STAGE_ANALYSE_BACKGROUND
};

struct visual_service {
    //cached whole data
	uint8_t* cached_data;
	unsigned int width;
	unsigned int height;
    
    // cached camera data
    DARRAY(uint8_t*) cached_camera_data;
    struct matrix4 *camera_trans_mat;
    unsigned int camera_width;
    unsigned int camera_height;
    
	enum visual_current_stage cur_stage;

    //render function
	void(*cached_source)(struct obs_scene_item* item);
	void(*visual_render)();
    
    pthread_mutex_t mutex;
    
    //control data
    uint8_t *cached_motion_mask;
    
    struct bounding_box bd_box;
    
    
    //not using currently
    struct bg_handler *background_handler;
};

EXPORT struct visual_service* get_visual_service();
EXPORT void cached_source(struct obs_scene_item* item);
EXPORT void visual_render();
EXPORT void viusal_set_item_visible(obs_sceneitem_t *item);

EXPORT bool visual_analyse_background_clicked(obs_properties_t * ppts, obs_property_t *p, void *data);
