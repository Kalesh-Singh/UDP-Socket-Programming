#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ECHOMAX 255

void DieWithError(char* errorMessage) {
	// Error handling function
	perror(errorMessage);
	exit(1);
}

int main(int argc, char *argv[])
{
	int sock;
	struct sockaddr_in echoServAddr;
	struct sockaddr_in echoClntAddr;
	unsigned int cliAddrLen;
	char echoBuffer[ECHOMAX];
	unsigned short echoServPort;
	int recvMsgSize;

	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <UDP SERVER PORT>\n", argv[0]) ;
		exit(1);
	}

	echoServPort = atoi(argv[1]);

	/* Create socket for sending/receiving datagrams */
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		DieWithError("socket() failed");

	/* Construct local address structure */
	memset(&echoServAddr, 0, sizeof(echoServAddr));
	echoServAddr.sin_family = AF_INET;
	echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	echoServAddr.sin_port = htons(echoServPort);	

	if (bind(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
		DieWithError("bind() failed");

	for (;;) /* Run forever */
	{
		/* Set the size of the in-out parameter */
		cliAddrLen = sizeof(echoClntAddr);
		
		/* Block until receive message from a client */
		if ((recvMsgSize = recvfrom(sock, echoBuffer, ECHOMAX, 0, (struct sockaddr *) &echoClntAddr, &cliAddrLen)) < 0)
			DieWithError("recvfrom() failed") ;
	
		printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr)) ;

		/* Send received datagram back to the client */
		if (sendto(sock, echoBuffer, recvMsgSize, 0, (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != recvMsgSize)
			DieWithError("sendto() sent a different number of bytes than expected");
	}
	// NOT REACHED
}


