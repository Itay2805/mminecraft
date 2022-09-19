#pragma once

#include <flecs/flecs.h>

extern ecs_world_t* g_ecs;

extern ECS_DECLARE(components);

void init_world_ecs();
