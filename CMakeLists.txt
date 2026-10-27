cmake_minimum_required(VERSION 3.20)

# Product project root: owns the game module DLL, the standalone game host and
# the editor host, and pulls the engine tree through ENGINE_ROOT.
project(minGameplayFramework LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ENGINE_ROOT "D:/Dev/GitRepo/minEngine" CACHE PATH
    "minEngine repository root (contains the minEngine/ CMake project)")
if(NOT EXISTS "${ENGINE_ROOT}/minEngine/CMakeLists.txt")
    message(FATAL_ERROR
        "ENGINE_ROOT must point at the minEngine repository root. "
        "Missing: ${ENGINE_ROOT}/minEngine/CMakeLists.txt")
endif()

# Pin the Debug artifact suffix before any target exists: the assimp subproject
# (configured while the engine tree is added) writes CMAKE_DEBUG_POSTFIX into the
# cache, so targets created before it and after it would otherwise disagree about
# the import-library name and the editor DLL would link a file that is never built.
set(CMAKE_DEBUG_POSTFIX "" CACHE STRING "Debug artifact suffix; kept empty so Debug output names match Release" FORCE)

# Product-local bin/; the engine's own sample hosts and test suite stay off.
set(MINENGINE_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin" CACHE PATH "" FORCE)
file(MAKE_DIRECTORY "${MINENGINE_OUTPUT_DIR}")
set(BUILD_GAME_MODULE_SAMPLE OFF CACHE BOOL "" FORCE)
set(BUILD_PLAYGROUND OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)

# This project's own doctest executable; independent of the engine suite above.
option(MGF_BUILD_TESTS "Build MGF_Tests (doctest) and register it with CTest" ON)

add_subdirectory("${ENGINE_ROOT}/minEngine" "${CMAKE_BINARY_DIR}/engine")

# Parent scope may not inherit non-CACHE vars from the engine tree; prefer CACHE / output dir.
if(NOT DEFINED EXECUTABLE_PATH OR "${EXECUTABLE_PATH}" STREQUAL "")
    set(EXECUTABLE_PATH "${MINENGINE_OUTPUT_DIR}")
endif()

if(NOT EXISTS "${MINENGINE_CMAKE_DIR}/MinEngineTargets.cmake")
    message(FATAL_ERROR "MinEngineTargets.cmake not found under engine cmake/")
endif()
include("${MINENGINE_CMAKE_DIR}/MinEngineTargets.cmake")
include("${MINENGINE_CMAKE_DIR}/MinEngineReflection.cmake")

set(GAME_MODULE_NAME minGameplayFramework)
set(GAME_MODULE_SRC_ROOT "${CMAKE_SOURCE_DIR}/Source/${GAME_MODULE_NAME}")
set(GAME_MODULE_GENERATED_DIR "${CMAKE_SOURCE_DIR}/Generated/Reflection")

# Reflection headers are discovered recursively under SRC_ROOT, so every file
# with ME_CLASS/ME_STRUCT metadata belongs in Source/minGameplayFramework.
add_library(${GAME_MODULE_NAME} SHARED
    "${GAME_MODULE_SRC_ROOT}/Module.cpp"
    "${GAME_MODULE_SRC_ROOT}/GameInstance/MinGameInstance.cpp"
    "${GAME_MODULE_SRC_ROOT}/GameplayContent/GameplayContentDef.cpp"
    "${GAME_MODULE_SRC_ROOT}/GameplayContent/GameplayContentRegistry.cpp"
    "${GAME_MODULE_SRC_ROOT}/GameplayContent/InventoryItem/ItemInstance.cpp"
    "${GAME_MODULE_SRC_ROOT}/GameplayContent/InventoryItem/ItemDefFragments/ExampleItemDefFragments.cpp"
    "${GAME_MODULE_SRC_ROOT}/Inventory/Examples/SlotBasedInventories/SlotBasedInventory.cpp"
    "${GAME_MODULE_SRC_ROOT}/Inventory/Examples/CategoryBasedInventories/CategoryBasedInventory.cpp"
    "${GAME_MODULE_SRC_ROOT}/Inventory/Examples/GridBasedInventories/GridBaseInventory.cpp"
)
set_target_properties(${GAME_MODULE_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${EXECUTABLE_PATH}"
    LIBRARY_OUTPUT_DIRECTORY "${EXECUTABLE_PATH}"
    ARCHIVE_OUTPUT_DIRECTORY "${EXECUTABLE_PATH}"
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)
if(MINGW)
    target_link_options(${GAME_MODULE_NAME} PRIVATE "LINKER:--export-all-symbols")
endif()
target_compile_definitions(${GAME_MODULE_NAME} PRIVATE MINGAMEPLAYFRAMEWORK_BUILD=1)
target_include_directories(${GAME_MODULE_NAME} PUBLIC "${GAME_MODULE_SRC_ROOT}")
target_link_libraries(${GAME_MODULE_NAME} PRIVATE minEngine)

minengine_add_reflection_codegen(${GAME_MODULE_NAME}
    SRC_ROOT "${GAME_MODULE_SRC_ROOT}"
    OUT_DIR "${GAME_MODULE_GENERATED_DIR}"
    MANIFEST "${GAME_MODULE_GENERATED_DIR}/manifest.json"
    CODEGEN_TARGET_NAME ${GAME_MODULE_NAME}_reflection_codegen
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# Target name must differ from the SHARED module target: the Windows filesystem
# is case-insensitive and would resolve both to the same artifact name.
minengine_add_game_executable(minGameplayFrameworkGame
    SOURCES "${CMAKE_SOURCE_DIR}/Source/Programs/GameMain.cpp"
    GAME_MODULE ${GAME_MODULE_NAME}
    OUTPUT_NAME minGameplayFramework
)

minengine_add_editor_executable(minGameplayFrameworkEditor
    MAIN_SOURCE "${ENGINE_ROOT}/minEngine/Editor/src/EditorMain.cpp"
    GAME_MODULE ${GAME_MODULE_NAME}
    OUTPUT_NAME minGameplayFrameworkEditor
)

# Both host executables run with cwd=bin, where PathRegistry looks for a sibling
# EngineConfig.meconfig; it points the engine default assets at the engine tree.
set(MGF_ENGINE_DEFAULT_ASSETS "${ENGINE_ROOT}/minEngine/Assets/EngineDefault")
if(IS_DIRECTORY "${MGF_ENGINE_DEFAULT_ASSETS}")
    file(RELATIVE_PATH MGF_ENGINE_DEFAULT_ASSETS_REL
        "${MINENGINE_OUTPUT_DIR}"
        "${MGF_ENGINE_DEFAULT_ASSETS}")
    string(REPLACE "\\" "/" MGF_ENGINE_DEFAULT_ASSETS_REL "${MGF_ENGINE_DEFAULT_ASSETS_REL}")
    file(WRITE "${MINENGINE_OUTPUT_DIR}/EngineConfig.meconfig"
"{
    \"$schemaVersion\": 1,
    \"$engineVersion\": \"0.0.9\",
    \"EngineDefaultAssetsRoot\": \"${MGF_ENGINE_DEFAULT_ASSETS_REL}\"
}
")
    message(STATUS "minGameplayFramework: wrote bin/EngineConfig.meconfig -> ${MGF_ENGINE_DEFAULT_ASSETS_REL}")
else()
    message(WARNING
        "minGameplayFramework: EngineDefault assets missing at ${MGF_ENGINE_DEFAULT_ASSETS}; "
        "bin/EngineConfig.meconfig was not written and editor built-in assets will not resolve")
endif()

minengine_add_package_game(minGameplayFrameworkGame
    STAGE_DIR "${CMAKE_SOURCE_DIR}/Staging/minGameplayFramework"
    CONTENT_DIR "${CMAKE_SOURCE_DIR}/Assets"
    # minengine_glfw = glfw3.dll, imported by both libminEngine.dll and the editor DLL;
    # a staged project without it fails to start.
    RUNTIME_TARGETS ${GAME_MODULE_NAME} minEngine minengine_glfw
)

if(MGF_BUILD_TESTS)
    set(MGF_DOCTEST_INCLUDE_DIR "")
    if(EXISTS "${ENGINE_ROOT}/minEngine/Third-Party/doctest/doctest.h")
        set(MGF_DOCTEST_INCLUDE_DIR "${ENGINE_ROOT}/minEngine/Third-Party/doctest")
    elseif(EXISTS "${ENGINE_ROOT}/minEngine/minEngine/Third-Party/doctest/doctest.h")
        set(MGF_DOCTEST_INCLUDE_DIR "${ENGINE_ROOT}/minEngine/minEngine/Third-Party/doctest")
    endif()
    if(NOT MGF_DOCTEST_INCLUDE_DIR)
        message(FATAL_ERROR
            "MGF_BUILD_TESTS=ON but doctest.h was not found under ${ENGINE_ROOT}; "
            "pass -DMGF_BUILD_TESTS=OFF to build without tests")
    endif()

    add_executable(MGF_Tests
        "${CMAKE_SOURCE_DIR}/tests/Main.cpp"
    )
    target_include_directories(MGF_Tests PRIVATE
        "${MGF_DOCTEST_INCLUDE_DIR}"
        "${CMAKE_SOURCE_DIR}/tests"
        "${GAME_MODULE_SRC_ROOT}"
    )
    target_link_libraries(MGF_Tests PRIVATE ${GAME_MODULE_NAME} minEngine)
    set_target_properties(MGF_Tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${EXECUTABLE_PATH}"
    )
    if(MINGW)
        target_link_options(MGF_Tests PRIVATE "LINKER:--export-all-symbols")
    endif()

    enable_testing()
    add_test(NAME MGF_Tests COMMAND MGF_Tests)
endif()

message(STATUS "minGameplayFramework: dual host (game + editor), game module SHARED, tests ${MGF_BUILD_TESTS}")
