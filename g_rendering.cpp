/*
 * Created by MajesticWaffle on 4/26/21.
 * Copyright (c) 2021 Thicc Industries. All rights reserved.
 * This software is under the: Thicc-Industries-Do-What-You-Want-I-Dont-Care License.
 */

#include "geode.h"

Video_Mode g_video_mode;
Font* g_def_font;

//Internal drawing functions
void rendering_debug_draw_box(Coord2d p1, Coord2d p2, Color c);

void rendering_draw_panel(Panel*p, Coord2i parent_position);
void rendering_draw_panel_box(Panel* p, Coord2i parent_position);
void rendering_draw_panel_text(Panel_Text* p, Coord2i parent_position);
void rendering_draw_panel_sprite(Panel_Sprite* p, Coord2i parent_position);


GLFWwindow* rendering_init_opengl(uint window_x, uint window_y, uint ws, uint rs, uint us){
    //Init GLFW
    if(glfwInit() != GLFW_TRUE){
        return nullptr;
    }

    //Auto calc window res and scale
    uint width = window_x;
    uint height;

    GLFWmonitor* mon = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(mon);

    double aspect = (double)mode->width / (double)mode -> height;

    height  = width * (1.0f/ aspect);

    int scale1 = mode->width / width;
    int scale2 = mode->height / height;

    int scale = std::max(scale1, scale2);
    scale -= 2;

    //Create window and set context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* windowptr = glfwCreateWindow(width * scale, height * scale, "Minicraft", nullptr, nullptr);
    glfwMakeContextCurrent(windowptr);
    glewInit();

    //Set up ortho projection
    glLoadIdentity();
    glOrtho(0, width, height, 0, 1, -1);
    glMatrixMode(GL_PROJECTION);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //Mouse mode
    glfwSetInputMode(windowptr, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSwapInterval(0);

    g_video_mode = {
            {(int)width, (int)height}, //resolution
            (uint)scale,    //Window scale
            rs,    //World scale
            us,    //Ui scale
            windowptr
    };

    return windowptr;
}

void rendering_draw_chunk(Chunk* chunk, Entity* viewport_e){

    int anim_frame = 0;

    //Two different textures means chunk must be animated.
    if(chunk -> render_texture[1] != nullptr){
        int anim_tick = g_time -> tick % TIME_TPS;
        int anim_rate_d = TIME_TPS / 4;
        anim_frame += anim_tick / anim_rate_d;
    }


    double tick_interp = g_time -> tick_delta / (1.0 / TIME_TPS) * (g_time -> paused ? 0 : 1);

    double viewport_x = viewport_e -> position.x + (viewport_e -> camera.position.x);
    double viewport_y = viewport_e -> position.y + (viewport_e -> camera.position.y);

    Coord2d delta_x = {viewport_e -> velocity.x * tick_interp, 0};

    if(entity_collision(viewport_e, delta_x) == delta_x)
        viewport_x += viewport_e->velocity.x * tick_interp;

    Coord2d delta_y = {0, viewport_e -> velocity.y * tick_interp};
    if(entity_collision(viewport_e, delta_y) == delta_y)
        viewport_y += viewport_e->velocity.y * tick_interp;

    if(chunk == nullptr)
        return;

    double chunk_x = chunk -> pos.x * (16 * 16) - (viewport_x - (g_video_mode.window_resolution.x / (2 * g_video_mode.world_scale) ));
    double chunk_y = chunk -> pos.y * (16 * 16) - (viewport_y - (g_video_mode.window_resolution.y / (2 * g_video_mode.world_scale) ));

    //Don't render loaded chunks if they're out of render distance
    if(chunk_x > g_video_mode.window_resolution.x || (chunk_x + 256) < 0)
        return;
    if(chunk_y > g_video_mode.window_resolution.y || (chunk_y + 256) < 0)
        return;

    if(chunk -> render_texture[anim_frame] == nullptr)
        world_chunk_refresh_metatextures(chunk);

    texture_bind(chunk -> render_texture[anim_frame], 0);

    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);{
        glTexCoord2d(0, 0); glVertex2d((chunk_x * g_video_mode.world_scale)                                   , (chunk_y * g_video_mode.world_scale));
        glTexCoord2d(1, 0); glVertex2d((chunk_x * g_video_mode.world_scale) + (256 * g_video_mode.world_scale), (chunk_y * g_video_mode.world_scale));
        glTexCoord2d(1, 1); glVertex2d((chunk_x * g_video_mode.world_scale) + (256 * g_video_mode.world_scale), (chunk_y * g_video_mode.world_scale) + (256 * g_video_mode.world_scale));
        glTexCoord2d(0, 1); glVertex2d((chunk_x * g_video_mode.world_scale)                                   , (chunk_y * g_video_mode.world_scale) + (256 * g_video_mode.world_scale));
    }
    glEnd();
    glDisable(GL_TEXTURE_2D);

    if(g_debug) {
        rendering_debug_draw_box({chunk_x, chunk_y}, {chunk_x + (16 * 16), chunk_y + (16 * 16)}, COLOR_GREEN);
        uint tile_scl = 16 * g_video_mode.world_scale;
        for(uint x = 0; x < 16; ++x) {
            for(uint y = 0; y < 16; ++y) {
                if (g_block_registry[chunk->background_tiles[(y * 16) + x]]->options & TILE_SOLID || g_block_registry[chunk->overlay_tiles[(y * 16) + x]]->options & TILE_SOLID ) {
                    glColor1c(COLOR_RED);
                    glLineWidth(2);
                    glBegin(GL_LINES);
                    {
                        glVertex2d((chunk_x * g_video_mode.world_scale) + (x * tile_scl),            (chunk_y * g_video_mode.world_scale) + (y * tile_scl));
                        glVertex2d((chunk_x * g_video_mode.world_scale) + (x * tile_scl) + tile_scl, (chunk_y * g_video_mode.world_scale) + (y * tile_scl));

                        glVertex2d((chunk_x * g_video_mode.world_scale) + (x * tile_scl),            (chunk_y * g_video_mode.world_scale) + (y * tile_scl) + tile_scl);
                        glVertex2d((chunk_x * g_video_mode.world_scale) + (x * tile_scl) + tile_scl, (chunk_y * g_video_mode.world_scale) + (y * tile_scl) + tile_scl);

                        glVertex2d((chunk_x * g_video_mode.world_scale) + (x * tile_scl), (chunk_y * g_video_mode.world_scale) + (y * tile_scl));
                        glVertex2d((chunk_x * g_video_mode.world_scale) + (x * tile_scl), (chunk_y * g_video_mode.world_scale) + (y * tile_scl) + tile_scl);

                        glVertex2d((chunk_x * g_video_mode.world_scale) + (x * tile_scl) + tile_scl, (chunk_y * g_video_mode.world_scale) + (y * tile_scl));
                        glVertex2d((chunk_x * g_video_mode.world_scale) + (x * tile_scl) + tile_scl, (chunk_y * g_video_mode.world_scale) + (y * tile_scl) + tile_scl);
                    }
                    glEnd();
                    glColor1c(COLOR_WHITE);
                }
            }
        }
    }
}

