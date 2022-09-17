#pragma once

#include <stdint.h>
#include "util/except.h"

/**
 * Start the server sender backend
 */
err_t backend_sender_start();

/**
 * Submit everything from the ring, called at the end of a tick
 * to make sure everything done during the tick is sent
 */
void backend_sender_submit();

/**
 * Sends packets from the backend, this is synchronized properly
 */
err_t backend_sender_send(int fd, uint8_t* buffer, int32_t size);
