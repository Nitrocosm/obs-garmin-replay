#include "wasapi-capture.h"

// Must include initguid.h FIRST before any Windows headers
#define INITGUID
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>

#include <obs-module.h>
#include <stdlib.h>
#include <string.h>

// Define GUIDs that may not be linked properly
DEFINE_GUID(CLSID_MMDeviceEnumerator, 0xbcde0395, 0xe52f, 0x467c,
    0x8e, 0x3d, 0xc4, 0x57, 0x92, 0x91, 0x69, 0x2e);
DEFINE_GUID(IID_IMMDeviceEnumerator, 0xa95664d2, 0x9614, 0x4f35,
    0xa7, 0x46, 0xde, 0x8d, 0xb6, 0x36, 0x17, 0xe6);
DEFINE_GUID(IID_IAudioClient, 0x1cb9ad4c, 0xdbfa, 0x4c32,
    0xb1, 0x78, 0xc2, 0xf5, 0x68, 0xa7, 0x03, 0xb2);
DEFINE_GUID(IID_IAudioCaptureClient, 0xc8adbd64, 0xe71e, 0x48a0,
    0xa4, 0xde, 0x18, 0x5c, 0x39, 0x5c, 0xd3, 0x17);

// Vosk requires 16kHz, 16-bit, mono
#define TARGET_SAMPLE_RATE 16000
#define TARGET_BITS_PER_SAMPLE 16
#define TARGET_CHANNELS 1

// Buffer duration in 100-nanosecond units (100ms)
#define BUFFER_DURATION_100NS 1000000

struct wasapi_capture {
    IMMDeviceEnumerator *device_enum;
    IMMDevice *device;
    IAudioClient *audio_client;
    IAudioCaptureClient *capture_client;
    WAVEFORMATEX *device_format;
    HANDLE event_handle;
    bool initialized;
    bool capturing;
    int source_sample_rate;
    int source_channels;
    int source_bits;

    // Resampling buffer
    float *resample_buffer;
    int resample_buffer_size;
};

wasapi_capture_t *wasapi_capture_create(const char *device_id)
{
    wasapi_capture_t *capture = calloc(1, sizeof(wasapi_capture_t));
    if (!capture) {
        return NULL;
    }

    HRESULT hr;

    // Create device enumerator
    hr = CoCreateInstance(
        &CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL,
        &IID_IMMDeviceEnumerator, (void **)&capture->device_enum);
    if (FAILED(hr)) {
        blog(LOG_ERROR, "[Garmin Replay] Failed to create device enumerator: 0x%08lX", hr);
        goto fail;
    }

    // Get the audio device
    if (device_id && strlen(device_id) > 0) {
        // Convert device ID to wide string
        int wide_len = MultiByteToWideChar(CP_UTF8, 0, device_id, -1, NULL, 0);
        wchar_t *wide_id = malloc(wide_len * sizeof(wchar_t));
        MultiByteToWideChar(CP_UTF8, 0, device_id, -1, wide_id, wide_len);

        hr = capture->device_enum->lpVtbl->GetDevice(
            capture->device_enum, wide_id, &capture->device);
        free(wide_id);

        if (FAILED(hr)) {
            blog(LOG_WARNING, "[Garmin Replay] Failed to get device '%s', using default: 0x%08lX",
                 device_id, hr);
            hr = capture->device_enum->lpVtbl->GetDefaultAudioEndpoint(
                capture->device_enum, eCapture, eConsole, &capture->device);
        }
    } else {
        hr = capture->device_enum->lpVtbl->GetDefaultAudioEndpoint(
            capture->device_enum, eCapture, eConsole, &capture->device);
    }

    if (FAILED(hr)) {
        blog(LOG_ERROR, "[Garmin Replay] Failed to get audio device: 0x%08lX", hr);
        goto fail;
    }

    // Activate audio client
    hr = capture->device->lpVtbl->Activate(
        capture->device, &IID_IAudioClient, CLSCTX_ALL,
        NULL, (void **)&capture->audio_client);
    if (FAILED(hr)) {
        blog(LOG_ERROR, "[Garmin Replay] Failed to activate audio client: 0x%08lX", hr);
        goto fail;
    }

    // Get the device's mix format
    hr = capture->audio_client->lpVtbl->GetMixFormat(
        capture->audio_client, &capture->device_format);
    if (FAILED(hr)) {
        blog(LOG_ERROR, "[Garmin Replay] Failed to get mix format: 0x%08lX", hr);
        goto fail;
    }

    capture->source_sample_rate = capture->device_format->nSamplesPerSec;
    capture->source_channels = capture->device_format->nChannels;
    capture->source_bits = capture->device_format->wBitsPerSample;

    blog(LOG_INFO, "[Garmin Replay] Device format: %d Hz, %d ch, %d bit",
         capture->source_sample_rate, capture->source_channels, capture->source_bits);

    // Initialize audio client in shared mode
    hr = capture->audio_client->lpVtbl->Initialize(
        capture->audio_client,
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
        BUFFER_DURATION_100NS,
        0,
        capture->device_format,
        NULL);
    if (FAILED(hr)) {
        blog(LOG_ERROR, "[Garmin Replay] Failed to initialize audio client: 0x%08lX", hr);
        goto fail;
    }

    // Create event for audio ready notification
    capture->event_handle = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!capture->event_handle) {
        blog(LOG_ERROR, "[Garmin Replay] Failed to create event handle");
        goto fail;
    }

    hr = capture->audio_client->lpVtbl->SetEventHandle(
        capture->audio_client, capture->event_handle);
    if (FAILED(hr)) {
        blog(LOG_ERROR, "[Garmin Replay] Failed to set event handle: 0x%08lX", hr);
        goto fail;
    }

    // Get capture client
    hr = capture->audio_client->lpVtbl->GetService(
        capture->audio_client, &IID_IAudioCaptureClient,
        (void **)&capture->capture_client);
    if (FAILED(hr)) {
        blog(LOG_ERROR, "[Garmin Replay] Failed to get capture client: 0x%08lX", hr);
        goto fail;
    }

    // Allocate resample buffer
    capture->resample_buffer_size = 8192;
    capture->resample_buffer = malloc(capture->resample_buffer_size * sizeof(float));
    if (!capture->resample_buffer) {
        goto fail;
    }

    capture->initialized = true;
    blog(LOG_INFO, "[Garmin Replay] WASAPI capture initialized");
    return capture;

