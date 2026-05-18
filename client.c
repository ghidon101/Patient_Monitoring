#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

#define PORT 8080
#define BUFFER_SIZE 1024

int main()
{
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        cerr << "Socket creation error." << endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        cerr << "Invalid address/ Address not supported." << endl;
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        cerr << "Connection failed." << endl;
        return -1;
    }

    cout << "Connected to the server." << endl;

    while (true)
    {
        string input;
        cout << "\nEnter command: ";
        getline(cin, input);

        if (input == "exit")
        {
            cout << "Exiting client." << endl;
            break;
        }

        send(sock, input.c_str(), input.length(), 0);

        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(sock, buffer, BUFFER_SIZE);
        if (valread > 0)
        {
            cout << "\nServer response:\n" << buffer << endl;
        }
        else
        {
            cerr << "Error reading response from server." << endl;
        }
    }

    close(sock);
    return 0;
}