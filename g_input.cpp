/*
 * Created by MajesticWaffle on 4/27/21.
 * Copyright (c) 2021 Thicc Industries. All rights reserved.
 * This software is under the: Thicc-Industries-Do-What-You-Want-I-Dont-Care License.
 */

#include "geode.h"

bool i_k_keys[GLFW_KEY_LAST + 1];
bool i_m_keys[GLFW_KEY_LAST + 1]; //GLFW_MOUSE_BUTTON_LAST is only 7, plenty of mice have more than that

int i_k_actions[GLFW_KEY_LAST + 1];
int i_m_actions[GLFW_KEY_LAST + 1];
Coord2d i_m_pos{0, 0};

void input_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
    i_k_keys[key]     = action != GLFW_RELEASE;
    i_k_actions[key]  = action;
}

void input_mouse_button_callback(GLFWwindow* window, int button, int action, int mods){
    i_m_keys[button]      = action != GLFW_RELEASE;
    i_m_actions[button]   = action;
}

void input_mouse_position_callback(GLFWwindow* window, double xpos, double ypos){
    i_m_pos.x = xpos;
    i_m_pos.y = ypos;
}

void input_register_callbacks(GLFWwindow* window){
    glfwSetKeyCallback(window, input_key_callback);
    glfwSetMouseButtonCallback(window, input_mouse_button_callback);
    glfwSetCursorPosCallback(window, input_mouse_position_callback);
}

bool input_get_key(int keycode)         { return i_k_keys[keycode]; }
bool input_get_button(int keycode)      { return i_m_keys[keycode]; }
Coord2d input_mouse_position()          { return i_m_pos; }

bool input_get_key_down(int keycode){ 
    if(i_k_actions[keycode] == GLFW_PRESS){
        i_k_actions[keycode] = -1;
        return true;
    }
    return false;
}

bool input_get_key_up(int keycode){ 
    if(i_k_actions[keycode] == GLFW_RELEASE){
        i_k_actions[keycode] = -1;
        return true;
    }
    return false;
}

bool input_get_button_down(int keycode){ 
    if(i_m_actions[keycode] == GLFW_PRESS){
        i_m_actions[keycode] = -1;
        return true;
    }
}

bool input_get_button_up(int keycode){ 
    if(i_m_actions[keycode] == GLFW_RELEASE){
        i_m_actions[keycode] = -1;
        return true;
    }
    return false;
}

void input_poll_input(){
    glfwPollEvents();
}

void input_tick(){
    //Reset all actions
    for(int i = 0; i <= GLFW_KEY_LAST; i++){
        i_m_actions[i] = -1;
        i_k_actions[i] = -1;
    }
    glfwPollEvents();
}