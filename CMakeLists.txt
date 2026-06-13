cmake_minimum_required(VERSION 3.28)

project(obs-tree VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# --- Dependencies --------------------------------------------------------
find_package(libobs REQUIRED)
find_package(obs-frontend-api REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui Svg)

# --- Plugin target -------------------------------------------------------
add_library(${PROJECT_NAME} MODULE)

target_sources(${PROJECT_NAME} PRIVATE
  src/plugin-main.cpp
  src/scene-tree.cpp
  src/scene-tree.hpp
  src/scene-tree-dock.cpp
  src/scene-tree-dock.hpp
  src/icons.cpp
  src/icons.hpp)

target_link_libraries(${PROJECT_NAME} PRIVATE
  OBS::libobs
  OBS::obs-frontend-api
  Qt6::Widgets
  Qt6::Core
  Qt6::Gui
  Qt6::Svg)

target_compile_definitions(${PROJECT_NAME} PRIVATE
  PLUGIN_VERSION="${PROJECT_VERSION}")

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

# --- Install (Linux portable plugin layout) ------------------------------
# OBS searches ~/.config/obs-studio/plugins/<name>/bin/64bit/<name>.so
# and ~/.config/obs-studio/plugins/<name>/data/
if(NOT DEFINED OBS_PLUGIN_DESTINATION)
  set(OBS_PLUGIN_DESTINATION "$ENV{HOME}/.config/obs-studio/plugins/${PROJECT_NAME}")
endif()

install(TARGETS ${PROJECT_NAME}
  LIBRARY DESTINATION "${OBS_PLUGIN_DESTINATION}/bin/64bit")
install(DIRECTORY data/
  DESTINATION "${OBS_PLUGIN_DESTINATION}/data")
