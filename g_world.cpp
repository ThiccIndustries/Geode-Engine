/*
 * Created by MajesticWaffle on 4/26/21.
 * Copyright (c) 2021 Thicc Industries. All rights reserved.
 * This software is under the: Thicc-Industries-Do-What-You-Want-I-Dont-Care License.
 */

#include "geode.h"
#include <filesystem>
#include <cstring> //Why is memcpy in here?

Chunk* g_chunk_buffer[9]{nullptr};
Chunk*  (*chunk_load_callback)(Coord2i coord);
void    (*chunk_unload_callback)(Chunk* chunk);
Texture* (*chunk_texture_callback)(Chunk* chunk);

void world_set_chunk_callbacks(
        Chunk*  (*load_callback)(Coord2i coord),
        void    (*unload_callback)(Chunk* chunk),
        Texture*(*texture_callback)(Chunk* chunk)
        ){
    chunk_load_callback     = load_callback;
    chunk_unload_callback   = unload_callback;
    chunk_texture_callback  = texture_callback;
}

void world_populate_chunk_buffer(Entity* viewport_e){
    int player_chunk_x = floor((viewport_e -> position.x + (viewport_e -> camera.position.x)) / 256);
    int player_chunk_y = floor((viewport_e -> position.y + (viewport_e -> camera.position.y)) / 256);

    for(int y = 0; y <= 2; ++y){
        for(int x = 0; x <= 2; ++x){
            int chunk_x = player_chunk_x + (x - 1);
            int chunk_y = player_chunk_y + (y - 1);
            int chunki = (y * 3) + x;


            if(g_chunk_buffer[chunki] != nullptr && g_chunk_buffer[chunki]->pos.x == chunk_x && g_chunk_buffer[chunki]->pos.y == chunk_y)
                continue;

            //Unload chunk already in buffer slot
            if(g_chunk_buffer[chunki] != nullptr) {
                if(chunk_unload_callback == nullptr)
                    error("Chunk unload funcptr not set.", "Chunk unload function pointer not set.");

                chunk_unload_callback(g_chunk_buffer[chunki]);
            }

            if(chunk_unload_callback == nullptr)
                error("Chunk load funcptr not set.", "Chunk Load function pointer not set.");

            g_chunk_buffer[chunki] = chunk_load_callback(Coord2i{chunk_x, chunk_y});
        }
    }

    std::cout << " " << std::endl;
}

