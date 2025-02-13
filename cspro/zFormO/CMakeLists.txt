project(zFormO)

include(../WASM/Definitions.cmake)

add_library(${PROJECT_NAME}
    Block.cpp
    Box.cpp
    Definitions.cpp
    DragOptions.cpp
    Field.cpp
    FieldColors.cpp
    Form.cpp
    FormBase.cpp
    FormFile.cpp
    Group.cpp
    ItemBase.cpp
    Level.cpp
    RepeatedItemSerializer.cpp
    Roster.cpp
    RosterCells.cpp
    RosterColumn.cpp
    Text.cpp
)
