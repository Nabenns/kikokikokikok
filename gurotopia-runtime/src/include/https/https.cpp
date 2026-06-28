#include "pch.hpp"
#include "https.hpp"
#include "server_data.hpp"
#include "metrics.hpp"
#include "admin.hpp"

#include <openssl/err.h>
#include <csignal>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h> // @note TCP_DEFER_ACCEPT
    #include <sys/socket.h>

    #define SOCKET int
    #define INVALID_SOCKET	(SOCKET)(~0)
    #define SOCKET_ERROR	(-1)

#endif

/* cross-platform socket close */
static void cross_close(SOCKET fd)
{
#ifdef _WIN32
    closesocket(fd);
#else // @note unix
    close(fd);
#endif
}

/* cross-platform error log */
static void cross_log(const char *message)
{
#ifdef _WIN32
    std::fprintf(stderr, "%s: %d\n", message, WSAGetLastError());
#else // @note unix
    std::fprintf(stderr, "%s: %s\n", message, strerror(errno));
#endif
}

void https::listener()
{
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    constexpr int enable = 1;

    /* https://docs.openssl.org/3.0/man3/SSL_CTX_new/#return-values */
    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx)
    {
        ERR_print_errors_fp(stderr);
    }

    /* https://docs.openssl.org/master/man3/SSL_CTX_use_certificate/#return-values */
    if (SSL_CTX_use_certificate_file(ctx, "resources/ctx/server.crt", SSL_FILETYPE_PEM) != 1 ||
        SSL_CTX_use_PrivateKey_file(ctx, "resources/ctx/server.key", SSL_FILETYPE_PEM)  != 1)
    {
        ERR_print_errors_fp(stderr);
    }

    SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);

#ifdef SIGPIPE // @note unix
    std::signal(SIGPIPE, SIG_IGN);
#endif

    /* https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-socket */
    SOCKET socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket == INVALID_SOCKET)
    {
        cross_log("socket function failed");
    }

#ifdef SO_REUSEADDR
    setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (char*)&enable, sizeof(enable));
#endif
#ifdef TCP_DEFER_ACCEPT // @note unix
    setsockopt(socket, IPPROTO_TCP, TCP_DEFER_ACCEPT, (char*)&enable, sizeof(enable));