void rendering_debug_draw_box(Coord2d p1, Coord2d p2, Color c){
    glColor1c(c);
    glLineWidth(2);
    glBegin(GL_LINES);{
        glVertex2d(p1.x * g_video_mode.world_scale, p1.y * g_video_mode.world_scale);
        glVertex2d(p2.x * g_video_mode.world_scale, p1.y * g_video_mode.world_scale);

        glVertex2d(p1.x * g_video_mode.world_scale, p2.y * g_video_mode.world_scale);
        glVertex2d(p2.x * g_video_mode.world_scale, p2.y * g_video_mode.world_scale);

        glVertex2d(p1.x * g_video_mode.world_scale, p1.y * g_video_mode.world_scale);
        glVertex2d(p1.x * g_video_mode.world_scale, p2.y * g_video_mode.world_scale);

        glVertex2d(p2.x * g_video_mode.world_scale, p1.y * g_video_mode.world_scale);
        glVertex2d(p2.x * g_video_mode.world_scale, p2.y * g_video_mode.world_scale);
    }
    glEnd();
    glColor1c(COLOR_WHITE);
}

void rendering_draw_entity(Entity* entity, Texture* atlas_texture, Entity* viewport_e){
    double tick_interp = g_time -> tick_delta / (1.0 / TIME_TPS) * (g_time -> paused ? 0 : 1);

    double viewport_x = (viewport_e -> position.x + (viewport_e -> camera.position.x));
    double viewport_y = (viewport_e -> position.y + (viewport_e -> camera.position.y));

    Coord2d delta_x = {viewport_e -> velocity.x * tick_interp, 0};
    if(entity_collision(viewport_e, delta_x) == delta_x)
        viewport_x += viewport_e->velocity.x * tick_interp;

    Coord2d delta_y = {0, viewport_e -> velocity.y * tick_interp};
    if(entity_collision(viewport_e, delta_y) == delta_y)
        viewport_y += viewport_e->velocity.y * tick_interp;

    double scl = g_video_mode.world_scale;
    double entity_x = entity -> position.x - (viewport_x - (g_video_mode.window_resolution.x / (2 * scl)));
    double entity_y = entity -> position.y - (viewport_y - (g_video_mode.window_resolution.y / (2 * scl)));


    delta_x = {entity -> velocity.x * tick_interp, 0};
    if(entity_collision(entity, delta_x) == delta_x)
        entity_x += entity->velocity.x * tick_interp;

    delta_y = {0, entity -> velocity.y * tick_interp};
    if(entity_collision(entity, delta_y) == delta_y)
        entity_y += entity->velocity.y * tick_interp;


    //Get texture for current state
    uint index = entity -> atlas_index;
    bool inv_x = false;

    if(entity -> spritesheet_size.x == 3) {
        switch (entity->direction) {
            case DIRECTION_NORTH:
                index += 1;
                inv_x = false;
                break;
            case DIRECTION_EAST:
                index += 2;
                inv_x = true;
                break;
            case DIRECTION_SOUTH:
                index += 0;
                inv_x = false;
                break;
            case DIRECTION_WEST:
                index += 2;
                inv_x = false;
                break;
        }
    }


    switch(entity -> move_state){
        case ENT_STATE_MOVING:
            int anim_tick = g_time -> tick % entity -> animation_rate;
            int anim_rate_d = entity -> animation_rate / entity -> frame_count;
            index += entity -> frame_order[(anim_tick / anim_rate_d)] * (atlas_texture -> width / atlas_texture -> tile_size);
            break;
    }

    uint texture_coord_x = index % (atlas_texture -> width / atlas_texture -> tile_size);
    uint texture_coord_y = index / (atlas_texture -> width / atlas_texture -> tile_size);

    double texture_uv_x = atlas_texture -> atlas_uvs.x * texture_coord_x;
    double texture_uv_y = atlas_texture -> atlas_uvs.y * texture_coord_y;
    
    texture_bind(atlas_texture, 0);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    glBegin(GL_QUADS);{
        glTexCoord2d(texture_uv_x + (atlas_texture -> atlas_uvs.x * inv_x),     texture_uv_y);                                 glVertex2d((entity_x * scl),                 (entity_y * scl));
        glTexCoord2d(texture_uv_x + (atlas_texture -> atlas_uvs.x * !inv_x),    texture_uv_y);                                 glVertex2d((entity_x * scl) + (16 * scl),    (entity_y * scl));
        glTexCoord2d(texture_uv_x + (atlas_texture -> atlas_uvs.x * !inv_x),    texture_uv_y + atlas_texture -> atlas_uvs.y);  glVertex2d((entity_x * scl) + (16 * scl),    (entity_y * scl) + (16 * scl));
        glTexCoord2d(texture_uv_x + (atlas_texture -> atlas_uvs.x * inv_x),     texture_uv_y + atlas_texture -> atlas_uvs.y);  glVertex2d((entity_x * scl),                 (entity_y * scl) + (16 * scl));
    }
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

    if(g_debug){
        Coord2d entity_cbounds_p1{
            entity_x + entity -> col_bounds.p1.x,
            entity_y + entity -> col_bounds.p1.y
        };
        Coord2d entity_cbounds_p2{
                entity_x + entity -> col_bounds.p2.x,
                entity_y + entity -> col_bounds.p2.y
        };

        Coord2d entity_hbounds_p1{
                entity_x + entity -> hit_bounds.p1.x,
                entity_y + entity -> hit_bounds.p1.y
        };
        Coord2d entity_hbounds_p2{
                entity_x + entity -> hit_bounds.p2.x,
                entity_y + entity -> hit_bounds.p2.y
        };

        rendering_debug_draw_box(entity_cbounds_p1, entity_cbounds_p2, COLOR_RED);
        rendering_debug_draw_box(entity_hbounds_p1, entity_hbounds_p2, COLOR_BLUE);
    }
}

