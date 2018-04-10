#include "rdt-server-helper.h"

int tries;
long bytesReceived;					// Number of bytes received
long bytesSent;						// Number of bytes sent
char sendPacketBuffer[BUFFER_SIZE + 1];
char recvPacketBuffer[BUFFER_SIZE + 1];
unsigned char seqNum;

void DieWithError(char* errorMessage) {
	// Error handling function
	perror(errorMessage);
	exit(1);
}

void ParseCommandLineArguments(int argc, char* argv[], unsigned short* serverPort, float* lossProbability, int* randomSeed) {
	if ((argc != 4)) { 					// Test for correct number of arguments
		fprintf(stderr, "Usage: %s <server_port> <loss_porbalility> <random_seed>\n", argv[0]);
		exit(1);
	}

	*serverPort = atoi(argv[1]);		// 1st arg: server port

	*lossProbability = atof(argv[2]);	// 2nd arg: loss probability
	if (*lossProbability < 0 ||*lossProbability >= 1) {
		printf("\nLoss Probabilty must be in the range [0, 1)\n\n");
		exit(EXIT_SUCCESS);
	}
	
	*randomSeed = atoi(argv[3]);		// 3rd arg: random seed		
}

void CatchAlarm(int ignored) {
	tries += 1;
}

void send_wait(int sock, float lossProbability, int randomSeed, const struct sockaddr_in* destAddr, socklen_t destAddrLen, const struct sockaddr_in* fromAddr, socklen_t fromAddrLen, void* restrict sendBuffer, unsigned long bytesToSend, void *restrict recvBuffer, unsigned long bytesToReceive) {

	// Make the packe to send
	unsigned long packetLen = makePacket(sendPacketBuffer, &seqNum, sendBuffer, bytesToSend);

	// Send the data 
	if ((bytesSent = lossy_sendto(lossProbability, randomSeed, sock, sendPacketBuffer, packetLen, (struct sockaddr *) destAddr, destAddrLen)) != packetLen)
		DieWithError("lossy_sendto() sent a different number of bytes than expected");

	// Increment the sequence number	
	seqNum = (seqNum + 1) % 2;

	// Reset tries
	tries = 0;

	// Set the Timeout
	alarm(TIMEOUT_SECS);

	// Invalidate the recv packet buffer
	memset (recvPacketBuffer, -1, BUFFER_SIZE + 1);
	
	while (((bytesReceived = recvfrom(sock, recvPacketBuffer, sizeof(char) + bytesToReceive, 0, (struct sockaddr *) fromAddr, &fromAddrLen)) < 0) || (recvPacketBuffer[0] != seqNum)) {
		printf("Expected Sequence Number = %d\n", seqNum);
		printf("Receiveq Sequnece Number = %d\n", recvPacketBuffer[0]);
		if (errno == EINTR)	{			// Alarm went off
			if (tries < MAX_TRIES) {	// Incremented by signal handler
				printf("recvfrom() timed out, %d more tries ...\n", MAX_TRIES - tries);
				
				// Resend the data
				if ((bytesSent = lossy_sendto(lossProbability, randomSeed, sock, sendPacketBuffer, packetLen, (struct sockaddr *) destAddr, destAddrLen)) != packetLen)
					DieWithError("lossy_sendto() sent a different number of bytes than expected");

				// Restart the timer
				alarm(TIMEOUT_SECS);
			} else
				DieWithError("No Response");
		} else
			DieWithError("recvfrom() failed");
	}

	// recvfrom() got something -- cancel the timeout
	alarm(0);

	// Extract the received packet
	char temp;
	extractPacket(recvPacketBuffer, &temp, recvBuffer, bytesToReceive);
}

unsigned long makePacket(char* packetBuffer, char* seqNum, void* restrict dataBuffer, unsigned long dataBytes) {
	memcpy(packetBuffer, seqNum, sizeof(char));
	memcpy(packetBuffer + sizeof(char), dataBuffer, dataBytes);
	return dataBytes + 1;
}

void extractPacket(char* packetBuffer, char* seqNum, void* restrict dataBuffer, unsigned long dataBytes) {
	memcpy(seqNum, packetBuffer, sizeof(char));
	memcpy(dataBuffer, packetBuffer + sizeof(char), dataBytes);
}
