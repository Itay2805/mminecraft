#pragma once

#include <frontend/connection.h>
#include <frontend/protocol.h>
#include <util/except.h>
#include <util/arena.h>
#include <util/list.h>

#include <flecs/flecs.h>
#include <uuid/uuid.h>
#include "world/world.h"

/**
 * The max view distance we support
 */
#define MAX_VIEW_DISTANCE 32

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Global event queues, swapped each tick
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum global_event_type {
    EVENT_NEW_PLAYER,
    EVENT_CONNECTION_CLOSED,
    EVENT_CHUNK_GENERATED,
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

typedef struct packet_event {
    list_entry_t entry;
    ecs_entity_t entity;
    uint32_t size;
    uint8_t data[];
} packet_event_t;

typedef struct global_queues {
    // a list of global_event_t
    list_t events;

    // a list of packet_t
    list_t packes;

    // the allocator for this tick
    arena_t arena;
} global_queues_t;

void global_queues_lock();

global_queues_t* get_global_queues();

void global_queues_unlock();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// chunk generation lists
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct chunk_generated {
    // the chunk entity that this was generated for
    ecs_entity_t chunk_entity;

    // the newly generated chunk
    chunk_t* chunk;

    // the sched task, that needs to be deleted, not
    // that you should wait for zero ref count before
    // actually trying to free it
    struct sched_task* task;
} chunk_generated_t;

/**
 * The queue used for the generated chunks
 */
extern struct mpscq m_generated_chunk_queue;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// backend start
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Start the backend on the current thread
 */
err_t backend_start();
