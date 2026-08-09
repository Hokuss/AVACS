#include <atomic>
#include <iostream>
#include <string>
#include <csignal>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "watcher.hpp"

std::atomic<bool> server_running{true};

void handle_sigint(int sig) {
    std::cout << "\n[Signal] Ctrl+C detected. Initiating graceful shutdown..." << std::endl;
    server_running = 0;
}

bool send_all(SOCKET socket, const char* data, size_t length) {
    size_t total_sent = 0;

    while (total_sent < length) {
        // Calculate how much data is left to send
        int bytes_left = static_cast<int>(length - total_sent);
        
        // send() takes a const char*, so we offset the pointer by total_sent
        int bytes_sent = send(socket, data + total_sent, bytes_left, 0);

        if (bytes_sent == SOCKET_ERROR) {
            std::cerr << "Send failed. Error: " << WSAGetLastError() << std::endl;
            return false;
        }

        total_sent += bytes_sent;
    }

    return true;
}

int main(){
    signal(SIGINT, handle_sigint);

    // 2. Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return 1;
    }

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed. Error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed. Error: " << WSAGetLastError() << std::endl;
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed. Error: " << WSAGetLastError() << std::endl;
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server is running on http://localhost:8080" << std::endl;
    std::cout << "Press Ctrl+C to stop." << std::endl;

    std::jthread watcher(loop,"asset/main.wx");

    // server loop
    while (server_running) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_socket, &readfds);

        timeval timeout;
        timeout.tv_sec = 1;  // 1 second timeout
        timeout.tv_usec = 0;

        int activity = select(0, &readfds, NULL, NULL, &timeout);

        if (activity == SOCKET_ERROR) {
            std::cerr << "Select error: " << WSAGetLastError() << std::endl;
            break;
        }

        if (activity == 0) continue; 

        SOCKET client_socket = accept(server_socket, NULL, NULL);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Accept failed. Error: " << WSAGetLastError() << std::endl;
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
                        con_len = 0; // Treat as no body, or send a 400 Bad Request
                    }

                    if (con_len > 0) {
                        size_t body_start = header_end_pos + 4;
                        size_t current_body_bytes = buffer.length() - body_start;

                        // Keep reading until we have the full body
                        while (current_body_bytes < (size_t)con_len) {
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
            closesocket(client_socket);
            continue; 
        }

        std::string_view request_view(buffer);

        

        std::string response = "HTTP/1.1 200 OK\r\n"
                       "Content-Type: text/plain\r\n"
                       "Content-Length: 13\r\n"
                       "Connection: close\r\n"
                       "\r\n"
                       "Hello, World!";
                
        if (!send_all(client_socket, response.c_str(), response.length())) {
            std::cerr << "Failed to send complete response to client." << std::endl;
        }

        closesocket(client_socket);

    }
    
    closesocket(server_socket);
    WSACleanup();
    std::cout << "Main Loop Closed." << std::endl;

    watcher.request_stop();

    std::cout<<"Clean stop of side loop..."<<std::endl;

    return 0;
}