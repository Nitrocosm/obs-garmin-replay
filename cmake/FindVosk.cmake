# FindVosk.cmake - Find Vosk speech recognition library

# Look for Vosk in deps folder or system paths
find_path(VOSK_INCLUDE_DIR
    NAMES vosk_api.h
    PATHS
        "${CMAKE_SOURCE_DIR}/deps/vosk/include"
        "$ENV{VOSK_ROOT}/include"
        "$ENV{PROGRAMFILES}/vosk/include"
    NO_DEFAULT_PATH
)

find_library(VOSK_LIBRARY
    NAMES vosk libvosk
    PATHS
        "${CMAKE_SOURCE_DIR}/deps/vosk/lib"
        "$ENV{VOSK_ROOT}/lib"
        "$ENV{PROGRAMFILES}/vosk/lib"
    NO_DEFAULT_PATH
)

# Find DLLs for Windows runtime
if(WIN32)
    set(VOSK_BIN_DIR "${CMAKE_SOURCE_DIR}/deps/vosk/bin")

    find_file(VOSK_DLL_PATH
        NAMES libvosk.dll vosk.dll
        PATHS "${VOSK_BIN_DIR}" "$ENV{VOSK_ROOT}/bin"
        NO_DEFAULT_PATH
    )

    # Vosk depends on MinGW runtime DLLs
    find_file(VOSK_GCC_DLL NAMES libgcc_s_seh-1.dll PATHS "${VOSK_BIN_DIR}" NO_DEFAULT_PATH)
    find_file(VOSK_STDCPP_DLL NAMES libstdc++-6.dll PATHS "${VOSK_BIN_DIR}" NO_DEFAULT_PATH)
    find_file(VOSK_PTHREAD_DLL NAMES libwinpthread-1.dll PATHS "${VOSK_BIN_DIR}" NO_DEFAULT_PATH)

    # Collect all DLLs
    set(VOSK_ALL_DLLS "")
    if(VOSK_DLL_PATH)
        list(APPEND VOSK_ALL_DLLS "${VOSK_DLL_PATH}")
    endif()
    if(VOSK_GCC_DLL)
        list(APPEND VOSK_ALL_DLLS "${VOSK_GCC_DLL}")
    endif()
    if(VOSK_STDCPP_DLL)
        list(APPEND VOSK_ALL_DLLS "${VOSK_STDCPP_DLL}")
    endif()
    if(VOSK_PTHREAD_DLL)
        list(APPEND VOSK_ALL_DLLS "${VOSK_PTHREAD_DLL}")
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Vosk
    REQUIRED_VARS VOSK_LIBRARY VOSK_INCLUDE_DIR
    FAIL_MESSAGE "Could not find Vosk. Please download from https://github.com/alphacep/vosk-api/releases and extract to deps/vosk/"
)

if(Vosk_FOUND)
    message(STATUS "Found Vosk: ${VOSK_LIBRARY}")
    message(STATUS "Vosk include: ${VOSK_INCLUDE_DIR}")
    if(VOSK_DLL_PATH)
        message(STATUS "Vosk DLL: ${VOSK_DLL_PATH}")
    endif()

    if(NOT TARGET Vosk::Vosk)
        add_library(Vosk::Vosk UNKNOWN IMPORTED)
        set_target_properties(Vosk::Vosk PROPERTIES
            IMPORTED_LOCATION "${VOSK_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${VOSK_INCLUDE_DIR}"
        )
    endif()
endif()

mark_as_advanced(VOSK_INCLUDE_DIR VOSK_LIBRARY VOSK_DLL_PATH)
