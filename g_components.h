/*
 * Created by MajesticWaffle on 5/18/22.
 * Copyright (c) 2022 Thicc Industries. All rights reserved.
 * This software is under the: Thicc-Industries-Do-What-You-Want-I-Dont-Care License.
 */
#include "geode.h"

#define COMP_TRANSFORM 0
#define COMP_RENDERER 1
#define COMP_COLLIDER 2

typedef struct Component{
    uint type = 0;
    void (*on_tick)(Entity* e, Component* c) = nullptr;
    void (*on_death)(Entity* e, Component* c) = nullptr;
    void (*on_create)(Entity* e, Component* c) = nullptr;
} Component;

typedef struct Transform{
    Component c;
    Camera  camera{8,8}; //All entities have a camera, because fuck you
    Coord2d position;
    Coord2d velocity;
    uint move_state;
    uint direction;
    Map* map;

    Transform(){
        c.type = COMP_TRANSFORM;
        c.on_tick = [](Entity* e, Component* c){
            Transform* transform = (Transform*)c;
            transform->move_state = ENT_STATE_STATIONARY;
            Coord2d deltas[2] = {{transform -> velocity.x, 0}, {0, transform->velocity.y}};

            for(uint i = 0; i < 2; ++i) {
                Coord2d new_delta = deltas[i];
                Collider* col = (Collider*)entity_get_component(e, COMP_COLLIDER);

                if(new_delta.x == 0.0 && new_delta.y == 0.0)
                    continue;

                if (col != nullptr)
                    new_delta = entity_collision(col, transform, new_delta);

                Coord2d new_ent_pos = {transform->position.x + new_delta.x,
                                       transform->position.y + new_delta.y};

                transform->position = new_ent_pos;

                transform->move_state = ENT_STATE_MOVING;
            }

            if (transform->velocity.x > 0)
                transform->direction = DIRECTION_EAST;
            if (transform->velocity.x < 0)
                transform->direction = DIRECTION_WEST;
            if (transform->velocity.y > 0)
                transform->direction = DIRECTION_SOUTH;
            if (transform->velocity.y < 0)
                transform->direction = DIRECTION_NORTH;
        };
    };
} Transform;

typedef struct Renderer{
    Component c;
    uint atlas_index;           //Index of the Upper-Left corner of sprites 3x3 sprite sheet //TODO: What?
    Coord2i spritesheet_size;   //Size of the sprite sheet { directions, frames }
    uint frame_count;           //Number of animation frames
    uint* frame_order;          //Order of frames in array
    uint animation_rate;        //Rate at which to animate movement speed

    Renderer(){
        c.type = COMP_RENDERER;
    }

} Renderer;

typedef struct Collider{
    Component c;
    BoundingBox col_bounds{};   //Collider for movement collision
    BoundingBox hit_bounds{};   //Collider for hits

    Collider(){
        c.type = COMP_COLLIDER;
    }

} Collider;