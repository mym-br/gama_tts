project(gama_tts)
cmake_minimum_required(VERSION 2.8)

if(UNIX)
    if(APPLE)
        set(CMAKE_CXX_FLAGS "-std=c++14 -stdlib=libc++")
    else()
        set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wextra -march=native")
    endif()
endif()

set(GAMA_TTS_VERSION_MAJOR 0)
set(GAMA_TTS_VERSION_MINOR 1)
set(GAMA_TTS_VERSION_PATCH 6)
set(GAMA_TTS_VERSION_STRING ${GAMA_TTS_VERSION_MAJOR}.${GAMA_TTS_VERSION_MINOR}.${GAMA_TTS_VERSION_PATCH})

if(MSVC)
    # To have M_PI defined in <cmath>.
    add_definitions(-D_USE_MATH_DEFINES)

    # Disable warnings about strcpy_s, strcat_s, etc.
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
endif()

include_directories(
    src
    src/rapidxml
    src/vtm
    src/vtm_control_model
    src/xml
)

set(LIBRARY_FILES
    src/ConfigurationData.cpp
    src/ConfigurationData.h
    src/Dictionary.cpp
    src/Dictionary.h
    src/Exception.h
    src/global.h
    src/Log.cpp
    src/Log.h
    src/Text.cpp
    src/Text.h
    src/VocalTractModelParameterValue.h
    src/WAVEFileWriter.cpp
    src/WAVEFileWriter.h

    src/en/letter_to_sound/apply_stress.cpp
    src/en/letter_to_sound/apply_stress.h
    src/en/letter_to_sound/check_word_list.cpp
    src/en/letter_to_sound/check_word_list.h
    src/en/letter_to_sound/clusters.h
    src/en/letter_to_sound/ends_with.cpp
    src/en/letter_to_sound/ends_with.h
    src/en/letter_to_sound/final_s.cpp
    src/en/letter_to_sound/final_s.h
    src/en/letter_to_sound/ie_to_y.cpp
    src/en/letter_to_sound/ie_to_y.h
    src/en/letter_to_sound/insert_mark.cpp
    src/en/letter_to_sound/insert_mark.h
    src/en/letter_to_sound/isp_trans.cpp
    src/en/letter_to_sound/isp_trans.h
    src/en/letter_to_sound/letter_to_sound.cpp
    src/en/letter_to_sound/letter_to_sound.h
    src/en/letter_to_sound/long_medial_vowels.cpp
    src/en/letter_to_sound/long_medial_vowels.h
    src/en/letter_to_sound/mark_final_e.cpp
    src/en/letter_to_sound/mark_final_e.h
    src/en/letter_to_sound/medial_s.cpp
    src/en/letter_to_sound/medial_s.h
    src/en/letter_to_sound/medial_silent_e.cpp
    src/en/letter_to_sound/medial_silent_e.h
    src/en/letter_to_sound/member.cpp
    src/en/letter_to_sound/member.h
    src/en/letter_to_sound/stresstables.h
    src/en/letter_to_sound/suffix.cpp
    src/en/letter_to_sound/suffix.h
    src/en/letter_to_sound/syllabify.cpp
    src/en/letter_to_sound/syllabify.h
    src/en/letter_to_sound/tail.h
    src/en/letter_to_sound/vowel_before.cpp
    src/en/letter_to_sound/vowel_before.h
    src/en/letter_to_sound/word_to_patphone.cpp
    src/en/letter_to_sound/word_to_patphone.h

    src/en/dictionary/DictionarySearch.cpp
    src/en/dictionary/DictionarySearch.h
    src/en/dictionary/suffix_list.h

    src/en/number_pronunciations.h

    src/en/phonetic_string_parser/PhoneticStringParser.cpp
    src/en/phonetic_string_parser/PhoneticStringParser.h

    src/en/text_parser/abbreviations.h
    src/en/text_parser/NumberParser.cpp
    src/en/text_parser/NumberParser.h
    src/en/text_parser/special_acronyms.h
    src/en/text_parser/TextParser.cpp
    src/en/text_parser/TextParser.h

    src/rapidxml/rapidxml.hpp

    src/vtm/BandpassFilter.h
    src/vtm/FIRFilter.h
    src/vtm/MovingAverageFilter.h
    src/vtm/NoiseFilter.h
    src/vtm/NoiseSource.h
    src/vtm/RadiationFilter.h
    src/vtm/ReflectionFilter.h
    src/vtm/SampleRateConverter.cpp
    src/vtm/SampleRateConverter.h
    src/vtm/Throat.h
    src/vtm/VocalTractModel0.h
    src/vtm/VTMConfiguration.cpp
    src/vtm/VTMConfiguration.h
    src/vtm/VTMUtil.h
    src/vtm/WavetableGlottalSource.h

    src/vtm_control_model/Category.h
    src/vtm_control_model/DriftGenerator.cpp
    src/vtm_control_model/DriftGenerator.h
    src/vtm_control_model/Controller.cpp
    src/vtm_control_model/Controller.h
    src/vtm_control_model/Equation.cpp
    src/vtm_control_model/Equation.h
    src/vtm_control_model/EventList.cpp
    src/vtm_control_model/EventList.h
    src/vtm_control_model/FormulaSymbol.cpp
    src/vtm_control_model/FormulaSymbol.h
    src/vtm_control_model/IntonationPoint.cpp
    src/vtm_control_model/IntonationPoint.h
    src/vtm_control_model/Model.cpp
    src/vtm_control_model/Model.h
    src/vtm_control_model/Parameter.h
    src/vtm_control_model/Posture.h
    src/vtm_control_model/PostureList.cpp
    src/vtm_control_model/PostureList.h
    src/vtm_control_model/Rule.cpp
    src/vtm_control_model/Rule.h
    src/vtm_control_model/Symbol.h
    src/vtm_control_model/Transition.cpp
    src/vtm_control_model/Transition.h
    src/vtm_control_model/VTMControlModelConfiguration.cpp
    src/vtm_control_model/VTMControlModelConfiguration.h
    src/vtm_control_model/XMLConfigFileReader.cpp
    src/vtm_control_model/XMLConfigFileReader.h
    src/vtm_control_model/XMLConfigFileWriter.cpp
    src/vtm_control_model/XMLConfigFileWriter.h

    src/xml/RapidXmlUtil.h
    src/xml/StreamXMLWriter.h
)

