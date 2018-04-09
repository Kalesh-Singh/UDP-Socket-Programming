#include "rdt-server-helper.h"

int tries;
long bytesReceived;					// Number of bytes received
long bytesSent;						// Number of bytes sent

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

void send_wait(int sock, float lossProbability, int randomSeed, const struct sockaddr_in* destAddr, socklen_t destAddrLen, const struct sockaddr_in* fromAddr, socklen_t fromAddrLen, const void* sendBuffer, unsigned long bytesToSend, void *restrict recvBuffer, unsigned long bytesToReceive) {

	// Send the data 
	if ((bytesSent = lossy_sendto(lossProbability, randomSeed, sock, sendBuffer, bytesToSend, (struct sockaddr *) destAddr, destAddrLen)) != bytesToSend)
		DieWithError("lossy_sendto() sent a different number of bytes than expected");

	// Reset tries
	tries = 0;

	// Set the Timeout
	alarm(TIMEOUT_SECS);
	
	while (((bytesReceived = recvfrom(sock, recvBuffer, bytesToReceive, 0, (struct sockaddr *) fromAddr, &fromAddrLen)) < 0)) {
		if (errno == EINTR)	{			// Alarm went off
			if (tries < MAX_TRIES) {	// Incremented by signal handler
				printf("recvfrom() timed out, %d more tries ...\n", MAX_TRIES - tries);
				
				// Resend the data
				if ((bytesSent = lossy_sendto(lossProbability, randomSeed, sock, sendBuffer, bytesToSend, (struct sockaddr *) destAddr, destAddrLen)) != bytesToSend)
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
}

