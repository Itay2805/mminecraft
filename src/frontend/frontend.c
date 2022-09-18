#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <liburing.h>
#include <malloc.h>
#include <unistd.h>
#include <pthread.h>
#include "frontend.h"
#include "protocol.h"

// TODO: whenever we run out of sqe, just submit and wait for one

typedef struct send_request_info {
    // the connection related to this request
    connection_t* connection;

    // the first was finished, waiting for the next one now
    bool got_first;

    // the buffer related to this request, so we can return it
    uint8_t* buffer;

    // space to use for encoding the packet sizes, both compressed and uncompressed
    uint8_t encoded_packet_size[10];
} send_request_info_t;

typedef enum operation_type {
    REQ_ACCEPT  = 0x0001000000000000,
    REQ_RECV    = 0x0002000000000000,
    REQ_SEND    = 0x0003000000000000,
} operation_type_t;

#define REQ_TYPE(x) ((x) & 0xffff000000000000)
#define REQ_DATA(x) ((x) & 0x00007fffffffffff)

#define PREAUTH_BUFFER_SIZE     256
#define POSTAUTH_BUFFER_SIZE    (1024 * 1024 * 2)

/**
 * The main ring
 */
static struct io_uring m_ring = { 0 };

/**
 * The fd of the server
 */
static int m_server_fd = -1;

/**
 * The address using for the accepted client address
 */
static struct sockaddr m_accept_client_addr;

/**
 * The address using for the accepted client address length
 */
static socklen_t m_accept_client_addr_len;

static bool m_accepting = false;

/**
 * Get an SQE, waiting for one to clear up if there
 * is not enough space right now
 */
#define GET_SQE \
    ({ \
        struct io_uring_sqe* __sqe = io_uring_get_sqe(&m_ring); \
        while (__sqe == NULL) { \
            io_uring_submit(&m_ring); \
            CHECK_ERRNO(io_uring_sqring_wait(&m_ring) == 0);    \
            __sqe = io_uring_get_sqe(&m_ring); \
        } \
        __sqe; \
    })

/**
 * Submit another recv action on the given conn
 */
static err_t add_recv(connection_t* conn) {
    err_t err = NO_ERROR;

    // skip this
    if (conn->disconnect) {
        goto cleanup;
    }

    struct io_uring_sqe* sqe = GET_SQE;
    io_uring_prep_recv(sqe, conn->fd, conn->buffer + conn->length, conn->buffer_size - conn->length, 0);
    sqe->user_data = (uint64_t)conn | REQ_RECV;

cleanup:
    return err;
}

static err_t create_server_socket(frontend_config_t* config) {
    err_t err = NO_ERROR;
    const int enable = 1;

    // start the socket
    m_server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    CHECK_ERRNO(m_server_fd >= 0);

    // allow for address reuse
    setsockopt(m_server_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    // bind to start a server
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(config->port),
        .sin_addr.s_addr = htonl(config->address)
    };
    CHECK_ERRNO(bind(m_server_fd, (struct sockaddr*)&addr, sizeof(addr)) == 0);

    // start listening
    CHECK_ERRNO(listen(m_server_fd, config->backlog) == 0);

    TRACE("Server started on %d.%d.%d.%d:%d!",
          config->address & 0xFF, (config->address >> 8) & 0xFF,
          (config->address >> 16) & 0xFF, (config->address >> 24) & 0xFF,
          config->port
    );

cleanup:
    if (IS_ERROR(err)) {
        SAFE_CLOSE(m_server_fd);
    }
    return err;
}

static frontend_config_t* m_config;

int frontend_get_max_player_count() {
    return m_config->max_player_count;
}