void rendering_draw_entities(Texture* atlas_texture, Entity* viewport_e){
    for(int i = g_entity_highest_id; i >= 0; i--){
        if(g_entity_registry[i] == nullptr)
            continue;

        rendering_draw_entity(g_entity_registry[i], atlas_texture, viewport_e);
    }
}

void rendering_draw_chunk_buffer(Entity* viewport_e){
    for(int i = 0; i < 9; ++i){
        rendering_draw_chunk(g_chunk_buffer[i], viewport_e);
    }
}

void rendering_draw_text(const std::string& text, uint size, Font* font, Color color, Coord2d pos){
    char c;         //Char being drawn
    uint ci;        //The index of the char in font -> font_atlas
    uint    ci_x,   ci_y;  //The 2D position of the char in font -> t
    double  uv_x,   uv_y;
    double  pos_x,  pos_y;
    int     tilesize = font -> t -> tile_size * size;
    pos_y = pos.y;

    //No point in doing this over and over
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glColor1c(color);
    texture_bind(font -> t, 0);

    for(int i = 0; i < text.length(); ++i){
        c = text.at(i);

        ci = font -> font_atlas.find(c);
        ci_y = ci / 10;
        ci_x = ci % 10;

        uv_x = font -> t -> atlas_uvs.x * ci_x;
        uv_y = font -> t -> atlas_uvs.y * ci_y;

        pos_x = pos.x + (i * tilesize);

        glColor3ub(0, 0, 0);
        glBegin(GL_QUADS);{
            glTexCoord2d(uv_x                           , uv_y                              ); glVertex2d(pos_x + 1           , pos_y + 1);
            glTexCoord2d(uv_x + font -> t -> atlas_uvs.x, uv_y                              ); glVertex2d(pos_x + 1 + tilesize, pos_y + 1);
            glTexCoord2d(uv_x + font -> t -> atlas_uvs.x, uv_y + font -> t -> atlas_uvs.y   ); glVertex2d(pos_x + 1 + tilesize, pos_y + 1 + tilesize);
            glTexCoord2d(uv_x                           , uv_y + font -> t -> atlas_uvs.y   ); glVertex2d(pos_x + 1           , pos_y + 1 + tilesize);
        }
        glEnd();

        glColor1c(color);
        glBegin(GL_QUADS);{
            glTexCoord2d(uv_x                           , uv_y                              ); glVertex2d(pos_x           , pos_y);
            glTexCoord2d(uv_x + font -> t -> atlas_uvs.x, uv_y                              ); glVertex2d(pos_x + tilesize, pos_y);
            glTexCoord2d(uv_x + font -> t -> atlas_uvs.x, uv_y + font -> t -> atlas_uvs.y   ); glVertex2d(pos_x + tilesize, pos_y + tilesize);
            glTexCoord2d(uv_x                           , uv_y + font -> t -> atlas_uvs.y   ); glVertex2d(pos_x           , pos_y + tilesize);
        }
        glEnd();
    }

    glColor1c(COLOR_WHITE);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
}

