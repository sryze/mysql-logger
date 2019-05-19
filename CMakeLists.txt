project(mysql-logger C)
cmake_minimum_required(VERSION 3.0)

if(POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Download the necessary MySQL plugin development headers.
#
# MySQL packages in some Linux distributions (e.g. Debian) do not contain all
# the header files needed for plugin development. Therefore we download them
# manually here. This also makes it easier to build the code on other platforms.
#
# Download source code for MariaDB 10.0.10 (first stable release in the 10.0
# branch) and unpack the include directory to CMAKE_CURRENT_BINARY_DIR/include.
#
set(mysql_src_url
    "https://downloads.mariadb.org/f/mariadb-10.0.10/kvm-tarbake-jaunty-x86/mariadb-10.0.10.tar.gz/from/http%3A//ftp.hosteurope.de/mirror/archive.mariadb.org/?serve")
set(mysql_src_checksum "14ce22b8197d4eae88d237776d47220f")
set(mysql_src_archive "${CMAKE_CURRENT_BINARY_DIR}/mariadb-10.0.10.tar.gz")
set(mysql_include_dir mariadb-10.0.10/include)
if(NOT MYSQL_INCLUDE_DIR)
  if(NOT EXISTS "${mysql_src_archive}")
    message("Downloading MySQL headers")
    file(DOWNLOAD
      "${mysql_src_url}"
      "${mysql_src_archive}"
      EXPECTED_HASH MD5=${mysql_src_checksum}
      SHOW_PROGRESS)
  endif()
  if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/${mysql_include_dir}")
    message("Extracting ${mysql_src_archive} to ${mysql_include_dir}")
    execute_process(COMMAND
      ${CMAKE_COMMAND} -E
      tar xzf "${mysql_src_archive}" -- "${mysql_include_dir}"
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
  endif()
endif()

find_path(MYSQL_INCLUDE_DIR mysql/plugin.h
          HINTS "${CMAKE_CURRENT_BINARY_DIR}/${mysql_include_dir}"
          NO_CMAKE_SYSTEM_PATH)
if(NOT MYSQL_INCLUDE_DIR)
  message(FATAL_ERROR "Could not find MySQL headers")
else()
  message("Found MySQL headers: ${MYSQL_INCLUDE_DIR}")
endif()

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ui")
add_executable(file_concat
  src/tools/file_concat.c
  src/strbuf.c
  src/string_ext.c
)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/src")
add_executable(cdump src/tools/cdump.c)

function(add_http_resource path header_path var_name)
  add_custom_command(
    OUTPUT "${header_path}"
    COMMAND $<TARGET_FILE:cdump> "${path}" ${var_name} "${header_path}"
    DEPENDS "${path}"
  )
endfunction()

add_http_resource(
  "${CMAKE_CURRENT_SOURCE_DIR}/ui/index.html"
  "${CMAKE_CURRENT_BINARY_DIR}/src/ui_index_html.h"
  ui_index_html
)
add_http_resource(
  "${CMAKE_CURRENT_SOURCE_DIR}/ui/index.css"
  "${CMAKE_CURRENT_BINARY_DIR}/src/ui_index_css.h"
  ui_index_css
)
add_http_resource(
  "${CMAKE_CURRENT_SOURCE_DIR}/ui/index.js"
  "${CMAKE_CURRENT_BINARY_DIR}/src/ui_index_js.h"
  ui_index_js
)

if(CMAKE_C_COMPILER_ID MATCHES ".*Clang.*")
  set(COMPILER_IS_CLANG 1)
endif()

add_definitions(-DMYSQL_DYNAMIC_PLUGIN)

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

if(CMAKE_COMPILER_IS_GNUCC OR COMPILER_IS_CLANG)
  add_definitions("-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=1")
endif()

include_directories(
  "${CMAKE_CURRENT_BINARY_DIR}/src"
  "${MYSQL_INCLUDE_DIR}"
  "${MYSQL_INCLUDE_DIR}/mysql"
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
  "${CMAKE_CURRENT_BINARY_DIR}/src/ui_index_html.h"
  "${CMAKE_CURRENT_BINARY_DIR}/src/ui_index_css.h"
  "${CMAKE_CURRENT_BINARY_DIR}/src/ui_index_js.h"
)
set_target_properties(logger PROPERTIES PREFIX "")

if(CMAKE_COMPILER_IS_GNUCC OR COMPILER_IS_CLANG)
  target_compile_options(logger PRIVATE "-fno-omit-frame-pointer")
endif()

if(WIN32)
  target_link_libraries(logger ws2_32)
endif()
if(UNIX)
  target_link_libraries(logger pthread)
  if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    target_link_libraries(logger "-Wl,-Bsymbolic")
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
target_include_directories(tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
if(WIN32)
  target_link_libraries(tests ws2_32)
endif()
