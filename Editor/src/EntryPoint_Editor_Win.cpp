#ifndef _WIN32
static_assert(false);
#endif //_WIN32

#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>
#include <thread>

#include <fmt/format.h>
#include <GLFW/glfw3.h>

#include <Enterprise/Core/Assertions.h>
#include <Enterprise/Core/PlatformHelpers_Win.h>
#include <Enterprise/Core/PlatformData_Win.h>

#include <Editor/EditorReloadFlags.h>
#include "ReloadSentinel_Win.h"

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <DbgHelp.h>

namespace fs = std::filesystem;

static void OnGlfwError(int error, const char* description)
{
    fmt::print(stderr, "GLFW error {}: {}\n", error, description);
}

static bool InitSymbolHandler(bool isDevelopmentMode)
{
    auto platformData = Enterprise::GetMutablePlatformData_Win();

    if (!DuplicateHandle(GetCurrentProcess(),
                         GetCurrentProcess(),
                         GetCurrentProcess(),
                         &(platformData->processHandle),
                         0,
                         FALSE,
                         DUPLICATE_SAME_ACCESS))
    {
        fmt::print(stderr,
                   "Could not obtain editor process handle! {}\n",
                   GetLastErrorAsString());
    }

    bool isSymbolHandlerInitialized = false;
    if (isDevelopmentMode)
    {
#ifdef ENTERPRISE_DEBUG
        std::string symbolFileSearchPath = fs::canonical("build\\Debug\\").string();
#elif defined(ENTERPRISE_DEV)
        std::string symbolFileSearchPath = fs::canonical("build\\Dev\\").string();
#elif defined(ENTERPRISE_RELEASE)
        std::string symbolFileSearchPath = fs::canonical("build\\Release\\").string();
#else
        static_assert(false);
#endif // ENTERPRISE_DEBUG

        isSymbolHandlerInitialized = SymInitialize(
            platformData->processHandle, symbolFileSearchPath.c_str(), TRUE);
    }
    else
    {
        isSymbolHandlerInitialized =
            SymInitialize(platformData->processHandle, NULL, TRUE);
    }

    if (!isSymbolHandlerInitialized)
    {
        fmt::print(
            stderr,
            "SymInitialize() failed! {}\nBacktraces will be unavailable this session\n",
            GetLastErrorAsString());
    }

    return isSymbolHandlerInitialized;
}

static void CleanUpSymbolHandler(const bool isSymbolHandlerInitialized)
{
    auto platformData = Enterprise::GetMutablePlatformData_Win();

    if (isSymbolHandlerInitialized && !SymCleanup(platformData->processHandle))
        fmt::print(stderr, "SymCleanup() failed! {}\n", GetLastErrorAsString());

    if (platformData->processHandle && !CloseHandle(platformData->processHandle))
        fmt::print(stderr,
                   "Could not close editor process handle! {}\n",
                   GetLastErrorAsString());

    platformData->processHandle = NULL;
}

static HANDLE StartReloadWatchThread(std::atomic_uchar* reloadFlagsOutPtr)
{
    HANDLE reloadWatchThread = CreateThread(
        NULL, 0, &WaitForEditorOrEngineRecompile, reloadFlagsOutPtr, 0, NULL);

    if (reloadWatchThread == NULL)
    {
        fmt::print(stderr,
                   "Error creating editor reload watch thread! {}\n",
                   GetLastErrorAsString());
    }

    return reloadWatchThread;
}

static void JoinReloadWatchThread(HANDLE reloadWatchThread)
{
    if (reloadWatchThread == NULL)
        return;

    if (WaitForSingleObject(reloadWatchThread, 0) != WAIT_OBJECT_0 &&
        CancelSynchronousIo(reloadWatchThread) == 0)
    {
        fmt::print(stderr,
                   "Error cancelling editor reload watch thread! {}\n",
                   GetLastErrorAsString());
    }

    switch (WaitForSingleObject(reloadWatchThread, 5000))
    {
    case WAIT_OBJECT_0: break;
    case WAIT_TIMEOUT:
        fmt::print(stderr, "Editor reload watch thread timed out during join!\n");
        break;
    case WAIT_FAILED:
        fmt::print(stderr,
                   "Error joining editor reload watch thread! {}\n",
                   GetLastErrorAsString());
        break;
    default: ASSERT_NOENTRY(); break;
    }

    if (CloseHandle(reloadWatchThread) == 0)
    {
        fmt::print(stderr,
                   "Error closing editor reload watch thread handle! {}\n",
                   GetLastErrorAsString());
    }
}

extern "C" __declspec(dllexport) unsigned char EditorMain(int argc, char* argv[])
{
    const char* editorReloadableFlag = "--development";
    bool isDevelopmentMode           = false;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], editorReloadableFlag) == 0)
        {
            isDevelopmentMode = true;
            break;
        }
    }

    bool isSymbolHandlerInitialized = InitSymbolHandler(isDevelopmentMode);

    HANDLE reloadWatchThread      = NULL;
    std::atomic_uchar reloadFlags = EditorReloadFlag_None;
    if (isDevelopmentMode)
        reloadWatchThread = StartReloadWatchThread(&reloadFlags);

    glfwSetErrorCallback(OnGlfwError);

    if (!glfwInit())
    {
        JoinReloadWatchThread(reloadWatchThread);
        CleanUpSymbolHandler(isSymbolHandlerInitialized);
        return EditorReloadFlag_None;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* mainWindowPtr =
        glfwCreateWindow(1024, 768, "Window Title", nullptr, nullptr);
    if (!mainWindowPtr)
    {
        glfwTerminate();
        JoinReloadWatchThread(reloadWatchThread);
        CleanUpSymbolHandler(isSymbolHandlerInitialized);
        return EditorReloadFlag_None;
    }

    while (!glfwWindowShouldClose(mainWindowPtr))
    {
        glfwWaitEvents();

        if (reloadFlags != EditorReloadFlag_None)
        {
            // Dump editor state here
            break;
        }
    }

    glfwTerminate();
    JoinReloadWatchThread(reloadWatchThread);
    CleanUpSymbolHandler(isSymbolHandlerInitialized);
    return reloadFlags;
}