void rendering_draw_cursor(Texture* ui_texture, uint atlas_index){
    double x, y;
    double uv_x, uv_y;

    double scl = g_video_mode.ui_scale;
    x = input_mouse_position().x / (g_video_mode.window_scale * scl);
    y = input_mouse_position().y / (g_video_mode.window_scale * scl);

    uv_x = (atlas_index % (ui_texture -> width / ui_texture -> tile_size))  * ui_texture -> atlas_uvs.x;
    uv_y = (atlas_index / (ui_texture -> height / ui_texture -> tile_size)) * ui_texture -> atlas_uvs.y;

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    texture_bind(ui_texture, 0);

    glBegin(GL_QUADS);{
        glTexCoord2d(uv_x                            , uv_y                             ); glVertex2d((x * scl)                                     , (y * scl));
        glTexCoord2d(uv_x + ui_texture -> atlas_uvs.x, uv_y                             ); glVertex2d((x * scl) + (ui_texture -> tile_size * scl)   , (y * scl));
        glTexCoord2d(uv_x + ui_texture -> atlas_uvs.x, uv_y + ui_texture -> atlas_uvs.y ); glVertex2d((x * scl) + (ui_texture -> tile_size * scl)   , (y * scl) + (ui_texture -> tile_size * scl));
        glTexCoord2d(uv_x                            , uv_y + ui_texture -> atlas_uvs.y ); glVertex2d((x * scl)                                     , (y * scl) + (ui_texture -> tile_size * scl));
    }
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
}