fail:
    wasapi_capture_destroy(capture);
    return NULL;
}

bool wasapi_capture_start(wasapi_capture_t *capture)
{
    if (!capture || !capture->initialized) {
        return false;
    }

    HRESULT hr = capture->audio_client->lpVtbl->Start(capture->audio_client);
    if (FAILED(hr)) {
        blog(LOG_ERROR, "[Garmin Replay] Failed to start capture: 0x%08lX", hr);
        return false;
    }

    capture->capturing = true;
    blog(LOG_INFO, "[Garmin Replay] Audio capture started");
    return true;
}

// Convert float audio to 16-bit signed
static void float_to_s16(const float *src, short *dst, int count)
{
    for (int i = 0; i < count; i++) {
        float sample = src[i];
        if (sample < -1.0f) sample = -1.0f;
        if (sample > 1.0f) sample = 1.0f;
        dst[i] = (short)(sample * 32767.0f);
    }
}

// Convert 32-bit signed to 16-bit signed
static void s32_to_s16(const int *src, short *dst, int count)
{
    for (int i = 0; i < count; i++) {
        dst[i] = (short)(src[i] >> 16);
    }
}

// Simple downmix to mono
static void stereo_to_mono_s16(const short *src, short *dst, int frames)
{
    for (int i = 0; i < frames; i++) {
        int left = src[i * 2];
        int right = src[i * 2 + 1];
        dst[i] = (short)((left + right) / 2);
    }
}

// Simple resampling (linear interpolation)
static int resample_linear(const short *src, int src_len, int src_rate,
                           short *dst, int dst_max, int dst_rate)
{
    if (src_rate == dst_rate) {
        int copy_len = src_len < dst_max ? src_len : dst_max;
        memcpy(dst, src, copy_len * sizeof(short));
        return copy_len;
    }

    double ratio = (double)src_rate / (double)dst_rate;
    int dst_len = (int)(src_len / ratio);
    if (dst_len > dst_max) {
        dst_len = dst_max;
    }

    for (int i = 0; i < dst_len; i++) {
        double src_idx = i * ratio;
        int idx0 = (int)src_idx;
        int idx1 = idx0 + 1;
        if (idx1 >= src_len) idx1 = src_len - 1;

        double frac = src_idx - idx0;
        dst[i] = (short)(src[idx0] * (1.0 - frac) + src[idx1] * frac);
    }

    return dst_len;
}

