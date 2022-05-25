/*
 * Created by MajesticWaffle on 5/24/22.
 * Copyright (c) 2022 Thicc Industries. All rights reserved.
 * This software is under the: Thicc-Industries-Do-What-You-Want-I-Dont-Care License.
 */

#define COMP_COLLIDER 2
typedef struct Collider{
    Component c;
    BoundingBox col_bounds{};   //Collider for movement collision
    BoundingBox hit_bounds{};   //Collider for hits

    Collider(){
        c.type = COMP_COLLIDER;
    }

} Collider;
