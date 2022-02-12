/*
 * Created by MajesticWaffle on 4/26/21.
 * Copyright (c) 2021 Thicc Industries. All rights reserved.
 * This software is under the: Thicc-Industries-Do-What-You-Want-I-Dont-Care License.
 */

#include "geode.h"
#include <filesystem>
#include <cstring> //Why is memcpy in here?

Chunk* g_chunk_buffer[9]{nullptr};
Chunk*  (*chunk_load_callback)(Map* map, Coord2i coord);
void    (*chunk_unload_callback)(Map* map, Chunk* chunk);
void world_modify_image(Map* map, Chunk* chunk, Image* meta_img, Texture* terrain_t, bool skip, bool tiles[256], bool overlays[256], int anim_index);

void world_set_chunk_callbacks(
        Chunk*  (*load_callback)(Map* map,Coord2i coord),
        void    (*unload_callback)(Map* map, Chunk* chunk)
    ){
    chunk_load_callback     = load_callback;
    chunk_unload_callback   = unload_callback;
}

void world_populate_chunk_buffer(Entity* viewport_e){
    int player_chunk_x = floor((viewport_e -> position.x + (viewport_e -> camera.position.x)) / 256);
    int player_chunk_y = floor((viewport_e -> position.y + (viewport_e -> camera.position.y)) / 256);

    for(int y = 0; y <= RENDER_DISTANCE * 2; ++y){
        for(int x = 0; x <= RENDER_DISTANCE * 2; ++x){
            int chunk_x = player_chunk_x + (x - RENDER_DISTANCE);
            int chunk_y = player_chunk_y + (y - RENDER_DISTANCE);
            int chunki = (y * 3) + x;


            if(g_chunk_buffer[chunki] != nullptr && g_chunk_buffer[chunki]->pos.x == chunk_x && g_chunk_buffer[chunki]->pos.y == chunk_y)
                continue;

            //Unload chunk already in buffer slot
            if(g_chunk_buffer[chunki] != nullptr) {
                if(chunk_unload_callback == nullptr)
                    error("Chunk unload funcptr not set.", "Chunk unload function pointer not set.");

                chunk_unload_callback(viewport_e-> map, g_chunk_buffer[chunki]);

                for(int i = 0; i < 2; i++){
                    if(g_chunk_buffer[chunki] -> render_texture[i] != nullptr) {
                        texture_destroy(g_chunk_buffer[chunki]->render_texture[i]);
                        g_chunk_buffer[chunki] -> render_texture[i] == nullptr;
                    }
                }

                delete[] g_chunk_buffer[chunki];
            }

            if(chunk_unload_callback == nullptr)
                error("Chunk load funcptr not set.", "Chunk Load function pointer not set.");

            g_chunk_buffer[chunki] = chunk_load_callback(viewport_e -> map, Coord2i{chunk_x, chunk_y});
        }
    }
}

void world_modify_chunk(Map* map, Coord2i ccoord, Coord2i tcoord, uint value){
    //Look for chunk in chunk buffer
    Chunk* chunkptr = world_get_chunk(ccoord);

    //Chunk isn't loaded
    if(chunkptr == nullptr){
        return;
    }

    chunkptr -> overlay_tiles[tcoord.x + (tcoord.y * 16)] = value;
    world_chunk_refresh_metatextures(map, chunkptr);
}


Chunk* world_get_chunk(Coord2i ccoord){
    for(Chunk* c : g_chunk_buffer){
        if(c == nullptr)
            continue;
        if(c -> pos.x == ccoord.x && c -> pos.y == ccoord.y){
            return c;
        }
    }
    return nullptr;
}

