#include <stdio.h>					// for printf() and fprintf() 
#include <sys/socket.h>				// for socket(), connect(), sendto(), and recvfrom()
#include <arpa/inet.h>				// for sockaddr_in and inet_addr()
#include <stdlib.h>					// for atoi()
#include <string.h> 				// for memset()
#include <unistd.h>					// for close()

#define BUFFER_SIZE 200				// Buffer size

void DieWithError(char* errorMessage) {		
	// Error handling function
	perror(errorMessage);
	exit(1);
}

int main(int argc, char* argv[]) {
	int sock;								// Socket
	struct sockaddr_in serverAddress;		// Local address
	struct sockaddr_in clientAddress;		// Client Address
	unsigned int clientAddrLen;				// Length of incoming message
	char buffer[BUFFER_SIZE];				// Buffer to send data
	unsigned short serverPort;				// Server port
	int recvMsgSize;						// Size of received message

	if (argc != 2) {						// Test for correct number of parameters
		fprintf(stderr, "Usage: %s <UDP SERVER PORT>\n", argv[0]);
		exit(1);
	}

	serverPort = atoi(argv[1]);		// First arg: local port

	// Create socket for sending/receiving datagrams
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		DieWithError("socket() failed");

	// Cosntruct local address structure
	memset(&serverAddress, 0, sizeof(serverAddress));		// Zero out structure 
	serverAddress.sin_family = AF_INET;						// Internet address family
	//serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);		// Any incoming interface 
	serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddress.sin_port = htons(serverPort);				// Local port

	// Bind to the local address
	if (bind(sock, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)
		DieWithError("bind() failed");

	while (1) { 		// Run forever
		// Set the size of the in-out parameter
		clientAddrLen = sizeof(clientAddress);
		
		printf("Waiting for message from client ...\n");
		
		// Block until receive message from a client
		if ((recvMsgSize = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &clientAddress, &clientAddrLen)) < 0)
			DieWithError("recvfrom() failed");

		printf("Receive messgage from client...\n");
		
		printf("Handling client %s\n", inet_ntoa(clientAddress.sin_addr));

		printf("Sending response to client...\n");

		// Send the received datagram back to client
		if (sendto(sock, buffer, recvMsgSize, 0, (struct sockaddr *) &clientAddress, 
sizeof(clientAddress)) != recvMsgSize)
			DieWithError("sendto() sent a different number of bytes than expected");

		printf("Sent response to client...\n");
	}

	// NOT REACHED
}
