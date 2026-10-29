from pathlib import Path

content = r"""# C++ REST API Client Using Native Windows WinHTTP
## Interview Preparation Guide

**Goal:** Explain and demonstrate how to call REST endpoints from C++ on Windows without a third-party HTTP library.

**Platform:** Windows, Visual Studio, MSVC  
**HTTP API:** WinHTTP (`winhttp.h`, `winhttp.lib`)  
**Methods:** GET, POST, PUT, DELETE

---

## 1. Explanation (about 2 minutes)

> I implemented a reusable REST API client in native C++ using the Windows WinHTTP API, without depending on third-party HTTP libraries such as libcurl.
>
> The goal was to create a generic HTTP communication layer supporting GET, POST, PUT, and DELETE.
>
> The reusable function accepts the server hostname, resource path, HTTP method, and an optional request body. It initializes a WinHTTP session, creates a connection context, builds the request, sends it, receives the response, reads the response body, and releases the WinHTTP handles.
>
> For POST and PUT, I send JSON in the request body with an appropriate `Content-Type` header. I retrieve the HTTP status code and response body so the caller can inspect the server's response.
>
> I chose WinHTTP because it is a native Windows API, supports HTTPS and proxy configuration, and avoids an additional HTTP-library dependency.
>
> For production readiness, I would add RAII wrappers for handles, configurable timeouts, authentication, response-size limits, structured logging, cancellation, and carefully designed retry policies.

**Interview tip:** Say that a feature is implemented only if you actually implemented and tested it. Present future improvements as planned enhancements.

---

## 2. Architecture

```text
C++ Application / MFC UI
          |
          | GET / POST / PUT / DELETE
          v
Reusable REST Client
SendRestRequest(host, path, method, body)
          |
          v
Windows WinHTTP API
Session -> Connection -> Request -> Response
          |
          v
HTTPS REST Server
          |
          v
HTTP status code + response body
```

### Design principle

Separate HTTP transport from application/business logic. One reusable function handles the common request lifecycle, while the method, path, headers, and body vary by operation.

---

## 3. Complete example: GET, POST, PUT and DELETE

Create a **C++ Console Application** in Visual Studio and put the following code in `main.cpp`.

This example uses JSONPlaceholder as a public test API. It simulates create, update, and delete operations; changes are not permanently stored.

```cpp
#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include <string>

#pragma comment(lib, "winhttp.lib")

struct HttpResponse
{
    DWORD statusCode = 0;
    std::string body;
    DWORD win32Error = ERROR_SUCCESS;
};

HttpResponse SendRestRequest(
    const std::wstring& host,
    const std::wstring& path,
    const std::wstring& method,
    const std::string& body = "")
{
    HttpResponse response;

    HINTERNET session = WinHttpOpen(
        L"MyCppRestClient/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);

    if (!session)
    {
        response.win32Error = GetLastError();
        return response;
    }

    HINTERNET connection = WinHttpConnect(
        session,
        host.c_str(),
        INTERNET_DEFAULT_HTTPS_PORT,
        0);

    if (!connection)
    {
        response.win32Error = GetLastError();
        WinHttpCloseHandle(session);
        return response;
    }

    HINTERNET request = WinHttpOpenRequest(
        connection,
        method.c_str(),
        path.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);

    if (!request)
    {
        response.win32Error = GetLastError();
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return response;
    }

    // Configure timeouts in milliseconds.
    if (!WinHttpSetTimeouts(request, 5000, 5000, 10000, 10000))
    {
        response.win32Error = GetLastError();
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return response;
    }

    const wchar_t* headers =
        L"Content-Type: application/json\r\n"
        L"Accept: application/json\r\n";

    LPVOID optionalData = body.empty()
        ? WINHTTP_NO_REQUEST_DATA
        : const_cast<char*>(body.data());

    DWORD bodySize = static_cast<DWORD>(body.size());

    BOOL ok = WinHttpSendRequest(
        request,
        headers,
        static_cast<DWORD>(-1L),
        optionalData,
        bodySize,
        bodySize,
        0);

    if (ok)
        ok = WinHttpReceiveResponse(request, nullptr);

    if (!ok)
    {
        response.win32Error = GetLastError();
    }
    else
    {
        DWORD statusSize = sizeof(response.statusCode);

        if (!WinHttpQueryHeaders(
                request,
                WINHTTP_QUERY_STATUS_CODE |
                    WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX,
                &response.statusCode,
                &statusSize,
                WINHTTP_NO_HEADER_INDEX))
        {
            response.win32Error = GetLastError();
        }
        else
        {
            // Read response incrementally. A production implementation
            // should also enforce a maximum response size.
            for (;;)
            {
                DWORD available = 0;

                if (!WinHttpQueryDataAvailable(request, &available))
                {
                    response.win32Error = GetLastError();
                    break;
                }

                if (available == 0)
                    break;

                std::string buffer(available, '\\0');
                DWORD bytesRead = 0;

                if (!WinHttpReadData(
                        request,
                        &buffer[0],
                        available,
                        &bytesRead))
                {
                    response.win32Error = GetLastError();
                    break;
                }

                response.body.append(buffer.data(), bytesRead);
            }
        }
    }

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);

    return response;
}

void PrintResponse(const char* operation, const HttpResponse& response)
{
    std::cout << "\\n===== " << operation << " =====\\n";

    if (response.win32Error != ERROR_SUCCESS)
    {
        std::cout << "WinHTTP/Windows error: "
                  << response.win32Error << '\\n';
        return;
    }

    std::cout << "HTTP status: " << response.statusCode << '\\n';
    std::cout << "Response body:\\n" << response.body << '\\n';
}

int main()
{
    const std::wstring host = L"jsonplaceholder.typicode.com";

    // GET: retrieve a resource.
    auto getResponse = SendRestRequest(
        host, L"/posts/1", L"GET");
    PrintResponse("GET", getResponse);

    // POST: submit a new resource.
    const std::string postJson =
        R"({"title":"My C++ Post","body":"Hello REST API","userId":1})";

    auto postResponse = SendRestRequest(
        host, L"/posts", L"POST", postJson);
    PrintResponse("POST", postResponse);

    // PUT: replace/update a resource.
    const std::string putJson =
        R"({"id":1,"title":"Updated Title","body":"Updated body","userId":1})";

    auto putResponse = SendRestRequest(
        host, L"/posts/1", L"PUT", putJson);
    PrintResponse("PUT", putResponse);

    // DELETE: request deletion of a resource.
    auto deleteResponse = SendRestRequest(
        host, L"/posts/1", L"DELETE");
    PrintResponse("DELETE", deleteResponse);

    return 0;
}
```

### Build requirements

1. Install Visual Studio with **Desktop development with C++** and a Windows SDK.
2. Create a C++ Console Application.
3. Add the code to `main.cpp`.
4. Build with MSVC. The `#pragma comment(lib, "winhttp.lib")` directive links WinHTTP automatically.
5. Run with an internet connection.

The code uses synchronous calls, so it can block while waiting for the server. For a UI application, perform the work on a background thread or use asynchronous WinHTTP.

---

## 4. The four HTTP methods

| Method | Example path | Purpose | Typical request body |
|---|---|---|---|
| GET | `/posts/1` | Retrieve a resource | Usually none |
| POST | `/posts` | Create or submit an operation | Often JSON |
| PUT | `/posts/1` | Replace a resource at a known URI | Often JSON |
| DELETE | `/posts/1` | Request deletion | Usually none |

The exact behavior is defined by the server's API contract. PUT is intended to be idempotent; POST generally is not.

### Example JSON bodies

**POST**
```json
{
  "title": "My C++ Post",
  "body": "Hello REST API",
  "userId": 1
}
```

**PUT**
```json
{
  "id": 1,
  "title": "Updated Title",
  "body": "Updated body",
  "userId": 1
}
```

---

## 5. WinHTTP APIs to explain

| API | Interview explanation |
|---|---|
| `WinHttpOpen` | Creates a WinHTTP session and configures proxy access. |
| `WinHttpConnect` | Creates a connection context for the server and port. |
| `WinHttpOpenRequest` | Creates the request with the HTTP method and path. |
| `WinHttpSetTimeouts` | Configures resolve, connect, send, and receive timeouts. |
| `WinHttpSendRequest` | Sends request headers and optional request data. |
| `WinHttpReceiveResponse` | Receives the response from the server. |
| `WinHttpQueryHeaders` | Retrieves response headers, including the HTTP status code. |
| `WinHttpQueryDataAvailable` | Checks how much response data is available to read. |
| `WinHttpReadData` | Reads response data into a buffer. |
| `WinHttpCloseHandle` | Releases a WinHTTP handle. |

---

## 6. Common interview questions and answers

### Q1. Why WinHTTP instead of a third-party library?

WinHTTP is a native Windows API with HTTPS and proxy support. It reduces external library dependencies and fits Windows-specific C++ applications. A cross-platform product might choose a different abstraction.

### Q2. What is the difference between a transport error and an HTTP error?

A transport error means the request could not complete at the WinHTTP/Windows level, for example due to a connection failure or timeout. An HTTP error is a server response such as `400`, `401`, `404`, or `500`. The response status should be checked even when WinHTTP succeeds.

### Q3. What do common status codes mean?

| Status | Meaning |
|---|---|
| `200 OK` | Request succeeded |
| `201 Created` | Resource created |
| `204 No Content` | Request succeeded with no response body |
| `400 Bad Request` | Invalid request |
| `401 Unauthorized` | Authentication required or invalid |
| `403 Forbidden` | Access denied |
| `404 Not Found` | Resource not found |
| `500 Internal Server Error` | Server-side failure |

Actual success codes depend on the API contract.

### Q4. How would you add bearer-token authentication?

Add an `Authorization` request header such as:

```text
Authorization: Bearer <access-token>
```

Do not hard-code real credentials in source control. Protect tokens and avoid logging them. Ensure the token is valid for the target API.

### Q5. Does WinHTTP parse JSON?

No. WinHTTP sends and receives bytes. JSON parsing is a separate concern. If third-party dependencies are allowed, use an approved JSON library; if not, use a suitable in-house parser rather than fragile string manipulation.

### Q6. How do you manage resources safely?

Every successful `WinHttpOpen`, `WinHttpConnect`, and `WinHttpOpenRequest` call returns a handle that must be closed. The sample closes handles on its error paths. In production C++, I would use RAII wrappers with custom deleters so cleanup happens automatically.

### Q7. How would you prevent an MFC UI from freezing?

The sample is synchronous. Run it on a worker thread or use asynchronous WinHTTP, and marshal completion results back to the UI thread safely.

### Q8. How would you add retries?

Retry only appropriate transient failures, with bounded attempts and backoff. Consider idempotency and the API contract. Do not blindly retry POST operations because a server may have completed the operation even if the client did not receive the response.

### Q9. How would you test the client?

Test successful responses, HTTP errors, invalid hostnames, timeouts, empty bodies, large responses, malformed server behavior, and authentication failures. Use a test endpoint or mock server and verify both status codes and response bodies.

### Q10. How would you improve the code for production?

- RAII wrappers for handles.
- Configurable connect/send/receive timeouts.
- Response-size limits and robust error propagation.
- Authentication and secure secret handling.
- Structured logging with sensitive data redacted.
- Cancellation and asynchronous operation where appropriate.
- Carefully designed retries and telemetry.
- Unit tests and integration tests.

---

## 7. Strong senior-level closing statement

> I separated HTTP transport from business logic and built a reusable request function rather than duplicating networking code for each HTTP method. WinHTTP handles the Windows HTTP and TLS transport, while the application controls the endpoint, method, headers, and payload. I also considered error classification and resource cleanup, and I would use RAII, bounded buffering, authentication, and asynchronous execution to harden the implementation for production.

**Be accurate in the interview:** distinguish what the sample actually implements from enhancements you would add.

---

## 8. Quick revision checklist

- [ ] Explain why WinHTTP is suitable for native Windows C++.
- [ ] Remember the request lifecycle: session → connection → request → send → receive → read → close.
- [ ] Explain GET, POST, PUT, and DELETE.
- [ ] Distinguish Win32 errors from HTTP status codes.
- [ ] Explain JSON headers and request bodies.
- [ ] Discuss RAII and handle cleanup.
- [ ] Explain synchronous versus asynchronous calls.
- [ ] Mention authentication, timeouts, response-size limits, testing, and safe retries.
"""

path = Path("/mnt/data/Cpp_REST_WinHTTP_Interview_Guide.md")
path.write_text(content, encoding="utf-8")
print(f"Created: {path}\nSize: {path.stat().st_size:,} bytes")