Map* world_map_read(uint id){
    FILE* texture_properties = fopen(get_resource_path(g_game_path, "maps/" + std::to_string(id) + ".map/texture.prp").c_str(), "rb");
    FILE* tiles_properties = fopen(get_resource_path(g_game_path, "maps/" + std::to_string(id) + ".map/tiles.prp").c_str(), "rb");

    if(!texture_properties){
        return nullptr;
    }

    if(!tiles_properties){
        return nullptr;
    }

    //Size of texture tiles
    uint texture_tilesize = 0;
    uint tile_count = 0;

    fread(&texture_tilesize, 1, sizeof(uint), texture_properties);
    fread(&tile_count, 1, sizeof(uint), tiles_properties);

    fclose(texture_properties);

    Texture* map_texture = texture_load_bmp(get_resource_path(g_game_path, "maps/" + std::to_string(id) + ".map/texture.bmp"), TEXTURE_MULTIPLE | TEXTURE_STORE, texture_tilesize);

    Map* map = new Map;
    map -> id = id;
    map -> tilemap = map_texture;
    map -> tile_count = tile_count;
    map -> tile_properties = new Block[tile_count];
    std::cout << map -> tile_count << std::endl;
    uint atlas_index, drop_id, drop_count;
    uchar options;

    for(uint i = 0; i < tile_count; ++i){
        fseek(tiles_properties, (0x10 * i) + 0x10, SEEK_SET);
        fread(&atlas_index, sizeof(uint), 1, tiles_properties);
        fread(&drop_id,     sizeof(uint), 1, tiles_properties);
        fread(&drop_count,  sizeof(uint), 1, tiles_properties);
        fread(&options,     sizeof(uchar), 1, tiles_properties);

        map -> tile_properties[i] = {atlas_index, options, drop_id, drop_count};
    }
    fclose(tiles_properties);

    return map;
}

void world_map_write(Map* map){
    std::filesystem::create_directories(get_resource_path(g_game_path, "maps/" + std::to_string(map -> id) + ".map/"));
    FILE* texture_properties = fopen(get_resource_path(g_game_path, "maps/" + std::to_string(map -> id) + ".map/texture.prp").c_str(), "wb");
    FILE* tiles_properties = fopen(get_resource_path(g_game_path, "maps/" + std::to_string(map -> id) + ".map/tiles.prp").c_str(), "wb");
    fwrite(&(map -> tilemap -> tile_size), sizeof(uint), 1, texture_properties);
    fclose(texture_properties);

    //Write tiles file
    fwrite(&(map -> tile_count), sizeof(uint), 1, tiles_properties);
    std::cout << map -> tile_count << std::endl;

    for(uint i = 0; i < map -> tile_count; ++i){
        std::cout << map -> tile_properties[i].atlas_index << std::endl;
        fseek(tiles_properties, (0x10 * i) + 0x10, SEEK_SET);
        fwrite(&(map -> tile_properties[i].atlas_index),    sizeof(uint), 1, tiles_properties);
        fwrite(&(map -> tile_properties[i].drop_id),        sizeof(uint), 1, tiles_properties);
        fwrite(&(map -> tile_properties[i].drop_count),     sizeof(uint), 1, tiles_properties);
        fwrite(&(map -> tile_properties[i].options),        sizeof(uchar), 1, tiles_properties);
    }
    fclose(tiles_properties);

}


Chunk* world_chunkfile_read(Map* map, Coord2i coord){
    FILE* chunkfile = fopen(get_resource_path(g_game_path, "maps/" + std::to_string(map->id) + ".map/chunks/c" + std::to_string(coord.x) + "x" + std::to_string(coord.y) + ".chk").c_str(), "rb");

    //Chunk is new (no chunkfile)
    if(!chunkfile){
        return nullptr;
    }

    Chunk* chunkptr = new Chunk;

    fread(&chunkptr -> pos.x, 1, sizeof(int), chunkfile);
    fread(&chunkptr -> pos.y, 1, sizeof(int), chunkfile);

    fseek(chunkfile, 0x10, SEEK_SET);

    fread(chunkptr -> overlay_tiles,    256, sizeof(uchar), chunkfile);
    fread(chunkptr -> background_tiles, 256, sizeof(uchar), chunkfile);

    fclose(chunkfile);

    return chunkptr;
}

