#include "device-enum.h"

#include <windows.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>

#include <obs-module.h>
#include <stdlib.h>
#include <string.h>

// GUIDs are defined in wasapi-capture.c

device_list_t *device_enum_microphones(void)
{
    device_list_t *list = calloc(1, sizeof(device_list_t));
    if (!list) {
        return NULL;
    }

    HRESULT hr;
    IMMDeviceEnumerator *enumerator = NULL;
    IMMDeviceCollection *collection = NULL;

    // Initialize COM for this thread if needed
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    bool com_initialized = SUCCEEDED(hr) || hr == S_FALSE;

    hr = CoCreateInstance(
        &CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL,
        &IID_IMMDeviceEnumerator, (void **)&enumerator);
    if (FAILED(hr)) {
        blog(LOG_ERROR, "[Garmin Replay] Failed to create device enumerator: 0x%08lX", hr);
        goto cleanup;
    }

    hr = enumerator->lpVtbl->EnumAudioEndpoints(
        enumerator, eCapture, DEVICE_STATE_ACTIVE, &collection);
    if (FAILED(hr)) {
        blog(LOG_ERROR, "[Garmin Replay] Failed to enumerate endpoints: 0x%08lX", hr);
        goto cleanup;
    }

    UINT count;
    hr = collection->lpVtbl->GetCount(collection, &count);
    if (FAILED(hr)) {
        goto cleanup;
    }

    for (UINT i = 0; i < count && list->count < MAX_DEVICES; i++) {
        IMMDevice *device = NULL;
        hr = collection->lpVtbl->Item(collection, i, &device);
        if (FAILED(hr)) {
            continue;
        }

        // Get device ID
        LPWSTR device_id = NULL;
        hr = device->lpVtbl->GetId(device, &device_id);
        if (SUCCEEDED(hr) && device_id) {
            WideCharToMultiByte(CP_UTF8, 0, device_id, -1,
                                list->devices[list->count].id,
                                MAX_DEVICE_ID_LEN - 1, NULL, NULL);
            CoTaskMemFree(device_id);
        }

        // Get friendly name
        IPropertyStore *props = NULL;
        hr = device->lpVtbl->OpenPropertyStore(device, STGM_READ, &props);
        if (SUCCEEDED(hr) && props) {
            PROPVARIANT name;
            PropVariantInit(&name);

            hr = props->lpVtbl->GetValue(props, &PKEY_Device_FriendlyName, &name);
            if (SUCCEEDED(hr) && name.pwszVal) {
                WideCharToMultiByte(CP_UTF8, 0, name.pwszVal, -1,
                                    list->devices[list->count].name,
                                    MAX_DEVICE_NAME_LEN - 1, NULL, NULL);
            }

            PropVariantClear(&name);
            props->lpVtbl->Release(props);
        }

        device->lpVtbl->Release(device);
        list->count++;
    }

    blog(LOG_INFO, "[Garmin Replay] Found %d microphone(s)", list->count);

cleanup:
    if (collection) {
        collection->lpVtbl->Release(collection);
    }
    if (enumerator) {
        enumerator->lpVtbl->Release(enumerator);
    }
    if (com_initialized) {
        CoUninitialize();
    }

    return list;
}

void device_list_free(device_list_t *list)
{
    free(list);
}
