/*
 * Created by MajesticWaffle on 6/9/22.
 * Copyright (c) 2022 Thicc Industries. All rights reserved.
 * This software is under the: Thicc-Industries-Do-What-You-Want-I-Dont-Care License.
 */

#include "geode_uiextensions.h"

/* UI Extensions */
Panel* uix_create_health_bar(Texture* t, uint atlas_active, uint atlas_inactive, uint length, int* value){
    Panel* bar = new Panel();
    bar -> type = PANEL_EMPTY;
    bar -> child_count = length;
    bar -> children = new Panel*[bar -> child_count];

    bar -> dynamic = true;

    struct Packet{
        uint atlas_1, atlas_2;
        int* val;
    };
    Packet* p = new Packet{atlas_active, atlas_inactive, value};

    bar -> packet = p;
    bar -> tick_func = [](Panel* p, void* v){
        Packet* pac = (Packet*)v;

        for(int i = 0; i < p -> child_count; ++i){
            ((Panel_Sprite*)p -> children[i]) -> atlas_index = i >= *(pac -> val) ? pac -> atlas_2 : pac -> atlas_1;
        }
    };

    for(int i = 0; i < bar -> child_count; ++i){
        Panel_Sprite* sp = new Panel_Sprite;
        sp -> p.position = {(int)t -> tile_size * i, 0};
        sp -> texture = t;
        sp -> atlas_index = atlas_active;

        bar -> children[i] = (Panel*)sp;
    }

    return bar;
}
Panel_Text* uix_create_int_display(Font* font, std::string prefix, int* value, uint update_interval){
    Panel_Text* t = new Panel_Text;

    t -> font = font;
    t -> text = prefix + std::to_string(*value);


    struct Packet{
        std::string prefix;
        int* val;
        long last_update;
        uint update_interval;
    };

    Packet* p = new Packet{prefix, value, g_time -> tick, update_interval};

    t -> p.dynamic = true;
    t -> p.packet = p;
    t -> p.tick_func =[](Panel* p, void* v){
        Packet* pac = (Packet*)v;
        if(g_time -> tick - pac -> last_update < pac -> update_interval)
            return;

        pac -> last_update = g_time -> tick;
        ((Panel_Text*)p) -> text = pac -> prefix + std::to_string(*(pac -> val));
    };

    return t;
}