void world_chunkfile_write(Map* map, Chunk* chunk){
    int cx = chunk -> pos.x;
    int cy = chunk -> pos.y;

    std::filesystem::create_directories(get_resource_path(g_game_path, "maps/" + std::to_string(map -> id) + ".map/chunks/"));
    FILE* chunkfile = fopen(get_resource_path(g_game_path, "maps/" + std::to_string(map -> id) + ".map/chunks/c" + std::to_string(cx) + "x" + std::to_string(cy) + ".chk").c_str(), "wb");

    fwrite(&cx, sizeof(int), 1, chunkfile);
    fwrite(&cy, sizeof(int), 1, chunkfile);

    fseek(chunkfile, 0x10, SEEK_SET);

    fwrite(chunk -> overlay_tiles,      sizeof(uchar), 256, chunkfile);
    fwrite(chunk -> background_tiles, sizeof(uchar), 256, chunkfile);

    fclose(chunkfile);
}

//TODO: Consider moving this to g_rendering.cpp?
// no.
void world_chunk_refresh_metatextures(Map* map, Chunk* chunk){
    bool anim_tiles[256] = {0};
    bool anim_overlays[256] = {0};
    bool anim = false;

    //Cleanup old to prevent leaks
    if(chunk -> render_texture[0] != nullptr) {
        texture_destroy(chunk->render_texture[0]);
        chunk -> render_texture[0] = nullptr;
    }

    Texture* terrain_t = map -> tilemap;

    if(terrain_t == nullptr){
        error("Error getting terrain texture.", "Chunk {" + std::to_string(chunk->pos.x) + ", " + std::to_string(chunk->pos.y) + "} did not receive a render texture.");
        return;
    }

    if(terrain_t -> imageData == nullptr){
        error("Rndr txtr not TEXTURE_STORE.", "Render texture recieved for Chunk {" + std::to_string(chunk->pos.x) + ", " + std::to_string(chunk->pos.y) + "} did not have TEXTURE_STORE set.");
    }

    for(int i = 0; i < 256; i++){
        if( map -> tile_properties[chunk -> background_tiles[i]].options & TILE_ANIMATED){

            anim_tiles[i] = true;
            anim = true;
        }

        if( map -> tile_properties[chunk -> overlay_tiles[i]].options & TILE_ANIMATED){
            anim_overlays[i] = true;
            anim = true;
        }
            
    }

    Image* meta_img = new Image;

    meta_img -> height    = 16 * terrain_t->tile_size;
    meta_img -> width     = 16 * terrain_t->tile_size;
    meta_img -> imageData = new uchar[ meta_img -> height * meta_img -> width * 4 ];

    for(uint i = 0; i < meta_img -> height * meta_img -> width * 4; ++i){
        meta_img->imageData[i] = 0;
    }

    world_modify_image(map, chunk, meta_img, terrain_t, false, nullptr, nullptr, 0);
    chunk -> render_texture[0] = texture_generate(meta_img, TEXTURE_SINGLE, 256);

    //No animated tiles, we're all done here.
    if(!anim){

        delete[] meta_img -> imageData;
        delete meta_img;

        chunk -> render_texture[3] = chunk -> render_texture[2] = chunk -> render_texture[1] = nullptr; //This should already be the case but its safer.

        return;
    }

    for(int i = 1; i < 4; i++){
        world_modify_image(map, chunk, meta_img, terrain_t, true, anim_tiles, anim_overlays, i);
        chunk -> render_texture[i] = texture_generate(meta_img, TEXTURE_SINGLE, 256);
    }

    delete[] meta_img -> imageData;
    delete meta_img;

}


