#include "unitslib.h"

long getFileSize(FILE* fp) {
	/* Return the file size; does not change the current position in the file. */
	long currPos = ftell(fp);
	fseek(fp, 0L, SEEK_END);
	long size = ftell(fp);
	fseek(fp, currPos, SEEK_SET);
	return size;
}

uint8_t getType(FILE* fp) {
	/* Returns the Type of the Unit */
	return fgetc(fp);
}

uint8_t getT0Amount(FILE* fp) {
	/* Returns the amount of a Type 0 Unit */
	return fgetc(fp);
}

void getT1Amount(FILE* fp, char* t1Amount) {
	/* Reads the amount of a Type 1 Unit into t1Amount */
	fread(t1Amount, sizeof(char), 3, fp);
}

int validateT1Amount(char* t1Amount) {
	/* Checks that the amount is only made up of numeric ASCII digits */
	int i;
	for(i = 0; i < 3; ++i) {
		if (t1Amount[i] < 48 && t1Amount[i] > 57) {
			return -1;		// If the byte is not a valid numeric ASCII digit
		}
	}
	return 0;
}

void t0AmountTot1Amount(uint8_t t0Amount, char* t1Amount) {
	/* Converts t0Amount to its corresponding Type 1 Amount and stores it into t1Amount */ 
	int i;
	for (i = 2; i >= 0; --i) {
		t1Amount[i] = (t0Amount % 10) + 48;
		t0Amount /= 10;
	}
}

uint16_t t1AmountTot0Amount(char* t1Amount) {
	/* Returns the Type 0 equivalent of a t1Amount */
	uint16_t t0Amount = -1;
	t0Amount = atoi(t1Amount);

    return t0Amount;
}

void populateT0Buffer(FILE* fp, uint16_t* buffer, uint8_t amount) {
	/* Reads the Numbers of a Type 0 Unit into buffer */
	fread(buffer, sizeof(uint16_t), amount, fp);
}

void printT0Numbers(uint16_t* buffer, uint8_t amount) {
	/* Prints the Type 0 Numbers of a Unit to the screen */
	int i;
	for (i = 0; i < amount; ++i) {
		// Converting from Big to Little Endian
		uint16_t number = (buffer[i] << 8) | (buffer[i] >> 8);
		if (i == amount-1)
			printf("%d", number);
		else
			printf("%d,", number);
	}
}

int sizeOfT1Numbers(FILE* fp, long fileSize, uint16_t amount) {
	/* Returns the size in bytes of the Numbers of a Type 1 Unit */
	long currPos = ftell(fp);
	int size = 0;
	uint8_t currByte;
	int prevCommaPos = -1;
	int numOfCommas = 0;
	while (1) {
		currByte = fgetc(fp);
		if((currByte == 0 || currByte == 1) && (numOfCommas == amount - 1)) {
			// printf("Number of commas = %d\n", numOfCommas);
			fseek(fp, -1, SEEK_CUR);
			break;
		}

		if ((currByte < 48 && currByte > 57) && (currByte != ','))
			return -1;	// Digit not numeric or a comma
		if (currByte == ',') {
			if ((ftell(fp) - prevCommaPos) < 2) {
				return -2;	// Comma followed by another comma
			}
			numOfCommas += 1;
		}

		size++;
		if(ftell(fp) == fileSize)
			break;
	}
	fseek(fp, currPos, SEEK_SET);
	return size;
}

void populateT1Buffer(FILE* fp, uint8_t* buffer, uint8_t amount) {
	/* Reads the Numbers of a Type 1 Unit into buffer */
	fread(buffer, sizeof(uint8_t), amount, fp);
}

void printT1Numbers(uint8_t* buffer, int unitSize) {
	/* Prints the Type 1 Numbers of a Unit to the screen */
	int i;
	for (i = 0; i < unitSize; ++i) {
		printf("%c", buffer[i]);
	}
}

