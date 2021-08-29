# Read more about configuring cmake with Qt at:
# http://qt-project.org/quarterly/view/using_cmake_to_build_qt_projects

ADD_DEFINITIONS(${QT_DEFINITIONS})

INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}/src/core
  ${CMAKE_CURRENT_BINARY_DIR}
)

SET(gui_SOURCES
  program_image_widget.cpp
  machine_widget.cpp
  child_window.cpp
  main_window.cpp
  main.cpp
)

SET(gui_HEADERS
  main_window.h
  child_window.h
  machine_widget.h
  program_image_widget.h
)

SET(gui_FORMS
  program_image_widget.ui
  machine_widget.ui
  main_window.ui
)

SET(gui_RESOURCES
  images.qrc
)

QT5_WRAP_CPP(gui_HEADERS_MOC ${gui_HEADERS})
QT5_WRAP_UI(gui_FORMS_HEADERS ${gui_FORMS})
QT5_ADD_RESOURCES(gui_RESOURCES_RCC ${gui_RESOURCES})

ADD_EXECUTABLE(gui
  p_gui_virtual_machine.cpp # this is not a UI file (no moc generated)
  ${gui_SOURCES} 
  ${gui_HEADERS_MOC}
  ${gui_FORMS_HEADERS}
  ${gui_RESOURCES_RCC}
)

TARGET_LINK_LIBRARIES(gui
  Qt5::Widgets
  PietCore
)
