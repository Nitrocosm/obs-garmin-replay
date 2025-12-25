#ifndef VOSK_ENGINE_H
#define VOSK_ENGINE_H

#include <stdbool.h>

// Opaque handle to Vosk engine instance
typedef struct vosk_engine vosk_engine_t;

// Create a new Vosk engine instance
// model_path: Path to the Vosk model directory
// Returns: Engine instance, or NULL on failure
vosk_engine_t *vosk_engine_create(const char *model_path);

// Process audio samples through the recognizer
// samples: 16-bit signed PCM samples at 16kHz mono
// count: Number of samples
// Returns: 1 if final result available, 0 if partial, -1 on error
int vosk_engine_process(vosk_engine_t *engine, const short *samples, int count);

// Get the final recognition result (JSON string)
// The returned string is valid until the next call to process or get_result
const char *vosk_engine_get_result(vosk_engine_t *engine);

// Get partial recognition result (JSON string)
const char *vosk_engine_get_partial_result(vosk_engine_t *engine);

// Reset the recognizer for the next utterance
void vosk_engine_reset(vosk_engine_t *engine);

// Destroy the engine and free resources
void vosk_engine_destroy(vosk_engine_t *engine);

#endif // VOSK_ENGINE_H
