/*
 * Created by MajesticWaffle on 4/26/21.
 * Copyright (c) 2021 Thicc Industries. All rights reserved.
 * This software is under the: Thicc-Industries-Do-What-You-Want-I-Dont-Care License.
 */

#include "geode.h"
#include <math.h>

Video_Mode g_video_mode;
Font* g_def_font;

//Internal drawing functions
void rendering_debug_draw_box(Coord2d p1, Coord2d p2, Color c);

void rendering_draw_panel(Panel*p, Coord2i parent_position);
void rendering_draw_panel_box(Panel* p, Coord2i parent_position);
void rendering_draw_panel_text(Panel_Text* p, Coord2i parent_position);
void rendering_draw_panel_sprite(Panel_Sprite* p, Coord2i parent_position);


GLFWwindow* rendering_init_opengl(uint window_x, uint window_y, uint ws, uint rs, uint us, const char* windowname, bool free_aspect){
    //Init GLFW
    if(glfwInit() != GLFW_TRUE){
        return nullptr;
    }


    int mon_count;
    GLFWmonitor** mons = glfwGetMonitors(&mon_count);
    const GLFWvidmode* mode = glfwGetVideoMode(mons[0]);

    double mon_aspect = (double)mode->height / (double)mode -> width;

    int scale1 = (mode->width / window_x) - 1;
    int scale2 = (mode->height / window_y) - 1;

    int scale = std::max(scale1, scale2);

    if(free_aspect){
        window_y = window_x * mon_aspect;
    }

    //scale = clampi(scale, 1, scale);

    //Create window and set context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* windowptr = glfwCreateWindow(window_x * scale, window_y * scale, windowname, nullptr, nullptr);


    glfwMakeContextCurrent(windowptr);
    glewInit();

    //Set up ortho projection
    glLoadIdentity();
    glOrtho(0, window_x, window_y, 0, 1, -1);
    glMatrixMode(GL_PROJECTION);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //Mouse mode
    glfwSetInputMode(windowptr, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSwapInterval(0);

    g_video_mode = {
            {(int)window_x, (int)window_y}, //resolution
            (uint)scale,    //Window scale
            rs,    //World scale
            us,    //Ui scale
            windowptr
    };

    return windowptr;
}

void rendering_draw_chunk(Chunk* chunk, Entity* viewport_e){
    Collider* col = entity_get_component<Collider>(viewport_e);
    int anim_frame = 0;

    if(chunk == nullptr)
        return;

    //Two different textures means chunk must be animated.
    if(chunk -> render_texture[1] != nullptr){
        int anim_tick = g_time -> tick % TIME_TPS;
        int anim_rate_d = TIME_TPS / 4;
        anim_frame += anim_tick / anim_rate_d;
    }

    Transform* transform = entity_get_component<Transform>(viewport_e);

    double tick_interp = g_time -> tick_delta / (1.0 / TIME_TPS) * (g_time -> paused ? 0 : 1);

    double viewport_x = transform -> position.x + (transform -> camera.position.x);
    double viewport_y = transform -> position.y + (transform -> camera.position.y);

    Coord2d delta_x = {transform -> velocity.x * tick_interp, 0};
    if(col == nullptr || entity_collision(col, viewport_e -> transform, delta_x) == delta_x) {
        viewport_x += viewport_e->transform->velocity.x * tick_interp;
    }

    Coord2d delta_y = {0, transform -> velocity.y * tick_interp};
    if(col == nullptr || entity_collision(col, viewport_e -> transform, delta_y) == delta_y) {

        viewport_y += viewport_e->transform->velocity.y * tick_interp;
    }

    double chunk_x = chunk -> pos.x * (16 * 16) - (viewport_x - (g_video_mode.window_resolution.x / (2 * g_video_mode.world_scale) ));
    double chunk_y = chunk -> pos.y * (16 * 16) - (viewport_y - (g_video_mode.window_resolution.y / (2 * g_video_mode.world_scale) ));

    //Don't render loaded chunks if they're out of render distance
    if(chunk_x > g_video_mode.window_resolution.x || (chunk_x + 256) < 0)
        return;
    if(chunk_y > g_video_mode.window_resolution.y || (chunk_y + 256) < 0)
        return;

    if(chunk -> render_texture[anim_frame] == nullptr)
        world_chunk_refresh_metatextures(transform->map, chunk);

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
                if (transform -> map->tile_properties[chunk->background_tiles[(y * 16) + x]].options & TILE_SOLID || (chunk->overlay_tiles[(y * 16) + x] != 0 && transform -> map->tile_properties[chunk->overlay_tiles[(y * 16) + x]].options & TILE_SOLID )) {
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

void rendering_draw_entity(Entity* entity, Entity* viewport_e){
    double tick_interp = g_time -> tick_delta / (1.0 / TIME_TPS) * (g_time -> paused ? 0 : 1);

    double viewport_x = (viewport_e -> transform -> position.x + (viewport_e -> transform -> camera.position.x));
    double viewport_y = (viewport_e -> transform -> position.y + (viewport_e -> transform -> camera.position.y));

    //Viewport Entity interp
    auto vcol = entity_get_component<Collider>(viewport_e);
    Coord2d delta_x = {viewport_e -> transform -> velocity.x * tick_interp, 0};
    if(vcol == nullptr || entity_collision(vcol, viewport_e -> transform, delta_x) == delta_x) {
        viewport_x += viewport_e->transform->velocity.x * tick_interp;
    }

    Coord2d delta_y = {0, viewport_e -> transform -> velocity.y * tick_interp};
    if(vcol == nullptr || entity_collision(vcol, viewport_e -> transform, delta_y) == delta_y)
        viewport_y += viewport_e->transform->velocity.y * tick_interp;

    double scl = g_video_mode.world_scale;
    double entity_x = entity -> transform -> position.x - (viewport_x - (g_video_mode.window_resolution.x / (2 * scl)));
    double entity_y = entity -> transform -> position.y - (viewport_y - (g_video_mode.window_resolution.y / (2 * scl)));

    //Entity Interp
    auto ecol = entity_get_component<Collider>(entity);
    delta_x = {entity -> transform -> velocity.x * tick_interp, 0};
    if(ecol == nullptr || entity_collision(ecol, entity->transform, delta_x) == delta_x)
        entity_x += entity -> transform -> velocity.x * tick_interp;

    delta_y = {0, entity -> transform -> velocity.y * tick_interp};
    if(ecol == nullptr || entity_collision(ecol, entity->transform, delta_y) == delta_y)
        entity_y += entity -> transform -> velocity.y * tick_interp;


    //Get texture for current state
    auto eren = entity_get_component<Renderer>(entity);
    uint direction = entity->transform->direction;
    uint index = eren -> atlas_index;
    bool inv_x = false;
    bool inv_y = false;

    switch(eren -> sheet_type){
        case SHEET_SINGLE:
            break;

        case SHEET_VH:
            if(direction == DIRECTION_EAST || direction == DIRECTION_WEST)
                index += 1;
            if(direction == DIRECTION_EAST)
                inv_x = true;
            if(direction == DIRECTION_NORTH)
                inv_y = true;

            break;
        case SHEET_UDH:
            if(direction == DIRECTION_NORTH)
                index += 1;
            if(direction == DIRECTION_EAST || direction == DIRECTION_WEST)
                index += 2;
            if(direction == DIRECTION_EAST)
                inv_x = true;
            break;

        case SHEET_UDLR:
            index += direction;
            break;
    }

    Texture* atlas_texture = eren -> atlas_texture;

    switch(entity -> transform -> move_state){
        case ENT_STATE_MOVING:
            int anim_tick = g_time -> tick % eren -> animation_rate;
            int anim_rate_d = eren -> animation_rate / eren -> frame_count;
            index += eren -> frame_order[(anim_tick / anim_rate_d)] * (atlas_texture -> width / atlas_texture -> tile_size);
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
        glTexCoord2d(texture_uv_x + (atlas_texture -> atlas_uvs.x * inv_x),     texture_uv_y + (atlas_texture -> atlas_uvs.y * inv_y));                                 glVertex2d((entity_x * scl),                 (entity_y * scl));
        glTexCoord2d(texture_uv_x + (atlas_texture -> atlas_uvs.x * !inv_x),    texture_uv_y + (atlas_texture -> atlas_uvs.y * inv_y));                                 glVertex2d((entity_x * scl) + (16 * scl),    (entity_y * scl));
        glTexCoord2d(texture_uv_x + (atlas_texture -> atlas_uvs.x * !inv_x),    texture_uv_y + (atlas_texture -> atlas_uvs.y * !inv_y));  glVertex2d((entity_x * scl) + (16 * scl),    (entity_y * scl) + (16 * scl));
        glTexCoord2d(texture_uv_x + (atlas_texture -> atlas_uvs.x * inv_x),     texture_uv_y + (atlas_texture -> atlas_uvs.y * !inv_y));  glVertex2d((entity_x * scl),                 (entity_y * scl) + (16 * scl));
    }
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

    if(g_debug && ecol != nullptr){
        Coord2d entity_cbounds_p1{
                entity_x + ecol -> col_bounds.p1.x,
                entity_y + ecol -> col_bounds.p1.y
        };
        Coord2d entity_cbounds_p2{
                entity_x + ecol -> col_bounds.p2.x,
                entity_y + ecol -> col_bounds.p2.y
        };

        Coord2d entity_hbounds_p1{
                entity_x + ecol -> hit_bounds.p1.x,
                entity_y + ecol -> hit_bounds.p1.y
        };
        Coord2d entity_hbounds_p2{
                entity_x + ecol -> hit_bounds.p2.x,
                entity_y + ecol -> hit_bounds.p2.y
        };

        rendering_debug_draw_box(entity_cbounds_p1, entity_cbounds_p2, COLOR_RED);
        rendering_debug_draw_box(entity_hbounds_p1, entity_hbounds_p2, COLOR_BLUE);
    }
}

void rendering_draw_entities(Entity* viewport_e){
    for(int i = g_entity_highest_id; i >= 0; i--){
        if(g_entity_registry[i] == nullptr || entity_get_component<Renderer>(g_entity_registry[i]) == nullptr)
            continue;

        rendering_draw_entity(g_entity_registry[i], viewport_e);
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
    int offset = ((g_video_mode.window_resolution.x / 2) - (title.length() * font -> t -> tile_size) / 2);
    int title_offset = ((double)(message.length() - title.length()) / 2.0) * font -> t -> tile_size;
    int message_offset = 0;
    int size_x_title = ((title.length()) * font -> t -> tile_size) + 4;
    int size_x_message = ((message.length()) * font -> t -> tile_size) + 4;


    if(size_x_title > size_x_message){
        offset = ((g_video_mode.window_resolution.x / 2) - (title.length() * font -> t -> tile_size) / 2);
        title_offset = 0;
        message_offset = ((double)(title.length() - message.length()) / 2.0) * font -> t -> tile_size;
    }

    int size_x = max(size_x_title, size_x_message);
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
    error_message -> p.position = {2 + message_offset, (int)font -> t -> tile_size * 2};
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
    Transform* transform = entity_get_component<Transform>(viewport_e);
    Coord2d position;

    int world_win_scl = g_video_mode.world_scale * g_video_mode.window_scale;
    position.x = (coord.x / world_win_scl) + (transform -> position.x + (transform -> camera.position.x) - (g_video_mode.window_resolution.x / (2 * g_video_mode.world_scale)));
    position.y = (coord.y / world_win_scl) + (transform -> position.y + (transform -> camera.position.y) - (g_video_mode.window_resolution.y / (2 * g_video_mode.world_scale)));

    return position;
}

void rendering_draw_panel(Panel* p){
    rendering_draw_panel(p, {0, 0});
}

void rendering_draw_panel(Panel* p, Coord2i parent_position){

    if(p == nullptr){
        return;
    }

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
        rendering_draw_panel(p -> children[i], parent_position + p -> position);
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

    texture_bind(p -> texture, 0);

    uint tiles_x = p->texture->width / p->texture->tile_size;
    uint tiles_y = p->texture->height / p->texture->tile_size;

    uint tile_px = p -> atlas_index % tiles_x;
    uint tile_py = p -> atlas_index / tiles_y;

    double uv_x = tile_px * p -> texture -> atlas_uvs.x;
    double uv_y = tile_py * p -> texture -> atlas_uvs.y;

    glEnable(GL_TEXTURE_2D);
    if(p -> p.has_background)
        glEnable(GL_BLEND);
    glBegin(GL_QUADS);{
        glTexCoord2d(uv_x,                                  uv_y);                                  glVertex2i(x1, y1);
        glTexCoord2d(uv_x + p -> texture -> atlas_uvs.x,    uv_y);                                  glVertex2i(x2, y1);
        glTexCoord2d(uv_x + p -> texture -> atlas_uvs.x,    uv_y + p -> texture -> atlas_uvs.y);    glVertex2i(x2, y2);
        glTexCoord2d(uv_x,                                  uv_y + p -> texture -> atlas_uvs.y);    glVertex2i(x1, y2);
    }
    glEnd();
    if(p -> p.has_background)
        glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}