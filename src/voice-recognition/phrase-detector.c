#include "phrase-detector.h"
#include <obs-module.h>

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// Trigger phrases - using only the distinctive parts that models recognize
// "Garmin" is a brand name that smaller models often don't recognize
static const char *TRIGGER_PHRASES[] = {
    "save video",         // English (garmin often recognized, but "save video" is key)
    "video speichern",    // German (garmin not in vocabulary)
    "enregistrer video"   // French (garmin may not be recognized)
};
static const int NUM_TRIGGER_PHRASES = 3;

// Minimum string for matching
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// Levenshtein distance for fuzzy string matching
static int levenshtein_distance(const char *s1, const char *s2)
{
    int len1 = (int)strlen(s1);
    int len2 = (int)strlen(s2);

    // Allocate matrix
    int *matrix = malloc((len1 + 1) * (len2 + 1) * sizeof(int));
    if (!matrix) {
        return 1000;  // Large distance on allocation failure
    }

    // Initialize first row and column
    for (int i = 0; i <= len1; i++) {
        matrix[i * (len2 + 1)] = i;
    }
    for (int j = 0; j <= len2; j++) {
        matrix[j] = j;
    }

    // Fill in the matrix
    for (int i = 1; i <= len1; i++) {
        for (int j = 1; j <= len2; j++) {
            int cost = (tolower((unsigned char)s1[i - 1]) ==
                        tolower((unsigned char)s2[j - 1])) ? 0 : 1;

            int del = matrix[(i - 1) * (len2 + 1) + j] + 1;
            int ins = matrix[i * (len2 + 1) + (j - 1)] + 1;
            int sub = matrix[(i - 1) * (len2 + 1) + (j - 1)] + cost;

            int min = del;
            if (ins < min) min = ins;
            if (sub < min) min = sub;

            matrix[i * (len2 + 1) + j] = min;
        }
    }

    int result = matrix[len1 * (len2 + 1) + len2];
    free(matrix);
    return result;
}

// Normalize text: lowercase, remove punctuation, collapse whitespace
static void normalize_text(const char *input, char *output, int max_len)
{
    int j = 0;
    bool last_was_space = false;

    for (int i = 0; input[i] && j < max_len - 1; i++) {
        unsigned char c = (unsigned char)input[i];

        if (isalnum(c)) {
            output[j++] = (char)tolower(c);
            last_was_space = false;
        } else if (isspace(c) || c == ',' || c == '.') {
            if (!last_was_space && j > 0) {
                output[j++] = ' ';
                last_was_space = true;
            }
        }
    }

    // Trim trailing space
    if (j > 0 && output[j - 1] == ' ') {
        j--;
    }

    output[j] = '\0';
}

// Extract text field from Vosk JSON result
// Simple JSON parsing - looks for "text" : "..." pattern
static bool extract_text_from_json(const char *json, char *text, int max_len)
{
    // Look for "text" field
    const char *text_key = strstr(json, "\"text\"");
    if (!text_key) {
        return false;
    }

    // Find the colon
    const char *colon = strchr(text_key + 6, ':');
    if (!colon) {
        return false;
    }

    // Find the opening quote
    const char *start = strchr(colon, '"');
    if (!start) {
        return false;
    }
    start++;  // Move past the quote

    // Find the closing quote
    const char *end = strchr(start, '"');
    if (!end) {
        return false;
    }

    // Copy the text
    int len = (int)(end - start);
    if (len >= max_len) {
        len = max_len - 1;
    }

    strncpy(text, start, len);
    text[len] = '\0';

    return len > 0;
}

// Check if a string contains all words of the trigger phrase
// More lenient than exact match - allows extra words
static bool contains_trigger_words(const char *text, const char *trigger)
{
    char trigger_copy[256];
    strncpy(trigger_copy, trigger, sizeof(trigger_copy) - 1);
    trigger_copy[sizeof(trigger_copy) - 1] = '\0';

    // Tokenize trigger phrase
    char *word = strtok(trigger_copy, " ");
    while (word) {
        // Check if this word appears in the text
        if (!strstr(text, word)) {
            return false;
        }
        word = strtok(NULL, " ");
    }

    return true;
}

float phrase_detector_check(const char *vosk_result_json, int sensitivity, int language)
{
    if (!vosk_result_json || sensitivity < 1 || sensitivity > 100) {
        return 0.0f;
    }

    // Extract text from JSON
    char raw_text[512];
    if (!extract_text_from_json(vosk_result_json, raw_text, sizeof(raw_text))) {
        return 0.0f;
    }

    // Normalize the recognized text
    char normalized[512];
    normalize_text(raw_text, normalized, sizeof(normalized));

    // Skip if too short
    if (strlen(normalized) < 5) {
        return 0.0f;
    }

    // Select the trigger phrase based on language
    // 0 = English, 1 = German, 2 = French
    const char *trigger_phrase;
    if (language >= 0 && language < NUM_TRIGGER_PHRASES) {
        trigger_phrase = TRIGGER_PHRASES[language];
    } else {
        trigger_phrase = TRIGGER_PHRASES[0];  // Default to English
    }

    // Debug: Log what we heard and what we're looking for
    blog(LOG_INFO, "[Garmin Replay] Heard: '%s' | Looking for: '%s' (lang=%d)",
         normalized, trigger_phrase, language);

    // Calculate maximum allowed edit distance based on sensitivity
    // sensitivity 100 = exact match (0 edits)
    // sensitivity 50 = moderate (allow ~15% errors)
    // sensitivity 1 = very loose (allow ~30% errors)
    float max_error_rate = (100.0f - (float)sensitivity) / 100.0f * 0.3f;

    float best_confidence = 0.0f;
    int phrase_len = (int)strlen(trigger_phrase);
    int max_distance = (int)(phrase_len * max_error_rate);

    // Method 1: Exact Levenshtein distance
    int distance = levenshtein_distance(normalized, trigger_phrase);

    if (distance <= max_distance) {
        float confidence = 1.0f - ((float)distance / (float)phrase_len);
        if (confidence > best_confidence) {
            best_confidence = confidence;
            blog(LOG_DEBUG, "[Garmin Replay] Match found: '%s' -> '%s' (dist=%d, conf=%.2f)",
                 normalized, trigger_phrase, distance, confidence);
        }
    }

    // Method 2: Check if all trigger words are present (more lenient)
    if (best_confidence < 0.6f) {
        if (contains_trigger_words(normalized, trigger_phrase)) {
            // Give partial credit for containing all words
            float word_confidence = 0.75f;
            if (word_confidence > best_confidence) {
                best_confidence = word_confidence;
                blog(LOG_DEBUG, "[Garmin Replay] Word match: '%s' contains '%s'",
                     normalized, trigger_phrase);
            }
        }
    }

    if (best_confidence > 0.5f) {
        blog(LOG_INFO, "[Garmin Replay] Trigger phrase detected in: '%s' (confidence: %.2f)",
             normalized, best_confidence);
    }

    return best_confidence;
}
