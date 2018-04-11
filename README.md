# The Application Design #
This TCP Application Protocol is used to send a file containing Units (as described below) from the client to server. The server receives the file and if it is correctly formatted it saves the file making any necessary translations between units as specified by the client. The server sends a response to the client indicating whether the operation was successuful or not.

## Architecture ##
This Application uses the Client/Server Architecture.

## Usage ##

### Client ###
The client can be compiled using the command:  
```gcc client.c -o client```  

The client should be invoked by the following command:  
```./client``` ```serverIP``` ```serverPort``` ```filePath``` ```toFormat``` ```toName```  

*Where:*
+ ```client``` is the name of the client executable file name
+ ```serverIP``` is the IP address of the server
+ ```serverPort``` is the TCP port of the server
+ ```filePath``` is the path of the file to be sent to the server. (The file path indicates the location of the file in the system on which the server runs. It includes the file name, and possibly the hierarchy ofdirectories.) There is no size limit of the file. 
+ ```toFormat``` indicates how the server should translate the received units. ```0``` means no translation, ```1``` means to only translate ```Type 0``` units to ```Type 1``` with ```Type 1``` units unchanged, ```2``` means to only translate ```Type 1``` units to ```Type 0``` with ```Type 0``` units unchanged, and ```3``` means to translate ```Type 0``` to ```Type 1``` and ```Type 1``` to ```Type 0```. 
+ ```toName``` is the name of the file the server should save the units to.

#### File Format ####
The content of the input file is a sequence of units. Each unit has one of the following two formats.
+ ```Type``` ```Amount``` ```Number1 Number2 ... NumberN```
+ ```Type``` ```Amount``` ```Number1, Number2, ... , NumberN```  

```Type``` is one byte with the binary value 0 or 1. The first format always has ```Type``` as 0, and the second always has ```Type``` as 1.
##### Type 0 Units #####
+ If ```Type``` is 0, the ```Amount``` is one byte. The binary value of ```Amount``` is the amount of numbers in the unit.
+ ```Number1``` through ```NumberN``` are the binary numbers, each taking 2 bytes.
##### Type 1 Units #####
+ If ```Type``` is 1, the ```Amount``` is 3 bytes. The three ASCII characters shows the amount of numbers in the unit. 
+ ```Number1``` through ```NumberN``` are unsigned integers no more than 65535 represented in
ASCII, separated by comma. There is no comma after the last number.

### Server ###
The server can be compiled using the command:  
```gcc server.c helper.h helper.c -o server```  

The server should be invoked by the following command:  
```./server``` ```port```  

*Where:*
+ ```server``` is the name of the server executable file name.
+ ```port``` is the port the server listens to.

## Protocol ##

### Message Types, Syntax and Semantics for Client ###
The client sends 2 messages to the server:
+ The first is a request to the server to receive the file.
+ The second is a response with the contents of the file.

#### Request ####
The request to the server to receive the file contains also contains client specified options that specify the operation the sever should perform on the file to be received. The client request has the following format:  
```toFormat``` ```toNameSize``` ```toName``` ```fileSize```  

*Where:*
+ ```toFormat``` is one byte.
+ ```toNameSize``` is one byte and therefore if the size of ```toName``` is greater than ```256```, the client throws an error ```FILE NAME TOO LONG``` and terminates.
+ ```toName``` is the name of the file to which the data must be written to on the server.
+ ```fileSize``` is the size in bytes of the file to be received by the server.

#### Response ####
Upon receiving a response from the server. The client compares the received integer response to the size of the ```options``` sent:

+ If the two values are the same, the client responds by sending the file containing the units to the server. The file is sent in chunks of 1 KB. Except for the last chunk which may be less.   
	```Chunk 1``` ```Chunk 2```.... ```Chunk N```
+ If the integer received is not the same size as the ```options``` sent, the client throws an error, ```ERROR SENDING OPTIONS``` and terminates.

### Message Types, Syntax and Semantics for Server ###
The server sends 2 responses to the client.

#### Response 1 ####
Upon receiving the ```options``` from the client the server send a response to client to indicate whether the ```options``` were correctly received or not. This is done by sending an ```integer``` value which is the amount of bytes received by the server.

#### Response 2 ####
After inidcating that the options were correctly received, the server continuously receives data from the client until it has received a total amount of bytes equal to that specified by ```fileSize``` in the received options:

+ If the file is correctly formatted the server performs the specified operations and responds with a byte containing the value ```0``` to the client. 
+ Else if the file is not correctly formatted, the server responds with a ```negative number``` depending on the type of format error detected.

### Rules For Sending Messages ###
+ Upon establishing a connection the client sends a ```Request``` to the server to receive the file.
+ The server sends a ```Response``` to the client acknowledging that it correctly received the ```options``` contained in the client's ```Request```.
+ Upon receiving the server's response acknowledging that it correctly received the ```options```, the client ```Responds``` by sending the file to the server.
+ Upon receiving the file from the cilent, the server sends a ```Response``` to the client indicating whether the received file was incorrectly formatted or that the requested translation was successful.

## Test Cases ##
The Input files used in the test can be found in the ```test_cases``` folder in the [GitHub Repository](https://github.com/ZonalWings/TCPProgramming)  

| Rationale | Input File | Expected Output | Actual Output | Error Observed |
| :------- | :---- | :-------------- | :------------ | :------------- |
| Testing for correct operation | ```practice_project_test_file_1``` | ```Success``` | ```Success``` | None |
| Testing for correct operation | ```practice_project_test_file_2``` | ```Success``` | ```Success``` | None |
| Testing for Incorrect Type | ```test_incorrect_type``` | ```Format error``` | ```Format error``` | None |
| Testing for Incorrect Amount (Non-Numeric ASCII Value in Type 1 Amount) | ```test_incorrect_amount``` | ```Format error``` | ```Format error``` | None |
| Testing for Incorrect Number (Non-Numeric ASCII Value in Type 1 Number) | ```test_incorrect_number``` | ```Format error``` | ```Format error``` | None |
| Testing for Consecutive Commas (In Type 1 Numbers) | ```test_consecutive_commas``` | ```Format error``` | ```Format error``` | None |
| Testing for Empty File | ```test_empty_file``` | ```Success``` | ```Success``` | None |

## Problems ##
+ There are no known problems with the client or server applications at present.

## References ##
+ Donahoo, M. J., & Calvert, K. L. (2001). TCP/IP SOCKETS IN C: Practical Guide for Programmers (The Practical Guide Series). San Francisco, CA: Morgan Kaufmann.

## Github Repository ##
[TCP Socket Programming](https://github.com/ZonalWings/TCPProgramming)

## Contributors ##
+ Kaleshwar Singh
