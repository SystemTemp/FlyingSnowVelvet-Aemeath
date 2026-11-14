cmake_minimum_required(VERSION 3.25)
project(AemeathPet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# 统一输出目录，方便直接打包。
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_library(aemeath_build_flags INTERFACE)
target_compile_features(aemeath_build_flags INTERFACE cxx_std_20)

if(MSVC)
    # 源码全部是 UTF-8（含中文注释与字面量）；/utf-8 让 cl 按 UTF-8 读源文件，
    # 否则在 cp936 系统上会报 C4819 / C3872。
    target_compile_options(aemeath_build_flags INTERFACE
        /utf-8
        /permissive-
        /W4
        /wd4127
        $<$<CONFIG:Release>:/O2>
    )
    target_compile_definitions(aemeath_build_flags INTERFACE
        UNICODE _UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
endif()

# ---------------------------------------------------------------------------
# 平台无关核心
# ---------------------------------------------------------------------------
add_library(AemeathPet.Core STATIC
    src/AemeathPet.Core/src/Pet/PetBehaviorStateMachine.cpp
    src/AemeathPet.Core/src/Pet/AnimationPlayer.cpp
    src/AemeathPet.Core/src/Pet/PetDirector.cpp
    src/AemeathPet.Core/src/Logging/Logger.cpp
    src/AemeathPet.Core/src/Platform/UserPaths.cpp
)
target_include_directories(AemeathPet.Core PUBLIC src/AemeathPet.Core/include)
target_link_libraries(AemeathPet.Core PUBLIC aemeath_build_flags)

# ---------------------------------------------------------------------------
# 服务接口（纯虚，无实现依赖）
# ---------------------------------------------------------------------------
add_library(AemeathPet.Services INTERFACE)
target_include_directories(AemeathPet.Services INTERFACE src/AemeathPet.Services/include)
target_link_libraries(AemeathPet.Services INTERFACE AemeathPet.Core aemeath_build_flags)

# ---------------------------------------------------------------------------
# 基础设施：HTTP、JSON、设置、聊天、论坛、宠物资源
# ---------------------------------------------------------------------------
add_library(AemeathPet.Infrastructure STATIC
    src/AemeathPet.Infrastructure/src/Json.cpp
    src/AemeathPet.Infrastructure/src/Config/JsonSettingsStore.cpp
    src/AemeathPet.Infrastructure/src/Http/WinHttpClient.cpp
    src/AemeathPet.Infrastructure/src/Http/MockHttpClient.cpp
    src/AemeathPet.Infrastructure/src/Chat/OpenAICompatibleChatService.cpp
    src/AemeathPet.Infrastructure/src/Chat/StreamingChatSession.cpp
    src/AemeathPet.Infrastructure/src/Chat/ChatProviderRegistry.cpp
    src/AemeathPet.Infrastructure/src/Forum/ForumApiClient.cpp
    src/AemeathPet.Infrastructure/src/Pet/PetAssetLoader.cpp
    src/AemeathPet.Infrastructure/src/Media/NullMediaServices.cpp
)
target_include_directories(AemeathPet.Infrastructure PUBLIC src/AemeathPet.Infrastructure/include)
target_link_libraries(AemeathPet.Infrastructure
    PUBLIC AemeathPet.Core AemeathPet.Services aemeath_build_flags
)
if(WIN32)
    target_link_libraries(AemeathPet.Infrastructure PRIVATE
        winhttp winmm ole32 oleaut32 uuid shlwapi version
    )
endif()

# ---------------------------------------------------------------------------
# 应用编排层
# ---------------------------------------------------------------------------
add_library(AemeathPet.Application STATIC
    src/AemeathPet.Application/src/AppController.cpp
)
target_include_directories(AemeathPet.Application PUBLIC src/AemeathPet.Application/include)
target_link_libraries(AemeathPet.Application
    PUBLIC AemeathPet.Core AemeathPet.Services AemeathPet.Infrastructure aemeath_build_flags
)

# ---------------------------------------------------------------------------
# 可运行的 Win32 桌宠宿主（Direct2D 自绘，无 WindowsAppSDK 依赖）
# ---------------------------------------------------------------------------
if(WIN32)
    add_executable(AemeathPet.Host WIN32
        src/AemeathPet.Host/main.cpp
        src/AemeathPet.Host/PetHostWindow.cpp
        src/AemeathPet.Host/PetRenderer.cpp
        src/AemeathPet.Host/TrayIcon.cpp
        src/AemeathPet.Host/SettingsDialog.cpp
        src/AemeathPet.Host/ChatDialog.cpp
    )
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/AemeathPet.Host/HostResources.rc")
        target_sources(AemeathPet.Host PRIVATE src/AemeathPet.Host/HostResources.rc)
    endif()
    target_include_directories(AemeathPet.Host PRIVATE src/AemeathPet.Host)
    target_link_libraries(AemeathPet.Host PRIVATE
        AemeathPet.Core AemeathPet.Services AemeathPet.Infrastructure AemeathPet.Application
        aemeath_build_flags
        d2d1 dwrite windowscodecs comctl32 shell32 user32 gdi32 ole32 uuid shlwapi
    )
    set_target_properties(AemeathPet.Host PROPERTIES
        OUTPUT_NAME "AemeathPet"
    )
endif()

# ---------------------------------------------------------------------------
# 测试
# ---------------------------------------------------------------------------
add_executable(AemeathPet.Tests
    src/AemeathPet.Tests/TestMain.cpp
    src/AemeathPet.Tests/EventBusTests.cpp
    src/AemeathPet.Tests/JsonTests.cpp
    src/AemeathPet.Tests/SettingsTests.cpp
    src/AemeathPet.Tests/HttpTests.cpp
    src/AemeathPet.Tests/AnimationTests.cpp
    src/AemeathPet.Tests/ChatTests.cpp
    src/AemeathPet.Tests/ForumTests.cpp
)
target_link_libraries(AemeathPet.Tests PRIVATE
    AemeathPet.Core AemeathPet.Services AemeathPet.Infrastructure AemeathPet.Application
    aemeath_build_flags
)

enable_testing()
add_test(NAME AemeathPet.Tests COMMAND AemeathPet.Tests)
