/*
 * Created by MajesticWaffle on 4/27/21.
 * Copyright (c) 2021 Thicc Industries. All rights reserved.
 * This software is under the: Thicc-Industries-Do-What-You-Want-I-Dont-Care License.
 */

#ifndef GEODE_GEODE_H
#define GEODE_GEODE_H

#include <string>
#include <iostream>
#include <map>
#include <cmath>
#include "GL/glew.h"
#include "GLFW/glfw3.h"

/*--- Settings, bitflags, and selectors ---*/

#define RENDER_SCALE 4
#define RENDER_DISTANCE 1
#define RENDER_WINX 240
#define RENDER_WINY 160

#define WORLD_PERLIN_SCALE 0.05
#define WORLD_WATER_SCALE 0.25

#define TEXTURE_MULTIPLE 0x01
#define TEXTURE_SINGLE   0x02
#define TEXTURE_STORE    0x04   //Duplicate the texture data for uh... reasons?

#define TILE_SOLID       0b00000001   //Tile has collisions
#define TILE_TEX_FLIP_X  0b00000010   //Tile should be rendered with texture flipped across X axis
#define TILE_TEX_FLIP_Y  0b00000100   //Tile should be rendered with texture flipped across Y axis
#define TILE_ANIMATED    0b00001000   //Animated tile

#define TILE_LAST TILE_ANIMATED
#define ENTITY_MAX 0xFFFF

#define STORAGE_SIZE 96

#define TIME_TPS 64

#define VOID_TILE {0, 0, 0, 0 }
/*--- Enums and structs ---*/

enum EntityState{
    ENT_STATE_STATIONARY,
    ENT_STATE_MOVING
};

enum MovementDirection{
    DIRECTION_NORTH,
    DIRECTION_EAST,
    DIRECTION_SOUTH,
    DIRECTION_WEST
};

typedef unsigned int  uint;     //Shorthand unsigned int type
typedef unsigned char uchar;    //Shorthand unsigned char type
typedef unsigned short ushort;  //Shorthand unsigned short type
typedef struct Option{
    bool value;
    bool previous;

    Option(const bool& value){
        this -> value = value;
        this -> previous = value;
    }

    Option& operator=(const bool& value){
        this -> value = value;
        return *this;
    }

    bool operator==(const bool& value) const{
        return this->value == value;
    }

    operator bool() { return this -> value; }

}Option;

//2D double coordinate
typedef struct Coord2d{
    double x, y;

    bool operator==(const Coord2d& c) const{
        return this->x == c.x && this->y == c.y;
    }

    Coord2d operator+(const Coord2d& c) const{
        return {this -> x + c.x, this -> y + c.y};
    }


}Coord2d;

//2D integer coordinate
typedef struct Coord2i{
    int x, y;

    bool operator==(const Coord2i& c) const{
        return this->x == c.x && this->y == c.y;
    }

    Coord2i operator+(const Coord2i& c) const{
        return {this -> x + c.x, this -> y + c.y};
    }
}Coord2i;

//Color struct
typedef struct Color{
    uchar r, g, b;
}Color;

//Video mode of the game window
typedef struct Video_Mode{
    Coord2i window_resolution;
    uint window_scale;
    uint world_scale;
    uint ui_scale;
    GLFWwindow* windowptr;
}Video_Mode;

//TODO: This is pointless
//Camera struct for player entity
typedef struct Camera{
    Coord2d position;
} Camera;

//Raw image data loaded from a BMP image
typedef struct Image{
    uint width;     //Size of image in pixels
    uint height;    //----------------------
  uchar* imageData; //Raw image data in RGB bmp format
} Image;

//OpenGL texture id with dimensions and texture uv information
typedef struct Texture{
    GLuint id;                  //OpenGL texture id
    uint width;                 //Size of texture in pixels
    uint height;                //------------------------
    uint tile_size;             //Size of tiles
    Coord2d atlas_uvs;          //Difference in UV coordinates per texture
    Image* image;               //Only used if TEXTURE_STORE is enabled on generation
} Texture;

typedef struct Font{
    Texture* t;
    std::string font_atlas;
} Font;

//World tile
typedef struct Block{
    uint atlas_index;   //Index of texture
    uchar options;      //Tile bitflags
    ushort packet1;     //ID of resource to drop
    ushort packet2;     //drop count
}Block;

//Simple AABB
typedef struct BoundingBox{
    Coord2d p1; //Upper Left coordinate
    Coord2d p2; //Upper Right coordinate
} BoundingBox;


//World chunk
typedef struct Chunk{
    Coord2i pos{};                  //Chunk coordinates
    uchar overlay_tiles[256]{};     //Base tiles
    uchar background_tiles[256]{};  //Overlay tiles
    Texture* render_texture[4] = {nullptr}; //Render textures
} Chunk;

