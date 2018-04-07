#include <stdlib.h>
#include <stdio.h>	
#include <string.h>	

extern int tries;

void DieWithError(char* errorMessage);
void ParseCommandLineArguments(int argc, char* argv[], char** serverIP, unsigned short* serverPort, char** filePath, unsigned char* toFormat, char** toName, unsigned char* toNameSize, float* lossProbability, int* randomSeed);
void CatchAlarm(int ignored);			// Handler for SIGALARM
