project(gama_tts)
cmake_minimum_required(VERSION 2.8)

if(UNIX)
    if(APPLE)
        set(CMAKE_CXX_FLAGS "-std=c++14 -stdlib=libc++")
    else()
        #set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wextra -march=native")
        set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wextra -Wno-strict-overflow")
    endif()
endif()

set(GAMA_TTS_VERSION_MAJOR 0)
set(GAMA_TTS_VERSION_MINOR 1)
set(GAMA_TTS_VERSION_PATCH 7)
set(GAMA_TTS_VERSION_STRING ${GAMA_TTS_VERSION_MAJOR}.${GAMA_TTS_VERSION_MINOR}.${GAMA_TTS_VERSION_PATCH})

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
    src/ParameterLogger.h
    src/StringMap.cpp
    src/StringMap.h
    src/Text.cpp
    src/Text.h
    src/VocalTractModelParameterValue.h
    src/WAVEFileWriter.cpp
    src/WAVEFileWriter.h

    src/english/ApplyStress.cpp
    src/english/ApplyStress.h
    src/english/EnglishTextParser.cpp
    src/english/EnglishTextParser.h
    src/english/IspTrans.cpp
    src/english/IspTrans.h
    src/english/LetterToSound.cpp
    src/english/LetterToSound.h
    src/english/NumberParser.cpp
    src/english/NumberParser.h
    src/english/NumberPronunciations.h
    src/english/Syllabify.cpp
    src/english/Syllabify.h

    src/rapidxml/rapidxml.hpp

    src/vtm/BandpassFilter.h
    src/vtm/Butterworth1LowpassFilter.h
    src/vtm/Butterworth2LowpassFilter.h
    src/vtm/DifferenceFilter.h
    src/vtm/FIRFilter.h
    src/vtm/MovingAverageFilter.h
    src/vtm/NoiseFilter.h
    src/vtm/NoiseSource.h
    src/vtm/PoleZeroRadiationImpedance.h
    src/vtm/RadiationFilter.h
    src/vtm/ReflectionFilter.h
    src/vtm/SampleRateConverter.h
    src/vtm/Throat.h
    src/vtm/VocalTractModel.cpp
    src/vtm/VocalTractModel.h
    src/vtm/VocalTractModel0.h
    src/vtm/VocalTractModel2.h
    src/vtm/VocalTractModel4.h
    src/vtm/VocalTractModel5.h
    src/vtm/VTMUtil.h
    src/vtm/WavetableGlottalSource.h

    src/vtm_control_model/Category.h
    src/vtm_control_model/DictionarySearch.cpp
    src/vtm_control_model/DictionarySearch.h
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
    src/vtm_control_model/PhoneticStringParser.cpp
    src/vtm_control_model/PhoneticStringParser.h
    src/vtm_control_model/Posture.h
    src/vtm_control_model/PostureList.cpp
    src/vtm_control_model/PostureList.h
    src/vtm_control_model/Rule.cpp
    src/vtm_control_model/Rule.h
    src/vtm_control_model/Symbol.h
    src/vtm_control_model/TextParser.cpp
    src/vtm_control_model/TextParser.h
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

if(UNIX AND NOT APPLE)
    include(GNUInstallDirs)
    install(TARGETS gamatts gama_tts
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(DIRECTORY src/ DESTINATION include/gama_tts FILES_MATCHING PATTERN "*.h")
    install(DIRECTORY src/ DESTINATION include/gama_tts FILES_MATCHING PATTERN "*.hpp")
    install(DIRECTORY data DESTINATION share/gama_tts)
    configure_file(gama_tts.pc.pre gama_tts.pc @ONLY)
    install(FILES ${CMAKE_BINARY_DIR}/gama_tts.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig/)
endif()
