/*
 * Created by MajesticWaffle on 5/24/22.
 * Copyright (c) 2022 Thicc Industries. All rights reserved.
 * This software is under the: Thicc-Industries-Do-What-You-Want-I-Dont-Care License.
 */

#define COMP_TRANSFORM 0
typedef struct Transform{
    Component c;
    Camera  camera{8,8}; //All entities have a camera, because fuck you
    Coord2d position = {0,0};
    Coord2d velocity = {0,0};
    uint move_state = 0;
    uint direction = 0;
    Map* map;

    Transform(){
        c.type = COMP_TRANSFORM;
        c.on_tick = [](Entity* e, Component* c){

            Transform* transform = (Transform*)c;
            transform->move_state = ENT_STATE_STATIONARY;
            Coord2d deltas[2] = {{transform -> velocity.x, 0}, {0, transform->velocity.y}};

            for(uint i = 0; i < 2; ++i) {
                Coord2d new_delta = deltas[i];
                Collider* col = entity_get_component<Collider>(e);

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
