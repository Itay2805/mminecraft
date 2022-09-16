
#include <stdlib.h>
#include <netinet/in.h>
#include <pthread.h>
#include "util/except.h"
#include "net/frontend.h"
#include "backend/backend.h"

static pthread_t m_frontend_thread;
static pthread_t m_backend_thread;

// TODO: synchronize or something

int main() {
    err_t err = NO_ERROR;

    // start the backend
    CHECK_ERRNO(pthread_create(&m_backend_thread, NULL, (void*(*)(void*)) backend_start, NULL) == 0);

    // TODO: there could be a tiny race between the backend start and the frontend start

    // start a frontend with default config
    static frontend_config_t config = {
        .port = 25565,
        .address = INADDR_ANY,
        .backlog = 512,
        .max_player_count = 20,
    };
    CHECK_ERRNO(pthread_create(&m_frontend_thread, NULL, (void*(*)(void*))frontend_start, &config) == 0);

    pthread_join(m_frontend_thread, NULL);
    pthread_join(m_backend_thread, NULL);

cleanup:
    return IS_ERROR(err) ? EXIT_FAILURE : EXIT_SUCCESS;
}
