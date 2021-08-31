CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)

PROJECT(piet)

IF(MSVC)
  ADD_COMPILE_OPTIONS(/W4 /WX)
ELSE()
  ADD_COMPILE_OPTIONS(-Wall -Wextra -pedantic -Werror)
ENDIF()

FIND_PACKAGE(Qt5 COMPONENTS Widgets REQUIRED)

ADD_SUBDIRECTORY(src)

# generating documentation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
  set(DOXYGEN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/doc)
  set(DOXYGEN_CONFIG ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc)
  configure_file(${DOXYGEN_DIR}/Doxyfile.in ${DOXYGEN_CONFIG} @ONLY)
  add_custom_target(doc ALL
    ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CONFIG}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS gui
    COMMENT "Generating API documentation with Doxygen" VERBATIM)
  execute_process(COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CONFIG}
    OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/doxygen.exec.log"
    ERROR_FILE "${CMAKE_CURRENT_BINARY_DIR}/doxygen.error.log")
else(DOXYGEN_FOUND)
  message(STATUS "Documentation could not be generated")
endif(DOXYGEN_FOUND)
