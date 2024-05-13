#include <iostream>
#include <fstream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <unistd.h>
#include <filesystem>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <cstring>
#include <fcntl.h>

using namespace std;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portNum;
    socklen_t clientLen;

    char buffer[1024];
    char input[1024]; // <- for filename without '\0' later

    struct sockaddr_in server_addr, client_addr;

    int n;

    // check if port number is provided
    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    // server creates a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error("ERROR opening socket");
    }

    // server builds address
    bzero((char *)&server_addr, sizeof(server_addr));
    portNum = atoi(argv[1]);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(portNum);

    // server binds socket to its address
    if (bind(sockfd, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
        error("ERROR on binding");

    // server now waits for connections
    listen(sockfd, 5);
    clientLen = sizeof(client_addr);

    // keep server socket open until terminate
    while (true)
    {
        // server accepts connection from client
        newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, &clientLen);

        if (newsockfd < 0)
        {
            error("ERROR on accept");
        }

        bzero(buffer, 1024); // sets the first n bytes of the byte area starting at buffer to zero (basically reset buffer at beginning of each loop)
        bzero(input, 1024); // sets the first n bytes of the byte area starting at input to zero (basically reset input at beginning of each loop)

        n = read(newsockfd, buffer, 1024); // server reads the message from the client

        if (n < 0)
        {
            error("ERROR reading from socket");
        }

        else
        {
            strncpy(input, buffer, strlen(buffer) - 1); // get the file name without the '\0'

            // if message is "terminate", close the temp socket/connection and then break out of loop and terminate server socket
            if (string(input) == "terminate")
            {
                cout << "Goodbye!" << endl;
                close(newsockfd);
                break;
            }

            // if message is "exit", close the temp socket/connection
            else if (string(input) == "exit")
            {
                close(newsockfd);
            }

            // message is neither "terminate" or "exit"
            else
            {
                // try to open up file with filename input
                int fd = open(input, O_RDONLY);
                cout << "A client requested the file " << input << endl;

                // if 0 bytes are read from the file then the file is missing
                if (fd < 0)
                {
                    write(newsockfd, "0", 1); // write one-byte message to client saying the file does not exist
                    cout << "That file is missing!" << endl
                         << endl;
                    close(newsockfd);  // close temp socket/connection
                    continue;
                }

                // file does exist
                else
                {
                    write(newsockfd, "1", 1); // write one-byte OK message to client
                    int nRead = 0;
                    int byteCount = 0;

                    // write the contents of the file to the client
                    do
                    {
                        nRead = read(fd, input, 1024);
                        byteCount += nRead;

                        if (nRead > 0)
                        {
                            write(newsockfd, input, nRead);
                        }
                    } while (nRead == 1024);

                    cout << "Sent " << byteCount << " bytes" << endl
                         << endl;

                    // close temp socket
                    close(newsockfd);
                }
            }
        }
    }

    // close server socket
    close(sockfd);

    return 0;
}