typedef struct Map{
    uint id;
    Texture* tilemap;
    uint tile_count;
    Block* tile_properties;
}Map;

//All Entity types must contain an Entity member 'e' as their first member
typedef struct Entity{
    uint        id;                         //id of the entity in g_entity_registry
    Coord2d     position;
    Coord2d     velocity;
    Map*        map;
    uint        atlas_index;                //Index of the Upper-Left corner of sprites 3x3 sprite sheet //TODO: What?
    Coord2i     spritesheet_size;           //Size of the sprite sheet { directions, frames }
    uint        frame_count;                //Number of animation frames
    uint*       frame_order;                //Order of frames in array
    BoundingBox col_bounds{};               //Bounding Box for entity
    BoundingBox hit_bounds{};               //Hitbox for entity
    Camera      camera{8,8};                //TODO: Seems strange to give every ent. a camera, but i guess it could allow cool stuff like spectating? P.S. might be useful for multiplayer too.
    uint        move_state;                 //Is entity moving
    uint        direction;                  //Direction entity facing
    uint        animation_rate;             //Rate at which to animate movement speed
    int         health;                     //
    uint        type;                       //Type enum of Entity. Def. ENT_GENERIC
    void        (*tick_func)(Entity* e);    //Function executed on tick
    void        (*death_func)(Entity* e);   //Function executed on death
} Entity;

typedef struct Time{
    long   tick = 0;            //The number of game ticks
    double tick_delta = 0;      //Real time since last tick
    double delta = 0;           //Time since last frame
    double global = 0;          //Absolute time since game was started
    int    fps = 0;
    bool   paused = false;  //Pause time
} Time;

//Timer reference ID
typedef struct Timer{
    long duration;
    long starting_tick;
} Timer;

enum PanelType{
    PANEL_EMPTY,
    PANEL_BOX,
    PANEL_TEXT,
    PANEL_SPRITE,
    PANEL_BUTTON
};

typedef struct Panel{
    Coord2i position;
    Coord2i size;
    Color   foreground_color;
    Color   background_color;
    bool    has_background;
    Panel*  parent = nullptr;
    Panel** children;
    uint    child_count = 0;
    uint    type = PANEL_EMPTY;
    
    /* Dynamic panels only */
    bool    dynamic = false;                                    //Dynamic panel or static panel
    bool    active = false;                                     //Dynamic panel active or not
    uint    id;                                                 //Dynamic panels only
    void*   packet;                                             //Dynamic panel packet
    void    (*tick_func)(Panel* p, void* packet) = nullptr;     //Dynamic panel tick func.
} Panel;

typedef struct Panel_Text{
    Panel p;
    Font* font;
    std::string text;

    Panel_Text(){ p.type = PANEL_TEXT; };

} Panel_Text;

typedef struct Panel_Sprite{
    Panel p;
    Texture* texture;
    uint atlas_index;
    Panel_Sprite(){
        p.has_background = true;
        p.type = PANEL_SPRITE;
    };

} Panel_Sprite;

void ui_button_tick(Panel* dp, void* v);
typedef struct Panel_Button{
    Panel p;

    void* packet;
    void (*click_func)(void* v);

    Panel_Button(){ 
        p.type = PANEL_BUTTON; 
        p.dynamic = true;
        p.active = false;
        p.tick_func = &ui_button_tick;
    };
} Panel_Button;

/*--- Color constants ---*/
const Color COLOR_RED   = {255, 0  , 0};
const Color COLOR_GREEN = {0  , 255, 0};
const Color COLOR_BLUE  = {0  , 0  , 255};
const Color COLOR_WHITE = {255, 255, 255};
const Color COLOR_BLACK = {0, 0, 0};

/*--- Global objects and registries. ---*/

//minicraft_time.cpp
extern Time* g_time;        //Global time object

//minicraft_world.cpp
extern Chunk* g_chunk_buffer[];    //Contains loaded chunks
extern Option g_overlays;

//minicraft_entity.cpp
extern Entity*  g_entity_registry[]; //All active entities
extern uint     g_entity_highest_id; //The highest entity ID active in g_entity_registry

//minicraft_main.cpp
extern std::string g_game_path; //Global reference to argv[0]
extern Option g_debug;

//minicraft_rendering.cpp
extern Video_Mode g_video_mode;
extern Font* g_def_font;

//g_ui.cpp
extern Panel*   g_dynamic_panel_registry[]; //All rendering dynamic panels
extern uint     g_dynamic_panel_highest_id; //Highest active UI panel    

/*--- Functions ---*/

