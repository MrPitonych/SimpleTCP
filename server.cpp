#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <zconf.h>
#include <mutex>
#include <iostream>
#include <vector>

#include "etcp.h"
#include <errno.h>

#define NUMBER_OF_READN_SYMBOLS 5
#define NUMBER_OF_CLIENTS 5
#define PORT 7500


std::mutex myMutex;

std::vector<int> arrayOfConnection;

typedef struct {
    int accept_socket;
    int client_sockets [NUMBER_OF_CLIENTS];
    char action;
} pthrData;

int readN( SOCKET fd, char *bp, size_t len )
{
    int cnt;
    int rc;

    cnt = len;
    while ( cnt > 0 )
    {
        rc = recv( fd, bp, cnt, 0 );

        if ( rc < 0 )
        {
            if ( errno == EINTR )
                continue;
            return -1;
        }
        if ( rc == 0 )
            return len - cnt;
        bp += rc;
        cnt -= rc;
    }
    return len;
}

char* toDelimeter(int rc, int s1, char buf [])
{
    char *read_buf;
    read_buf =(char*) malloc (sizeof(char));
    if (read_buf == NULL)
    {
        perror( "failed to allocate memory" );
        exit(1);
    }
    int i=0;
    while (buf[0]!='\n')
    {
        rc = recv( s1, buf, 1, 0 );
        if ( rc <= 0 )
        {
            perror( "recv call failed" );
            exit(1);
        }
        read_buf[i]=buf[0];
        i++;
    }

    return read_buf;
}

void* threadFunction( void* threadData )
{
    auto data = ( int* ) threadData;
    int s1 = *data;
    char result [NUMBER_OF_READN_SYMBOLS];
    int rc = -1;

    while ( true ) {
        memset(result,0,sizeof(result));
        rc = readN( s1, result, NUMBER_OF_READN_SYMBOLS );


        if (rc == -1) {
            break;
        }
        std::string res(result);
        if (res.empty()) {
            printf("result is null\n");
        } else {
            printf( "%d : %s\n", s1, result );
            fflush(stdout);
        }
    }

    myMutex.lock();
    arrayOfConnection.erase(std::remove(arrayOfConnection.begin(), arrayOfConnection.end(), s1), arrayOfConnection.end());
    myMutex.unlock();
}

void* acceptThread( void* threadData )
{
    auto data = ( pthrData* ) threadData;
    int client_socket;
    pthread_t thread;
    while ( true ) {

        if ( arrayOfConnection.size() < NUMBER_OF_CLIENTS) {
            client_socket = accept( data->accept_socket, nullptr, nullptr );
            if ( client_socket < 0 ) {
                break;
            }

            pthread_create( &thread, nullptr, threadFunction, &client_socket );

            myMutex.lock();
            arrayOfConnection.push_back( client_socket );
            myMutex.unlock();
        }
    }

    for ( int i = 0; i < arrayOfConnection.size(); i++ ) {
        shutdown( arrayOfConnection[i], SHUT_RDWR );
        close( arrayOfConnection[i] );
        pthread_join( thread, NULL );

    }
}

int main()
{

    std::string userCommand;
    struct sockaddr_in local;

    local.sin_family = AF_INET;
    local.sin_port = htons( PORT );
    local.sin_addr.s_addr = htonl( 0xC0A82B22 );

    auto threadData = ( pthrData* ) malloc( sizeof( pthrData ) );

    threadData->accept_socket = socket( AF_INET, SOCK_STREAM, 0 );
    bind( threadData->accept_socket, ( struct sockaddr * ) &local, sizeof( local ) );
    listen( threadData->accept_socket, 5 );


    std::cout << "Server binding to port : " << PORT << std::endl;
    std::cout << "Press 'help' to call a hint!" << std::endl;

    auto thread = ( pthread_t* ) malloc( sizeof( pthread_t ) );
    pthread_create( thread, nullptr, acceptThread, threadData );

    while ( true ) {

        std::cin >> userCommand;

                       if ( userCommand == "get" || userCommand == "g" ) {
                    printf( "get\n" );
                    printf( "Number of connections: %d\n", (int) arrayOfConnection.size() );
                    for (int i = 0; i < arrayOfConnection.size(); i++) {
                        printf( "%d. Socket client: %d\n", i + 1 , arrayOfConnection[i] );
                    }
                } else if (userCommand == "exit" || userCommand == "e" ) {
                    printf( "exit\n" );
                    break;
                } else if ( userCommand == "help" || userCommand == "h" ) {
                    printf( "'get' -- info of client\n"
                            "'kill' -- disconnect client\n"
                            "'exit' -- exit\n");
                } else if ( userCommand == "kill" || userCommand == "k" ) {

                    if (arrayOfConnection.empty()) {
                        std::cout << "No open connections" << std::endl;
                    } else {
                        printf("Kill user, enter number of client\n");

                        int client_number = -1;

                        for (int i = 0; i < arrayOfConnection.size(); i++) {
                            printf( "%d. Socket client: %d\n", i + 1, arrayOfConnection[i] );
                        }

                scanf("%d", &client_number);

                printf("The client has been disconnected\n", client_number - 1);

                shutdown(arrayOfConnection[client_number - 1], SHUT_RDWR);
                close(arrayOfConnection[client_number - 1]);
            }
        }else {
            std::cout << userCommand << ": command not found" << std::endl;
        }
    }

    shutdown( threadData->accept_socket, SHUT_RDWR );
    close( threadData->accept_socket );

    pthread_join( *thread, NULL );

    free(thread );
    free(threadData );

    exit( 0 );
}
