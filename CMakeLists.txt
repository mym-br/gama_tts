project(gama_tts)
cmake_minimum_required(VERSION 2.8)

if(UNIX)
    if(APPLE)
        set(CMAKE_CXX_FLAGS "-std=c++17 -stdlib=libc++")
    else()
        set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -Wextra -Wno-strict-overflow")
    endif()
endif()

include_directories(
    src
    src/rapidxml
    src/text_parser
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

    src/rapidxml/rapidxml.hpp

    src/text_parser/english/ApplyStress.cpp
    src/text_parser/english/ApplyStress.h
    src/text_parser/english/EnglishText.cpp
    src/text_parser/english/EnglishText.h
    src/text_parser/english/EnglishTextParser.cpp
    src/text_parser/english/EnglishTextParser.h
    src/text_parser/english/IspTrans.cpp
    src/text_parser/english/IspTrans.h
    src/text_parser/english/LetterToSound.cpp
    src/text_parser/english/LetterToSound.h
    src/text_parser/english/NumberParser.cpp
    src/text_parser/english/NumberParser.h
    src/text_parser/english/NumberPronunciations.h
    src/text_parser/english/Syllabify.cpp
    src/text_parser/english/Syllabify.h
    src/text_parser/DictionarySearch.cpp
    src/text_parser/DictionarySearch.h
    src/text_parser/TextParser.cpp
    src/text_parser/TextParser.h

    src/vtm/BandpassFilter.h
    src/vtm/Butterworth1LowpassFilter.h
    src/vtm/Butterworth2LowpassFilter.h
    src/vtm/DifferenceFilter.h
    src/vtm/MovingAverageFilter.h
    src/vtm/NoiseFilter.h
    src/vtm/NoiseSource.h
    src/vtm/PoleZeroRadiationImpedance.h
    src/vtm/RadiationFilter.h
    src/vtm/ReflectionFilter.h
    src/vtm/RosenbergBGlottalSource.h
    src/vtm/SampleRateConverter.h
    src/vtm/Throat.h
    src/vtm/VocalTractModel.cpp
    src/vtm/VocalTractModel.h
    src/vtm/VocalTractModel0.h
    src/vtm/VocalTractModel2.h
    src/vtm/VocalTractModel4.h
    src/vtm/VocalTractModel5.h
    src/vtm/VTMUtil.cpp
    src/vtm/VTMUtil.h
    src/vtm/WavetableGlottalSource.h
    src/vtm/WavetableGlottalSourceFIRFilter.h

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
    src/vtm_control_model/IntonationRhythm.cpp
    src/vtm_control_model/IntonationRhythm.h
    src/vtm_control_model/Model.cpp
    src/vtm_control_model/Model.h
    src/vtm_control_model/Parameter.h
    src/vtm_control_model/Pho1Parser.cpp
    src/vtm_control_model/Pho1Parser.h
    src/vtm_control_model/PhoneticStringParser.cpp
    src/vtm_control_model/PhoneticStringParser.h
    src/vtm_control_model/Posture.cpp
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

    src/xml/RapidXmlUtil.cpp
    src/xml/RapidXmlUtil.h
    src/xml/StreamXMLWriter.h
)

add_library(gamatts STATIC ${LIBRARY_FILES})

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
endif()