void writeType0(FILE* out, uint8_t amount, uint16_t* buffer) {
	/* Writes a Type 0 Unit to the out file, given Type 0 data */

	// Set the type to 0
	uint8_t type = 0;

	// Determine the size of data to be written
	long size = 1 + 1 + (amount * 2);

	// Create a write buffer 
	uint8_t writeBuffer[size];

	// Populate the write buffer
	memcpy(writeBuffer, &type, 1);
	memcpy(writeBuffer + 1, &amount, 1);
	memcpy(writeBuffer + 2, buffer, amount * 2);

	// Write the contents of the write buffer to the file
	fwrite(writeBuffer, sizeof(uint8_t), size, out);

}

void writeType0FromType1(FILE* out, uint8_t amount, uint8_t* buffer, int unitSize) {
	/* Writes a Type 0 Unit to the out file, given Type 1 data */

	// Set the type to 0
	uint8_t type = 0;


	// Convert the Type 0 Numbers to Type 1 Numbers
	char t1Nums[amount][5];
	memset(t1Nums, 48, sizeof(char) * amount * 5);

	int pos = unitSize - 1;
	int i, j;
	int prevJ;
	for (int i = amount - 1; i >= 0; --i) {
		for (int j = 4; j >= 0; --j) {
			if (buffer[pos] == ',') {
				pos--;
				if (prevJ == 0) {
					i++;
				}
				prevJ = j;
				break;
			} else {
				t1Nums[i][j] = buffer[pos];
				pos--;
				if (pos < 0) {
					prevJ = j;
					break;
				}
			}
			prevJ = j;
		}
	}

	uint16_t t0Nums[amount];

	for (int i = 0; i < amount; ++i) {
		uint16_t t0Num = atoi(t1Nums[i]);
		t0Nums[i] = (t0Num << 8) | (t0Num >> 8);
	}

	// Determine the size of data to be written
	long size = 1 + 1 + (amount * 2);

	// Create a write buffer 
	uint8_t writeBuffer[size];

	// Populate the write buffer
	memcpy(writeBuffer, &type, 1);
	memcpy(writeBuffer + 1, &amount, 1);
	memcpy(writeBuffer + 2, t0Nums, amount * 2);


	// Write the contents of the write buffer to the file
	fwrite(writeBuffer, sizeof(uint8_t), size, out);
}

 void writeType1FromType0 (FILE* out, uint8_t amount, uint16_t* buffer) {
	/* Writes a Type 1 Unit to the out file, given Type 0 data */

	// Create a write buffer
	char writeBuffer[6000]; 	/* will never be more than 6000; since 
	amount must be <= 999, Numbers must have <= 5 digits (i.e < 65535)  
	and at most 998 commas */

	// Zero the write buffer
	memset(writeBuffer, 0, 6000);

	// Initialize the size of data to be written
	int size = 0;
	
	// Add the type to the write buffer
	uint8_t type = 1;
	memcpy(writeBuffer + size, &type, 1);
	size += 1;

	// Convert the Type 0 Amount to a Type 1 Amount
	char t1Amount[3];
	t0AmountTot1Amount(amount, t1Amount);

	// Add the amount to the write buffer
	memcpy(writeBuffer + size, t1Amount, 3);
	size += 3;

	// Convert the Type 0 Numbers to Type 1 Numbers and add them to the buffer
	char comma = ',';
	int i;
	for (i = 0; i < amount; ++i) {
		// Convert from Big to Little Endian
		uint16_t number = (buffer[i] << 8) | (buffer[i] >> 8);
		char tempNum[6];
		int length = snprintf(tempNum, 6, "%d", number);
		memcpy(writeBuffer + size, tempNum, length);
		size += length;
		if (i != amount - 1) {
			memcpy(writeBuffer + size, &comma, 1);
			size += 1;
		}
	}
	// Write the contents of the write buffer to the file
	fwrite(writeBuffer, sizeof(uint8_t), size, out);
}

void writeType1(FILE* out, char* t1Amount, uint8_t* buffer, int unitSize) {
	/* Writes a Type 1 Unit to the out file, given Type 1 data */

	// Set the type to 0
	uint8_t type = 1;

	// Determine the size of data to be written
	long size = 1 + 3 + unitSize;

	// Create a write buffer 
	uint8_t writeBuffer[size];

	// Populate the write buffer
	memcpy(writeBuffer, &type, 1);
	memcpy(writeBuffer + 1, t1Amount, 3);
	memcpy(writeBuffer + 4, buffer, unitSize);

	// Write the contents of the write buffer to the file
	fwrite(writeBuffer, sizeof(uint8_t), size, out);

}

