#include "httpclient.h"

#include <Windows.h>

#pragma comment(lib, "winhttp.lib")
#include <memory>
#include <winhttp.h>

static HINTERNET hSession = nullptr;
static std::mutex sessionMutex;

struct AsyncRequestContext
{
    std::function<void(const std::string &)> callback;
    std::string response;
    HINTERNET hConnect = nullptr;
    HINTERNET hRequest = nullptr;
    std::unique_ptr<char[]> buffer;
};

static void CALLBACK AsyncCallback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength)
{
    AsyncRequestContext *context = reinterpret_cast<AsyncRequestContext *>(dwContext);
    if (!context)
        return;

    switch (dwInternetStatus)
    {
    case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
        WinHttpReceiveResponse(context->hRequest, nullptr);
        break;
    case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
        WinHttpQueryDataAvailable(context->hRequest, nullptr);
        break;
    case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE:
    {
        DWORD bytesAvailable = *reinterpret_cast<DWORD *>(lpvStatusInformation);
        if (bytesAvailable > 0)
        {
            context->buffer = std::make_unique<char[]>(bytesAvailable);
            WinHttpReadData(context->hRequest, context->buffer.get(), bytesAvailable, nullptr);
        }
        else
        {
            context->callback(context->response);
            WinHttpCloseHandle(context->hRequest);
            WinHttpCloseHandle(context->hConnect);
            delete context;
        }
        break;
    }
    case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
    {
        DWORD bytesRead = dwStatusInformationLength;
        if (bytesRead > 0)
        {
            context->response.append(context->buffer.get(), bytesRead);
            WinHttpQueryDataAvailable(context->hRequest, nullptr);
        }
        else
        {
            context->callback(context->response);
            WinHttpCloseHandle(context->hRequest);
            WinHttpCloseHandle(context->hConnect);
            delete context;
        }
        break;
    }
    case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
        context->callback("");
        WinHttpCloseHandle(context->hRequest);
        WinHttpCloseHandle(context->hConnect);
        delete context;
        break;
    }
}

namespace HTTPClient
{
    void Initialize()
    {
        std::lock_guard<std::mutex> lock(sessionMutex);
        if (!hSession)
        {
            hSession = WinHttpOpen(L"GW2LogProofs/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC);
            if (hSession)
            {
                WinHttpSetStatusCallback(hSession, AsyncCallback, WINHTTP_CALLBACK_FLAG_ALL_COMPLETIONS, 0);
            }
        }
    }

    std::future<std::string> GetRequestAsync(const std::wstring &wUrl)
    {
        auto promise = std::make_shared<std::promise<std::string>>();
        auto future = promise->get_future();
        GetRequestAsync(wUrl, [promise](const std::string &result)
                        { promise->set_value(result); });
        return future;
    }

    void GetRequestAsync(const std::wstring &wUrl, std::function<void(const std::string &)> callback)
    {
        Initialize();
        if (!hSession)
        {
            callback("");
            return;
        }

        URL_COMPONENTS urlComp = {};
        urlComp.dwStructSize = sizeof(urlComp);
        urlComp.dwSchemeLength = -1;
        urlComp.dwHostNameLength = -1;
        urlComp.dwUrlPathLength = -1;

        if (!WinHttpCrackUrl(wUrl.c_str(), 0, 0, &urlComp))
        {
            callback("");
            return;
        }

        std::wstring hostName(urlComp.lpszHostName, urlComp.dwHostNameLength);
        std::wstring urlPath(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);

        AsyncRequestContext *context = new AsyncRequestContext();
        context->callback = callback;
        context->hConnect = WinHttpConnect(hSession, hostName.c_str(), urlComp.nPort, 0);
        if (!context->hConnect)
        {
            callback("");
            delete context;
            return;
        }

        DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
        context->hRequest = WinHttpOpenRequest(context->hConnect, L"GET", urlPath.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
        if (!context->hRequest)
        {
            callback("");
            WinHttpCloseHandle(context->hConnect);
            delete context;
            return;
        }

        WinHttpSendRequest(context->hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, reinterpret_cast<DWORD_PTR>(context));
    }

    void Shutdown()
    {
        std::lock_guard<std::mutex> lock(sessionMutex);
        if (hSession)
        {
            WinHttpCloseHandle(hSession);
            hSession = nullptr;
        }
    }
} // namespace HTTPClient
