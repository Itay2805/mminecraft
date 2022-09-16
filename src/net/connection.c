#include "connection.h"
#include "protocol.h"
#include "frontend.h"

#include <json-c/json.h>

#include <sys/socket.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

// TODO: autogenerate
#define PROTOCOL_VERSION (754)

static uint8_t recv_buffer_take_byte(connection_t* connection) {
    connection->length--;
    return connection->buffer[connection->offset++];
}

#define PCHECK(expr, ...) CHECK_ERROR(expr, ERROR_PROTOCOL_VIOLATION, ## __VA_ARGS__)

/**
 * The amount of players currently connected
 */
static int m_player_connections_count = 0;

/**
 * The amount of connections in total
 */
static int m_total_connections_count = 0;

int get_player_connections_count() {
    return m_player_connections_count;
}

int get_total_connections_count() {
    return m_total_connections_count;
}

/**
 * Packet dispatching logic.
 *
 * Handshake/Status/Login stages    -> frontend handling
 * Play                             -> backend handling
 *
 * Yes, this function is very ugly, but for the sake of having it being a fast-path for packets that I don't
 * want to handle in a nicer way, we are going to do it like so, they are a very limited set of packets that
 * need anything like that, so I really don't care.
 */
static err_t dispatch_packet(connection_t* connection, const uint8_t* buffer, size_t size) {
    err_t err = NO_ERROR;
    void* buffer_start = (void*)buffer;
    json_object* root = NULL;

    // all packets in game have an id of less than one, so make sure the id
    // is always one and take it (essentially ignoring the fact its a varint)
    uint8_t packet_id = buffer[0];
    PCHECK((packet_id & 0x80) == 0);

    switch (connection->state) {
        // quick handling, move it right away
        case PROTOCOL_STATE_HANDSHAKING: {
            PCHECK(packet_id == 0x00);

            // make sure the version is always fitting into a 2 byte varint
            buffer++; size--;
            int32_t protocol_version = DECODE_VARINT(buffer, size);

            // skip the server address string and port :shrug:
            // the size check also accounts the varint which can only be one byte
            uint8_t server_address_length = DECODE_VARINT(buffer, size);
            PCHECK(size == server_address_length + sizeof(uint16_t) + 1);
            buffer += server_address_length + sizeof(uint16_t); size -= server_address_length + sizeof(uint16_t);

            // set the next state
            uint8_t next_state = *buffer;
            PCHECK(next_state == PROTOCOL_STATE_STATUS || next_state == PROTOCOL_STATE_LOGIN);
            connection->state = next_state;

            // if we are going to login state then check that the version is actually valid, if not
            // then just send the disconnect packet and return
            if (next_state == PROTOCOL_STATE_LOGIN && protocol_version != PROTOCOL_VERSION) {
                // TODO: queue for disconnect and return
            }

            // we no longer need the buffer
            SAFE_FREE(buffer_start);
        } break;

        // for status we are going to handle this right in here as well, because of
        // how simple it is
        case PROTOCOL_STATE_STATUS: {
            buffer++; size--;
            switch (packet_id) {
                // request
                case 0x00: {
                    PCHECK(size == 0);

                    //
                    // create the root object
                    //
                    root = json_object_new_object();
                    CHECK_ERRNO(root != NULL);

                    // create the version field
                    json_object* version = json_object_new_object();
                    json_object_object_add(version, "name", json_object_new_string("1.16.5"));
                    json_object_object_add(version, "protocol", json_object_new_int(PROTOCOL_VERSION));
                    json_object_object_add(root, "version", version);

                    json_object* players = json_object_new_object();
                    json_object_object_add(players, "max", json_object_new_int(frontend_get_max_player_count()));
                    json_object_object_add(players, "online", json_object_new_int(m_player_connections_count));
                    json_object_object_add(root, "players", players);

                    json_object* description = json_object_new_object();
                    json_object_object_add(description, "text", json_object_new_string("hello world!"));
                    json_object_object_add(root, "description", description);

                    //
                    // get the final json
                    //
                    size_t json_len = 0;
                    const char* str = json_object_to_json_string_length(root, 0, &json_len);
                    CHECK_ERRNO(str != NULL);

                    // allocate it
                    size_t rlen = json_len + varint_size((int32_t)json_len) + 1;
                    void* rbuffer = malloc(rlen);
                    uint8_t* rbuffer_ptr = rbuffer;

                    // set the response id
                    *rbuffer_ptr++ = 0x00;

                    // write the length and string itself
                    rbuffer_ptr += varint_write(json_len, rbuffer_ptr);
                    memcpy(rbuffer_ptr, str, json_len);

                    // send it
                    CHECK_AND_RETHROW(frontend_send(connection, rbuffer, rlen));

                    // we no longer need the buffer
                    SAFE_FREE(buffer_start);
                } break;

                // ping
                case 0x01: {
                    PCHECK(size == sizeof(int64_t));
                    int64_t payload = (int64_t)__builtin_bswap64(*(int64_t*)buffer);

                    // calculate the size we need
                    int32_t rsize = varint_size(0x01);
                    PCHECK(rsize >= 1);
                    rsize += sizeof(int64_t);
                    void* rbuffer = malloc(rsize);
                    uint8_t* rbuffer_ptr = rbuffer;

                    // construct the packet
                    *rbuffer_ptr++ = 0x01;
                    *(int64_t*)rbuffer_ptr = payload;

                    // send it
                    CHECK_AND_RETHROW(frontend_send(connection, rbuffer, rsize));

                    // we no longer need the buffer
                    SAFE_FREE(buffer_start);
                } break;

                // assume invalid packet
                default: PCHECK(false);
            }
        } break;

        // for login we are going to move the code into another place since it does a
        // bit more heavy stuff, but it is handled by the server as well
        case PROTOCOL_STATE_LOGIN: {
            // TODO: if we got to play state then switch the buffer
            CHECK_FAIL();

            // we no longer need the buffer
            SAFE_FREE(buffer_start);
        } break;

        case PROTOCOL_STATE_PLAY: {
            // TODO: the buffer is not owned by this
            CHECK_FAIL();
        } break;
    }

cleanup:
    // if we got an error always free this buffer
    if (!IS_ERROR(err)) {
        SAFE_FREE(buffer_start);
    }

    // free the root object if we have any
    if (root != NULL) {
        json_object_put(root);
    }

    return err;
}

