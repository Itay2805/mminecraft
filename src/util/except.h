#pragma once

#include <stdio.h>
#include <errno.h>
#include <string.h>

typedef enum err {
    // no error
    NO_ERROR,

    // a generic failure
    ERROR_CHECK_FAILED,

    // the protocol has been violated, the client
    // should be logged out
    ERROR_PROTOCOL_VIOLATION,

    // out of memory when allocating in our own code
    ERROR_OUT_OF_MEMORY,

    // below this are errors set by errno
    ERROR_ERRNO_BASE,
} err_t;

#define STATIC_ASSERT(x) _Static_assert(x, #x);

#define IS_ERROR(err) ((err) != NO_ERROR)

#define TRACE(fmt, ...) printf("[*] " fmt "\n", ## __VA_ARGS__)
#define WARN(fmt, ...)  printf("[!] " fmt "\n", ## __VA_ARGS__)
#define ERROR(fmt, ...) printf("[-] " fmt "\n", ## __VA_ARGS__)
#define DEBUG(fmt, ...) printf("[?] " fmt "\n", ## __VA_ARGS__)

static inline const char* err2str(err_t err) {
    switch (err) {
        case NO_ERROR: return "No error";
        case ERROR_CHECK_FAILED: return "Check failed";
        default: return strerror((int)err - ERROR_ERRNO_BASE);
    }
}

#define CHECK_ERROR_LABEL(expr, error, label, ...) \
    do { \
        if (!(expr)) { \
            err = error; \
            ERROR("Check `%s` failed with error `%s` at %s (%s:%d)", #expr, err2str(err), __FUNCTION__, __FILE__, __LINE__); \
            goto label; \
        } \
    } while (0)

#define CHECK_ERROR(expr, error, ...) CHECK_ERROR_LABEL(expr, error, cleanup, ## __VA_ARGS__)
#define CHECK_LABEL(expr, label, ...) CHECK_ERROR_LABEL(expr, ERROR_CHECK_FAILED, label, ## __VA_ARGS__)
#define CHECK(expr, ...) CHECK_ERROR_LABEL(expr, ERROR_CHECK_FAILED, cleanup, ## __VA_ARGS__)

#define CHECK_ERRNO(expr, ...) CHECK_ERROR_LABEL(expr, ERROR_ERRNO_BASE + errno, cleanup, ## __VA_ARGS__)

#define CHECK_AND_RETHROW_LABEL(error, label) \
    do { \
        err = error; \
        if (IS_ERROR(err)) { \
            ERROR("\trethrown at %s (%s:%d)", __FUNCTION__, __FILE__, __LINE__); \
            goto label; \
        } \
    } while (0)

#define CHECK_AND_RETHROW(error) CHECK_AND_RETHROW_LABEL(error, cleanup)

#define CHECK_FAIL_ERROR(error, ...) CHECK_ERROR(0, error, ## __VA_ARGS__)
#define CHECK_FAIL(...) CHECK(0, ## __VA_ARGS__)

#define SAFE_CLOSE(sock) \
    do { \
        if (sock >= 0) { \
            close(sock); \
            sock = -1; \
        } \
    } while (0)

#define SAFE_FREE(ptr) \
    do { \
        if (ptr != NULL) { \
            free(ptr); \
            ptr = NULL; \
        } \
    } while (0)
