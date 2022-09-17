#pragma once

#include <uuid/uuid.h>
#include "util/except.h"
#include "util/list.h"
#include "net/protocol.h"
#include "net/connection.h"
#include "flecs/flecs.h"

typedef enum global_event_type {
    EVENT_NEW_PLAYER,
    EVENT_CONNECTION_CLOSED,
} global_event_type_t;

typedef struct new_player_event {
    // the new player's name
    char name[MAX_STRING_SIZE(16) + 1];

    // the new players uuid
    uuid_t uuid;

    // the related connection
    connection_t* connection;
} new_player_event_t;

typedef struct connection_closed_event {
    // the entity the connection was related to
    ecs_entity_t entity;
} connection_closed_event_t;

typedef struct global_event {
    // the list of entries
    list_entry_t entry;

    // the event type
    global_event_type_t type;

    // the data
    union {
        new_player_event_t new_player;
        connection_closed_event_t connection_closed;
    };
} global_event_t;

// TODO: have this contain a tick arena for all the allocations
//       we are going to do that we are going to free on a flip

typedef struct global_queues {
    // a list of new_player_event
    list_t events;
} global_queues_t;

void global_queues_lock();

global_queues_t* get_global_queues();

void global_queues_unlock();

/**
 * Start the backend on the current thread
 */
err_t backend_start();
