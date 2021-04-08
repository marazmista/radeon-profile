set(TS_FILES
    strings.it.ts
    strings.pl.ts
    strings.hr.ts
    strings.ru.ts
)

# Generate a list of input files
set(LST_FILE "${CMAKE_CURRENT_BINARY_DIR}/sources.txt")
file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/*.h" "${CMAKE_SOURCE_DIR}/*.cpp" "${CMAKE_SOURCE_DIR}/*.ui")
string(REPLACE ";" "\n" LST_SOURCES "${SOURCES}")
file(GENERATE
    OUTPUT   ${LST_FILE}
    CONTENT "${LST_SOURCES}"
)

foreach(TS ${TS_FILES})
    set(TS "${CMAKE_CURRENT_SOURCE_DIR}/${TS}")
    list(APPEND TS_FILES_ABS "${TS}")
    # Add a command to update ts files
    add_custom_command(
        OUTPUT  ${TS}
        COMMAND Qt5::lupdate
        ARGS    -no-obsolete -silent "@${LST_FILE}" -ts ${TS}
        DEPENDS ${SOURCES}
    )
    # Add a command to compile qm files
    get_filename_component(QM ${TS} NAME_WLE)
    set(QM ${QM}.qm)
    list(APPEND QM_FILES ${QM})
    list(APPEND QM_FILES_ABS "${CMAKE_CURRENT_BINARY_DIR}/${QM}")
    add_custom_command(
        OUTPUT          ${QM}
        COMMAND         Qt5::lrelease
        ARGS            -silent ${TS} -qm ${QM}
        MAIN_DEPENDENCY ${TS}
    )
endforeach()

# Install qm files
install(
    FILES       ${QM_FILES_ABS}
    DESTINATION share/${PROJECT_NAME}
)

add_custom_target(translations
    ALL
    DEPENDS ${QM_FILES}
)
