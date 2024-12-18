#include "PlatformHelpers_Win.h"

#ifndef ENTERPRISE_WINDOWS
static_assert(false);
#endif

#include <string>
#include <string_view>

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace Misc::Win
{

std::string GetLastErrorMessage()
{
    DWORD errorCode = GetLastError();

    LPVOID messageBuffer = nullptr;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  errorCode,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR)&messageBuffer,
                  0,
                  NULL);

    std::string formattedMessage = std::to_string(errorCode) + ", " + ToUtf8((LPTSTR)messageBuffer);

    LocalFree(messageBuffer);

    return formattedMessage;
}

std::string ToUtf8(const WCHAR* wideString, size_t length)
{
    if (length == 0)
        length = wcslen(wideString);

    if (length == 0)
        return std::string();

    std::string convertedString(WideCharToMultiByte(CP_UTF8, 0, wideString, (int)length, NULL, 0, NULL, NULL), 0);

    WideCharToMultiByte(
        CP_UTF8, 0, wideString, (int)length, &convertedString[0], (int)convertedString.size(), NULL, NULL);

    return convertedString;
}

std::wstring ToWchar(const std::string_view narrowString)
{
    if (narrowString.length() == 0)
        return std::wstring();

    std::wstring convertedString(MultiByteToWideChar(CP_UTF8, 0, narrowString.data(), -1, NULL, 0), 0);

    MultiByteToWideChar(CP_UTF8, 0, narrowString.data(), -1, convertedString.data(), (int)convertedString.size());

    return convertedString;
}

} // namespace Misc::Win
