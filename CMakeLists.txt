cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

# obs-tree always needs the frontend API (dock registration) and Qt (the dock UI),
# so both features are ON by default rather than the template's opt-in defaults.
option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" ON)
option(ENABLE_QT "Use Qt functionality" ON)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui Svg)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets Qt6::Gui Qt6::Svg)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

target_sources(
  ${CMAKE_PROJECT_NAME}
  PRIVATE
    src/plugin-main.cpp
    src/scene-tree.cpp
    src/scene-tree.hpp
    src/scene-tree-dock.cpp
    src/scene-tree-dock.hpp
    src/icons.cpp
    src/icons.hpp
)

# The sources read PLUGIN_VERSION directly (no generated plugin-support module).
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE PLUGIN_VERSION="${PROJECT_VERSION}")

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

# --- Local dev install (portable Linux layout) ---------------------------
# `dev-install` copies the freshly built module + data/ into OBS's portable
# plugin dir for the local build -> install -> restart-OBS loop on Linux. It is
# a no-root convenience target, separate from CI's standard system install.
#
#   cmake --build build_x86_64 --target dev-install   # then restart OBS
#
# Override the destination with -DOBS_PLUGIN_DESTINATION=/path.
if(OS_LINUX OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
  if(NOT DEFINED OBS_PLUGIN_DESTINATION)
    set(OBS_PLUGIN_DESTINATION "$ENV{HOME}/.config/obs-studio/plugins/${CMAKE_PROJECT_NAME}")
  endif()

  add_custom_target(
    dev-install
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${OBS_PLUGIN_DESTINATION}/bin/64bit"
    COMMAND
      "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
      "${OBS_PLUGIN_DESTINATION}/bin/64bit/"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/data" "${OBS_PLUGIN_DESTINATION}/data"
    DEPENDS ${CMAKE_PROJECT_NAME}
    COMMENT "Installing ${CMAKE_PROJECT_NAME} into OBS portable plugin dir: ${OBS_PLUGIN_DESTINATION}"
    VERBATIM
  )
endif()
