/*
 * Created by MajesticWaffle on 4/29/21.
 * Copyright (c) 2021 Thicc Industries. All rights reserved.
 * This software is under the: Thicc-Industries-Do-What-You-Want-I-Dont-Care License.
 */

#include "geode.h"

Entity* g_entity_registry[ENTITY_MAX];
uint g_entity_highest_id = 0;

Entity* entity_create(int id_override){
    Entity* entity = new Entity();
    entity -> transform = entity_add_component<Transform>(entity);
    entity -> id = id_override;
    g_entity_registry[id_override] = entity;
    if(id_override > g_entity_highest_id)
        g_entity_highest_id = id_override;
    return entity;
}

Entity* entity_create(){
    Entity* entity = new Entity();
    entity -> transform = entity_add_component<Transform>(entity);
    for(int i = 0; i < ENTITY_MAX; ++i){
        if(g_entity_registry[i] == nullptr){
            entity -> id = i;
            g_entity_registry[i] = entity;

            if(i > g_entity_highest_id)
                g_entity_highest_id = i;

            break;
        }
    }

    return entity;
}

Component* entity_get_component(Entity* entity, uint type){
    for(auto & component : entity -> components){
        if(component != nullptr && component->type == type)
            return component;
    }
    return nullptr;
}

//Safely delete Entity pointer while also cleaning entity_registry
void entity_delete(uint id){
    //Recalculate g_entity_highest_id
    if(id == g_entity_highest_id){
        for(uint i = g_entity_highest_id; i >= 0; --i){
            if(g_entity_registry[i] != nullptr) {
                g_entity_highest_id = i;
                break;
            }
        }
    }

    delete g_entity_registry[id];
    g_entity_registry[id] = nullptr;

}

void entity_tick(){
    for(int i = 0; i <= g_entity_highest_id; ++i){
        Entity* e = g_entity_registry[i];

        if(e == nullptr)
            continue;

        //Skip entities which are dead but not deleted yet
        if(e -> health <= 0)
            continue;

        //Tick entity
        for(auto & component : e->components){
            if(component != nullptr && component->on_tick != nullptr)
                component->on_tick(e, component);
        }

    }
}

Coord2d entity_collision(Collider* col, Transform* transform, Coord2d delta){
    //   << delta.x << " " << delta.y << std::endl;
    Coord2d new_ent_pos = transform -> position + delta;

    //Global coordinates of bounding box, with new desired position
    BoundingBox global_bb = { {col->col_bounds.p1.x + new_ent_pos.x, col->col_bounds.p1.y + new_ent_pos.y},
                              {col->col_bounds.p2.x + new_ent_pos.x, col->col_bounds.p2.y + new_ent_pos.y} };

    Coord2i chunk;
    Coord2i tile{ (int)(transform->position.x / 16),
                  (int)(transform->position.y  / 16) };

    Chunk* chunkptr;
    Coord2i rel_tile;

    //Loop though tiles
    for(int x = tile.x - 2; x < tile.x + 2; x++){
        for(int y = tile.y - 2; y < tile.y + 2; y++){
            chunk = { (int)floor((double)x / 16.0),
                      (int)floor((double)y / 16.0) };

            rel_tile = { (int)((double)x - (chunk.x * 16)),
                         (int)((double)y - (chunk.y * 16)) };

            chunkptr = world_get_chunk(chunk);

            if(chunkptr == nullptr)
                continue;

            //Tile is not solid, no need to check AABBa
            uint index = chunkptr->background_tiles[rel_tile.x + (rel_tile.y * 16) ];
            if(!((transform -> map->tile_properties[ chunkptr->background_tiles[rel_tile.x + (rel_tile.y * 16) ] ].options & TILE_SOLID)
             || (chunkptr -> background_tiles[rel_tile.x + (rel_tile.y * 16) ] == 0)
             || (transform -> map->tile_properties[ chunkptr->overlay_tiles[rel_tile.x + (rel_tile.y * 16) ] ].options & TILE_SOLID)))
                continue;

            BoundingBox tile_bb = { {(rel_tile.x * 16.0) + (chunk.x * 256.0)      , (rel_tile.y * 16.0) + (chunk.y * 256.0)},
                                    {(rel_tile.x * 16.0) + (chunk.x * 256.0) + 16 , (rel_tile.y * 16.0) + (chunk.y * 256.0) + 16} };

            //Test AABB vs AABB
            if(entity_AABB(tile_bb, global_bb)){
                double maximum_delta_x = 0, maximum_delta_y = 0;

                if(delta.x > 0) {
                    maximum_delta_x = delta.x - (global_bb.p2.x + 0.001 - tile_bb.p1.x);

                }
                else if(delta.x < 0){
                    maximum_delta_x = delta.x - (global_bb.p1.x - 0.001 - tile_bb.p2.x);
                }
                if(delta.y > 0){
                    maximum_delta_y = delta.y - (global_bb.p2.y + 0.001 - tile_bb.p1.y);
                }
                else if(delta.y < 0){
                    maximum_delta_y = delta.y - (global_bb.p1.y - 0.001 - tile_bb.p2.y);
                }

                return Coord2d{maximum_delta_x, maximum_delta_y};
            }
        }
    }

    //Check all entities

    for(int i = 0; i <= g_entity_highest_id; ++i){
        if(g_entity_registry[i] == nullptr)
            continue;

        auto* check_col = (Collider*)entity_get_component(g_entity_registry[i], COMP_COLLIDER);
        auto* check_transform = (Transform*) entity_get_component(g_entity_registry[i], COMP_TRANSFORM);

        if(check_col == nullptr)
            continue;

        if(check_transform == transform)
            continue;

        BoundingBox ent_g_bb = {
                {check_col->col_bounds.p1.x + check_transform->position.x, check_col->col_bounds.p1.y + check_transform->position.y},
                {check_col->col_bounds.p2.x + check_transform->position.x, check_col->col_bounds.p2.y + check_transform->position.y},
        };

        if(entity_AABB(ent_g_bb, global_bb)){
            double maximum_delta_x = 0, maximum_delta_y = 0;

            if(check_transform->velocity.x > 0) {
                maximum_delta_x = check_transform->velocity.x - (global_bb.p2.x + 0.001 - ent_g_bb.p1.x);

            }
            else if(check_transform->velocity.x < 0){
                maximum_delta_x = check_transform->velocity.x - (global_bb.p1.x - 0.001 - ent_g_bb.p2.x);
            }
            if(check_transform->velocity.y > 0){
                maximum_delta_y = check_transform->velocity.y - (global_bb.p2.y + 0.001 - ent_g_bb.p1.y);
            }
            else if(check_transform->velocity.y < 0){
                maximum_delta_y = check_transform->velocity.y - (global_bb.p1.y - 0.001 - ent_g_bb.p2.y);
            }


            return Coord2d{maximum_delta_x, maximum_delta_y};
        }
    }

    return delta;
}

