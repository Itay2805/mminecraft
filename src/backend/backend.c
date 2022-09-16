#include <time.h>
#include <pthread.h>
#include <stdlib.h>
#include "backend.h"
#include "world.h"

#include "backend/components/player.h"
#include "backend/components/entity.h"

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

    // reset it
    m_global_queues[m_current_queue].new_players = INIT_LIST(m_global_queues[m_current_queue].new_players);

    global_queues_unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Main tick event loop
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static err_t safe_sleep(long millis) {
    err_t err = NO_ERROR;

    if (millis < 0) {
        WARN("We are lagging!");
        goto cleanup;
    }

    // calculate the initial time to sleep for
    struct timespec req = {
        .tv_sec = millis / 1000,
        .tv_nsec = (millis - (millis / 1000)) * 1000000L
    };

    // sleep and ignore any interrupt by a signal
    while ((nanosleep(&req, &req)) != 0) {
        CHECK_ERRNO(errno == EINTR);
    }

cleanup:
    return err;
}

err_t backend_start() {
    err_t err = NO_ERROR;

    // reset both by doing two switches
    global_queues_switch();
    global_queues_switch();

    // initialize the ecs
    init_world_ecs();

    // for counting tps
    time_t start = time(NULL);
    int tps = 0;

    // Start the monitoring for easy monitoring
    ecs_singleton_set(g_ecs, EcsRest, {0});
    ECS_IMPORT(g_ecs, FlecsMonitor);

    // for monitoring
    long avg_tick_time = 0;
    long min_tick_time = 0;
    long max_tick_time = 0;

    while (1) {
        struct timespec tick_start;
        CHECK_ERRNO(clock_gettime(CLOCK_MONOTONIC, &tick_start) == 0);

        // switch the queues
        global_queues_switch();

        // tick handling
        global_queues_t* q = get_current_queues();

        // start by processing the global events that should be done before all the systems run
        new_player_event_t* new_player;
        while ((new_player = LIST_ENTRY(list_pop(&q->new_players), new_player_event_t, entry)) != NULL) {
            ecs_entity_t entity = ecs_new_id(g_ecs);
            ecs_add(g_ecs, entity, entity_position_t);
            ecs_add(g_ecs, entity, entity_rotation_t);
            ecs_add(g_ecs, entity, entity_velocity_t);

            player_name_t* name = ecs_get_mut(g_ecs, entity, player_name_t);
            STATIC_ASSERT(sizeof(name->name) == sizeof(new_player->name));
            memcpy(name->name, new_player->name, sizeof(name->name));

            entity_id_t* id = ecs_get_mut(g_ecs, entity, entity_id_t);
            uuid_copy(id->uuid, new_player->uuid);

            // TODO: debug only
            ecs_set_name(g_ecs, entity, name->name);

            // delete and move to next entry
            free(new_player);
        }

        // Process the world itself
        ecs_progress(g_ecs, 1.0f);

        // increment the tps for fun and profit
        tps++;

        struct timespec tick_end;
        CHECK_ERRNO(clock_gettime(CLOCK_MONOTONIC, &tick_end) == 0);

        // sleep for the duration we need to sleep
        long tick_duration = (tick_end.tv_sec - tick_start.tv_sec) * 1000 +
                             (tick_end.tv_nsec - tick_start.tv_nsec) / 1000000000;
        CHECK_AND_RETHROW(safe_sleep((1000 / 20) - tick_duration));

        // monitoring
        if (tick_duration > max_tick_time) max_tick_time = tick_duration;
        if (tick_duration < min_tick_time) min_tick_time = tick_duration;
        avg_tick_time = (avg_tick_time + tick_duration) / 2;

        // ticks per second for fun and profit
        if (time(NULL) - start >= 1) {
            TRACE("TPS: %d, Times: min=%ldms, avg=%ldms, max=%ldms", tps,
                  min_tick_time, avg_tick_time, max_tick_time);
            start = time(NULL);
            tps = 0;
        }
    }

cleanup:
    return err;
}
