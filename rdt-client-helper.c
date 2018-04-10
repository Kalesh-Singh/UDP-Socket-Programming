#include "rdt-client-helper.h"

int tries;
long bytesReceived;					// Number of bytes received
long bytesSent;						// Number of bytes sent
char sendPacketBuffer[BUFFER_SIZE + 1];
char recvPacketBuffer[BUFFER_SIZE + 1];
unsigned char seqNum = 0;

void DieWithError(char* errorMessage) {
	// Error handling function
	perror(errorMessage);
	exit(1);
}

void ParseCommandLineArguments(int argc, char* argv[], char** serverIP, unsigned short* serverPort, char** filePath, unsigned char* toFormat, char** toName, unsigned char* toNameSize, float* lossProbability, int* randomSeed) {
	if ((argc != 8)) { 					// Test for correct number of arguments
		fprintf(stderr, "Usage: %s <server_IP> <server_port> <file_path> <to_format> <to_name> <loss_porbalility> <random_seed>\n", argv[0]);
		exit(1);
	}

	*serverIP = argv[1];					// First arg: server IP address (dotted quad)

	int temp = atoi(argv[2]);				// Second arg: server port number
	if (temp < 0 || temp > 65535) {
		printf("\nInvalid Port Number: ");
		printf("Port number range is 0 to 65535\n\n");
		exit(EXIT_SUCCESS);
	}
	*serverPort = temp;
		
	*filePath = argv[3];					// Third arg: file to send

	temp = atoi(argv[4]);					// Fourth arg: to format
	if (temp < 0 || temp > 3) {
		printf("\nInvalid value for to_format\n\n");
		printf("to_format Options:\n");
		printf("\t0 - Write units to [to_name] without performing any conversion\n");
		printf("\t1 - Convert Type 0 Units to Type 1 (leaving Type 1 units unchanged), before writing units to [to_name]\n");
		printf("\t2 - Convert Type 1 Units to Type 0 (leaving Type 0 units unchanged), before writing units to [to_name]\n");
		printf("\t3 - Convert Type 0 Units to Type 1 and Type 1 Units to Type 0, before writing units to [to_name]\n\n");
		exit(EXIT_SUCCESS);
	}
	*toFormat = temp;

	*toName = argv[5];						// 5th arg: to name

	temp = strlen(*toName);
	if (temp > 256) {
		printf("\nFILE NAME TOO LONG: to_name cannot be greater than 256 characters\n\n");
		exit(EXIT_SUCCESS); 
	}
	*toNameSize = temp;

	*lossProbability = atof(argv[6]);					// 6th arg: loss probability
	if (*lossProbability < 0 ||*lossProbability >= 1) {
		printf("\nLoss Probabilty must be in the range [0, 1)\n\n");
		exit(EXIT_SUCCESS);
	}
	

	*randomSeed = atoi(argv[7]);			// 7th arg: random seed
		
}

void CatchAlarm(int ignored) {
	tries += 1;
}

void send_wait(int sock, float lossProbability, int randomSeed, const struct sockaddr_in* destAddr, socklen_t destAddrLen, const struct sockaddr_in* fromAddr, socklen_t fromAddrLen, void* restrict sendBuffer, unsigned long bytesToSend, void *restrict recvBuffer, unsigned long bytesToReceive) {

	// Icrement the sequence number
	seqNum = (seqNum + 1) % 2;

	// Make the packet to send
	unsigned long packetLen = makePacket(sendPacketBuffer, &seqNum, sendBuffer, bytesToSend);

	// Send the data 
	if ((bytesSent = lossy_sendto(lossProbability, randomSeed, sock, sendPacketBuffer, packetLen, (struct sockaddr *) destAddr, destAddrLen)) != packetLen)
		DieWithError("lossy_sendto() sent a different number of bytes than expected");

	// Reset tries
	tries = 0;

	// Set the Timeout
	alarm(TIMEOUT_SECS);

	// Invalidate the recv packet buffer
	memset (recvPacketBuffer, -1, BUFFER_SIZE + 1);
	
	while (((bytesReceived = recvfrom(sock, recvPacketBuffer, sizeof(char) + bytesToReceive, 0, (struct sockaddr *) fromAddr, &fromAddrLen)) < 0) || (recvPacketBuffer[0] != seqNum)) {
		printf("Expected Sequence Number = %d\n", seqNum);
		printf("Received Sequence Number = %d\n", recvPacketBuffer[0]);
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