if(UNIX)
    add_library(gamatts SHARED ${LIBRARY_FILES})
    set_target_properties(gamatts PROPERTIES
        VERSION ${GAMA_TTS_VERSION_STRING}
        SOVERSION ${GAMA_TTS_VERSION_MAJOR})
else()
    add_library(gamatts STATIC ${LIBRARY_FILES})
endif()

add_executable(gama_tts
    src/main.cpp
)
target_link_libraries(gama_tts gamatts)

add_executable(gama_tts_vtm
    src/vtm/gama_tts_vtm.cpp
)
target_link_libraries(gama_tts_vtm gamatts)

if(UNIX AND NOT APPLE)
    include(GNUInstallDirs)
    install(TARGETS gamatts gama_tts gama_tts_vtm
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(DIRECTORY src/ DESTINATION include/gama_tts FILES_MATCHING PATTERN "*.h")
    install(DIRECTORY src/ DESTINATION include/gama_tts FILES_MATCHING PATTERN "*.hpp")
    install(DIRECTORY data DESTINATION share/gama_tts)
    configure_file(gama_tts.pc.pre gama_tts.pc @ONLY)
    install(FILES ${CMAKE_BINARY_DIR}/gama_tts.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig/)
endif()

#add_dependencies(gama_tts gamatts)
#add_dependencies(gama_tts_vtm gamatts)
