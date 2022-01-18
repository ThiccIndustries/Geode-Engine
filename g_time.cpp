/*
 * Created by MajesticWaffle on 4/29/21.
 * Copyright (c) 2021 Thicc Industries. All rights reserved.
 * This software is under the: Thicc-Industries-Do-What-You-Want-I-Dont-Care License.
 */

#include "geode.h"

Time* g_time = new Time;

typedef struct I_Callback{
    long duration;
    long starting_tick;
    void (* callback)(void* passthough);
    void* passthough_ptr;
}I_Callback;

I_Callback *callbacks[256]{nullptr};

int i_callback_highest_id = 0;

void (* tick_callback)();

int time_timer_purge();


void time_update_time(double glfw_time){
    g_time -> delta = glfw_time - g_time -> global;
    g_time -> global = glfw_time;

    if(g_time -> paused) {
        return;
    }

    if(g_time -> tick_delta >= 1.0 / TIME_TPS) {
        g_time->tick++;
        tick_callback();
        input_tick();

        g_time -> tick_delta = 0;
    }
    else
        g_time -> tick_delta += g_time -> delta;

    //Tick all callback timers
    for(uint i = 0; i <= i_callback_highest_id; i++){
        if(callbacks[i] == nullptr) {
            continue;
        }

        if(g_time -> tick >= (callbacks[i] -> starting_tick + callbacks[i] -> duration)){
            callbacks[i] -> callback( callbacks[i] -> passthough_ptr );
            callbacks[i] = nullptr;

            //Recalculate g_entity_highest_id
            if(i == i_callback_highest_id){
                for(int j = 255; j >= 0; j--){
                    if(callbacks[j] != nullptr) {
                        i_callback_highest_id = j;
                        break;
                    }
                }
            }

        }
    }
}

int time_get_framerate(){
   return (int)(1.0 / g_time -> delta);
}

Timer* time_timer_start(long duration){
    Timer* t = new Timer;

    t -> duration = duration;
    t -> starting_tick = g_time -> tick;


    return t;
}

void time_callback_start(long duration, void (*callback_function)(void* passthough_data), void* passthough_data){
    I_Callback* callback = new I_Callback{duration, g_time -> tick, callback_function, passthough_data};

    for(int i = 0; i < 255; ++i){
        if(callbacks[i] == nullptr){
            callbacks[i] = callback;

            if(i > i_callback_highest_id)
                i_callback_highest_id = i;

            break;
        }
    }
}

bool time_timer_finished(Timer*& t){
    if(t == nullptr)
        return false;

    //timer is finished
    if(g_time -> tick >= (t -> starting_tick + t -> duration)){
        time_timer_cancel(t);
        return true;
    }else
        return false;

}

void time_set_tick_callback(void (*callback_function)()){
    tick_callback = callback_function;
}


void time_timer_cancel(Timer*& t){
    if(t == nullptr)
        return;

    delete t;
    t = nullptr;
}