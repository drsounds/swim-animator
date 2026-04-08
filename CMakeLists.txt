cmake_minimum_required(VERSION 3.16)
project(spacely VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(wxWidgets REQUIRED COMPONENTS core base aui xml)
include(${wxWidgets_USE_FILE})

set(SOURCES
    src/App.cpp
    src/MainFrame.cpp
    src/Document.cpp
    src/View.cpp
    src/DrawDoc.cpp
    src/DrawView.cpp
    src/DrawCommands.cpp
    src/PropPanel.cpp
    src/Palette.cpp
    src/ColorSwatchPanel.cpp
    src/SpaDoc.cpp
    src/SpaView.cpp
    src/SpaSaveAsDialog.cpp
    src/AssetManagerPanel.cpp
    src/NewDrawingDialog.cpp
)

add_executable(spacily-app WIN32 MACOSX_BUNDLE ${SOURCES})

target_link_libraries(spacily-app PRIVATE ${wxWidgets_LIBRARIES})
target_include_directories(spacily-app PRIVATE src)

set_target_properties(spacily-app PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "se.spacify.spacely"
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

