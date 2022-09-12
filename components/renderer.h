/*
 * Created by MajesticWaffle on 5/24/22.
 * Copyright (c) 2022 Thicc Industries. All rights reserved.
 * This software is under the: Thicc-Industries-Do-What-You-Want-I-Dont-Care License.
 */

#define COMP_RENDERER 1
typedef struct Renderer{
    Component c;
    Texture* atlas_texture;
    SheetType sheet_type;
    uint atlas_index = 0;       //Override default atlas location
    uint frame_count;           //Number of animation frames
    uint* frame_order;          //Order of frames in array
    uint animation_rate;        //Rate at which to animate movement speed

    Renderer(){
        c.type = COMP_RENDERER;
    }

} Renderer;
