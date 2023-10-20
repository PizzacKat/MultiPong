cmake_minimum_required(VERSION 3.22)
project(MultiPong)

function(copy_files_recursive target source_dir target_dir)
    file(GLOB_RECURSE files RELATIVE ${source_dir} ${source_dir}/*)
    foreach(file ${files})
        add_custom_command(
                TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${source_dir}/${file}"
                "${target_dir}/${file}"
        )
    endforeach()
endfunction()

set(CMAKE_CXX_STANDARD 20)

include(CPM.cmake)

CPMAddPackage("gh:SFML/SFML#2.6.0")

include_directories(include)

add_executable(Server server.cpp)
add_executable(Game game.cpp)

copy_files_recursive(
        Game
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:Game>/assets
)

target_link_libraries(Game sfml-graphics sfml-window sfml-system)
target_include_directories(Game PUBLIC "${SFML_SOURCE_DIR}/include")