//minicraft_texture.cpp
Texture*    texture_generate(Image* img, uchar texture_load_options, uint tile_size);               //Generate Texture Object
Texture*    texture_load_bmp(const std::string& path, uchar texture_load_options, uint tile_size);  //Load a 24-bit BMP
void        texture_bind(Texture* t, GLuint sampler);                                               //Bind texture to GL_TEXTURE_2D
void        texture_destroy(Texture* t);                                                            //Delete texture

//minicraft_world.cpp
void world_set_chunk_callbacks(
        Chunk*      (*load_callback)(Map* map, Coord2i coord),
        void        (*unload_callback)(Map* map, Chunk* chunk)
);  //Set callbacks for world loading

void    world_populate_chunk_buffer(Entity* viewport_e);                //Populate Chunk Buffer
Chunk*  world_get_chunk(Coord2i ccoord);                                //Find a chunk in g_chunk_buffer
Map*    world_map_read(uint id);
void    world_map_write(Map* map);
void    world_modify_chunk(Coord2i ccoord, Coord2i tcoord, uint value); //Set tile of chunk to value
Chunk*  world_chunkfile_read(Map* map, Coord2i ccoord);  //read chunk from file
void    world_chunkfile_write(Map* map, Chunk* chunk);   //write chunk to chunkfile
void    world_chunk_refresh_metatextures(Map* map, Chunk* chunk);                  //Refresh chunk rendering metatexture
void    write_map_resource(Map* map, const std::string& filename, void* data, size_t size);
void    read_map_resource(Map* map, const std::string& filename, void* out, size_t size);

//minicraft_rendering.cpp
GLFWwindow* rendering_init_opengl(uint window_x, uint window_y, uint ws, uint rs, uint us, const char* title, bool free_aspect);    //Init OpenGL, GLFW, and create window
void        rendering_draw_chunk(Chunk* chunk, Entity* viewport_e);
void        rendering_draw_entity(Entity* entity, Texture* atlas_texture, Entity* viewport_e);                                      //Draw an entity
void        rendering_draw_entities(Texture* atlas_texture, Entity* viewport_e);                                                    //Draw all entities in g_entity_registry
void        rendering_draw_chunk_buffer(Entity* viewport_e);
void        rendering_draw_text(const std::string& text, uint size, Font* font, Color color, Coord2d pos);                          //Draw text
void        rendering_draw_bar(uint value, Coord2i position, Texture* ui_texture, uint atlas_index);
void        rendering_draw_cursor(Texture* ui_texture, uint atlas_index);
void        rendering_draw_dialog(const std::string& title, const std::string& message, Font* font);                                //Draw a dialog
void        rendering_draw_panel(Panel* panel);
Coord2d     rendering_viewport_to_world_pos(Entity* viewport_e, Coord2d pos);                                                       //Get world position of viewport position
void        rendering_update_chunk_texture(Chunk* c);                                                                               //Update chunk texture

//minicraft_input.cpp
void input_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);   //Keyboard callback
void input_mouse_button_callback(GLFWwindow* window, int button, int actions, int mods);    //Mouse button callback
void input_mouse_position_callback(GLFWwindow* window, double xpos, double ypos);           //Mouse position callback
void input_register_callbacks(GLFWwindow* window);                                          //Register all callbacks with GLFW
bool input_get_key(int keycode);            //Get key status
bool input_get_button(int keycode);         //Get button status

bool input_get_key_down(int keycode);       //Get keyboard button down event
bool input_get_key_up(int keycode);         //Get keyboard button up event
bool input_get_button_down(int keycode);    //Get new mouse stroke
bool input_get_button_up(int keycode);      //Get mouse button release

//Only use per tick
bool input_get_key_down_tick(int keycode);       //Get keyboard button down event
bool input_get_key_up_tick(int keycode);         //Get keyboard button up event
bool input_get_button_down_tick(int keycode);    //Get new mouse stroke
bool input_get_button_up_tick(int keycode);      //Get mouse button release

Coord2d input_mouse_position();             //Get mouse position
void input_poll_input();                    //Input poll
void input_tick();

//minicraft_input.cpp
void time_update_time(double glfw_time);                    //Update time
void time_set_tick_callback(void (*callback_function)());   //Add a function pointer to list of functions called every tick
Timer* time_timer_start(long duration);                     //Start a timer and return a timer id
void time_callback_start(long duration, void (*callback_function)(void* passthough), void* passthough); //Start callback timer
bool time_timer_finished(Timer*& t);                        //check if timer is finished, if finished, deletes the timer
void time_timer_cancel(Timer*& t);                          //end and delete the timer

