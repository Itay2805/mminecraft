#include <sys/sysinfo.h>
#include <bits/stdint-intn.h>
#include <stdlib.h>
#include "generator.h"
#include "util/except.h"
#include "util/sched.h"
#include "util/mpscq.h"
#include "backend/backend.h"

/**
 * The scheduler itself
 */
static struct scheduler m_sched;

/**
 * memory needed for the scheduler
 */
static void* m_sched_memory;

err_t init_generator() {
    err_t err = NO_ERROR;

    // start up the scheduler
    sched_size size = 0;
    scheduler_init(&m_sched, &size, SCHED_DEFAULT, NULL);
    m_sched_memory = calloc(size, 1);
    CHECK_ERRNO(m_sched_memory != NULL);

    scheduler_start(&m_sched, m_sched_memory);
    TRACE("started world gen, using %d threads", m_sched.threads_num);

cleanup:
    return err;
}

typedef struct chunk_gen_task {
    // must be first so we can pass it to the backend
    // for polling and freeing
    struct sched_task task;
    world_t* world;
    ecs_entity_t entity;
    chunk_position_t position;
} chunk_gen_task_t;

static void do_chunk_gen(void* arg, struct scheduler* s, struct sched_task_partition p, sched_uint thread_num) {
    chunk_gen_task_t* gen = arg;

    // do the generation
    clock_t start = clock();
    chunk_t* chunk = gen->world->generate(gen->world->seed, gen->position);
    clock_t end = clock();
    TRACE("took %f seconds to generate chunk at %d.%d",
          (end - start) / (float)CLOCKS_PER_SEC,
          gen->position.x, gen->position.z);

    // add it to the world
    chunk->position = gen->position;
    world_set_chunk(&g_overworld, chunk);

    // create the new event
    chunk_generated_t* event = malloc(sizeof(chunk_generated_t));
    event->task = &gen->task;
    event->chunk = chunk;
    event->chunk_entity = gen->entity;

    // try to add it
    while (!mpscq_enqueue(&m_generated_chunk_queue, event));
}

void generator_queue(world_t* world, chunk_position_t position, ecs_entity_t entity) {
    chunk_gen_task_t* task = malloc(sizeof(chunk_gen_task_t));
    task->world = world;
    task->position = position;
    task->entity = entity;

    scheduler_add(&m_sched, &task->task, do_chunk_gen, task, 1, 1);
}
