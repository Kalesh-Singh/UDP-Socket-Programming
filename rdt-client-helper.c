#include "rdt-client-helper.h"

int tries = 0;

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
