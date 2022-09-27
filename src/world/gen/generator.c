#include "generator.h"

#include <backend/backend.h>
#include <util/except.h>
#include <util/mpscq.h>

#include <C-Thread-Pool/thpool.h>

#include <sys/sysinfo.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * The scheduler itself
 */
static threadpool m_thread_pool = NULL;

err_t init_generator() {
    err_t err = NO_ERROR;

    int thread_num = get_nprocs();
    m_thread_pool = thpool_init(thread_num);
    TRACE("started world gen, using %d threads", thread_num);

cleanup:
    return err;
}

typedef struct chunk_gen_task {
    // must be first so we can pass it to the backend
    // for polling and freeing
    chunk_generated_t event;

    // data we need for the task
    chunk_position_t position;
    world_t* world;
} chunk_gen_task_t;

static void do_chunk_gen(void* arg) {
    chunk_gen_task_t* gen = arg;

    // do the generation
    clock_t start = clock();
    chunk_t* chunk = gen->world->generate(gen->world->seed, gen->position);
    clock_t end = clock();
    float time = (float)(end - start) / (float)CLOCKS_PER_SEC;
    if (time >= 0.50) {
        TRACE("took %f seconds to generate chunk at %d.%d",
              (end - start) / (float)CLOCKS_PER_SEC,
              gen->position.x, gen->position.z);
    }

    // add it to the world
    chunk->position = gen->position;
    world_set_chunk(&g_overworld, chunk);

    // create the new event
    gen->event.chunk = chunk;

    // try to add it
    while (!mpscq_enqueue(&m_generated_chunk_queue, gen)) {
        while (mpscq_capacity(&m_generated_chunk_queue) - mpscq_count(&m_generated_chunk_queue) == 0) {
            sched_yield();
        }
    }
}

void generator_queue(world_t* world, chunk_position_t position, ecs_entity_t entity) {
    chunk_gen_task_t* task = malloc(sizeof(chunk_gen_task_t));
    task->world = world;
    task->position = position;
    task->event.chunk_entity = entity;

    thpool_add_work(m_thread_pool, do_chunk_gen, task);
}
