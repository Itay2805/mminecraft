#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "flecs/flecs.h"
#include "world/world.h"
#include "backend/components/entity.h"

void send_join_game(int fd, ecs_entity_t entity_id);

void send_chunk_data(int fd, chunk_t* chunk);

void send_full_update_light(int fd, chunk_t* chunk);

void send_unload_chunk(int fd, chunk_position_t position);

void send_player_position_and_look(int fd, int32_t teleport_id, EntityPosition position, EntityRotation rotation);

void send_keep_alive(int fd, uint64_t keep_alive);

void send_update_view_position(int fd, chunk_position_t position);
