#pragma once

#include "entity.h"

#include <frontend/connection.h>
#include <frontend/protocol.h>

#include <flecs/flecs.h>

// sets the connection of the player
typedef struct {
    // the name of the player
    char* name;

    // the connection of this player
    connection_t* connection;
} PlayerConnection;
extern ECS_COMPONENT_DECLARE(PlayerConnection);

// used to indicate the position of the player as the client sees it
typedef EntityPosition PlayerPosition;
extern ECS_COMPONENT_DECLARE(PlayerPosition);

// sets the view distance of the player
typedef struct {
    uint8_t view_distance;
} PlayerViewDistance;
extern ECS_COMPONENT_DECLARE(PlayerViewDistance);

// the game mode of the player
typedef enum {
    GAME_MODE_SURVIVAL,
    GAME_MODE_CREATIVE,
    GAME_MODE_ADVENTURE,
    GAME_MODE_SPECTATOR,
} PlayerGameMode;
extern ECS_COMPONENT_DECLARE(PlayerGameMode);

// marks that the player is waiting for a teleport
// with the given id
typedef struct {
    int32_t id;
} PlayerTeleportRequest;
extern ECS_COMPONENT_DECLARE(PlayerTeleportRequest);

// set when you want to disconnect the player
typedef struct {
    const char* reason;
} DisconnectPlayer;
extern ECS_COMPONENT_DECLARE(DisconnectPlayer);

extern ECS_DECLARE(PlayerHasChunk);
extern ECS_DECLARE(PlayerNeedsChunk);

void init_player_ecs();
