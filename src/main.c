
#include <stdlib.h>
#include <netinet/in.h>
#include "util/except.h"
#include "net/frontend.h"

int main() {
    err_t err = NO_ERROR;

    // start a server with default config
    frontend_config_t config = {
        .port = 25565,
        .address = INADDR_ANY,
        .backlog = 512,
        .max_player_count = 20,
    };
    CHECK_AND_RETHROW(frontend_start(&config));

cleanup:
    return IS_ERROR(err) ? EXIT_FAILURE : EXIT_SUCCESS;
}
