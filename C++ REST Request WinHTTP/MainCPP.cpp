
#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include <string>
#pragma comment(lib, "winhttp.lib")
using namespace std;
/*
******************************************************************************************
*                                 API	                                                **
******************************************************************************************
*   WinHttpOpen	            Creates the HTTP session and configures proxy behavior.     **
*   WinHttpConnect	        Creates a connection context for the target server.         *
*   WinHttpOpenRequest      Creates a request with the HTTP method and resource path.   *
*   WinHttpSendRequest	    Sends request headers and the optional body.                *
*   WinHttpReceiveResponse	Waits for response headers and makes the response available.*
*   WinHttpQueryHeaders	    Retrieves the HTTP status code and response headers.        *
*   WinHttpReadData	        Reads response data in chunks.                              *
*   WinHttpCloseHandle	    Releases WinHTTP handles.                                   *
*****************************************************************************************
*/
struct HttpResponse
{
    DWORD statusCode = 0;
    string body;
};

// Generic REST function for GET, POST, PUT and DELETE.
HttpResponse SendRestRequest(const wstring& host,const wstring& path,const wstring& method, const string& body = "")
{
   HttpResponse response;

   HINTERNET session = WinHttpOpen(L"MyCppRestClient/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);
   if (!session)
   {
        cerr << "WinHttpOpen failed: "<< GetLastError() << '\n';
        return response;
   }
   HINTERNET connection = WinHttpConnect(session,host.c_str(),INTERNET_DEFAULT_HTTPS_PORT,0);
   if (!connection)
   {
        cerr << "WinHttpConnect failed: " << GetLastError() << '\n';
        WinHttpCloseHandle(session);
        return response;
    }
   HINTERNET request = WinHttpOpenRequest(connection,method.c_str(),path.c_str(),nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE);
    if (!request)
    {
        std::cerr << "WinHttpOpenRequest failed: "<< GetLastError() << '\n';
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return response;
    }
    // Tell the server we are sending/accepting JSON.
    const wchar_t* headers =L"Content-Type: application/json\r\n" L"Accept: application/json\r\n";

    LPCVOID data = body.empty()
        ? WINHTTP_NO_REQUEST_DATA
        : static_cast<LPCVOID>(body.data());

    DWORD dataSize = static_cast<DWORD>(body.size());
    BOOL ok = WinHttpSendRequest(request,headers,static_cast<DWORD>(-1L),const_cast<LPVOID>(data),dataSize,dataSize,0);
    if (ok)
    {
        ok = WinHttpReceiveResponse(request, nullptr);
    }
    if (!ok)
    {
        std::cerr << "HTTP request failed: "<< GetLastError() << '\n';
    }
    else
    {
        // Get HTTP status code.
        DWORD size = sizeof(response.statusCode);
        WinHttpQueryHeaders(request,WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&response.statusCode,
            &size,WINHTTP_NO_HEADER_INDEX);
        // Read response body.
        DWORD available = 0;

        while (WinHttpQueryDataAvailable(request, &available) && available > 0)
        {
           string buffer(available, '\0');
           DWORD bytesRead = 0;

            if (!WinHttpReadData(request,&buffer[0],available,&bytesRead))
            {
                cerr << "Response read failed: "<< GetLastError() << '\n';
                break;
            }
            response.body.append(buffer.data(), bytesRead);
        }
    }

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);

    return response;
}

void PrintResponse(const HttpResponse& response)
{
    cout << "HTTP Status: " << response.statusCode << "\n";
    cout << "Response:\n"<< response.body << "\n\n";
}

int main()
{
    const wstring host = L"jsonplaceholder.typicode.com";

    // 1. GET: Retrieve a resource.
    cout << "===== GET =====\n";
    HttpResponse getResponse = SendRestRequest(host, L"/posts/1", L"GET");
    PrintResponse(getResponse);

    // 2. POST: Create a resource.
    cout << "===== POST =====\n";
    string postJson =R"({"title":"My C++ Post","body":"Hello REST API from Satish Box","userId":1})";

    HttpResponse postResponse = SendRestRequest(host, L"/posts", L"POST", postJson);
    PrintResponse(postResponse);

    // 3. PUT: Replace/update a resource.
    cout << "===== PUT =====\n";
    string putJson = R"({"id":1,"title":"Updated Title","body":"Updated body","userId":1})";
    HttpResponse putResponse = SendRestRequest(host, L"/posts/1", L"PUT", putJson);
    PrintResponse(putResponse);

    // 4. DELETE: Delete a resource.
    cout << "===== DELETE =====\n";
    HttpResponse deleteResponse = SendRestRequest(host, L"/posts/1", L"DELETE");
    PrintResponse(deleteResponse);

    return 0;
}
