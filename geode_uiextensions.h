/*
 * Created by MajesticWaffle on 6/9/22.
 * Copyright (c) 2022 Thicc Industries. All rights reserved.
 * This software is under the: Thicc-Industries-Do-What-You-Want-I-Dont-Care License.
 */

#ifndef GEODE_UIEXTENSIONS_H
#define GEODE_UIEXTENSIONS_H

#include "geode.h"

Panel* uix_create_health_bar(Texture* t, uint atlas_active, uint atlas_inactive, uint length, int* value);
Panel_Text* uix_create_int_display(Font* font, std::string prefix, int* value, uint update_interval);

void ui_refresh_checkbox(Panel* checkbox);

template <typename T>
Panel* uix_create_checkbox(int size, Color fg, Color bg, T* out, T sel, T dsel){
    size *= 4;
    Panel_Button* button = new Panel_Button();

    Panel* box = new Panel();
    Panel* indicator = new Panel();


    button -> p.child_count = 2;
    button -> p.children = new Panel*[button -> p.child_count]{box, indicator};

    box -> type = PANEL_BOX;
    box -> size = {size, size};
    box -> background_color = bg;
    box -> foreground_color = fg;
    box -> has_background = true;

    indicator -> type = PANEL_BOX;
    indicator -> position = {size / 4, size / 4};
    indicator -> size = {size / 2, size / 2};
    indicator -> foreground_color = *out == sel ? bg : fg;
    indicator -> has_background = false;

    typedef struct Packet{
        Color fg;
        Color bg;
        Panel* indicator;
        T* out;
        T sel;
        T dsel;
    } Packet;

    Packet *p = new Packet{fg, bg, indicator, out, sel, dsel};

    button -> p.position = {0, 0};
    button -> p.size = {size, size};
    button -> packet = p;
    button -> click_func = [](void* v){
        Packet* p = (Packet*)v;
        *p -> out = *p -> out == p -> sel ? p -> dsel : p -> sel;
        p -> indicator -> foreground_color = *(p -> out) == p -> sel ? p -> bg : p -> fg;
    };

    return (Panel*)button;
}

template <typename T>
void uix_refresh_checkbox(Panel* checkbox){
    Panel_Button* cb = (Panel_Button*)checkbox;
    typedef struct Packet{
        Color fg;
        Color bg;
        Panel* indicator;
        T* out;
        T sel;
        T dsel;
    } Packet;

    Packet* p = (Packet*)cb -> packet;
    p -> indicator -> foreground_color = *(p -> out) == p -> sel ? p -> bg : p -> fg;
}

#endif //GEODE_UIEXTENSIONS_H
