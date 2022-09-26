#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "flecs/flecs.h"
#include "world/world.h"
#include "backend/components/entity.h"

void send_join_game(int fd, ecs_entity_t entity_id);

void send_chunk_data(int fd, chunk_t* chunk);

void send_player_position_and_look(int fd, int32_t teleport_id, EntityPosition position, EntityRotation rotation);
