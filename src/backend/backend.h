#pragma once

#include <uuid/uuid.h>
#include "util/except.h"
#include "util/list.h"
#include "net/protocol.h"

typedef struct new_player_event {
    // the list of entries
    list_entry_t entry;

    // the new player's name
    char name[MAX_STRING_SIZE(16) + 1];

    // the new players uuid
    uuid_t uuid;
} new_player_event_t;

// TODO: have this contain a tick arena for all the allocations
//       we are going to do that we are going to free on a flip

typedef struct global_queues {
    // a list of new_player_event
    list_t new_players;
} global_queues_t;

void global_queues_lock();

global_queues_t* get_global_queues();

void global_queues_unlock();

/**
 * Start the backend on the current thread
 */
err_t backend_start();
