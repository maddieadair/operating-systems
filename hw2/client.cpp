#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <sys/stat.h>
#include <filesystem>
#include <cstring>
#include <fcntl.h>

using namespace std;

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{

    int sockfd, portNum, n;
    struct sockaddr_in server_address;
    struct hostent *server_host;

    char buffer[1024];
    size_t bufferSize = 1024;
    char message[1024];
    char *mmm = message;
    size_t messageMaxLength = 1024;
    size_t messageLength;

    char newBuff[1024]; // <- for filename without '\0' later

    // check that the server has the client's hostname and port number
    if (argc < 3)
    {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(0);
    }

    // convert string argv[0] into an integer
    portNum = atoi(argv[2]);

    do
    {
        // create a socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
        {
            error("ERROR opening socket");
        }

        // client builds server's host address
        server_host = gethostbyname(argv[1]);
        if (server_host == NULL)
        {
            fprintf(stderr, "ERROR, no such host\n");
            exit(0);
        }

        // client builds address
        bzero((char *)&server_address, sizeof(server_address));
        server_address.sin_family = AF_INET;
        bcopy((char *)server_host->h_addr,
              (char *)&server_address.sin_addr.s_addr,
              server_host->h_length);
        server_address.sin_port = htons(portNum);

        // client connects to server
        if (connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
        {
            error("ERROR connecting");
        }

        // sets the first n bytes of the byte area starting at buffer to zero (basically reset buffer at beginning of each loop)
        bzero(buffer, 1024);
        // sets the first n bytes of the byte area starting at newBuff to zero (basically reset newBuff at beginning of each loop))
        bzero(newBuff, 1024);

        // get filename/command
        cout << "Please enter a file name: ";
        fgets(message, 1024, stdin);

        // if filename is "terminate", send terminate message to server then break out of loop and terminate
        if (string(message) == "terminate\n")
        {
            n = write(sockfd, message, strlen(message));
            break;
        }

        // if filename is "exit", send an exit message to server then break out of loop and terminate
        else if (string(message) == "exit\n")
        {
            n = write(sockfd, message, strlen(message));
            break;
        }

        // if filename is neither "exit" or "terminate"
        else
        {
            // write filename to server
            int wBytes = write(sockfd, message, strlen(message));

            if (wBytes < 0)
            {
                error("ERROR writing to socket");
            }

            // sets the first n bytes of the byte area starting at buffer to zero
            bzero(buffer, 1024);

            // get the filename without '\0' for writing to new file later
            strncpy(newBuff, message, strlen(message) - 1);

            // read buffer response from server
            int rBytes = read(sockfd, buffer, 1);

            // if one-byte message == '0', aka the file is missing
            if (buffer[0] == '0')
            {
                cout << "File " << newBuff << " is missing" << endl
                     << endl;
            }
            else if (rBytes < 0)
            {
                error("ERROR reading from socket");
            }

            // one-byte messsage == '1' aka OK, the file exists
            else
            {
                // create new file with the same name as that of the requested filename
                int fd = open(newBuff, O_WRONLY | O_CREAT, 0600);
                int nRead = 0;
                int byteCount = 0;

                // write the contents of the buffer to the file
                do
                {
                    nRead = read(sockfd, buffer, 1024);
                    byteCount += nRead;
                    if (nRead > 0)
                    {
                        write(fd, buffer, nRead);
                    }
                } while (nRead == 1024);

                cout << "Received file " << newBuff << " (" << byteCount << " bytes)" << endl
                     << endl;
            }
        }
    } while (string(message) != "terminate" || string(message) != "exit");


    // close client socket
    close(sockfd);

    return 0;
}