#pragma once

#include "util/except.h"

#include "connection.h"

/**
 * The full config for the server.
 *
 * The most important thing this contains is the configurations for how the io-uring should be handled,
 * which changes how much potential memory it is going to use, and of course it may use less
 */
typedef struct frontend_config {
    // the port we are going to use for the server (native endian)
    uint16_t port;

    // the address we are going to use for the server (native endian)
    uint32_t address;

    // The max amount of players that can log in
    int32_t max_player_count;

    // the backlog size, also indicates how many pre-auth connections are allowed
    size_t backlog;
} frontend_config_t;

/**
 * Start the frontend server, this recvs packets from the minecraft protocol, does basic parsing
 * and passes them to the backend.
 */
err_t frontend_start(frontend_config_t* config);

/**
 * Send data through the frontend server, must be called from the same thread as the frontend server.
 * Packet must be allocated with buffer_pool_rent and it will be returned back
 */
err_t frontend_send(connection_t* connection, uint8_t* buffer, int32_t size);

/**
 * Request the frontend to accept again, should be called whenever a connection
 * not in play mode is disconnected
 */
err_t frontend_accept();