Entity* entity_hit(Collider* col, Transform* transform){

    //   << delta.x << " " << delta.y << std::endl;

    Coord2d new_ent_pos = transform -> position + transform->velocity;

    //Global coordinates of bounding box, with new desired position
    BoundingBox global_bb = { {col->hit_bounds.p1.x + new_ent_pos.x, col->hit_bounds.p1.y + new_ent_pos.y},
                              {col->hit_bounds.p2.x + new_ent_pos.x, col->hit_bounds.p2.y + new_ent_pos.y} };

    //Check all entities

    for(int i = 0; i <= g_entity_highest_id; ++i){
        if(g_entity_registry[i] == nullptr)
            continue;
        auto* check_col = (Collider*)entity_get_component(g_entity_registry[i], COMP_COLLIDER);
        auto* check_transform = (Transform*) entity_get_component(g_entity_registry[i], COMP_TRANSFORM);

        if(check_col == nullptr)
            continue;

        if(check_col == col)
            continue;

        BoundingBox ent_g_bb = {
                {check_col->hit_bounds.p1.x + check_transform->position.x, check_col->hit_bounds.p1.y + check_transform->position.y},
                {check_col->hit_bounds.p2.x + check_transform->position.x, check_col->hit_bounds.p2.y + check_transform->position.y},
        };

        if(entity_AABB(ent_g_bb, global_bb))
            return g_entity_registry[i];
    }

    return nullptr;
}

void entity_damage(Entity* entity, uint damage){
    if(entity -> health > 0) {
        entity->health -= damage;
        if(entity -> health < 0)
            entity -> health = 0;
    }

    if(entity -> health == 0){
        for(auto & component : entity->components){
            if(component != nullptr && component->on_death != nullptr)
                component->on_death(entity, component);
        }
    }
}

void entity_kill(Entity* entity){
    for(auto & component : entity->components){
        if(component != nullptr && component->on_death != nullptr)
            component->on_death(entity, component);
    }
}


bool entity_AABB(BoundingBox a, BoundingBox b){
    return (a.p1.x < b.p2.x && a.p2.x > b.p1.x) && (a.p1.y < b.p2.y && a.p2.y > b.p1.y);
}