void rendering_draw_dialog(const std::string& title, const std::string& message, Font* font){
    //How many pixels to offset message by to center it
    int offset = ((g_video_mode.window_resolution.x / 2) - (message.length() * font -> t -> tile_size) / 2);
    int title_offset = ((message.length() - title.length()) / 2) * font -> t -> tile_size;
    int size_x = ((message.length()) * font -> t -> tile_size) + 4;
    int size_y = (4 * font -> t -> tile_size);

    Panel_Text* error_title = new Panel_Text();
    error_title -> p.has_background = false;
    error_title -> p.foreground_color = {255, 255, 255};
    error_title -> p.position = {2 + title_offset, (int)font -> t -> tile_size};
    error_title -> text = title;
    error_title -> font = font;

    Panel_Text* error_message = new Panel_Text();
    error_message -> p.has_background = false;
    error_message -> p.foreground_color = {255, 255, 255};
    error_message -> p.position = {2, (int)font -> t -> tile_size * 2};
    error_message -> text = message;
    error_message -> font = font;

    Panel* error = new Panel();
    error -> position = {offset, (int)(RENDER_WINY / 2) - (int)(2 * font -> t -> tile_size)};
    error -> size = {size_x, size_y};

    error -> type = PANEL_BOX;
    error -> has_background = true;
    error -> background_color = {128, 128, 128};
    error -> foreground_color = {0, 0, 0};
    error -> child_count = 2;

    error -> children = new Panel*[2]{ (Panel*)error_title, (Panel*)error_message};

    rendering_draw_panel(error);

    delete error_title;
    delete error_message;
    delete error;
}


Coord2d rendering_viewport_to_world_pos(Entity* viewport_e, Coord2d coord){
    Coord2d position;

    int world_win_scl = g_video_mode.world_scale * g_video_mode.window_scale;
    position.x = (coord.x / world_win_scl) + (viewport_e -> position.x + (viewport_e -> camera.position.x) - (g_video_mode.window_resolution.x / (2 * g_video_mode.world_scale)));
    position.y = (coord.y / world_win_scl) + (viewport_e -> position.y + (viewport_e -> camera.position.y) - (g_video_mode.window_resolution.y / (2 * g_video_mode.world_scale)));

    return position;
}

void rendering_draw_panel(Panel* p){ 

    rendering_draw_panel(p, {0, 0}); 
    }