//minicraft_entity.cpp
Entity* entity_create(Entity* entity);                                          //Add entity to entity_registry and assign id. Returns pointer address for convenience
void    entity_move(Entity* entity, Coord2d delta, bool respect_collisions);    //Move an entity
void    entity_delete(uint id);                                                 //Removes entity to entity_registry and deletes Entity
void    entity_tick();                                                          //Ticks all entities
Coord2d entity_collision(Entity* entity, Coord2d delta);                        //Check entity collision
Entity* entity_hit(Entity* entity, Coord2d delta);                              //Check entity hits
bool    entity_AABB(BoundingBox a, BoundingBox b);                              //Check AABB collision
void    entity_damage(Entity* entity, uint damage);
Entity* entity_create_from_type(uint e);                                        //Defined by game

//g_ui.cpp
void ui_tick();
void ui_dynamic_panel_activate(Panel* dp);
void ui_dynamic_panel_deactivate(Panel* dp);
Coord2i ui_panel_global_position(Panel* p);

//UI Constructors TODO: Move to separate UI Extensions header
Panel* ui_create_health_bar(Texture* t, uint atlas_active, uint atlas_inactive, uint length, int* value);
Panel_Text* ui_create_int_display(Font* font, std::string prefix, int* value, uint update_interval);
void ui_refresh_checkbox(Panel* checkbox);

template <typename T>
Panel* ui_create_checkbox(int size, Color fg, Color bg, T* out, T sel, T dsel){
    size *= 4;
    Panel_Button* button = new Panel_Button();

    Panel* box = new Panel();
    Panel* indicator = new Panel();


    button -> p.child_count = 2;
    button -> p.children = new Panel*[button -> p.child_count]{box, indicator};

    box -> type = PANEL_BOX;
    box -> size = {size, size};
    box -> background_color = bg;
    box -> foreground_color = fg;
    box -> has_background = true;

    indicator -> type = PANEL_BOX;
    indicator -> position = {size / 4, size / 4};
    indicator -> size = {size / 2, size / 2};
    indicator -> foreground_color = *out == sel ? bg : fg;
    indicator -> has_background = false;

    typedef struct Packet{
        Color fg;
        Color bg;
        Panel* indicator;
        T* out;
        T sel;
        T dsel;
    } Packet;

    Packet *p = new Packet{fg, bg, indicator, out, sel, dsel};

    button -> p.position = {0, 0};
    button -> p.size = {size, size};
    button -> packet = p;
    button -> click_func = [](void* v){
        Packet* p = (Packet*)v;
        *p -> out = *p -> out == p -> sel ? p -> dsel : p -> sel;
        p -> indicator -> foreground_color = *(p -> out) == p -> sel ? p -> bg : p -> fg;
    };

    return (Panel*)button;
}

template <typename T>

void ui_refresh_checkbox(Panel* checkbox){
    Panel_Button* cb = (Panel_Button*)checkbox;
    typedef struct Packet{
        Color fg;
        Color bg;
        Panel* indicator;
        T* out;
        T sel;
        T dsel;
    } Packet;

    Packet* p = (Packet*)cb -> packet;
    p -> indicator -> foreground_color = *(p -> out) == p -> sel ? p -> bg : p -> fg;
}

/*---inline util functions---*/

//Fake GL function to use Color instead of 3ub
inline void glColor1c(const Color& c){
    glColor3ub(c.r, c.g, c.b);
}

inline bool option_changed(Option* o){
    if(o -> previous != o -> value){
        o -> previous = o -> value;
        return true;
    }

    return false;
}

//Exit the game with an error message
inline void error(const std::string& error_message, const std::string& console) {
    if (g_debug){
           std::cout << console << std::endl;
    }
    Timer* t = time_timer_start(TIME_TPS * 10);

    while(!time_timer_finished(t) && !glfwWindowShouldClose(g_video_mode.windowptr)){
        rendering_draw_dialog("GEODE ERROR", error_message, g_def_font);
        glfwSwapBuffers(g_video_mode.windowptr);
        glfwPollEvents();
        time_update_time(glfwGetTime());
    }

    glfwSetWindowShouldClose(g_video_mode.windowptr, 1);
}

//Clamp int between min and max
inline int clampi(int a, int min, int max){
    if(a > max) return max;
    if(a < min) return min;
    return a;
}

//Clamp uint between min and max
inline uint clampui(uint a, uint min, uint max){
    if(a > max) return max;
    if(a < min) return min;
    return a;
}

//Simple distance calculation
inline double distancec2d(Coord2d a, Coord2d b){
    return sqrt( std::pow((b.x - a.x), 2) +  std::pow((b.y - a.y), 2));
}


inline std::string get_resource_path(const std::string& executable_path, const std::string& resource_name){
    uint substri = executable_path.find_last_of('/');
    return executable_path.substr(0, substri) + "/" + resource_name;
}

#endif
