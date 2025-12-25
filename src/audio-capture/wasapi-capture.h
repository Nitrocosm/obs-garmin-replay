#ifndef WASAPI_CAPTURE_H
#define WASAPI_CAPTURE_H

#include <stdbool.h>

// Opaque handle to WASAPI capture instance
typedef struct wasapi_capture wasapi_capture_t;

// Create a new WASAPI capture instance
// device_id: Device ID string, or NULL/empty for default microphone
// Returns: Capture instance, or NULL on failure
wasapi_capture_t *wasapi_capture_create(const char *device_id);

// Start audio capture
// Returns: true on success
bool wasapi_capture_start(wasapi_capture_t *capture);

// Read audio samples from the capture buffer
// buffer: Output buffer for 16-bit signed samples
// max_samples: Maximum number of samples to read
// Returns: Number of samples read, 0 if no data, -1 on error
int wasapi_capture_read(wasapi_capture_t *capture, short *buffer, int max_samples);

// Stop audio capture
void wasapi_capture_stop(wasapi_capture_t *capture);

// Destroy the capture instance and free resources
void wasapi_capture_destroy(wasapi_capture_t *capture);

// Get the sample rate of the capture
int wasapi_capture_get_sample_rate(wasapi_capture_t *capture);

#endif // WASAPI_CAPTURE_H
