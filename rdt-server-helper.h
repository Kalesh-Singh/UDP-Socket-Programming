#include <stdlib.h>
#include <stdio.h>	
#include <string.h>	
#include <errno.h>					// for errno, EINTR
#include <signal.h> 				// for sigaction()
#include <unistd.h>					// for alarm()
#include "sendlib.h"				// for lossy_sendto()

#define BUFFER_SIZE 1000			// Buffer size
#define TIMEOUT_SECS 2				// Timeout seconds
#define MAX_TRIES 10				// Max tries before terminating (Peer inactive)

extern int tries;
extern long bytesReceived;					// Number of bytes received
extern long bytesSent;						// Number of bytes sent

void DieWithError(char* errorMessage);
void ParseCommandLineArguments(int argc, char* argv[], unsigned short* serverPort, float* lossProbability, int* randomSeed);
void CatchAlarm(int ignored);			// Handler for SIGALARM

void send_wait(int sock, float lossProbability, int randomSeed, const struct sockaddr_in* destAddr, socklen_t destAddrLen, const struct sockaddr_in* fromAddr, socklen_t fromAddrLen, const void* sendBuffer, unsigned long bytesToSend, void *restrict recvBuffer, unsigned long bytesToReceive);