void world_modify_image(Map* map, Chunk* chunk, Image* meta_img, Texture* terrain_t, bool skip, bool tiles[256], bool overlays[256], int anim_frame){
    uint tiles_x = terrain_t->width / terrain_t->tile_size; //Number of tiles wide

    for(uint y = 0; y < meta_img -> height; ++y) {
        uint ctile_y = y / 16;   //Position of tile inside chunk
        uint pixel_y = y % 16;  //Position of pixel inside tile
        for (uint x = 0; x < meta_img -> width; ++x) {
            uint ctile_x = x / 16;       //Position of tile inside chunk
            uint pixel_x = x % 16;       //Position of pixel inside tile
            
            uint ctile_i = (ctile_y * 16) + ctile_x;

            if(skip && (!tiles[ctile_i] && !overlays[ctile_i]))
                continue;

            if(chunk -> background_tiles[ctile_x + (ctile_y * 16)] == 0xFF)
                continue;

            //Non animated tiles should be skipped at this point.

            Block block = map -> tile_properties[chunk -> background_tiles[ctile_x + (ctile_y * 16)]];

            uint tile = block.atlas_index;

            if(block.options & TILE_TEX_FLIP_X)
                pixel_x = terrain_t->tile_size - pixel_x - 1;

            if(block.options & TILE_TEX_FLIP_Y)
                pixel_y = terrain_t->tile_size - pixel_y - 1;

            uint tile_x = tile % tiles_x;
            uint tile_y = tile / tiles_x;

            uint meta_pixel = (meta_img->width * 4 * y) + (x * 4);

            uint anim_index = 0;

            if(skip)
                anim_index = anim_frame * (tiles[ctile_i] ? 1 : 0);

            uint render_pixel = (((tile_x * terrain_t->tile_size) + pixel_x) * 4) + ( (((tile_y + anim_index) * terrain_t->tile_size) + pixel_y) * terrain_t->width * 4);

            memcpy(&(meta_img -> imageData[meta_pixel]), &(terrain_t -> imageData[render_pixel]), 4);

            /* Overlay tiles */

            if(chunk->overlay_tiles[ctile_x + (ctile_y * 16)] == 0)
                continue;

            block = map -> tile_properties[chunk -> overlay_tiles[ctile_x + (ctile_y * 16)]];

            tile = block.atlas_index;

            if(block.options & TILE_TEX_FLIP_X)
                pixel_x = terrain_t->tile_size - pixel_x - 1;

            if(block.options & TILE_TEX_FLIP_Y)
                pixel_y = terrain_t->tile_size - pixel_y - 1;

            tile_x = tile % tiles_x;
            tile_y = tile / tiles_x;

            anim_index = 0;

            if(skip)
                anim_index = anim_frame * (overlays[ctile_i] ? 1 : 0);

            render_pixel = (((tile_x * terrain_t->tile_size) + pixel_x) * 4) + ( (((tile_y + anim_index) * terrain_t->tile_size) + pixel_y) * terrain_t->width * 4);
            if(terrain_t -> imageData[render_pixel + 3] == 0)
                continue;

            memcpy(&(meta_img -> imageData[meta_pixel]), &(terrain_t -> imageData[render_pixel]), 3);
        }
    }
}

void write_map_resource(Map* map, const std::string& filename, void* data, size_t size){
    std::filesystem::create_directories(get_resource_path(g_game_path, "maps/" + std::to_string(map -> id) + ".map/"));
    FILE* file = fopen((get_resource_path(g_game_path, "maps/" + std::to_string(map -> id) + ".map/") + filename).c_str(), "wb");
    fwrite(data, size, 1, file);
    fclose(file);
}

void read_map_resource(Map* map,const std::string& filename, void* out, size_t size){
    FILE* file = fopen((get_resource_path(g_game_path, "maps/" + std::to_string(map -> id) + ".map/") + filename).c_str(), "rb");

    if(!file)
        return;

    fread(out, size, 1, file);
    fclose(file);
}
