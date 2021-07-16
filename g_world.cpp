/*
 * Created by MajesticWaffle on 4/26/21.
 * Copyright (c) 2021 Thicc Industries. All rights reserved.
 * This software is under the: Thicc-Industries-Do-What-You-Want-I-Dont-Care License.
 */

#include "geode.h"
#include <filesystem>

Chunk* g_chunk_buffer[RENDER_DISTANCE * RENDER_DISTANCE * 4];
Chunk*  (*chunk_load_callback)(Coord2i coord);
void    (*chunk_unload_callback)(Chunk* chunk);

void world_set_chunk_callbacks(
        Chunk*  (*load_callback)(Coord2i coord),
        void    (*unload_callback)(Chunk* chunk)
        ){
    chunk_load_callback     = load_callback;
    chunk_unload_callback   = unload_callback;
}

void world_populate_chunk_buffer(Entity* viewport_e){
    int player_chunk_x = (int)(viewport_e -> position.x + (viewport_e -> camera.position.x)) / 256;
    int player_chunk_y = (int)(viewport_e -> position.y + (viewport_e -> camera.position.x)) / 256;

    for(int x = 0; x < RENDER_DISTANCE * 2; x++){
        for(int y = 0; y < RENDER_DISTANCE * 2; y++){
            int chunk_x = player_chunk_x - (x - RENDER_DISTANCE);
            int chunk_y = player_chunk_y - (y - RENDER_DISTANCE);
            int chunki = x + (y * RENDER_DISTANCE * 2);

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
}

void world_modify_chunk(Coord2i ccoord, Coord2i tcoord, uint value){
    //Look for chunk in chunk buffer
    Chunk* chunkptr = world_get_chunk(ccoord);

    //Chunk isn't loaded
    if(chunkptr == nullptr){
        return;
    }

    chunkptr -> foreground_tiles[tcoord.x + (tcoord.y * 16)] = value;
}


Chunk* world_get_chunk(Coord2i ccoord){
    for(Chunk* c : g_chunk_buffer){
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
    fread(chunkptr -> foreground_tiles, 256, sizeof(uchar), chunkfile);

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
    fwrite(chunk -> foreground_tiles,   sizeof(uchar), 256, chunkfile);

    fclose(chunkfile);
}
