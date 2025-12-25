#ifndef REPLAY_BUFFER_H
#define REPLAY_BUFFER_H

#include <stdbool.h>

// Save the current replay buffer
// Returns: true if save was initiated successfully
bool replay_buffer_save(void);

// Save the replay buffer and restart it
// Returns: true on success
bool replay_buffer_save_and_restart(void);

// Check if replay buffer is currently active
bool replay_buffer_is_active(void);

#endif // REPLAY_BUFFER_H
