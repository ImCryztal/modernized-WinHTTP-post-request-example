//21.04.21 by Cryztal
#include <iostream>
#include <Windows.h>
#include <winhttp.h>
#include <memory>
#include <system_error>

/// Basic POST request using WinHTTP, heavily based off: https://docs.microsoft.com/en-us/windows/win32/api/winhttp/nf-winhttp-winhttpreaddata

/// WinHTTP errors don't seem to be handled very well. Neither FormatMessage nor std::system_category().message(GetLastError()) gives a good error message.
/// TODO: Improve error handling, initially planned on calling GetLastError() in catch, but that doesn't work. (ret = 0)
/// TODO 2: Quit C++

int main() {

    const auto url = std::wstring(L"http://localhost/");

    try {
        //how to name this?
        using raii_uptr_t = std::unique_ptr<std::remove_pointer<HINTERNET>::type, decltype(&WinHttpCloseHandle)>;

        /// Use WinHttpOpen to obtain a session handle.
        const auto session = raii_uptr_t(
                WinHttpOpen(
                        L"A WinHTTP Example Program/1.0",
                        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                        WINHTTP_NO_PROXY_NAME,
                        WINHTTP_NO_PROXY_BYPASS, 0),
                &::WinHttpCloseHandle);

        if (!session)
            throw std::runtime_error(
                    "Failed to obtain session handle -> " + std::system_category().message(GetLastError()));

        /// Crack URL
        URL_COMPONENTS components{};
        components.dwStructSize = sizeof(components);
        components.dwHostNameLength = (DWORD) -1;
        components.dwUrlPathLength = (DWORD) -1;

        if (!WinHttpCrackUrl(url.c_str(), static_cast<DWORD>(url.length()), 0, &components))
            throw std::runtime_error("Failed to crack url -> " + std::system_category().message(GetLastError()));

        /// if components.lpszHostName is null, we default to localhost
        const auto hostname = (!components.lpszHostName) ? L"localhost" : std::wstring(components.lpszHostName,
                                                                                       components.dwHostNameLength);

        /// Specify an HTTP server.
        const auto connect = raii_uptr_t(
                WinHttpConnect(
                        session.get(),
                        hostname.c_str(),
                        components.nPort, 0),
                &::WinHttpCloseHandle);

        if (!connect)
            throw std::runtime_error(
                    "Failed to specify HTTP target -> " + std::system_category().message(GetLastError()));

        /// Create an HTTP request handle.
        const auto request = raii_uptr_t(
                WinHttpOpenRequest(
                        connect.get(),
                        L"POST",
                        components.lpszUrlPath,
                        nullptr,
                        WINHTTP_NO_REFERER,
                        WINHTTP_DEFAULT_ACCEPT_TYPES,
                        0),
                &::WinHttpCloseHandle);

        /// Prepare request
        const auto content_type = L"Content-Type: application/x-www-form-urlencoded";
        const auto body = std::string("arg1=1&arg2=2");

        /// Send request
        if (!WinHttpSendRequest(
                request.get(),
                content_type,
                wcslen(content_type),
                (LPVOID) (body.data()),
                body.length(),
                body.length(),
                0))
            throw std::runtime_error(
                    "Failed to send HTTP request  -> " + std::system_category().message(GetLastError()));

        /// End request by receiving response
        if (!WinHttpReceiveResponse(request.get(), nullptr))
            throw std::runtime_error(
                    "Failed to receive HTTP response -> " + std::system_category().message(GetLastError()));

        auto response_size = 0u, bytes_read = 0u;
        std::string out_buffer;

        do {
            response_size = 0u;

            /// Check for available data.
            if (!WinHttpQueryDataAvailable(request.get(), &response_size))
                throw std::runtime_error(
                        "Failed to query response -> " + std::system_category().message(GetLastError()));

            /// No more data
            if (!response_size)
                break;

            // is this the best way?
            out_buffer.resize(response_size + 1ul);
            ZeroMemory(out_buffer.data(), response_size + 1);

            if (!WinHttpReadData(request.get(), (LPVOID) (out_buffer.data()),
                                 response_size, &bytes_read))
                throw std::runtime_error(
                        "Failed to read response data -> " + std::system_category().message(GetLastError()));

            std::cout << "Response: " << out_buffer << std::endl;

            // This condition should never be reached since WinHttpQueryDataAvailable
            // reported that there are bits to read.
            if (!bytes_read)
                break;

        } while (response_size);
    }
    catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
