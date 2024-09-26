# Icinga 2 | (c) 2024 Icinga GmbH | GPLv2+
#
# Determines the current Icinga 2 git version similar to a "git describe" command, but in a more reliable way.
function(git_get_version _var)
  if (NOT GIT_FOUND)
    find_package(Git QUIET)
    if (NOT GIT_FOUND)
      set(${_var} "GIT-NOTFOUND" PARENT_SCOPE)
      return()
    endif ()
  endif ()

  # Determine the current git HEAD commit in "git rev-parse --show-superproject-working-tree --short @" manner.
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" rev-parse --show-superproject-working-tree --short @
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RESULT_VARIABLE exitcode
    OUTPUT_VARIABLE hash
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
  if (NOT exitcode EQUAL 0)
    set(${_var} "HEAD-HASH-NOTFOUND" PARENT_SCOPE)
    return()
  endif ()

  execute_process(
    COMMAND "${GIT_EXECUTABLE}" -c versionsort.suffix=- tag --list --sort=-version:refname
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RESULT_VARIABLE exitcode
    OUTPUT_VARIABLE gittags
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
  if (NOT exitcode EQUAL 0)
    set(${_var} "${exitcode}-TAGS-NOTFOUND" PARENT_SCOPE)
    return()
  endif ()

  # We're only interested in the first/latest tag
  string(REGEX REPLACE "\n.*" "" gittag "${gittags}")
  string(STRIP "${gittag}" gittag)

  execute_process(
    COMMAND "${GIT_EXECUTABLE}" rev-list "${gittag}.." --count
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RESULT_VARIABLE exitcode
    OUTPUT_VARIABLE commits
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
  if (NOT exitcode EQUAL 0)
    # Something went wrong so just return the latest tag
    set(${_var} "${gittag}" PARENT_SCOPE)
    return()
  endif ()

  set(${_var} "${gittag}-${commits}-g${hash}" PARENT_SCOPE)
endfunction()
