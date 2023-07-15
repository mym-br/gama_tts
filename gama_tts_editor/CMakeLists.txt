cmake_minimum_required(VERSION 3.8.0 FATAL_ERROR)

project(gama_tts_editor)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ui ui/interactive)
set(CMAKE_AUTORCC ON)

set(GAMATTS_QT_VERSION "5" CACHE STRING "Qt version used in the build.")

if(GAMATTS_QT_VERSION STREQUAL "6")
    find_package(Qt6 COMPONENTS Core Gui Widgets PrintSupport)
    add_compile_definitions(USING_QT6=1)
else()
    find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Widgets PrintSupport)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(FFTW3F REQUIRED IMPORTED_TARGET fftw3f)
pkg_check_modules(JACK REQUIRED IMPORTED_TARGET jack)

if(UNIX)
    if(NOT APPLE)
        set(CMAKE_CXX_FLAGS "-Wall -Wextra -march=native")
    endif()
endif()

set(gama_tts_editor_SRC
    src/AppConfig.h
    src/AudioPlayer.cpp
    src/AudioPlayer.h
    src/AudioWorker.cpp
    src/AudioWorker.h
    src/Clipboard.cpp
    src/Clipboard.h
    src/DataEntryWindow.cpp
    src/DataEntryWindow.h
    src/editor_global.h
    src/Figure2DWidget.cpp
    src/Figure2DWidget.h
    src/interactive/AnalysisWindow.cpp
    src/interactive/AnalysisWindow.h
    src/interactive/FFTW.cpp
    src/interactive/FFTW.h
    src/interactive/InteractiveAudio.cpp
    src/interactive/InteractiveAudio.h
    src/interactive/InteractiveVTMConfiguration.cpp
    src/interactive/InteractiveVTMConfiguration.h
    src/interactive/InteractiveVTMWindow.cpp
    src/interactive/InteractiveVTMWindow.h
    src/interactive/ParameterLineEdit.cpp
    src/interactive/ParameterLineEdit.h
    src/interactive/ParameterSlider.cpp
    src/interactive/ParameterSlider.h
    src/interactive/SignalDFT.cpp
    src/interactive/SignalDFT.h
    src/IntonationParametersWindow.cpp
    src/IntonationParametersWindow.h
    src/IntonationWidget.cpp
    src/IntonationWidget.h
    src/IntonationWindow.cpp
    src/IntonationWindow.h
    src/JackClient.cpp
    src/JackClient.h
    src/JackConfig.cpp
    src/JackConfig.h
    src/JackRingbuffer.cpp
    src/JackRingbuffer.h
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/ParameterModificationSynthesis.cpp
    src/ParameterModificationSynthesis.h
    src/ParameterModificationWidget.cpp
    src/ParameterModificationWidget.h
    src/ParameterModificationWindow.cpp
    src/ParameterModificationWindow.h
    src/ParameterWidget.cpp
    src/ParameterWidget.h
    src/PostureEditorWindow.cpp
    src/PostureEditorWindow.h
    src/PrototypeManagerWindow.cpp
    src/PrototypeManagerWindow.h
    src/qt_model/CategoryModel.cpp
    src/qt_model/CategoryModel.h
    src/qt_model/ParameterModel.cpp
    src/qt_model/ParameterModel.h
    src/qt_model/SymbolModel.cpp
    src/qt_model/SymbolModel.h
    src/RuleManagerWindow.cpp
    src/RuleManagerWindow.h
    src/RuleTesterWindow.cpp
    src/RuleTesterWindow.h
    src/Synthesis.cpp
    src/Synthesis.h
    src/SynthesisWindow.cpp
    src/SynthesisWindow.h
    src/TransitionEditorWindow.cpp
    src/TransitionEditorWindow.h
    src/TransitionPoint.cpp
    src/TransitionPoint.h
    src/TransitionWidget.cpp
    src/TransitionWidget.h

    ui/DataEntryWindow.ui
    ui/interactive/AnalysisWindow.ui
    ui/IntonationParametersWindow.ui
    ui/IntonationWindow.ui
    ui/MainWindow.ui
    ui/PostureEditorWindow.ui
    ui/PrototypeManagerWindow.ui
    ui/RuleManagerWindow.ui
    ui/RuleTesterWindow.ui
    ui/SynthesisWindow.ui
    ui/TransitionEditorWindow.ui
    ui/ParameterModificationWindow.ui

    resource/gama_tts_editor.qrc
)

add_executable(gama_tts_editor ${gama_tts_editor_SRC})

target_include_directories(gama_tts_editor PRIVATE
    src
    src/interactive
    src/qt_model

    ${FFTW3F_INCLUDE_DIRS}
    ${JACK_INCLUDE_DIRS}

    ../gama_tts/src
    ../gama_tts/src/text_parser
    ../gama_tts/src/vtm
    ../gama_tts/src/vtm_control_model
)

target_link_libraries(gama_tts_editor
    Qt::Core
    Qt::Gui
    Qt::Widgets
    Qt::PrintSupport

    PkgConfig::FFTW3F
    PkgConfig::JACK

    debug     ${CMAKE_SOURCE_DIR}/../build-gama_tts-debug/libgamatts.a
    optimized ${CMAKE_SOURCE_DIR}/../build-gama_tts/libgamatts.a
)

if(UNIX AND NOT APPLE)
    install(TARGETS gama_tts_editor
        RUNTIME DESTINATION bin)
endif()
