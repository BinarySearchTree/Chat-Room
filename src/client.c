/**********************************************************************************************
***********************************************************************************************
**  Created by Yiheng Gao, 4/16/2018, for project 3                                          **
**                                                                                           **
**  Client for network communication using sockets                                           **
**                                                                                           **
**  Functions:                                                                               **
**    - Internal:                                                                            **
**        int connectToServer(char *hn, char *port);  // client connect to server            **
**        void logIn(int sd, char *name);             // user log in                         **
**        void displayMenu();                         // display UI text                     **
**        void displayName(char *buffer);             // display the message from server     **
**        void displayMessage(char *message);         // display the message from server     **
**        char getUserChoice();                       // read user choice from command line  **
**        int ack(int sd);                            // get acknowledge when log in         **
**                                                                                           **
***********************************************************************************************
**********************************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

#define BUFFERSIZE		100		
#define NAMELENGTH		80      // each name is 80 characters maximum 
#define MENUSIZE		7       // 7 choices total for menu
#define MESSAGELENGTH	80      // 80 characters maximum for each message
#define MESSAGESIZE		10		// 10 messages max for a user

extern int writeStream(int sd, char *content);    // external function declare from communicate.c
extern int readStream(int sd, char *content);     // external function declare from communicate.c

static int connectToServer(char *hn, char *port); // client connect to server
static void logIn(int sd, char *name);            // user log in
static void displayMenu();                        // display UI text
static void displayName(char *buffer);            // display names from the server
static void displayMessage(char *message);        // display my messages from the server
static char getUserChoice();                      // read menu choice from command line
static int ack(int sd);                           // get acknowledge when log in


/****************************************************************
* Func:   Connect client, user log in, interact with user       *
* Param:  int argc, arguments count                             *
*         char *argv[], arguments value                         *
* Return: 0 normal exit                                         *
****************************************************************/
int main(int argc, char *argv[])
{	
	char name[NAMELENGTH];    // user's name
	int sd;
	char command;             // read from command line
	
	// check for command line arguments
	if (argc != 3)
	{
		printf("Usage: client host port\n");
		exit(1);
	}
	
	sd = connectToServer(argv[1], argv[2]);
	logIn(sd, name);
	if(ack(sd) == -1)		// log in error, namely, duplicate log in
	{
		printf("User already log in!\n");
		exit(1);
	}
	
	command = getUserChoice();
	while(command != '7')    // 7. Exit
	{
		switch(command)
		{
			case '1':        // display the names of all known users
			{
				char *buffer = (char *)malloc(sizeof(char) * (NAMELENGTH * 100));    // maximum known users: 100

				writeStream(sd, "1");
				readStream(sd, buffer);
				
				printf("\nKnown users:\n");
                displayName(buffer);
				
				free(buffer);
				
				break;
			}
				
			case '2':         // display the names of all currently connected users
			{
				char *buffer = (char *)malloc(sizeof(char) * (NAMELENGTH * 100));     // maximum known users: 100

				writeStream(sd, "2");
				readStream(sd, buffer);
				
				printf("\nCurrently connected users:\n");
				displayName(buffer);
				
				free(buffer);
				
				break;
			}
			
			case '3':          // send a text message to a particular user
			{
				char *recip = (char *)malloc(sizeof(char) * (NAMELENGTH+1));
				char *message = (char *)malloc(sizeof(char) * (MESSAGELENGTH+1));
				
				writeStream(sd, "3");
				
				printf("Enter recipient's name: ");      // get recipient from command line
				fgets(recip, NAMELENGTH, stdin);
				recip[strlen(recip)-1] = '\0';
				
				printf("Enter a message: ");
				fgets(message, MESSAGELENGTH, stdin);    // get a message from command line
				message[strlen(message)-1] = '\0';
				
				writeStream(sd, recip);                  // send to server
				writeStream(sd, message);
				
				free(recip);
				free(message);
			}
				break;
				
			case '4':          // send a text message to all currently connected users
			{
				char *message = (char *)malloc(sizeof(char) * (MESSAGELENGTH+1));
				
				writeStream(sd, "4");
				
				printf("Enter a message: ");             // get a message from command line
				fgets(message, MESSAGELENGTH, stdin);
				message[strlen(message)-1] = '\0';
				
				writeStream(sd, message);                // send the message to server
				
				free(message);
			}
				break;
			
			case '5':          // send a text message to all known users
			{
				char *message = (char *)malloc(sizeof(char) * (MESSAGELENGTH+1));
				
				writeStream(sd, "5");
				
				printf("Enter a message: ");
				fgets(message, MESSAGELENGTH, stdin);     // get a message from command line
				message[strlen(message)-1] = '\0';
				
				writeStream(sd, message);                 // send the message to server
				
				free(message);
			}
				break;
				
			case '6':          // Get my messages
			{
				char *message = (char *)malloc(sizeof(char) * (MESSAGESIZE * (MESSAGELENGTH + NAMELENGTH + 20)));

				writeStream(sd, "6");
				
				readStream(sd, message);
				
				displayMessage(message);
				
				free(message);
			}
				break;
				
			default:
				printf("%c, Invalid command, retry!\n", command);
				break;
		}
		// read next command
		command = getUserChoice();
	}
	
	writeStream(sd, "7");      // Inform the server that I'm log out
	
	close(sd);                 // close the socket
}


