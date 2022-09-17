#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "util/except.h"
#include "flecs/flecs.h"

typedef enum connection_recv_state {
    // for deconding the varint, which is a max of 3 bytes
    CONN_DECODE_PACKET_SIZE_1,
    CONN_DECODE_PACKET_SIZE_2,
    CONN_DECODE_PACKET_SIZE_3,

    // for compressed packets, go to here to get the uncompressed size
    CONN_DECODE_DATA_SIZE_1,
    CONN_DECODE_DATA_SIZE_2,
    CONN_DECODE_DATA_SIZE_3,

    // go to here to wait for the full packet
    CONN_GOT_FULL_PACKET,
} connection_recv_state_t;

typedef enum protocol_state {
    PROTOCOL_STATE_HANDSHAKING,
    PROTOCOL_STATE_STATUS,
    PROTOCOL_STATE_LOGIN,
    PROTOCOL_STATE_PLAY,
} protocol_state_t;

typedef struct connection {
    // the ref count on this connection struct
    int ref_count;

    // the fd of the connection
    int fd;

    // the entity related to this connection
    ecs_entity_t entity;

    // the protocol state
    protocol_state_t state;

    // don't queue anything new for the client since
    // we are working on disconnecting it
    bool disconnect;

    // the buffer itself
    int buffer_size;
    uint8_t* buffer;

    // the ring buffer
    int32_t length;
    int32_t offset;

    // true if the packets should be compressed
    bool compressed;

    // the current packet size
    connection_recv_state_t recv_state;

    // the size of the raw packet
    int packet_size;

    // the size of the uncompressed data
    int data_size;
} connection_t;

int get_player_connections_count();

int get_total_connections_count();

/**
 * Create a new connection
 */
connection_t* create_connection();

/**
 * Increase the ref count
 */
connection_t* put_connection(connection_t* connection);

/**
 * Decrease ref count and free if needed
 */
void release_connection(connection_t* connection);

/**
 * Disconnect from the given connection
 */
void disconnect_connection(connection_t* connection);

/**
 * Called whenever the conn got data to process
 */
err_t connection_on_recv(connection_t* conn);
