/*
 * Created by MajesticWaffle on 5/26/22.
 * Copyright (c) 2022 Thicc Industries. All rights reserved.
 * This software is under the: Thicc-Industries-Do-What-You-Want-I-Dont-Care License.
 */

#define COMP_ITEM 3

typedef struct ItemRenderer{
    Component c;

    uint collector_type;
    double collection_raidus;

    ItemRenderer(){
        c.type = COMP_ITEM;
        collector_type = 4; //COMP_INVENTORY hardcode
        collection_raidus = 1.0;

        c.on_create = [](Entity* e, Component* c){
            auto item_obj = e;
            auto renderer = entity_get_component<Renderer>(e);
            auto comp = (ItemRenderer*)c;

            item_obj->health = 99999;
            renderer->spritesheet_size = {1, 1};
            renderer->frame_count = 1;
            renderer->frame_order = new uint[]{0};
            renderer->animation_rate  = TIME_TPS;
        };
    }

} Item;