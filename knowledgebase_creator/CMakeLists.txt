cmake_minimum_required(VERSION 2.8.3)
project(knowledgebase_creator)

find_package(catkin REQUIRED qt_gui asp_commons asp_solver)

find_package(Qt5Core REQUIRED)
get_target_property(Qt5Core_location Qt5::Core LOCATION)
find_package(Qt5Gui REQUIRED)
get_target_property(Qt5Gui_location Qt5::Gui LOCATION)
find_package(Qt5Network REQUIRED)
get_target_property(Qt5Network_location Qt5::Network LOCATION)
find_package(Qt5Widgets REQUIRED)
get_target_property(Qt5Widgets_location Qt5::Widgets LOCATION)
find_package(Qt5WebKitWidgets REQUIRED)
get_target_property(Qt5WebKitWidgets_location Qt5::WebKitWidgets LOCATION)
find_package(Qt5Script REQUIRED)
get_target_property(Qt5Script_location Qt5::Script LOCATION)
SET(QT_USE_QTXML TRUE)
SET(QT_WRAP_CPP TRUE)

## Use c++ 11x std
set(CMAKE_CXX_FLAGS "-std=c++11 -fPIC ${CMAKE_CXX_FLAGS}")
if (${CMAKE_EXTRA_GENERATOR} MATCHES "Eclipse CDT4")
    set(CMAKE_CXX_COMPILER_ARG1 "-std=c++11" CACHE STRING "C++ version for eclipse" FORCE)
    set(CMAKE_ECLIPSE_VERSION "4.5" CACHE STRING "Eclipse version" FORCE)
    add_definitions(-DCMAKE_ECLIPSE_GENERATE_SOURCE_PROJECT=TRUE)
endif (${CMAKE_EXTRA_GENERATOR} MATCHES "Eclipse CDT4")

set(knowledgebaseCreator_SRCS
        src/main.cpp
        src/gui/KnowledgebaseCreator.cpp
        src/gui/SettingsDialog.cpp
        src/gui/ModelSettingDialog.cpp
        src/containers/SolverSettings.cpp
        src/containers/ConceptNetCall.cpp
        src/containers/ConceptNetEdge.cpp
        src/containers/ConceptNetConcept.cpp
        src/commands/NewSolverCommand.cpp
        src/commands/ChangeSolverSettingsCommand.cpp
        src/commands/ConceptNetQueryCommand.cpp
        src/commands/FactsQueryCommand.cpp
        src/commands/GroundCommand.cpp
        src/commands/LoadSavedProgramCommand.cpp
        src/commands/SolveCommand.cpp
        src/commands/VariableQueryCommand.cpp
        src/commands/LoadBackgroundKnowledgeCommand.cpp
        src/commands/AddCommand.cpp
        src/commands/AssignExternalCommand.cpp
        src/handler/SaveLoadHandler.cpp
        src/handler/CommandHistoryHandler.cpp
        src/handler/ExternalTableHandler.cpp
        src/commands/OfferServicesCommand.cpp
        )

set(knowledgebaseCreator_HDRS
        include/gui/KnowledgebaseCreator.h
        include/gui/SettingsDialog.h
        include/gui/ModelSettingDialog.h
        include/containers/SolverSettings.h
        include/containers/ConceptNetCall.h
        include/containers/ConceptNetEdge.h
        include/containers/ConceptNetConcept.h
        include/commands/Command.h
        include/commands/NewSolverCommand.h
        include/commands/ChangeSolverSettingsCommand.h
        include/commands/ConceptNetQueryCommand.h
        include/commands/FactsQueryCommand.h
        include/commands/GroundCommand.h
        include/commands/LoadSavedProgramCommand.h
        include/commands/SolveCommand.h
        include/commands/VariableQueryCommand.h
        include/commands/LoadBackgroundKnowledgeCommand.h
        include/commands/AddCommand.h
        include/commands/AssignExternalCommand.h
        include/commands/OfferServicesCommand.h
        include/handler/SaveLoadHandler.h
        include/handler/CommandHistoryHandler.h
        include/handler/ExternalTableHandler.h
        )

set(knowledgebaseCreator_UIS
        ui/knowledgebasecreator.ui
        ui/settingsdialog.ui
        ui/modelsettings.ui
        )

set(knowledgebaseCreator_INCLUDE_DIRECTORIES
        include
        ${Qt5Core_INCLUDE_DIRS}
        ${Qt5Gui_INCLUDE_DIRS}
        ${Qt5Network_INCLUDE_DIRS}
        ${Qt5Widgets_INCLUDE_DIRS}
        ${Qt5WebKitWidgets_INCLUDE_DIRS}
        "${CATKIN_DEVEL_PREFIX}/${CATKIN_GLOBAL_INCLUDE_DESTINATION}"
        )

if (NOT EXISTS "${CATKIN_DEVEL_PREFIX}/${CATKIN_GLOBAL_INCLUDE_DESTINATION}")
    file(MAKE_DIRECTORY "${CATKIN_DEVEL_PREFIX}/${CATKIN_GLOBAL_INCLUDE_DESTINATION}")
endif ()

include_directories(${knowledgebaseCreator_INCLUDE_DIRECTORIES} ${catkin_INCLUDE_DIRS})

catkin_package(
        INCLUDE_DIRS ${knowledgebaseCreator_INCLUDE_DIRECTORIES}
        LIBRARIES ${PROJECT_NAME}
        CATKIN_DEPENDS qt_gui asp_commons asp_solver

)

file(GLOB QT_RESOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} resources/*.qrc)

QT5_ADD_RESOURCES(QT_RESOURCES_CPP ${QT_RESOURCES})
QT5_WRAP_CPP(knowledgebaseCreator_MOCS ${knowledgebaseCreator_HDRS})
# ensure generated header files are being created in the devel space
set(_cmake_current_binary_dir "${CMAKE_CURRENT_BINARY_DIR}")
set(CMAKE_CURRENT_BINARY_DIR "${CATKIN_DEVEL_PREFIX}/${CATKIN_GLOBAL_INCLUDE_DESTINATION}")
QT5_WRAP_UI(knowledgebaseCreator_UIS_H ${knowledgebaseCreator_UIS})
set(CMAKE_CURRENT_BINARY_DIR "${_cmake_current_binary_dir}")

add_executable(${PROJECT_NAME} ${knowledgebaseCreator_SRCS} ${knowledgebaseCreator_MOCS} ${knowledgebaseCreator_UIS_H} ${QT_RESOURCES_CPP})
target_link_libraries(${PROJECT_NAME} ${catkin_LIBRARIES} ${Qt5Widgets_location} ${Qt5Core_location} ${Qt5Gui_location} ${Qt5Network_location} ${Qt5Script_location} ${Qt5WebKitWidgets_location} clingo)
qt5_use_modules(${PROJECT_NAME} Core Gui Network Widgets WebKitWidgets Script)

find_package(class_loader)
class_loader_hide_library_symbols(${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME}
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_GLOBAL_BIN_DESTINATION}
        )

install(DIRECTORY include/${PROJECT_NAME}/
        DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
        )
