#include <stdlib.h>
#include <stdio.h>	
#include <string.h>	
#include <errno.h>					// for errno, EINTR
#include <signal.h> 				// for sigaction()
#include <unistd.h>					// for alarm()
#include "sendlib.h"				// for lossy_sendto()

#define BUFFER_SIZE 1000			// Buffer size
#define TIMEOUT_SECS 2				// Timeout seconds
#define MAX_TRIES 5					// Max tries before terminating (Peer inactive)

extern int tries;
extern float lossProbability;
extern int randomSeed;
extern int sock;							// Socket Descriptor
extern struct sockaddr_in serverAddress;	// Server address
extern struct sockaddr_in fromAddress;		// Source address of the received message
extern unsigned char receivedACK;
extern unsigned long bytesToReceive;		// Number of bytes to receive from server
extern unsigned long bytesReceived;			// Number of bytes received
extern unsigned int fromSize;				// In-out of address size for recvfrom()
extern unsigned long bytesToSend;			// Number of bytes to send to the server
extern unsigned long bytesSent;				// Number of bytes sent

void DieWithError(char* errorMessage);
void ParseCommandLineArguments(int argc, char* argv[], char** serverIP, unsigned short* serverPort, char** filePath, unsigned char* toFormat, char** toName, unsigned char* toNameSize, float* lossProbability, int* randomSeed);
void CatchAlarm(int ignored);			// Handler for SIGALARM

void send_wait(const void* buffer, unsigned long len);
