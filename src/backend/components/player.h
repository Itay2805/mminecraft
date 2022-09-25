#pragma once

#include <frontend/connection.h>
#include <frontend/protocol.h>

#include <flecs/flecs.h>

// sets the connection of the player
typedef struct player {
    // the name of the player
    char* name;

    // the connection of this player
    connection_t* connection;
} player_t;
extern ECS_COMPONENT_DECLARE(player_t);

// sets the view distance of the player
typedef struct player_view_distance {
    uint8_t view_distance;
} player_view_distance_t;
extern ECS_COMPONENT_DECLARE(player_view_distance_t);

// the game mode of the player
typedef enum game_mode {
    GAME_MODE_SURVIVAL,
    GAME_MODE_CREATIVE,
    GAME_MODE_ADVENTURE,
    GAME_MODE_SPECTATOR,
} game_mode_t;
extern ECS_COMPONENT_DECLARE(game_mode_t);

// marks that the player is waiting for a teleport
// with the given id
typedef struct player_teleport {
    int32_t id;
} player_teleport_t;
extern ECS_COMPONENT_DECLARE(player_teleport_t);

// tags
extern ECS_DECLARE(players);

void init_player_ecs();
