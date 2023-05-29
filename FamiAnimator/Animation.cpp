#include "Animation.h"

AnimationFrame::AnimationFrame()
{
    x = 0;
    y = 0;
    duration_ms = 0;
    bound_box_x = 0;
    bound_box_y = 0;
    bound_box_width = 16;
    bound_box_height = 16;
    lock_movement = false;
    lock_direction = false;
    lock_attack = false;
    damage_box = false;
    damage_box_x = 0;
    damage_box_y = 0;
    damage_box_width = 16;
    damage_box_height = 16;
}