/****************************************************************
* Func:   Connect to server using socket                        *
* Param:  char *hn, host name from command line                 *
*         char *port_name, port name from command line          *
* Return: int sd, socket description                            *
****************************************************************/
int connectToServer(char *hn, char *port_name)
{
	char hostname[NAMELENGTH];
	int sd;
	int port;
	int count;
	struct sockaddr_in pin;
	struct hostent *hp;
	
	strncpy(hostname, hn, sizeof(hostname));    // get host name
	hostname[NAMELENGTH-1] = '\0';
	port = atoi(port_name);                     // get port
	
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)    // create an Internet domain stream socket
	{
		perror("Error creating socket");
		exit(1);
	}
	
	if ((hp = gethostbyname(hostname)) == 0)             // lookup host machine information
	{
		perror("Error on gethostname call\n");
		exit(1);
	}
	
	// fill in the socket address structure with host information
	memset(&pin, 0, sizeof(pin));
	pin.sin_family = AF_INET;
	pin.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
	pin.sin_port = htons(port);                                      // convert to network byte order
	
	printf("Connecting to %s: %d\n\n", hostname, port);
	
	if (connect(sd, (struct sockaddr *) &pin, sizeof(pin)) == -1)    // connect to port on host
	{
		perror("Error on connect call\n");
		exit(1);
	}
	
	return sd;
}


/****************************************************************
* Func:   User log in                                           *
* Param:  int sd, socket description                            *
*         char *name, user name from command line               *
* Return: none                                                  *
****************************************************************/
void logIn(int sd, char *name)
{
	printf("Enter your name: ");      // get user name
	fgets(name, NAMELENGTH, stdin);
	name[strlen(name)-1] = '\0';

	writeStream(sd, name);            // log in
}


/****************************************************************
* Func:   Display UI text                                       *
* Param:  none                                                  *
* Return: none                                                  *
****************************************************************/
void displayMenu()
{
	char *menu[MENUSIZE] = {"1. Display the names of all known users.",
	                        "2. Display the names of all currently connected users.",
							"3. Send a text message to a particular user.",
							"4. Send a text message to all currently connected users.",
							"5. Send a text message to all known users.",
							"6. Get my messages.",
							"7. Exit."};
	int i;
	
	printf("\n");
	for(i = 0; i < MENUSIZE; i++)
		printf("%s\n", menu[i]);
	printf("Enter your choice: ");
}

/****************************************************************
* Func:   Output names received from server                     *
* Param:  char *buffer, content to be displayed                 *
* Return: none                                                  *
****************************************************************/
void displayName(char *buffer)
{
    int i=0, count = 1;

    printf("  %d. ", count++);
    while(buffer[i]!='\0')
    {
        printf("%c", buffer[i]);
        if(buffer[i] == '\n' && buffer[i+1] != '\0')
			printf("  %d. ", count++);
	
	    i++;
    }
	printf("\n");
}

/****************************************************************
* Func:   Output messages received from user                    *
* Param:  char *message, content to be displayed                *
* Return: none                                                  *
****************************************************************/
void displayMessage(char *message)
{
    int i=0, count = 1;

    // Format output
	printf("\nYour messages:\n");
	
    printf("  %d. From ", count++);
    while(message[i]!='\0')
    {
        printf("%c", message[i]);
        if(message[i] == '\n' && message[i+1] != '\0')
			printf("  %d. From ", count++);
	
	    i++;
    }
	printf("\n");
}


/****************************************************************
* Func:   Get the acknowledge when user try to log in           *
* Param:  int sd, socket description                            *
* Return: 0 log in success                                      *
*         1 log in failure (duplicate log in)                   *
****************************************************************/
int ack(int sd)
{
	int result = 0;
	char *buffer = (char *)malloc(sizeof(char) * BUFFERSIZE);  // 'E\0' or 'S\0'
	
	readStream(sd, buffer);      // read log in result: 'E': error, 'S': success
	
	if(buffer[0] == 'E')
	{
		result = -1;
	}
	free(buffer);
	
	return result;
}


/****************************************************************
* Func:   Get user choice from command line                     *
* Param:  none                                                  *
* Return: char, user choice                                     *
****************************************************************/
char getUserChoice()
{
	char tmp;
	char *buffer = (char *)malloc(sizeof(char) * BUFFERSIZE);
	
	displayMenu();                      // UI
	
	fgets(buffer, BUFFERSIZE, stdin);   // read user choice from command line
	tmp = buffer[0];
	
	free(buffer);
	
	return tmp;
}