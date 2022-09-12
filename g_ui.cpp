
#include "geode.h"

Panel* g_dynamic_panel_registry[0xFF];
void ui_dynamic_panel_deactivate_id(uint id);
uint g_dynamic_panel_highest_id = 0;

void ui_dynamic_panel_activate(Panel* dp){
    if(!dp -> dynamic){
        error("St panel used as dyn.", "Static panel used as dynamic panel.");
        return;
    }

    if(dp -> active){
        std::cout << "Panel already active!" << std::endl;
        return;
    }

    for(int i = 0; i < 0xFF; ++i){
        if(g_dynamic_panel_registry[i] == nullptr){
            g_dynamic_panel_registry[i] = dp;

            if(i > g_dynamic_panel_highest_id)
                g_dynamic_panel_highest_id = i;

            dp -> id = i;
            dp -> active = true;

            break;
        }
    }
}

void ui_dynamic_panel_deactivate(Panel* dp){
    if(!dp -> dynamic){
        error("St panel used as dyn.", "Static panel used as dynamic panel.");
        return;
    }

    if(!dp -> active){
        return;
    }

    ui_dynamic_panel_deactivate_id(dp -> id);
}

void ui_dynamic_panel_deactivate_id(uint id){
    if(id == g_dynamic_panel_highest_id){
        for(uint i = g_dynamic_panel_highest_id; i >= 0; --i){
            if(g_dynamic_panel_registry[i] != nullptr) {
                g_dynamic_panel_highest_id = i;
                break;
            }
        }
    }

    g_dynamic_panel_registry[id] -> active = false;
    g_dynamic_panel_registry[id] = nullptr;
}

void ui_tick(){

    for(uint i = 0; i <= g_dynamic_panel_highest_id; ++i){
        Panel* test = g_dynamic_panel_registry[i];

        if(g_dynamic_panel_registry[i] == nullptr)
            continue;

        int child = g_dynamic_panel_registry[i] -> child_count;
        for(uint j = 0; j < child; ++j){
            if(g_dynamic_panel_registry[i] == nullptr)
                break;

            if(g_dynamic_panel_registry[i] -> children[j] -> dynamic){
                g_dynamic_panel_registry[i] -> children[j] -> tick_func(g_dynamic_panel_registry[i] -> children[j], g_dynamic_panel_registry[i] -> children[j] -> packet);
            }
        }

        if(g_dynamic_panel_registry[i] != nullptr && g_dynamic_panel_registry[i] -> tick_func != nullptr)
            g_dynamic_panel_registry[i] -> tick_func(g_dynamic_panel_registry[i], g_dynamic_panel_registry[i] -> packet);
    }
}

void ui_button_tick(Panel* p, void* v){
    Panel_Button* pb = (Panel_Button*)p;
    Coord2d pos = input_mouse_position();
    pos.x = pos.x / (g_video_mode.window_scale);
    pos.y = pos.y / (g_video_mode.window_scale);

    Coord2i ppos = ui_panel_global_position(p);

    if( (pos.x >= ppos.x && pos.x <= ppos.x + p -> size.x)
    &&  (pos.y >= ppos.y && pos.y <= ppos.y + p -> size.y)
    &&  input_get_button_down_tick(GLFW_MOUSE_BUTTON_1)){
        
        pb -> click_func(pb -> packet);
    }
}

Coord2i ui_panel_global_position(Panel* p){
    Coord2i ppos = p -> position;

    //Recurses up the parentage
    Panel* parent = p -> parent;
    while(parent != nullptr){
        ppos = ppos + parent->position;
        parent = parent -> parent;
    }
    return ppos;
}