int wasapi_capture_read(wasapi_capture_t *capture, short *buffer, int max_samples)
{
    if (!capture || !capture->initialized || !capture->capturing) {
        return -1;
    }

    // Wait for audio data with timeout
    DWORD result = WaitForSingleObject(capture->event_handle, 100);
    if (result != WAIT_OBJECT_0) {
        return 0;  // No data yet
    }

    BYTE *data;
    UINT32 frames_available;
    DWORD flags;

    HRESULT hr = capture->capture_client->lpVtbl->GetBuffer(
        capture->capture_client, &data, &frames_available, &flags, NULL, NULL);
    if (FAILED(hr)) {
        return -1;
    }

    if (frames_available == 0) {
        capture->capture_client->lpVtbl->ReleaseBuffer(
            capture->capture_client, 0);
        return 0;
    }

    int samples_out = 0;

    if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
        // Fill with silence
        int silence_samples = frames_available;
        if (silence_samples > max_samples) silence_samples = max_samples;
        memset(buffer, 0, silence_samples * sizeof(short));
        samples_out = silence_samples;
    } else {
        // Temporary buffer for format conversion
        short *temp_buffer = malloc(frames_available * capture->source_channels * sizeof(short));
        short *mono_buffer = malloc(frames_available * sizeof(short));

        if (!temp_buffer || !mono_buffer) {
            free(temp_buffer);
            free(mono_buffer);
            capture->capture_client->lpVtbl->ReleaseBuffer(
                capture->capture_client, frames_available);
            return -1;
        }

        // Convert to 16-bit
        WAVEFORMATEX *fmt = capture->device_format;
        if (fmt->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
            (fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE && fmt->wBitsPerSample == 32)) {
            float_to_s16((const float *)data, temp_buffer,
                         frames_available * capture->source_channels);
        } else if (fmt->wBitsPerSample == 32) {
            s32_to_s16((const int *)data, temp_buffer,
                       frames_available * capture->source_channels);
        } else if (fmt->wBitsPerSample == 16) {
            memcpy(temp_buffer, data, frames_available * capture->source_channels * sizeof(short));
        } else {
            // Unsupported format
            memset(temp_buffer, 0, frames_available * capture->source_channels * sizeof(short));
        }

        // Convert to mono
        if (capture->source_channels >= 2) {
            stereo_to_mono_s16(temp_buffer, mono_buffer, frames_available);
        } else {
            memcpy(mono_buffer, temp_buffer, frames_available * sizeof(short));
        }

        // Resample to target rate
        samples_out = resample_linear(mono_buffer, frames_available,
                                       capture->source_sample_rate,
                                       buffer, max_samples,
                                       TARGET_SAMPLE_RATE);

        free(temp_buffer);
        free(mono_buffer);
    }

    capture->capture_client->lpVtbl->ReleaseBuffer(
        capture->capture_client, frames_available);

    return samples_out;
}

void wasapi_capture_stop(wasapi_capture_t *capture)
{
    if (!capture || !capture->initialized) {
        return;
    }

    if (capture->capturing && capture->audio_client) {
        capture->audio_client->lpVtbl->Stop(capture->audio_client);
        capture->capturing = false;
        blog(LOG_INFO, "[Garmin Replay] Audio capture stopped");
    }
}

void wasapi_capture_destroy(wasapi_capture_t *capture)
{
    if (!capture) {
        return;
    }

    wasapi_capture_stop(capture);

    if (capture->capture_client) {
        capture->capture_client->lpVtbl->Release(capture->capture_client);
    }
    if (capture->audio_client) {
        capture->audio_client->lpVtbl->Release(capture->audio_client);
    }
    if (capture->device) {
        capture->device->lpVtbl->Release(capture->device);
    }
    if (capture->device_enum) {
        capture->device_enum->lpVtbl->Release(capture->device_enum);
    }
    if (capture->event_handle) {
        CloseHandle(capture->event_handle);
    }
    if (capture->device_format) {
        CoTaskMemFree(capture->device_format);
    }
    if (capture->resample_buffer) {
        free(capture->resample_buffer);
    }

    free(capture);
}

int wasapi_capture_get_sample_rate(wasapi_capture_t *capture)
{
    return TARGET_SAMPLE_RATE;
}
