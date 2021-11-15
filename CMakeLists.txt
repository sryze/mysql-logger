cmake_minimum_required(VERSION 3.0)

if(POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW)
endif()
if(POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif()

project(mysql-logger
  VERSION 0.1.2
  LANGUAGES C
)

################################################################################
# Common Build Setup
################################################################################

if(CMAKE_C_COMPILER_ID MATCHES ".*Clang.*")
  set(COMPILER_IS_CLANG 1)
endif()

if(WIN32)
  add_definitions(-DWIN32_LEAN_AND_MEAN)
endif()

add_compile_options("$<$<CONFIG:DEBUG>:-DDEBUG>")

if(MSVC)
  add_compile_definitions(/Wall)
elseif(CMAKE_COMPILER_IS_GNUCC OR COMPILER_IS_CLANG)
  add_compile_options(-Wall)
  add_compile_options("$<$<CONFIG:RELEASE>:-Werror>")
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
  "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/favicon.ico"
  "${CMAKE_CURRENT_BINARY_DIR}/src/ui_favicon_ico.h"
  ui_favicon_ico
)
add_http_resource(
  "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/index.html"
  "${CMAKE_CURRENT_BINARY_DIR}/src/ui_index_html.h"
  ui_index_html
)
add_http_resource(
  "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/index.css"
  "${CMAKE_CURRENT_BINARY_DIR}/src/ui_index_css.h"
  ui_index_css
)
add_http_resource(
  "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/index.js"
  "${CMAKE_CURRENT_BINARY_DIR}/src/ui_index_js.h"
  ui_index_js
)

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()
if(WIN32)
  add_definitions(-DUSE_LIGHTWEIGHT_MUTEXES)
endif()

if(CMAKE_COMPILER_IS_GNUCC OR COMPILER_IS_CLANG)
  add_definitions("-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=1")
endif()

include_directories("${CMAKE_CURRENT_BINARY_DIR}/src")

set(SOURCES
  src/base64.c
  src/base64.h
  src/config.c
  src/config.h
  src/defs.h
  src/error.c
  src/error.h
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
  src/time.c
  src/time.h
  src/thread.c
  src/thread.h
  src/ws.c
  src/ws.h
  "${CMAKE_CURRENT_BINARY_DIR}/src/ui_favicon_ico.h"
  "${CMAKE_CURRENT_BINARY_DIR}/src/ui_index_html.h"
  "${CMAKE_CURRENT_BINARY_DIR}/src/ui_index_css.h"
  "${CMAKE_CURRENT_BINARY_DIR}/src/ui_index_js.h"
)

if(COMMAND mysql_add_plugin)
  ################################################################################
  # In-Source Build
  ################################################################################

  # Must compile code as C++ because MySQL headers don't compile as plain C...
  set_source_files_properties(src/logger.c PROPERTIES LANGUAGE CXX)

  mysql_add_plugin(logger
    ${SOURCES}
    MODULE_ONLY
    MODULE_OUTPUT_NAME "logger"
  )
else()
  ################################################################################
  # Standalone Build
  ################################################################################

  set_property(GLOBAL PROPERTY USE_FOLDERS ON)

  # BUILD DEPENDENCIES
  #
  # Debian/Ubuntu:
  # apt-get install mariadb-server libmariadbclient-dev libmariadb-dev-compat
  #
  # Fedora (and probably CentOS):
  # dnf install mariadb-server mariadb-devel mariadb-connector-c-devel
  #
  # macOS:
  # brew install mariadb
  #
  # Windows and others:
  # Use the official MySQL/MariaDB installer for your operating system or build
  # the server from source code.

  find_path(MYSQL_INCLUDE_DIR
            mysql/plugin.h
            PATH_SUFFIXES mysql
                          mysql/server
                          mysql/server/mysql
                          mariadb
                          mariadb/server
                          mariadb/server/mysql
            HINTS "${MYSQL_INCLUDE_DIR}")
  if(MYSQL_INCLUDE_DIR)
    include(CheckCSourceCompiles)
    set(CMAKE_REQUIRED_INCLUDES "${MYSQL_INCLUDE_DIR}")
    check_c_source_compiles("
      #include <mysql/plugin.h>
      #include <mysql/plugin_audit.h>
      int main() {}
    " MYSQL_INCLUDES_OK)
    set(CMAKE_REQUIRED_INCLUDES "")
    if(MYSQL_INCLUDES_OK)
      set(MYSQL_INCLUDE_DIR_INTERNAL ${MYSQL_INCLUDE_DIR})
    else()
      message("Ignoring incomplete/broken MySQL plugin headers: ${MYSQL_INCLUDE_DIR}")
    endif()
  endif()
  if(NOT MYSQL_INCLUDE_DIR_INTERNAL)
    message(FATAL_ERROR "Could NOT find MySQL plugin headers")
  else()
    message("Found MySQL plugin headers: ${MYSQL_INCLUDE_DIR}")
  endif()

  include_directories(
    "${MYSQL_INCLUDE_DIR_INTERNAL}"
    "${MYSQL_INCLUDE_DIR_INTERNAL}/mysql"
  )

  add_definitions(-DMYSQL_DYNAMIC_PLUGIN)

  add_library(logger SHARED ${SOURCES})

  set_target_properties(logger PROPERTIES PREFIX "")
  if(NOT WIN32)
    # MySQL server doesn't support .dylib extension.
    set_target_properties(logger PROPERTIES SUFFIX ".so")
  endif()

  ################################################################################
  # Tests
  ################################################################################

  include(CTest)

  if(BUILD_TESTING)
    add_executable(logger_tests
      src/config.c
      src/http.c
      src/json.c
      src/socket_ext.c
      src/strbuf.c
      src/string_ext.c
      tests/all_tests.c
      tests/config_tests.c
      tests/config_tests.h
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
    target_include_directories(logger_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    if(WIN32)
      target_link_libraries(logger_tests ws2_32)
    endif()

    add_test(NAME run_logger_tests COMMAND $<TARGET_FILE:logger_tests>)
  endif()

  ################################################################################
  # Packaging
  ################################################################################

  install(FILES $<TARGET_FILE:logger> DESTINATION .)

  set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
  if(WIN32 OR APPLE)
    set(CPACK_GENERATOR ZIP)
  elseif(UNIX)
    set(CPACK_GENERATOR TGZ)
  endif()

  include(CPack)
endif()

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
