/*
 * Created by MajesticWaffle on 4/27/21.
 * Copyright (c) 2021 Thicc Industries. All rights reserved.
 * This software is under the: Thicc-Industries-Do-What-You-Want-I-Dont-Care License.
 */

#include "geode.h"

bool i_k_keys[GLFW_KEY_LAST + 1];
bool i_m_keys[GLFW_MOUSE_BUTTON_LAST + 1];

int i_k_actions[GLFW_KEY_LAST + 1];
int i_m_actions[GLFW_KEY_LAST + 1];

int i_k_tick_actions[GLFW_KEY_LAST + 1];
int i_m_tick_actions[GLFW_KEY_LAST + 1];

Coord2d i_m_pos{0, 0};
Coord2d i_m_scroll{0, 0};
bool i_m_scrollup=false;
bool i_m_scrolldown=false;
bool i_m_scrollleft=false;
bool i_m_scrollright=false;

void input_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
    i_k_keys[key]           = action != GLFW_RELEASE;
    i_k_actions[key]        = action;
    i_k_tick_actions[key]   = action;
}

void input_mouse_button_callback(GLFWwindow* window, int button, int action, int mods){
    i_m_keys[button]            = action != GLFW_RELEASE;
    i_m_actions[button]         = action;
    i_m_tick_actions[button]    = action;
}

void input_mouse_position_callback(GLFWwindow* window, double xpos, double ypos){
    i_m_pos.x = xpos;
    i_m_pos.y = ypos;
}
void input_mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
    //Precision offsets
    i_m_scroll.x = xoffset;
    i_m_scroll.y = yoffset;

    //Tests for typical use
    //Horizontal scrolling
    if(xoffset<0){
		i_m_scrollleft=true;
    }else if(xoffset>0){
		i_m_scrollright=true;
    }
    //Vertical scrolling
    if(yoffset<0){
		i_m_scrollup=true;
    }else if(yoffset>0){
		i_m_scrolldown=true;
    }
}

void input_register_callbacks(GLFWwindow* window){
    glfwSetKeyCallback(window, input_key_callback);
    glfwSetMouseButtonCallback(window, input_mouse_button_callback);
    glfwSetCursorPosCallback(window, input_mouse_position_callback);
    glfwSetScrollCallback(window, input_mouse_scroll_callback);
}

bool input_get_key(int keycode)         { return i_k_keys[keycode]; }
bool input_get_button(int keycode)      { return i_m_keys[keycode]; }
bool input_get_key_down(int keycode)    { return i_k_actions[keycode] == GLFW_PRESS; }
bool input_get_key_up(int keycode)      { return i_k_actions[keycode] == GLFW_RELEASE; }
bool input_get_button_down(int keycode) { return i_m_actions[keycode] == GLFW_PRESS; }
bool input_get_button_up(int keycode)   { return i_m_actions[keycode] == GLFW_RELEASE; }

bool input_get_key_down_tick(int keycode)    { return i_k_tick_actions[keycode] == GLFW_PRESS; }
bool input_get_key_up_tick(int keycode)      { return i_k_tick_actions[keycode] == GLFW_RELEASE; }
bool input_get_button_down_tick(int keycode) { return i_m_tick_actions[keycode] == GLFW_PRESS; }
bool input_get_button_up_tick(int keycode)   { return i_m_tick_actions[keycode] == GLFW_RELEASE; }

Coord2d input_mouse_position() { return i_m_pos; }

void input_poll_input(){
    //Reset all actions
    for(int i = 0; i <= GLFW_KEY_LAST; i++){
        i_m_actions[i] = -1;
        i_k_actions[i] = -1;
    }
    //Reset scroll offsets
    i_m_scroll.x = 0;
    i_m_scroll.y = 0;
    //Reset scroll bools
	bool i_m_scrollup=false;
	bool i_m_scrolldown=false;
	bool i_m_scrollleft=false;
	bool i_m_scrollright=false;

    glfwPollEvents();
}

void input_tick(){
    //Reset all actions
    for(int i = 0; i <= GLFW_KEY_LAST; i++){
        i_m_tick_actions[i] = -1;
        i_k_tick_actions[i] = -1;
    }
}
