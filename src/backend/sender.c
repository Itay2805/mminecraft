#include <liburing.h>
#include <pthread.h>
#include <malloc.h>
#include "sender.h"
#include "net/protocol.h"

static struct io_uring m_ring = { 0 };

typedef struct send_request_info {
    // the first was finished, waiting for the next one now
    bool got_first;

    // the buffer related to this request, so we can return it
    uint8_t* buffer;

    // space to use for encoding the packet sizes, both compressed and uncompressed
    uint8_t encoded_packet_size[10];
} send_request_info_t;


err_t backend_sender_start() {
    err_t err = NO_ERROR;

    TRACE("backend sender started");

    // create the ring
    struct io_uring_params params = { 0 };
    CHECK_ERRNO(io_uring_queue_init_params(2048, &m_ring, &params) == 0);

    while (1) {
        // submit and wait, we want to wake up on anything
        io_uring_submit_and_wait(&m_ring, 1);

        struct io_uring_cqe* cqe = NULL;
        unsigned head = 0;
        int count = 0;

        // iterate the cqe
        io_uring_for_each_cqe(&m_ring, head, cqe) {
            count++;

            // TODO: do we want any kind of handling for errors?

            // clean it up properly
            send_request_info_t* info = (send_request_info_t*)cqe->user_data;
            if (info->got_first) {
                free(info->buffer);
                free(info);
            } else {
                info->got_first = true;
            }
        }

        io_uring_cq_advance(&m_ring, count);
    }

cleanup:
    return err;
}

/**
 * To sync access to the ring
 */
static pthread_mutex_t m_sqe_ring_mutex = PTHREAD_MUTEX_INITIALIZER;

void backend_sender_submit() {
    pthread_mutex_lock(&m_sqe_ring_mutex);
    io_uring_submit(&m_ring);
    pthread_mutex_unlock(&m_sqe_ring_mutex);
}

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

err_t backend_sender_send(int fd, uint8_t* buffer, int32_t size) {
    err_t err = NO_ERROR;

    // set a place for this
    size_t needed_size = varint_size(size);
    CHECK(needed_size >= 1);

    // get a new request
    send_request_info_t* request = malloc(sizeof(send_request_info_t));
    CHECK_ERRNO(request != NULL);
    memset(request, 0, sizeof(*request));

    // prepare the packet sizes and the data if needed
    uint8_t* ptr = request->encoded_packet_size;

    // TODO: compress the packet in here, probably using another buffer

    // write the size in here
    ptr += varint_write(size, ptr);

    pthread_mutex_lock(&m_sqe_ring_mutex);

    // get a request structure and write the varint to it
    struct io_uring_sqe* sqe = GET_SQE;
    io_uring_prep_send(sqe, fd, request->encoded_packet_size, ptr - request->encoded_packet_size, 0);
    io_uring_sqe_set_flags(sqe, IOSQE_IO_LINK);
    sqe->user_data = (uint64_t)request;

    // now that we sent the size, send the other thing
    sqe = GET_SQE;
    io_uring_prep_send(sqe, fd, buffer, size, 0);
    sqe->user_data = (uint64_t)request;

    pthread_mutex_unlock(&m_sqe_ring_mutex);

cleanup:
    return err;
}
