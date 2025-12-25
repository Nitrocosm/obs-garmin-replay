#ifndef PHRASE_DETECTOR_H
#define PHRASE_DETECTOR_H

// Check if the Vosk result JSON contains a trigger phrase
// vosk_result_json: The JSON result string from Vosk
// sensitivity: Sensitivity level (1-100), higher = stricter matching
// language: Language to check (0=English, 1=German, 2=French)
// Returns: Confidence level (0.0 - 1.0), or 0 if no match
float phrase_detector_check(const char *vosk_result_json, int sensitivity, int language);

#endif // PHRASE_DETECTOR_H
