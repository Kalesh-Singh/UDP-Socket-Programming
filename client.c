#include <stdio.h>					// for print() and fprintf()
#include <sys/socket.h>				// for socket(), connect(), sendto(), and recvfrom()
#include <arpa/inet.h>				// for sockaddr_in and inet_addr()
#include <stdlib.h>					// for atoi()
#include <string.h>					// for memset()
#include <unistd.h>					// for close()

#define BUFFER_SIZE	 200			// Buffer size

void DieWithError(char* errorMessage) {
	// Error handling function
	perror(errorMessage);
	exit(1);
}

int main(int argc, char* argv[]) {
	int sock;							// Socket descriptor
	struct sockaddr_in serverAddress;	// Server address
	struct sockaddr_in fromAddress;		// Source address of the received message
	unsigned char serverPort;			// Server port
	unsigned int fromSize;				// In-out of address size for recvfrom()
	char* serverIP;						// IP address of server
	char* msgToSend;					// Message to send to server
	char buffer[BUFFER_SIZE + 1];		// Buffer for receiving messages
	int msgToSendLen;					// Length of message to send
	int receivedMsgLen;					// Length of received message

	if ((argc < 3) || (argc > 4)) { 	// Test for correct number of arguments
		fprintf(stderr, "Usage: %s <Server IP> <Echo Word> [<Echo Port>]\n", argv[0]);
		exit(1);
	}

	serverIP = argv[1];			// First arg: server IP address (dotted quad)
	msgToSend = argv[2];		// Second arg: message to send

	if ((msgToSendLen = strlen(msgToSend)) > BUFFER_SIZE)		// Check message length
		DieWithError("Message too long");

	if (argc == 4)
		serverPort = atoi(argv[3]);		// Use the give port; if any
	else
		serverPort = 7;					// 7 is a well known port for echo service

	// Create a datagram/UDP socket
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		DieWithError("socket() failed");

	// Construct the server address structure
	memset(&serverAddress, 0, sizeof(serverAddress));			// Zero out structure
	serverAddress.sin_family = AF_INET;							// Internet addr family
	serverAddress.sin_addr.s_addr = inet_addr(serverIP);		// Server IP address
	serverAddress.sin_port = htons(serverPort);					// Server Port

	printf("Sending message to server...\n");

	// Send the message to the server
	if (sendto(sock, msgToSend, msgToSendLen, 0, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) != msgToSendLen)
		DieWithError("sendto() sent a different number of bytes than expected");

	printf("Sent message to server...\n");
	printf("Wainting for response from server ...\n");

	// Receive a response
	fromSize = sizeof(fromAddress);
	if ((receivedMsgLen = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &fromAddress, &fromSize)) != msgToSendLen)
		DieWithError("recvfrom() failed");

	printf("Received response from server ...\n");

	if (serverAddress.sin_addr.s_addr != fromAddress.sin_addr.s_addr) {
		fprintf(stderr, "Error: received a packet from unknown source.\n");
		exit(1);
	}

	// null-terminate the received data
	buffer[receivedMsgLen] = '\0';
	printf("Received: %s\n", buffer);			// Print the recieved message

	close(sock);
	exit(0);
}