connection_t* create_connection() {
    connection_t* connection = malloc(sizeof(connection_t));
    if (connection == NULL) {
        return NULL;
    }

    m_total_connections_count++;

    // setup the connection
    memset(connection, 0, sizeof(*connection));
    connection->ref_count = 1;

    return connection;
}

connection_t* put_connection(connection_t* connection) {
    connection->ref_count++;
    return connection;
}

static void do_disconnect(connection_t* connection) {
    if (connection->fd >= 0) {
        shutdown(connection->fd, SHUT_RDWR);
        close(connection->fd);
        connection->fd = -1;
    }

}

void release_connection(connection_t* connection) {
    if (--connection->ref_count == 0) {
        // disconnect
        do_disconnect(connection);

        // free everything
        free(connection->buffer);
        free(connection);

        // we no longer have that count
        m_total_connections_count--;

        // tell frontend it can accept again
        frontend_accept();
    }
}

void disconnect_connection(connection_t* connection) {
    // close the connection
    do_disconnect(connection);

    // release the reference
    release_connection(connection);
}

err_t connection_on_recv(connection_t* conn) {
    err_t err = NO_ERROR;

    // for compressed connections we need to go to the data size afterwards
    connection_recv_state_t after_varint = conn->compressed ? CONN_DECODE_DATA_SIZE_1 : CONN_GOT_FULL_PACKET;

    while (conn->length > 0) {
        switch (conn->recv_state) {
            //----------------------------------------------------------------------------------------------------------
            // Decode the packet size
            //----------------------------------------------------------------------------------------------------------

            // get the first byte, we are not going to check the packet size since we will always have
            // more data available for such small packets
            case CONN_DECODE_PACKET_SIZE_1: {
                uint8_t b = recv_buffer_take_byte(conn);
                conn->packet_size = b & VARINT_SEGMENT_BITS;
                conn->recv_state = (b & VARINT_CONTINUE_BIT) ? CONN_DECODE_PACKET_SIZE_2 : after_varint;
            } break;

            // get the second byte, we nede to check the packet size in here since we might have buffers
            // that are small enough already
            case CONN_DECODE_PACKET_SIZE_2: {
                uint8_t b = recv_buffer_take_byte(conn);
                conn->packet_size |= (b & VARINT_SEGMENT_BITS) << 7;
                PCHECK(conn->packet_size <= conn->buffer_size);
                conn->recv_state = (b & VARINT_CONTINUE_BIT) ? CONN_DECODE_PACKET_SIZE_3 : after_varint;
            } break;

            // get the third byte, gotta check the size
            case CONN_DECODE_PACKET_SIZE_3: {
                uint8_t b = recv_buffer_take_byte(conn);
                conn->packet_size |= (b & VARINT_SEGMENT_BITS) << 14;
                PCHECK(conn->packet_size <= conn->buffer_size);
                PCHECK((b & VARINT_CONTINUE_BIT) == 0);
                conn->recv_state = after_varint;
            } break;

            //----------------------------------------------------------------------------------------------------------
            // Decode the data size (if compressed)
            //----------------------------------------------------------------------------------------------------------

            // get the first byte, we are not going to check the packet size since we will always have
            // more data available for such small packets
            case CONN_DECODE_DATA_SIZE_1: {
                uint8_t b = recv_buffer_take_byte(conn);
                conn->data_size = b & VARINT_SEGMENT_BITS;
                conn->recv_state = (b & VARINT_CONTINUE_BIT) ? CONN_DECODE_DATA_SIZE_2 : CONN_GOT_FULL_PACKET;
            } break;

            // get the second byte, we nede to check the packet size in here since we might have buffers
            // that are small enough already
            case CONN_DECODE_DATA_SIZE_2: {
                uint8_t b = recv_buffer_take_byte(conn);
                conn->data_size |= (b & VARINT_SEGMENT_BITS) << 7;
                PCHECK(conn->data_size <= conn->buffer_size);
                conn->recv_state = (b & VARINT_CONTINUE_BIT) ? CONN_DECODE_DATA_SIZE_3 : CONN_GOT_FULL_PACKET;
            } break;

            // get the third byte, gotta check the size
            case CONN_DECODE_DATA_SIZE_3: {
                uint8_t b = recv_buffer_take_byte(conn);
                conn->data_size |= (b & VARINT_SEGMENT_BITS) << 14;
                PCHECK(conn->data_size <= conn->buffer_size);
                PCHECK((b & VARINT_CONTINUE_BIT) == 0);
                conn->recv_state = CONN_GOT_FULL_PACKET;
            } break;

            //----------------------------------------------------------------------------------------------------------
            // Process the full packet
            //----------------------------------------------------------------------------------------------------------

            // got the full thing, take the data we can take
            case CONN_GOT_FULL_PACKET: {
                // make sure we don't want more than we allow for the client to allocate

                // packet must be at least one byte which is the packet id
                PCHECK(conn->packet_size >= 1);

                // check we have enough data right now
                if (conn->length < conn->packet_size) {
                    // we don't copy to the start, set the offset to zero, and wait for next time
                    memmove(conn->buffer, conn->buffer + conn->offset, conn->length);
                    conn->offset = 0;
                    goto cleanup;
                }

                // we got the data, allocate and decompress if needed
                void* buffer;
                int32_t data_size;
                if (conn->compressed && conn->data_size != 0) {
                    data_size = conn->data_size;
                    // TODO: decompress
                    CHECK_FAIL();
                } else {
                    data_size = conn->packet_size;
                    buffer = malloc(data_size);
                    memcpy(buffer, conn->buffer + conn->offset, data_size);
                    conn->offset += data_size;
                    conn->length -= data_size;
                }

                // submit the packet
                CHECK_AND_RETHROW(dispatch_packet(conn, buffer, data_size));

                // return it to the initial state
                conn->recv_state = CONN_DECODE_PACKET_SIZE_1;
            } break;
        }
    }

    // we are out of data, so reset the offset
    conn->offset = 0;

cleanup:
    return err;
}
