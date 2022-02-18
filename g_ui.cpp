
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
        //TODO debug warning
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

        if(g_dynamic_panel_registry[i] -> tick_func != nullptr)
            g_dynamic_panel_registry[i] -> tick_func(g_dynamic_panel_registry[i], g_dynamic_panel_registry[i] -> packet);

        for(uint j = 0; j < g_dynamic_panel_registry[i] -> child_count; ++j){
            if(g_dynamic_panel_registry[i] -> children[j] -> dynamic){
                Panel* test = g_dynamic_panel_registry[i] -> children[j];
                g_dynamic_panel_registry[i] -> children[j] -> tick_func(g_dynamic_panel_registry[i] -> children[j], g_dynamic_panel_registry[i] -> children[j] -> packet);
            }
        }
    }
}

void ui_button_tick(Panel* p, void* v){
    Panel_Button* pb = (Panel_Button*)p;
    Coord2d pos = input_mouse_position();
    pos.x = pos.x / (g_video_mode.window_scale);
    pos.y = pos.y / (g_video_mode.window_scale);

    Coord2i ppos = p -> position;

    //Recurses up the parentage
    Panel* parent = p -> parent;
    while(parent != nullptr){
        ppos = ppos + parent->position;
        parent = parent -> parent;
    }

    if( (pos.x >= ppos.x && pos.x <= ppos.x + p -> size.x)
    &&  (pos.y >= ppos.y && pos.y <= ppos.y + p -> size.y)
    &&  input_get_button_down_tick(GLFW_MOUSE_BUTTON_1)){
        
        pb -> click_func(pb -> packet);
    }
}

/* UI Extensions */
Panel* ui_create_health_bar(Texture* t, uint atlas_active, uint atlas_inactive, uint length, int* value){
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

Panel_Text* ui_create_int_display(Font* font, std::string prefix, int* value, uint update_interval){
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