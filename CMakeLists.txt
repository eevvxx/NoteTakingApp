# CMakeLists.txt
cmake_minimum_required(VERSION 3.16) # Minimum version Qt 6 often requires

project(NoteApp VERSION 0.1 LANGUAGES CXX)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 17) # Use C++17 standard
set(RESOURCE_FILES resources.qrc)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON) # Enable automatic Meta-Object Compiler processing (Qt specific)
set(CMAKE_AUTORCC ON) # Enable automatic Resource Compiler processing (Qt specific)
set(CMAKE_AUTOUIC ON) # Enable automatic UI Compiler processing (Qt specific)


set(RESOURCE_FILES resources.qrc)

# Find the Qt 6 package, specifically the Widgets module
# Adjust the path if Qt isn't found automatically. You might need to set CMAKE_PREFIX_PATH.
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Add include directories
include_directories(include)

# Add source files
add_executable(NoteApp
    src/main.cpp
    src/MainWindow.cpp
    include/MainWindow.h
    ${RESOURCE_FILES}
)

# Link against the necessary Qt modules
target_link_libraries(NoteApp PRIVATE Qt6::Widgets)

# --- Optional: For installing (useful later) ---
# include(GNUInstallDirs)
# install(TARGETS NoteApp
#     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#     BUNDLE DESTINATION .
# )