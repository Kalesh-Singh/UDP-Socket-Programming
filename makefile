all:
	gcc rdt-server.c rdt-server-helper.* sendlib.* unitslib.* -o server
	gcc rdt-client.c rdt-client-helper.* sendlib.* -o client