void rendering_draw_panel(Panel* p, Coord2i parent_position){
    
    switch (p -> type) {
        case PANEL_BOX:
            rendering_draw_panel_box(p, parent_position);
            break;
        case PANEL_TEXT:
            rendering_draw_panel_text( (Panel_Text*)p, parent_position);
            break;
        case PANEL_SPRITE:
            rendering_draw_panel_sprite( (Panel_Sprite*)p, parent_position);
            break;
    }

    //Draw all children

    for(uint i = 0; i < p -> child_count; ++i){

        rendering_draw_panel(p -> children[i], p -> position);
    }
}

void rendering_draw_panel_box(Panel* p, Coord2i parent_position){
    
    int x1 = p -> position.x + parent_position.x;
    int x2 = x1 + p -> size.x;

    int y1 = p -> position.y + parent_position.y;
    int y2 = y1 + p -> size.y;
    
    if(p -> has_background){
        glColor1c(p -> background_color);
        glBegin(GL_QUADS);{
            glVertex2i(x1 - 1, y1 - 1);
            glVertex2i(x2 + 1, y1 - 1);
            glVertex2i(x2 + 1, y2 + 1);
            glVertex2i(x1 - 1, y2 + 1);
        }
    }

    glEnd();
    glColor1c(p -> foreground_color);
    glBegin(GL_QUADS);{
        glVertex2i(x1, y1);
        glVertex2i(x2, y1);
        glVertex2i(x2, y2);
        glVertex2i(x1, y2);
    }
    glEnd();
    glColor1c({255, 255, 255});
}

void rendering_draw_panel_text(Panel_Text* p, Coord2i parent_position){
    int x1 = p -> p.position.x + parent_position.x;
    int y1 = p -> p.position.y + parent_position.y;
    
    int x2 = x1 + p -> font -> t -> tile_size * p -> text.length();
    int y2 = y1 + p -> font -> t -> tile_size;

    if(p -> p.has_background){
        glColor1c(p -> p.background_color);
        glBegin(GL_QUADS);{
            glVertex2i(x1, y1);
            glVertex2i(x2, y1);
            glVertex2i(x2, y2);
            glVertex2i(x1, y2);
        }
        glEnd();
    }
    glColor1c({255, 255, 255});

    rendering_draw_text(p -> text, 1, p -> font, p -> p.foreground_color, Coord2d{(double)x1, (double)y1});
}

void rendering_draw_panel_sprite(Panel_Sprite* p, Coord2i parent_position){
    
    int x1 = p -> p.position.x + parent_position.x;
    int y1 = p -> p.position.y + parent_position.y;
    
    int x2 = x1 + p -> texture -> tile_size;
    int y2 = y1 + p -> texture -> tile_size;

    /*if(p -> p.has_background){
        glColor1c(p -> p.background_color);
        glBegin(GL_QUADS);{
            glVertex2i(x1, y1);
            glVertex2i(x2, y1);
            glVertex2i(x2, y2);
            glVertex2i(x1, y2);
        }
        glEnd();
    }*/

    texture_bind(p -> texture, 0);

    double uv_x = (p -> atlas_index % (p -> texture -> width / p -> texture -> tile_size)) * p -> texture -> atlas_uvs.x;
    double uv_y = (p -> atlas_index / (p -> texture -> height / p -> texture -> tile_size)) * p -> texture -> atlas_uvs.y;

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBegin(GL_QUADS);{
        glTexCoord2d(uv_x,                                  uv_y);                                  glVertex2i(x1, y1);
        glTexCoord2d(uv_x + p -> texture -> atlas_uvs.x,    uv_y);                                  glVertex2i(x2, y1);
        glTexCoord2d(uv_x + p -> texture -> atlas_uvs.x,    uv_y + p -> texture -> atlas_uvs.y);    glVertex2i(x2, y2);
        glTexCoord2d(uv_x,                                  uv_y + p -> texture -> atlas_uvs.y);    glVertex2i(x1, y2);
    }
    glEnd();
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}