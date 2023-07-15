project(gama_tts_speech_dispatcher_module)
cmake_minimum_required(VERSION 2.8)

if(UNIX)
    if(APPLE)
        set(CMAKE_CXX_FLAGS "-std=c++17 -stdlib=libc++")
    else()
        set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -Wextra")
    endif()
endif()

include(${CMAKE_SOURCE_DIR}/src/rtaudio/RtAudio.cmake)

set(SOURCE_FILES
    src/main.cpp
    src/ModuleConfiguration.cpp
    src/ModuleConfiguration.h
    src/ModuleController.cpp
    src/ModuleController.h
    src/rtaudio/RtAudio.cpp
    src/rtaudio/RtAudio.h
    src/SynthesizerController.cpp
    src/SynthesizerController.h
    src/Util.cpp
    src/Util.h
)

include_directories(
    src
    src/rtaudio
    ../gama_tts/src
    ../gama_tts/src/text_parser
    ../gama_tts/src/vtm
    ../gama_tts/src/vtm_control_model
)

add_executable(sd_gama_tts ${SOURCE_FILES})

IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(sd_gama_tts
        -L../gama_tts-build-debug -l:libgamatts.a
        ${RTAUDIO_LINKLIBS}
    )
ELSE(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(sd_gama_tts
        -L../gama_tts-build -l:libgamatts.a
        ${RTAUDIO_LINKLIBS}
    )
ENDIF(CMAKE_BUILD_TYPE STREQUAL "Debug")

if(UNIX AND NOT APPLE)
    install(TARGETS sd_gama_tts
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/lib${LIB_DIR_SUFFIX}/speech-dispatcher-modules/)
    configure_file(conf/gama_tts.conf.pre gama_tts.conf @ONLY)
    install(FILES ${CMAKE_BINARY_DIR}/gama_tts.conf DESTINATION /etc/speech-dispatcher/modules/)
endif()
