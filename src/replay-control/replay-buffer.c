#include "replay-buffer.h"
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <windows.h>

bool replay_buffer_save(void)
{
    // Check if replay buffer is active
    if (!obs_frontend_replay_buffer_active()) {
        blog(LOG_WARNING, "[Garmin Replay] Replay buffer is not active");
        return false;
    }

    blog(LOG_INFO, "[Garmin Replay] Saving replay buffer...");

    // Save the replay buffer
    // This is an asynchronous operation - the save happens in the background
    obs_frontend_replay_buffer_save();

    blog(LOG_INFO, "[Garmin Replay] Replay buffer save initiated");
    return true;
}

bool replay_buffer_save_and_restart(void)
{
    // Check if replay buffer is active
    if (!obs_frontend_replay_buffer_active()) {
        blog(LOG_WARNING, "[Garmin Replay] Replay buffer is not active");
        return false;
    }

    blog(LOG_INFO, "[Garmin Replay] Saving replay buffer...");

    // Save current buffer
    obs_frontend_replay_buffer_save();

    // Wait for save to complete (give it up to 10 seconds for large buffers)
    blog(LOG_INFO, "[Garmin Replay] Waiting for save to complete...");
    Sleep(3000);  // 3 second delay to let save complete

    blog(LOG_INFO, "[Garmin Replay] Stopping replay buffer...");

    // Stop the replay buffer
    obs_frontend_replay_buffer_stop();

    // Wait for the buffer to actually stop (up to 5 seconds)
    int wait_count = 0;
    const int max_wait = 50;  // 50 * 100ms = 5 seconds
    while (obs_frontend_replay_buffer_active() && wait_count < max_wait) {
        Sleep(100);
        wait_count++;
    }

    if (wait_count >= max_wait) {
        blog(LOG_WARNING, "[Garmin Replay] Timeout waiting for replay buffer to stop");
    } else {
        blog(LOG_INFO, "[Garmin Replay] Replay buffer stopped after %d ms", wait_count * 100);
    }

    // Small additional delay to ensure clean state
    Sleep(500);

    // Restart the buffer
    blog(LOG_INFO, "[Garmin Replay] Starting replay buffer...");
    obs_frontend_replay_buffer_start();

    // Wait for it to actually start
    wait_count = 0;
    while (!obs_frontend_replay_buffer_active() && wait_count < max_wait) {
        Sleep(100);
        wait_count++;
    }

    if (obs_frontend_replay_buffer_active()) {
        blog(LOG_INFO, "[Garmin Replay] Replay buffer restarted successfully");
    } else {
        blog(LOG_WARNING, "[Garmin Replay] Failed to restart replay buffer");
    }

    return true;
}

bool replay_buffer_is_active(void)
{
    return obs_frontend_replay_buffer_active();
}
