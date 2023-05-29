#pragma once

#include <QString>
#include <vector>

struct AnimationFrame
{
    QString name;
    int x;
    int y;
    int duration_ms;
    int bound_box_x;
    int bound_box_y;
    int bound_box_width;
    int bound_box_height;
    bool lock_movement;
    bool lock_direction;
    bool lock_attack;
    bool damage_box;
    int damage_box_x;
    int damage_box_y;
    int damage_box_width;
    int damage_box_height;

    AnimationFrame();
};

struct Animation
{
    QString name;
    std::vector<AnimationFrame> frames;
    bool loop;
};
