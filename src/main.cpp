#include <atomic>
#include <iostream>
#include <string>
#include <csignal>
#include <thread>
#include <cctype>
#include <string_view>
#include "watcher.hpp"

// =======================================================================
// Cross-Platform Socket Abstraction Layer
// =======================================================================
#ifdef _WIN32
    // Windows Headers
    #include <winsock2.h>
    #include <ws2tcpip.h>
    // Note: When compiling with msys2/MinGW, ensure you link with: -lws2_32
    #pragma comment(lib, "Ws2_32.lib") 

    namespace net {
        using SocketType = SOCKET;
        constexpr SocketType InvalidSocket = INVALID_SOCKET;
        constexpr int SocketError = SOCKET_ERROR;

        inline bool init() {
            WSADATA wsaData;
            return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
        }
        inline void cleanup() { WSACleanup(); }
        inline void close_socket(SocketType s) { closesocket(s); }
        inline int get_last_error() { return WSAGetLastError(); } // Returns int error code
    }
#else
    // macOS / Linux (POSIX) Headers
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <cerrno>
    #include <cstring>
    #include <sys/select.h>

    namespace net {
        using SocketType = int;
        constexpr SocketType InvalidSocket = -1;
        constexpr int SocketError = -1;

        inline bool init() { return true; } // No initialization needed on POSIX
        inline void cleanup() {}            // No cleanup needed on POSIX
        inline void close_socket(SocketType s) { close(s); }
        inline std::string get_last_error() { return std::strerror(errno); } // Returns string error message
    }
#endif
// =======================================================================


std::atomic<bool> server_running{true};

void handle_sigint(int sig) {
    std::cout << "\n[Signal] Ctrl+C detected. Initiating graceful shutdown..." << std::endl;
    server_running = false;
}

bool send_all(net::SocketType socket_fd, const char* data, size_t length) {
    size_t total_sent = 0;

    while (total_sent < length) {
        int bytes_left = static_cast<int>(length - total_sent);
        int bytes_sent = send(socket_fd, data + total_sent, bytes_left, 0);

        if (bytes_sent == net::SocketError) {
            std::cerr << "Send failed. Error: " << net::get_last_error() << std::endl;
            return false;
        }

        total_sent += bytes_sent;
    }

    return true;
}

int main() {
    signal(SIGINT, handle_sigint);

    // Initialize networking (does WSAStartup on Windows, does nothing on Mac)
    if (!net::init()) {
        std::cerr << "Network initialization failed." << std::endl;
        return 1;
    }

    net::SocketType server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == net::InvalidSocket) {
        std::cerr << "Socket creation failed. Error: " << net::get_last_error() << std::endl;
        net::cleanup();
        return 1;
    }

    // Mac/Linux lock ports in TIME_WAIT after closing. This prevents "Address already in use" errors.
    // Windows behaves differently and doesn't explicitly need this for standard restarts.
#ifndef _WIN32
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == net::SocketError) {
        std::cerr << "setsockopt failed. Error: " << net::get_last_error() << std::endl;
    }
#endif

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == net::SocketError) {
        std::cerr << "Bind failed. Error: " << net::get_last_error() << std::endl;
        net::close_socket(server_socket);
        net::cleanup();
        return 1;
    }

    if (listen(server_socket, SOMAXCONN) == net::SocketError) {
        std::cerr << "Listen failed. Error: " << net::get_last_error() << std::endl;
        net::close_socket(server_socket);
        net::cleanup();
        return 1;
    }

    std::cout << "Server is running on http://localhost:8080" << std::endl;
    std::cout << "Press Ctrl+C to stop." << std::endl;

    std::jthread watcher(loop, "asset/main.wx");
    // std::jthread start_vm(vm_loop);

    // server loop
    while (server_running) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_socket, &readfds);

        timeval timeout;
        timeout.tv_sec = 1;  // 1 second timeout
        timeout.tv_usec = 0;

        // Windows ignores the first arg. POSIX requires it to be highest_fd + 1.
        // Passing (int)(server_socket + 1) works perfectly for both!
        int activity = select(static_cast<int>(server_socket + 1), &readfds, NULL, NULL, &timeout);

        if (activity == net::SocketError) {
#ifndef _WIN32
            // On Mac/Linux, Ctrl+C interrupts select() and throws an EINTR error. 
            // We can safely ignore it and break the loop.
            if (errno == EINTR) break;
#endif
            std::cerr << "Select error: " << net::get_last_error() << std::endl;
            break;
        }

        if (activity == 0) continue; 

        net::SocketType client_socket = accept(server_socket, NULL, NULL);
        if (client_socket == net::InvalidSocket) {
            std::cerr << "Accept failed. Error: " << net::get_last_error() << std::endl;
            continue;
        }

        // --- HANDLE THE REQUEST HERE ---
        std::string buffer;
        char chunk[1024];
        int bytes_rec = 0;
        size_t header_end_pos = std::string::npos;
        const size_t MAX_HEADER_SIZE = 26214400;

        while (header_end_pos == std::string::npos) {
            bytes_rec = recv(client_socket, chunk, sizeof(chunk), 0);
            if (bytes_rec <= 0) break; // Client disconnected or error

            buffer.append(chunk, bytes_rec);
            header_end_pos = buffer.find("\r\n\r\n");

            if (buffer.length() > MAX_HEADER_SIZE && header_end_pos == std::string::npos) {
                std::cerr << "Request header too large. Dropping client." << std::endl;
                break;
            }
        }

        if (header_end_pos != std::string::npos) {
            std::string headers = buffer.substr(0, header_end_pos);
            
            std::string headers_lower = headers;
            for (char &c : headers_lower) c = std::tolower(c);

            size_t cl_pos = headers_lower.find("content-length:");

            if (cl_pos != std::string::npos) {
                size_t val_start = cl_pos + 15;
                while (val_start < headers_lower.length() && headers_lower[val_start] == ' ') {
                    val_start++;
                }
                
                size_t val_end = headers_lower.find("\r\n", val_start);
                
                if (val_end != std::string::npos) {
                    std::string len_str = headers.substr(val_start, val_end - val_start);
                    int con_len = 0;
                    
                    try {
                        con_len = std::stoi(len_str);
                    } catch (const std::exception& e) {
                        std::cerr << "Invalid Content-Length value." << std::endl;
                        con_len = 0; 
                    }

                    if (con_len > 0) {
                        size_t body_start = header_end_pos + 4;
                        size_t current_body_bytes = buffer.length() - body_start;

                        while (current_body_bytes < static_cast<size_t>(con_len)) {
                            bytes_rec = recv(client_socket, chunk, sizeof(chunk), 0);
                            if (bytes_rec <= 0) break;

                            buffer.append(chunk, bytes_rec);
                            current_body_bytes += bytes_rec;
                        }
                    }
                }
            }
        }

        if (header_end_pos == std::string::npos) {
            net::close_socket(client_socket);
            continue; 
        }

        std::string_view request_view(buffer);

        request main_request(buffer);
        std::string response = main_request.process();
                
        if (!send_all(client_socket, response.c_str(), response.length())) {
            std::cerr << "Failed to send complete response to client." << std::endl;
        }

        net::close_socket(client_socket);
    }
    
    net::close_socket(server_socket);
    net::cleanup(); // Does WSACleanup() on Windows, does nothing on Mac
    
    std::cout << "Main Loop Closed." << std::endl;

    watcher.request_stop();
    // start_vm.request_stop();

    std::cout << "Clean stop of side loop..." << std::endl;

    return 0;
}