#endif
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(443);
    socklen_t addrlen = sizeof(addr);

    /* https://learn.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-bind */
    if (bind(socket, (struct sockaddr*)&addr, addrlen) == SOCKET_ERROR)
    {
        cross_log("could not bind socket");
    }

    const std::string Content =
        std::format(
            "server|{}\n"
            "port|{}\n"
            "type|{}\n"
            "type2|{}\n" // @todo remove for older clients
            "#maint|{}\n"
            "loginurl|{}\n"
            "meta|{}\n"
            "RTENDMARKERBS1001", 
            gServer_data.server, gServer_data.port, gServer_data.type, gServer_data.type2, gServer_data.maint, gServer_data.loginurl, gServer_data.meta
        );
    const std::string response =
        std::format(
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: {}\r\n"
            "Connection: close\r\n\r\n"
            "{}",
            Content.size(), Content);

    /* https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-listen */
    if (listen(socket, SOMAXCONN) == SOCKET_ERROR)
    {
        cross_log("failed to listen on socket");
    }
    else std::printf("listening on %s:%hu\n", gServer_data.server.c_str(), gServer_data.port);
    
    while (true)
    {
        /* https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-accept */
        SOCKET fd = accept(socket, reinterpret_cast<sockaddr*>(&addr), &addrlen);
        if (fd == INVALID_SOCKET) continue;
        
        SSL *ssl = SSL_new(ctx);
        if (!ssl) {
            cross_close(fd);
            continue;
        }
        if (SSL_set_fd(ssl, fd) != 1) {
            cross_close(fd);
            continue;
        }
        if (SSL_accept(ssl) > 0)
        {
            // Read HTTP request headers until we find \r\n\r\n (end of headers)
            char buf[8192] = {};
            int total = 0;
            while (total < static_cast<int>(sizeof(buf)) - 1)
            {
                int n = SSL_read(ssl, buf + total, static_cast<int>(sizeof(buf)) - 1 - total);
                if (n <= 0) break;
                total += n;
                if (std::string_view(buf, total).contains("\r\n\r\n")) break;
            }

            if (total > 0)
            {
                std::string_view headers(buf, total);
                std::string req_full(buf, total);

                // If this is POST, read the body (based on Content-Length header)
                auto cl_pos = headers.find("Content-Length:");
                if (cl_pos != std::string_view::npos)
                {
                    auto val_start = headers.find_first_of("0123456789", cl_pos);
                    if (val_start != std::string_view::npos)
                    {
                        auto val_end = headers.find_first_not_of("0123456789", val_start);
                        int content_len = 0;
                        try { content_len = std::stoi(std::string(headers.substr(val_start, val_end - val_start))); }
                        catch (...) { content_len = 0; }

                        int header_end = headers.find("\r\n\r\n");
                        int body_read = (header_end == std::string_view::npos) ? 0 : (total - (header_end + 4));

                        // Read remaining body if needed
                        while (body_read < content_len && total < static_cast<int>(sizeof(buf)) - 1)
                        {
                            int n = SSL_read(ssl, buf + total, std::min(
                                static_cast<int>(sizeof(buf)) - 1 - total,
                                content_len - body_read));
                            if (n <= 0) break;
                            total += n;
                            body_read += n;
                        }
                        req_full = std::string(buf, total);
                    }
                }

                std::string_view req = req_full;
                // Accept any POST to /growtopia/server_data.php (works behind reverse proxy)
                if (req.contains("POST") && req.contains("/growtopia/server_data.php"))
                {
                    SSL_write(ssl, response.c_str(), response.size());
                }
                // Metrics endpoint
                else if (req.contains("GET") && req.contains("/growtopia/metrics"))
                {
                    std::string metrics_body = metrics_json();
                    std::string metrics_resp = std::format(
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/plain\r\n"
                        "Content-Length: {}\r\n"
                        "Connection: close\r\n\r\n"
                        "{}",
                        metrics_body.size(), metrics_body
                    );
                    SSL_write(ssl, metrics_resp.c_str(), metrics_resp.size());
                }
                // Admin: online players list
                else if (req.contains("GET") && (req.contains("/growtopia/online") || req.contains("/admin/online")))
                {
                    std::string body = online_json();
                    std::string resp = std::format(
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: {}\r\n"
                        "Access-Control-Allow-Origin: *\r\n"
                        "Connection: close\r\n\r\n"
                        "{}",
                        body.size(), body
                    );
                    SSL_write(ssl, resp.c_str(), resp.size());
                }
                // Admin: broadcast
                else if (req.contains("POST") && (req.contains("/growtopia/broadcast") || req.contains("/admin/broadcast")))
                {
                    // parse JSON body
                    auto body_start = req.find("\r\n\r\n");
                    std::string json_body;
                    if (body_start != std::string::npos)
                        json_body = std::string(req.substr(body_start + 4));

                    // extract admin and msg fields (simple parsing)
                    std::string admin = "WebAdmin";
                    std::string msg;

                    auto admin_pos = json_body.find("\"admin\"");
                    if (admin_pos != std::string::npos)
                    {
                        auto val_start = json_body.find("\"", admin_pos + 8);
                        if (val_start != std::string::npos)
                        {
                            auto val_end = json_body.find("\"", val_start + 1);
                            if (val_end != std::string::npos)
                                admin = json_body.substr(val_start + 1, val_end - val_start - 1);
                        }
                    }

                    auto msg_pos = json_body.find("\"message\"");
                    if (msg_pos != std::string::npos)
                    {
                        auto val_start = json_body.find("\"", msg_pos + 10);
                        if (val_start != std::string::npos)
                        {
                            auto val_end = json_body.find("\"", val_start + 1);
                            if (val_end != std::string::npos)
                                msg = json_body.substr(val_start + 1, val_end - val_start - 1);
                        }
                    }

                    broadcast_message(admin, msg);
                    // audit logged via web API (avoids DB mutex cross-thread)

                    std::string result = "{\"status\":\"ok\"}";
                    std::string resp = std::format(
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: {}\r\n"
                        "Access-Control-Allow-Origin: *\r\n"
                        "Connection: close\r\n\r\n"
                        "{}",
                        result.size(), result
                    );
                    SSL_write(ssl, resp.c_str(), resp.size());
                }
                // Admin: setrole — routed via web API (Next.js → docker exec), NOT here
                // DB operations from HTTPS thread cause mutex deadlock with game thread
                else if (req.contains("POST") && (req.contains("/growtopia/setrole") || req.contains("/admin/setrole")))
                {
                    std::string result = "{\"status\":\"delegated\",\"note\":\"Use PATCH /api/players for role changes\"}";
                    std::string resp = std::format(
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: {}\r\n"
                        "Access-Control-Allow-Origin: *\r\n"
                        "Connection: close\r\n\r\n"
                        "{}",
                        result.size(), result
                    );
                    SSL_write(ssl, resp.c_str(), resp.size());
                }
                // Admin: execute server command — use web API or docker exec
                else if (req.contains("POST") && (req.contains("/growtopia/cmd") || req.contains("/admin/cmd")))
                {
                    std::string result = "{\"status\":\"delegated\",\"note\":\"Use PATCH /api/players with action=servercmd\"}";
                    std::string resp = std::format(
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: {}\r\n"
                        "Access-Control-Allow-Origin: *\r\n"
                        "Connection: close\r\n\r\n"
                        "{}",
                        result.size(), result
                    );
                    SSL_write(ssl, resp.c_str(), resp.size());
                }
                // Items JSON (paginated) — reads from items vector in memory
                else if (req.contains("GET") && req.contains("/growtopia/items"))
                {
                    int page = 0;
                    auto qm = req.find("page=");
                    if (qm != std::string::npos)
                    {
                        auto p = req.find_first_of("0123456789", qm);
                        if (p != std::string::npos)
                        {
                            try { page = std::stoi(std::string(req.substr(p, req.find_first_not_of("0123456789", p) - p))); }
                            catch (...) { page = 0; }
                        }
                    }
                    std::string body = items_json(page);
                    std::string resp = std::format(
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: {}\r\n"
                        "Access-Control-Allow-Origin: *\r\n"
                        "Connection: close\r\n\r\n"
                        "{}",
                        body.size(), body
                    );
                    SSL_write(ssl, resp.c_str(), resp.size());
                }
                // Store JSON — reads from shouhin_tachi vector in memory
                else if (req.contains("GET") && req.contains("/growtopia/store"))
                {
                    std::string body = store_json();
                    std::string resp = std::format(
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: {}\r\n"
                        "Access-Control-Allow-Origin: *\r\n"
                        "Connection: close\r\n\r\n"
                        "{}",
                        body.size(), body
                    );
                    SSL_write(ssl, resp.c_str(), resp.size());
                }
                // Config JSON — reads from gServer_data
                else if (req.contains("GET") && req.contains("/growtopia/config"))
                {
                    std::string body = config_json();
                    std::string resp = std::format(
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: {}\r\n"
                        "Access-Control-Allow-Origin: *\r\n"
                        "Connection: close\r\n\r\n"
                        "{}",
                        body.size(), body
                    );
                    SSL_write(ssl, resp.c_str(), resp.size());
                }
            }
        }
        else ERR_print_errors_fp(stderr);

        SSL_shutdown(ssl);
        SSL_free(ssl);
        cross_close(fd);
    }
}

#ifndef _WIN32
    #undef SOCKET
#endif