void world_modify_chunk(Coord2i ccoord, Coord2i tcoord, uint value){
    //Look for chunk in chunk buffer
    Chunk* chunkptr = world_get_chunk(ccoord);

    //Chunk isn't loaded
    if(chunkptr == nullptr){
        return;
    }

    chunkptr -> overlay_tiles[tcoord.x + (tcoord.y * 16)] = value;
    world_chunk_refresh_metatextures(chunkptr);
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

Chunk* world_chunkfile_read(const std::string& path, Coord2i coord){
    FILE* chunkfile = fopen(get_resource_path(g_game_path, path + "/c" + std::to_string(coord.x) + "x" + std::to_string(coord.y) + ".cf").c_str(), "rb");

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

void world_chunkfile_write(const std::string& path, Chunk* chunk){
    int cx = chunk -> pos.x;
    int cy = chunk -> pos.y;

    std::filesystem::create_directories(get_resource_path(g_game_path, path));
    FILE* chunkfile = fopen(get_resource_path(g_game_path, path + "/c" + std::to_string(cx) + "x" + std::to_string(cy) + ".cf").c_str(), "wb");

    fwrite(&cx, sizeof(int), 1, chunkfile);
    fwrite(&cy, sizeof(int), 1, chunkfile);

    fseek(chunkfile, 0x10, SEEK_SET);

    fwrite(chunk -> overlay_tiles,      sizeof(uchar), 256, chunkfile);
    fwrite(chunk -> background_tiles, sizeof(uchar), 256, chunkfile);

    fclose(chunkfile);
}

//TODO: Consider moving this to g_rendering.cpp?
// no.
void world_chunk_refresh_metatextures(Chunk* chunk){
    //Cleanup old to prevent leaks
    if(chunk -> render_texture != nullptr) {
        texture_destroy(chunk->render_texture);
        chunk -> render_texture = nullptr;
    }

    Texture* terrain_t = chunk_texture_callback(chunk);

    if(terrain_t == nullptr){
        error("Error getting terrain texture.", "Chunk {" + std::to_string(chunk->pos.x) + ", " + std::to_string(chunk->pos.y) + "} did not receive a render texture.");
        return;
    }

    if(terrain_t -> imageData == nullptr){
        error("Rndr txtr not TEXTURE_STORE.", "Render texture recieved for Chunk {" + std::to_string(chunk->pos.x) + ", " + std::to_string(chunk->pos.y) + "} did not have TEXTURE_STORE set.");
    }


    Image* meta_img = new Image;

    meta_img -> height    = 16 * terrain_t->tile_size;
    meta_img -> width     = 16 * terrain_t->tile_size;
    meta_img -> imageData = new uchar[ meta_img -> height * meta_img -> width * 4 ];

    uint tiles_x = terrain_t->width / terrain_t->tile_size; //Number of tiles wide

    for(uint i = 0; i < meta_img -> height * meta_img -> width * 4; ++i){
        meta_img->imageData[i] = 255;
    }

    for(uint y = 0; y < meta_img -> height; ++y) {
        uint ctile_y = y / 16;   //Position of tile inside chunk
        uint pixel_y = y % 16;  //Position of pixel inside tile
        for (uint x = 0; x < meta_img -> width; ++x) {
            uint ctile_x = x / 16;       //Position of tile inside chunk
            uint pixel_x = x % 16;       //Position of pixel inside tile

            Block* block = g_block_registry[chunk -> background_tiles[ctile_x + (ctile_y * 16)]];

            uint tile = block -> atlas_index;

            if(block -> options & TILE_TEX_FLIP_X)
                pixel_x = terrain_t->tile_size - pixel_x - 1;

            if(block -> options & TILE_TEX_FLIP_Y)
                pixel_y = terrain_t->tile_size - pixel_y - 1;

            uint tile_x = tile % tiles_x;
            uint tile_y = tile / tiles_x;

            uint meta_pixel = (meta_img->width * 4 * y) + (x * 4);
            uint render_pixel = (((tile_x * terrain_t->tile_size) + pixel_x) * 4) + ( ((tile_y * terrain_t->tile_size) + pixel_y) * terrain_t->width * 4);

            memcpy(&(meta_img -> imageData[meta_pixel]), &(terrain_t -> imageData[render_pixel]), 4);

            if(chunk->overlay_tiles[ctile_x + (ctile_y * 16)] == 0)
                continue;

            block = g_block_registry[chunk -> overlay_tiles[ctile_x + (ctile_y * 16)]];

            tile = block -> atlas_index;

            if(block -> options & TILE_TEX_FLIP_X)
                pixel_x = terrain_t->tile_size - pixel_x - 1;

            if(block -> options & TILE_TEX_FLIP_Y)
                pixel_y = terrain_t->tile_size - pixel_y - 1;

            tile_x = tile % tiles_x;
            tile_y = tile / tiles_x;

            render_pixel = (((tile_x * terrain_t->tile_size) + pixel_x) * 4) + ( ((tile_y * terrain_t->tile_size) + pixel_y) * terrain_t->width * 4);
            if(terrain_t -> imageData[render_pixel + 3] == 0)
                continue;

            memcpy(&(meta_img -> imageData[meta_pixel]), &(terrain_t -> imageData[render_pixel]), 3);
        }
    }

    chunk -> render_texture = texture_generate(meta_img, TEXTURE_SINGLE, 256);
    delete[] meta_img -> imageData;
    delete meta_img;
}