err_t frontend_start(frontend_config_t* config) {
    err_t err = NO_ERROR;

    m_config = config;

    // start the server
    CHECK_AND_RETHROW(create_server_socket(config));

    // create the ring
    struct io_uring_params params = { 0 };
    CHECK_ERRNO(io_uring_queue_init_params(2048, &m_ring, &params) == 0);

    // make sure fast poll is supported
    CHECK(params.features & IORING_FEAT_FAST_POLL);

    // add the first accept for the server
    CHECK_AND_RETHROW(frontend_accept());

    while (1) {
        // submit and wait, we want to wake up on anything
        io_uring_submit_and_wait(&m_ring, 1);

        struct io_uring_cqe* cqe = NULL;
        unsigned head = 0;
        int count = 0;

        // iterate the cqe
        io_uring_for_each_cqe(&m_ring, head, cqe) {
            count++;

            // no data means this is an accept
            switch (REQ_TYPE(cqe->user_data)) {
                case REQ_ACCEPT: {
                    int client_fd = cqe->res;
                    CHECK_ERROR(client_fd >= 0, ERROR_ERRNO_BASE + -client_fd);

                    // no longer accepting
                    m_accepting = false;

                    // allocate it
                    connection_t *conn = create_connection();
                    if (conn == NULL) {
                        // we are out of memory, just don't allow the client
                        // to join and close its socket
                        close(client_fd);
                    } else {
                        // set it up
                        conn->fd = client_fd;
                        conn->buffer_size = PREAUTH_BUFFER_SIZE;
                        conn->buffer = malloc(conn->buffer_size);

                        // make sure that we can recv on it now
                        CHECK_AND_RETHROW(add_recv(conn));
                    }

                    // only add more accepts if we have the capacity to hold more connections
                    if ((get_total_connections_count() - get_player_connections_count()) < config->backlog) {
                        CHECK_AND_RETHROW(frontend_accept());
                    }
                } break;

                // on send completion just free the buffer we used for sending
                case REQ_SEND: {
                    send_request_info_t* req = (send_request_info_t*)REQ_DATA(cqe->user_data);
                    if (cqe->res <= 0) {
                        // the send has failed, disconnect the client
                        disconnect_connection(req->connection);
                    }

                    if (req->got_first) {
                        // free the buffer
                        free(req->buffer);

                        // release the connection since we no longer reference it
                        release_connection(req->connection);
                    } else {
                        req->got_first = true;
                    }
                } break;

                // on recv process the data that we got
                case REQ_RECV: {
                    connection_t* conn = (connection_t*)REQ_DATA(cqe->user_data);

                    if (cqe->res > 0) {
                        // set the lenght of the current data
                        conn->length += cqe->res;

                        // process it
                        err_t client_err = connection_on_recv(conn);
                        if (client_err == ERROR_PROTOCOL_VIOLATION) {
                            // got something weird from the client, disconnect the connection
                            disconnect_connection(conn);
                        } else {
                            // make sure no other error happened
                            CHECK_AND_RETHROW(client_err);

                            // add again the recv
                            CHECK_AND_RETHROW(add_recv(conn));
                        }
                    } else {
                        // we got data
                        // we either had an error or a socket got closed
                        // on the other side, either way shutdown and release
                        // the connection, this will notify everything higher
                        // up and close the socket when we are out of references
                        disconnect_connection(conn);
                    }
                } break;
            }
        }

        io_uring_cq_advance(&m_ring, count);
    }

cleanup:
    return err;
}

err_t frontend_send(connection_t* connection, uint8_t* buffer, int32_t size) {
    err_t err = NO_ERROR;

    // set a place for this
    size_t needed_size = varint_size(size);
    CHECK(needed_size >= 1);

    // get a new request
    send_request_info_t* request = malloc(sizeof(send_request_info_t));
    CHECK_ERRNO(request != NULL);
    memset(request, 0, sizeof(*request));

    // we are going to store this
    request->connection = put_connection(connection);

    // prepare the packet sizes and the data if needed
    uint8_t* ptr = request->encoded_packet_size;
    if (connection->compressed) {
        // TODO: compress the packet in here, probably using another buffer
        CHECK_FAIL();
    } else {
        // write the size in here
        ptr += varint_write(size, ptr);
    }

    // get a request structure and write the varint to it
    struct io_uring_sqe* sqe = GET_SQE;
    io_uring_prep_send(sqe, connection->fd, request->encoded_packet_size, ptr - request->encoded_packet_size, 0);
    io_uring_sqe_set_flags(sqe, IOSQE_IO_LINK);
    sqe->user_data = (uint64_t)request | REQ_SEND;

    // now that we sent the size, send the other thing
    sqe = GET_SQE;
    CHECK_ERRNO(sqe != NULL);
    io_uring_prep_send(sqe, connection->fd, buffer, size, 0);
    sqe->user_data = (uint64_t)request | REQ_SEND;

cleanup:
    return err;
}

err_t frontend_accept() {
    err_t err = NO_ERROR;

    if (m_accepting) {
        goto cleanup;
    }

    m_accept_client_addr_len = sizeof(m_accept_client_addr);
    m_accept_client_addr = (struct sockaddr){ 0 };

    struct io_uring_sqe* sqe = GET_SQE;
    io_uring_prep_accept(sqe, m_server_fd, &m_accept_client_addr, &m_accept_client_addr_len, 0);
    sqe->user_data = REQ_ACCEPT;

    m_accepting = true;

cleanup:
    return err;
}
