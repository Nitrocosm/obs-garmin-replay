#include "vosk-engine.h"
#include <vosk_api.h>
#include <obs-module.h>

#include <stdlib.h>
#include <string.h>

#define VOSK_SAMPLE_RATE 16000.0f

struct vosk_engine {
    VoskModel *model;
    VoskRecognizer *recognizer;
    bool initialized;
};

// Grammar JSON to limit vocabulary to trigger phrases
// This improves recognition accuracy by constraining the search space
static const char *TRIGGER_GRAMMAR =
    "["
    "\"garmin save video\", "
    "\"garmin video speichern\", "
    "\"garmin enregistrer video\", "
    "\"garmin\", "
    "\"save video\", "
    "\"video speichern\", "
    "\"enregistrer video\", "
    "\"[unk]\""
    "]";

vosk_engine_t *vosk_engine_create(const char *model_path)
{
    vosk_engine_t *engine = calloc(1, sizeof(vosk_engine_t));
    if (!engine) {
        return NULL;
    }

    // Set Vosk log level (0 = errors only)
    vosk_set_log_level(0);

    blog(LOG_INFO, "[Garmin Replay] Loading Vosk model from: %s", model_path);

    // Load the model
    engine->model = vosk_model_new(model_path);
    if (!engine->model) {
        blog(LOG_ERROR, "[Garmin Replay] Failed to load Vosk model from: %s", model_path);
        blog(LOG_ERROR, "[Garmin Replay] Please download a model from https://alphacephei.com/vosk/models");
        free(engine);
        return NULL;
    }

    // Create recognizer with grammar for better accuracy
    // The grammar limits what the recognizer will output
    engine->recognizer = vosk_recognizer_new_grm(
        engine->model,
        VOSK_SAMPLE_RATE,
        TRIGGER_GRAMMAR);

    if (!engine->recognizer) {
        blog(LOG_WARNING, "[Garmin Replay] Grammar mode not available, using standard recognizer");
        // Fallback to standard recognizer
        engine->recognizer = vosk_recognizer_new(engine->model, VOSK_SAMPLE_RATE);
    }

    if (!engine->recognizer) {
        blog(LOG_ERROR, "[Garmin Replay] Failed to create Vosk recognizer");
        vosk_model_free(engine->model);
        free(engine);
        return NULL;
    }

    // Enable word timestamps for better phrase detection
    vosk_recognizer_set_words(engine->recognizer, 1);

    engine->initialized = true;
    blog(LOG_INFO, "[Garmin Replay] Vosk engine initialized successfully");

    return engine;
}

int vosk_engine_process(vosk_engine_t *engine, const short *samples, int count)
{
    if (!engine || !engine->initialized || !engine->recognizer) {
        return -1;
    }

    // Process audio through Vosk
    // Returns 1 if silence detected (end of utterance), 0 otherwise
    return vosk_recognizer_accept_waveform_s(engine->recognizer, samples, count);
}

const char *vosk_engine_get_result(vosk_engine_t *engine)
{
    if (!engine || !engine->initialized || !engine->recognizer) {
        return NULL;
    }

    return vosk_recognizer_result(engine->recognizer);
}

const char *vosk_engine_get_partial_result(vosk_engine_t *engine)
{
    if (!engine || !engine->initialized || !engine->recognizer) {
        return NULL;
    }

    return vosk_recognizer_partial_result(engine->recognizer);
}

void vosk_engine_reset(vosk_engine_t *engine)
{
    if (engine && engine->recognizer) {
        vosk_recognizer_reset(engine->recognizer);
    }
}

void vosk_engine_destroy(vosk_engine_t *engine)
{
    if (!engine) {
        return;
    }

    if (engine->recognizer) {
        vosk_recognizer_free(engine->recognizer);
    }

    if (engine->model) {
        vosk_model_free(engine->model);
    }

    free(engine);

    blog(LOG_INFO, "[Garmin Replay] Vosk engine destroyed");
}
