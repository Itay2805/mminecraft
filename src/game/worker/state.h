

#include <game/logic/entity.h>
#include <game/logic/chunk.h>

#include <util/list.h>

#include <stdint.h>

///////////////////////////////////////////////////////////////////////////////
// Worker recv_state
///////////////////////////////////////////////////////////////////////////////

/**
 * Stores the kind of update we have in here
 */
typedef enum state_kind {
    STATE_MAX,
} state_kind_t;

/**
 * The game recv_state, this is not actually a complete one, but just a partial one
 * that the worker is going to have available.
 * 
 * Changes to this struct are only permitted after a tick, and before the next 
 * tick.
 */
typedef struct game_state {
    struct {
        // the entity id for quick lookup
        uint32_t key;
        
        // the pointer to the entity copy
        entity_t* value;
    }* entities;

    struct {
        // The position of the chunk
        chunk_pos_t key;

        // the pointer to the chunk copy
        chunk_t* value;
    }* chunks;
} game_state_t;

///////////////////////////////////////////////////////////////////////////////
// State updates
//
// TODO: figure the best structure to hold this, right now we are using a 
//       linked list because it is easy to add new entries
///////////////////////////////////////////////////////////////////////////////

typedef struct state_updates {
    list_t updates;
    // TODO: flags for things that are being 
    //       updated for easier merging
} state_updates_t;

typedef enum state_update_kind {
    STATE_UPDATE_MAX,
} state_update_kind_t;

typedef struct state_update {
    state_update_kind_t kind;
    list_entry_t entry;
    char payload[0];
} state_update_t;
