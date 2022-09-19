#include "backend.h"

#include "protocol.h"
#include "sender.h"
#include "world.h"

#include <backend/components/player.h>
#include <backend/components/entity.h>
#include <backend/components/chunk.h>
#include <world/world.h>
#include <util/defs.h>

#include <sys/sysinfo.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Global events
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * The global queues instance
 */
static global_queues_t m_global_queues[2];

/**
 * The current queue that is used
 */
static int m_current_queue = 1;

static pthread_mutex_t m_global_queues_mutex;

void global_queues_lock() {
    pthread_mutex_lock(&m_global_queues_mutex);
}

void global_queues_unlock() {
    pthread_mutex_unlock(&m_global_queues_mutex);
}

global_queues_t* get_global_queues() {
    return &m_global_queues[m_current_queue];
}

static global_queues_t* get_current_queues() {
    return &m_global_queues[1 - m_current_queue];
}

static void global_queues_switch() {
    global_queues_lock();
    m_current_queue = 1 - m_current_queue;
    global_queues_unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Handle global events
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static err_t handle_global_events() {
    err_t err = NO_ERROR;

    // tick handling
    global_queues_t* q = get_current_queues();

    // start by processing the global events that should be done before all the systems run
    global_event_t* global_event;
    while ((global_event = LIST_ENTRY(list_pop(&q->events), global_event_t, entry)) != NULL) {
        switch (global_event->type) {
            // the player got disconnected, remove it from the world
            case EVENT_CONNECTION_CLOSED: {
                // get the player entity
                ecs_entity_t entity = global_event->connection_closed.entity;
                const player_t* player = ecs_get(g_ecs, entity, player_t);

                // release the connection
                release_connection(player->connection);

                // delete the entity
                ecs_delete(g_ecs, global_event->connection_closed.entity);
            } break;

            case EVENT_NEW_PLAYER: {
                new_player_event_t* event = &global_event->new_player;

                // create the entity
                ecs_entity_t entity = ecs_new_id(g_ecs);
                ecs_add_pair(g_ecs, entity, EcsChildOf, players);
                ecs_add(g_ecs, entity, entity_position_t);
                ecs_add(g_ecs, entity, entity_rotation_t);
                ecs_add(g_ecs, entity, entity_velocity_t);
                ecs_add(g_ecs, entity, game_mode_t);

                // create the player component
                player_t* player = ecs_get_mut(g_ecs, entity, player_t);
                STATIC_ASSERT(sizeof(player->name) == sizeof(event->name));
                memcpy(player->name, event->name, sizeof(player->name));
                player->connection = event->connection;

                // set the entity id for the player
                entity_id_t* id = ecs_get_mut(g_ecs, entity, entity_id_t);
                uuid_copy(id->uuid, event->uuid);

                // set the game mode
                game_mode_t* mode = ecs_get_mut(g_ecs, entity, game_mode_t);
                *mode = GAME_MODE_CREATIVE;

                // set the name for easily looking up the player
                ecs_set_name(g_ecs, entity, player->name);

                // set the entity for the connection and release it
                event->connection->entity = entity;

                // send the joined game thing
                send_join_game(player->connection->fd, entity);
            } break;

            default:
                CHECK_FAIL();
        }
    }

cleanup:
    return err;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Handle player specific events
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define PCHECK(expr, ...) CHECK_ERROR_LABEL(expr, ERROR_PROTOCOL_VIOLATION, next_packet, ## __VA_ARGS__)

static err_t handle_packets() {
    err_t err = NO_ERROR;

    global_queues_t* q = get_current_queues();

    packet_event_t* packet;
    while ((packet = LIST_ENTRY(list_pop(&q->packes), packet_event_t, entry)) != NULL) {
        size_t size = packet->size;
        uint8_t* data = packet->data;

        uint8_t packet_id = *data++; size--;
        switch (packet_id) {
            // client settings
            case 0x05: {
                // skip the locale
                PCHECK(size > 1);
                uint8_t locale_len = *data++; size--;
                PCHECK((locale_len & VARINT_CONTINUE_BIT) == 0);
                PCHECK(size > locale_len);
                data += locale_len;
                size -= locale_len;

                // parse the rest, making sure varints are the size we expect them
                PCHECK(size == 5);
                uint8_t view_distance = *data++; size--;
                uint8_t chat_mode = *data++; size--; PCHECK((chat_mode & VARINT_CONTINUE_BIT) == 0);
                data++; size--; // chat colors
                data++; size--; // displayed skin parts
                uint8_t main_hand = *data++; size--; PCHECK((main_hand & VARINT_CONTINUE_BIT) == 0);

                // put in a safe range
                view_distance = MIN(view_distance, MAX_VIEW_DISTANCE);

                // TODO: save more settings
                ecs_set(g_ecs, packet->entity, player_view_distance_t, { .view_distance = view_distance });
            } break;

            default:
                TRACE("Entity %llu sent %d bytes of packet %d", packet->entity & ECS_ENTITY_MASK, packet->size, packet_id);
                TRACE_HEX(packet->data, packet->size);
                break;
        }

    next_packet:
        if (err == ERROR_PROTOCOL_VIOLATION) {
            // if we got a protocol violation queue player for disconnect
            // and continue onwards
            const player_t* player = ecs_get(g_ecs, packet->entity, player_t);
            disconnect_connection(player->connection);
        }
        CHECK_AND_RETHROW(err);
    }

cleanup:
    return err;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Main tick event loop
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * This runs at the end of the tick and makes sure that everything is fully finalized
 */
static void finalize_tick(ecs_iter_t* it) {
    // reset the arena, since we no longer need it
    arena_reset(&get_current_queues()->arena);

    // now submit everything
    backend_sender_submit();
}

/**
 * Whenever the view distance is set we are going to re-calculate which chunks the player needs
 * and register on them
 */
static void player_view_distance_set(ecs_iter_t* it) {
    player_view_distance_t* distance = ecs_field(it, player_view_distance_t, 1);
    entity_position_t* position = ecs_field(it, entity_position_t, 2);

    for (int i = 0; i < it->count; i++) {
        chunk_position_t pos = (chunk_position_t){
            .x = (int)position[i].x >> 4,
                .z = (int)position[i].z >> 4
        };
        int64_t radius = distance[i].view_distance / 2;

        for (int64_t z = -radius; z <= radius; z++) {
            for (int64_t x = -radius; x <= radius; x++) {
                chunk_position_t p = (chunk_position_t){ .x = pos.x + x, .z = pos.z + z };
                ecs_entity_t chunk = get_ecs_chunk(p);
                ecs_add_pair(g_ecs, it->entities[i], entity_chunk, chunk);
            }
        }
    }
}

static void set_systems() {
    ECS_OBSERVER(g_ecs, player_view_distance_set, EcsOnAdd, player_view_distance_t, [in] entity_position_t);
}

err_t backend_start() {
    err_t err = NO_ERROR;

    // reset the queues

    arena_init(&m_global_queues[0].arena);
    m_global_queues[0].events = INIT_LIST(m_global_queues[0].events);
    m_global_queues[0].packes = INIT_LIST(m_global_queues[0].packes);

    arena_init(&m_global_queues[1].arena);
    m_global_queues[1].events = INIT_LIST(m_global_queues[1].events);
    m_global_queues[1].packes = INIT_LIST(m_global_queues[1].packes);

    // initialize the ecs
    init_world_ecs();

    // set the amount of thread to be the max amount and leave at least one for anything
    // else so stuff like frontend, backend and so on can still find somewhere to run on
    // while the tick is happening
    int32_t thread_count = get_nprocs() - 1;
    if (thread_count <= 0) {
        thread_count = 1;
    }
    TRACE("Configuring to run %d threads for ECS", thread_count);
    ecs_set_threads(g_ecs, thread_count);

    // set to 20tps
    int32_t target_tps = 20;
    TRACE("Configuring to run at %d ticks per second", target_tps);
    ecs_set_target_fps(g_ecs, (float)target_tps);

    // Start the monitoring for easy monitoring
    TRACE("flecs server started");
    ecs_singleton_set(g_ecs, EcsRest, {0});
    ECS_IMPORT(g_ecs, FlecsMonitor);

    // finalize the tick at this moment
    // TODO: way to force it is actually running at the very very very end
    ECS_SYSTEM(g_ecs, finalize_tick, EcsOnStore, 0);

    // setup all the systems
    set_systems();

    // setup the world
    CHECK_AND_RETHROW(init_world());

    while (1) {
        // switch the queues
        global_queues_switch();

        // handle global events that come from the frontend
        CHECK_AND_RETHROW(handle_global_events());

        // handle the packets we got from the player
        CHECK_AND_RETHROW(handle_packets());

        // Process the world itself
        ecs_progress(g_ecs, 0.0f);
    }

cleanup:
    return err;
}
