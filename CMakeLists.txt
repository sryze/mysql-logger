project(mysql-logger C)
cmake_minimum_required(VERSION 3.0)

if(POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ui)
add_executable(file-concat src/tools/file_concat.c src/strbuf.c src/string_ext.c)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ui/index.html
  COMMAND $<TARGET_FILE:file-concat>
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/index.html
    ${CMAKE_CURRENT_BINARY_DIR}/ui/index.html
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ui
  DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/index.html
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/index.css
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/index.js
)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/src)
add_executable(cdump src/tools/cdump.c)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/src/ui_index_html.h
  COMMAND $<TARGET_FILE:cdump>
    ${CMAKE_CURRENT_BINARY_DIR}/ui/index.html
    ui_index_html
    ${CMAKE_CURRENT_BINARY_DIR}/src/ui_index_html.h
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/ui/index.html
)

if(CMAKE_C_COMPILER_ID MATCHES ".*Clang.*")
  set(COMPILER_IS_CLANG 1)
endif()

add_definitions(-DMYSQL_DYNAMIC_PLUGIN)

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

if(CMAKE_COMPILER_IS_GNUCC OR COMPILER_IS_CLANG)
  add_definitions(-D_FORTIFY_SOURCE=1)
endif()

find_path(MYSQL_INCLUDE_DIR mysql/plugin.h)
if(NOT MYSQL_INCLUDE_DIR)
  message(FATAL_ERROR "Could not find MySQL headers")
endif()

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}/src
  ${MYSQL_INCLUDE_DIR}
  ${MYSQL_INCLUDE_DIR}/mysql
)

add_library(logger SHARED
  src/base64.c
  src/base64.h
  src/bool.h
  src/hex.c
  src/hex.h
  src/http.c
  src/http.h
  src/json.c
  src/json.h
  src/logger.c
  src/sha1.c
  src/sha1.h
  src/socket_ext.c
  src/socket_ext.h
  src/strbuf.c
  src/strbuf.h
  src/string_ext.c
  src/string_ext.h
  src/thread.c
  src/thread.h
  src/ws.c
  src/ws.h
  ${CMAKE_CURRENT_BINARY_DIR}/src/ui_index_html.h
)
set_target_properties(logger PROPERTIES PREFIX "")

if(CMAKE_COMPILER_IS_GNUCC OR COMPILER_IS_CLANG)
  target_compile_options(logger PRIVATE -fno-omit-frame-pointer)
endif()

if(WIN32)
  target_link_libraries(logger ws2_32)
endif()
if(UNIX)
  target_link_libraries(logger pthread)
  if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    target_link_libraries(logger -Wl,-Bsymbolic)
  endif()
endif()

add_executable(tests
  src/http.c
  src/json.c
  src/socket_ext.c
  src/strbuf.c
  src/string_ext.c
  tests/all_tests.c
  tests/http_tests.c
  tests/http_tests.h
  tests/json_tests.c
  tests/json_tests.h
  tests/strbuf_tests.c
  tests/strbuf_tests.h
  tests/string_ext_tests.c
  tests/string_ext_tests.h
  tests/test.h
)
target_include_directories(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
if(WIN32)
  target_link_libraries(tests ws2_32)
endif()