int writeUnits(FILE* in, FILE* out, char toFormat) {
	/* Writes the Units rean from in to out performing the appropriate conversions as specified by toFormat */

	// Get the size of the File in bytes
    long fileSize = getFileSize(in);
	
    // Go back to the beginning of the file
    rewind(in);

	if (toFormat != -1)
		printf("\nDATA RECIEVED FROM CLIENT:\n");
	else 
		printf("\nDATA WRITTEN ON SERVER:\n");

    while (ftell(in) < fileSize - 1) {
        // Get the Type of the Unit
		uint8_t type = getType(in);
        printf("Type: %d\t\t", type);

        if (type == 0) {
            // Get the amount in the unit
            uint8_t amount = getT0Amount(in);
	
			// Convert the Type 0 Amount to a Type 1 Amount
			char t1Amount[3]; 
			t0AmountTot1Amount(amount, t1Amount);

			// Print the amount
			char printBuffer[4];
			memcpy(printBuffer, t1Amount, 3);
			printBuffer[3] = '\0';
            printf("Amount: %s\t", printBuffer);

            // Get the Numbers in the Type 0 Unit
            uint16_t buffer[amount];
			populateT0Buffer(in, buffer, amount);

			// Print the Numbers to the Screen
			printT0Numbers(buffer, amount);
			printf("\n");

			if ((toFormat == 0) || (toFormat == 2)) {
				// Write the Type 0 Unit to the output file
				writeType0(out, amount, buffer);;
			} else if ((toFormat == 1) || (toFormat == 3)) {
				// Write Type 1 Unit to the output file
				writeType1FromType0 (out, amount, buffer);
			}

        } else if (type == 1) {
            // Get the amount in the unit
            char t1Amount[3];
            getT1Amount(in, t1Amount);

			// Check for valid amount
			if (validateT1Amount(t1Amount) < 0) {
				fprintf(stderr, "INVALID AMOUNT (TYPE 1): Expects numeric ASCII characters.\n");
				return -1;
			}

			// Print the amount to the screen
			char printBuffer[4];
			memcpy(printBuffer, t1Amount, 3);
			printBuffer[3] = '\0';
			printf("Amount: %s\t", printBuffer);

            // Convert the t1Amount to an integer value
            uint16_t amount = (uint16_t) t1AmountTot0Amount(t1Amount);

			// Get the size of the Type 1 Unit Numbers in bytes
			int unitSize = sizeOfT1Numbers(in, fileSize, amount);
			
			// Check for errors in the format of the numbers
			if (unitSize == -1) {
				fprintf(stderr, "TYPE 1 FORMAT ERROR: Each byte is expected to be either a numeric ASCII digit or ASCII comma.\n");
				return -1;
			} else if (unitSize == -2) {
				fprintf(stderr, "TYPE 1 FORMAT ERROR: Commas must be separated by at least 1 numberic ASCII digit.\n");
				return -2;
			}

			// Get the Numbers in Type 1 Unit
			uint8_t buffer[unitSize];
			populateT1Buffer(in, buffer, unitSize);

			// Print the Numbers to the screen
			printT1Numbers(buffer, unitSize);
			printf("\n");

			if ((toFormat == 0) || (toFormat == 1)) {
				// Write Type 1 to the out file
				writeType1(out, t1Amount, buffer, unitSize);
			} else if ((toFormat == 2) || (toFormat == 3)) {
				// Write Type 0 to the out file
				writeType0FromType1(out, amount, buffer, unitSize);
			}

		} else {
			fprintf(stderr, "INVALID UNIT TYPE: Expects 0 or 1.\n");
			return -1;
		}		
    } 

	if (toFormat != -1) {
		writeUnits(out, in, -1);
	}

	// Write successful
	return 0;
}

