#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

const int PORT = 8080;
const std::string UPLOAD_DIR = "./";

void handleFileUpload(SOCKET clientSocket)
{
    char buffer[8192];
    std::string requestBody;
    bool isPostMethod = false;
    bool isFileSection = false;
    std::string boundary;

    // Read HTTP request
    int bytesRead;
    while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0)
    {
        requestBody.append(buffer, bytesRead);

        // Find the boundary from the content type header
        if (!boundary.empty())
        {
            if (requestBody.find(boundary) != std::string::npos)
            {
                isFileSection = true;
                break;
            }
        }
        else
        {
            size_t boundaryPos = requestBody.find("boundary=");
            if (boundaryPos != std::string::npos)
            {
                size_t start = boundaryPos + 9;
                size_t end = requestBody.find("\r\n", start);
                boundary = "--" + requestBody.substr(start, end - start);
            }
        }
    }

    if (isFileSection)
    {
        // Extract the file content
        size_t fileStartPos = requestBody.find("\r\n\r\n") + 4;
        size_t fileEndPos = requestBody.find(boundary, fileStartPos);
        std::string fileContent = requestBody.substr(fileStartPos, fileEndPos - fileStartPos);

        // Extract the filename
        size_t filenamePos = requestBody.find("Content-Disposition: form-data; name=\"file\"; filename=\"");
        filenamePos = requestBody.find("\"", filenamePos + 1) + 1;
        size_t filenameEndPos = requestBody.find("\"", filenamePos);
        std::string filename = requestBody.substr(filenamePos, filenameEndPos - filenamePos);

        // Save the file
        std::ofstream file(UPLOAD_DIR + filename, std::ios::binary);
        if (file)
        {
            file.write(fileContent.c_str(), fileContent.size());
            file.close();
        }
    }

    std::cout << "File Created In HDD Successfully" << std::endl;

    // Send response with CORS headers
    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Access-Control-Allow-Origin: *\r\n" // Allow any origin
        "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "\r\n"
        "File uploaded successfully!";

    send(clientSocket, response.c_str(), response.size(), 0);

    closesocket(clientSocket);
}

int main()
{
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed!" << std::endl;
        return -1;
    }

    // Create socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET)
    {
        std::cerr << "Error creating socket" << std::endl;
        WSACleanup();
        return -1;
    }

    // Set up server address
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "Error binding socket" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    // Listen for incoming connections
    if (listen(serverSocket, 5) == SOCKET_ERROR)
    {
        std::cerr << "Error listening on socket" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    std::cout << "Server started on http://localhost:" << PORT << std::endl;
    std::cout << "Waiting for connections..." << std::endl;

    // Accept client connections and handle them
    while (true)
    {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET)
        {
            std::cerr << "Error accepting client" << std::endl;
            continue;
        }

        std::cout << "Connection established!" << std::endl;

        // Handle the request
        handleFileUpload(clientSocket);
    }

    // Clean up
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}