/********************************************************************************************************
*********************************************************************************************************
**  Created by Yiheng Gao, 4/16/2018, for project 3                                                    **
**                                                                                                     **
**  Server for network communication using sockets                                                     **
**                                                                                                     **
**  Functions:                                                                                         **
**    - Internal:                                                                                      **
**        int serverInit(int port)              // Initial server                                      **
**        void* handleClient(void *)	        // Thread handle function                              **
**        void recordMessage(int loc, char *message, char *from, char *time)                           **
**                                              // Record the message to recipient message table       **
**        int recordUser(int sd, char *name)    // Record the user info to known user table            **
**        int userLoc(char *name)               // Find the user location in known user table          **
**        char* getKnownUserName()              // Package known user name into a string               **
**        char* getActiveUserName()             // Package currently connected user name into a string **
**        void getTime();                       // Get system time                                     **
**        char* getMyMessage(int loc);          // Package a user message into a string                **
**                                                                                                     **
*********************************************************************************************************
********************************************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#define NAMESIZE 80				// max characters for a name
#define MESSAGESIZE 10			// max 10 messages for a user
#define MESSAGELENGTH 80        // max 80 characters for a message
#define USERSIZE 100            // max 100 known users in the system
#define BUFSIZE 80
#define HOST_NAME_LIMIT 80


// function declare
extern int writeStream(int sd, char *content);    // external function
extern int readStream(int sd, char *content);     // external function

static int serverInit(int port);                  // initial server
static void* handleClient(void *);			      // thread handle function, each active client has a thread
static void recordMessage(int loc, char *message, char *from, char *time);    // record recipient's message
static int recordUser(int sd, char *name);        // record user information (if user is unknown)
static int userLoc(char *name);                   // find the user location in known user table
static char* getKnownUserName();                  // package known user name into a string so that the server could send to client
static char* getActiveUserName();                 // package currently connected user into a string
static void getTime();                            // obtain system time
static char* getMyMessage(int loc);               // package a user's message into a string


typedef struct{
	char name[NAMESIZE];
	int sd;
	char *from[MESSAGESIZE];        // record who send the message
	char *time[MESSAGESIZE];        // record the time message send
	char *message[MESSAGESIZE];     // message content
	int messageCount;
} UserInfo;

typedef struct{
	UserInfo user[USERSIZE];
	int count;
} UserTable;

UserTable userTab;          // known user table

sem_t user_count;           // semaphore for UserTable.count
sem_t user_each[USERSIZE];  // semaphore for UserTable.user[i]


/************************************************************************
* Func:   Initial server, running server, create thread for each client *
* Param:  int argc, arguments count                                     *
*         char *argv[], arguments value                                 *
* Return: 0 normal exit                                                 *
************************************************************************/
int main(int argc, char *argv[])
{
	int sd;
	
	int sd_current;
	int *sd_client;
	int addrlen;
	struct sockaddr_in pin;
	pthread_t tid;			 // thread id
	pthread_attr_t attr;
	
	// check for command line arguments
	if (argc != 2)
	{
		printf("Usage: server port\n");
		exit(1);
	}
	
	sd = serverInit(atoi(argv[1]));

	// wait for a client to connect
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);    // use detached threads
	addrlen = sizeof(pin);
	while(1)
	{
		if ((sd_current = accept(sd, (struct sockaddr *) &pin, (socklen_t *) &addrlen)) == -1)
		{
			perror("Error on accept call\n");
			exit(1);
		}
		
		sd_client = (int *)(malloc(sizeof(sd_current)));
		*sd_client = sd_current;
		
		pthread_create(&tid, &attr, handleClient, sd_client);       // create thread for each client
	}
	
	// close socket
	close(sd);
}

/*********************************************
** Function:  Process client requirement    **
** Parameter: socket description(void *)    **
** Return:    None                          **
*********************************************/
void* handleClient(void *arg)
{
	char *name = (char *)malloc(sizeof(char) * NAMESIZE);
	char time[20];              // system time, format: 2018/04/15 08:20 PM
	int sd = *((int *)arg);     // get sd from arg
	free(arg);

	int loc;
	char command[2];       		// format: "2'\0'"
	
	readStream(sd, name);       // read a message from the client
	getTime(time);              // obtain system time	
	
	sem_wait(&user_count);
	loc = userLoc(name);        // find user location in known user table
	if(loc != -1)	            // known user
	{
		sem_wait(&user_each[loc]);
		if(userTab.user[loc].sd == -1)    // duplicate check, -1 means non active user
		{
			userTab.user[loc].sd = sd;    // update sd
			sem_post(&user_each[loc]);
			
			printf("%s, Connection by known user %s\n", time, name);
		}
		else                              // duplicate log in, error!
		{
			sem_post(&user_each[loc]);
			
			// error handle!
			writeStream(sd, "E");		  // Error occur, connection failed!
			printf("%s, %s duplicate log in, force out.\n", time, name);
			close(sd);
		}
	}
	else                                  // unknown user
	{
		loc = recordUser(sd, name);       // record the unknown user as known user
		printf("%s, Connection by unknown user %s\n", time, name);
	}
	sem_post(&user_count);
	
	writeStream(sd, "S");                 // log in success
	
	// interact with users
	readStream(sd, command);
	while(command[0] != '7')              // 7. Exit
	{
		char *tmp; 
		getTime(time);
		switch(command[0])
		{
			case '1':                     // display the names of all known users
				sem_wait(&user_count);
				tmp = getKnownUserName(); // get known users
				sem_post(&user_count);
				
				writeStream(sd, tmp);     // send to client
				free(tmp);

				printf("%s, %s displays all known users.\n", time, name);

				break;
			
			case '2':                         // display the names of all currently connected users
				sem_wait(&user_count);
				tmp = getActiveUserName();    // get currently connected users
				sem_post(&user_count);
				
				writeStream(sd, tmp);         // send to client
				free(tmp);

				printf("%s, %s displays all connected users\n", time, name);
				break;
				
			case '3':                         // send a text message to a particular user
			{
				char *toUser = (char *)malloc(sizeof(char) * NAMESIZE);
				char *message = (char *)malloc(sizeof(char) * MESSAGELENGTH);
				
			    readStream(sd, toUser);    // get receiver's name
				readStream(sd, message);   // message content
	
				// find user, if not exist, then record
				sem_wait(&user_count);
				int location = userLoc(toUser);                // find the recipient
				if(location == -1)                             // unkown user
				{
					location = recordUser(-1, toUser);         // not active at present
				}
				sem_post(&user_count);

				sem_wait(&user_each[location]);
				recordMessage(location, message, name, time);  // record the message to recipient
				sem_post(&user_each[location]);
									
				printf("%s, %s posts a message for %s\n", time, name, toUser);
				
				free(toUser);      
				free(message);
				break;
			}
			
			case '4':                           // send a text message to all currently connected users
			{
				char *message = (char *)malloc(sizeof(char) * MESSAGELENGTH);
				
				readStream(sd, message);        // read message
	
				sem_wait(&user_count);
				int tCount = userTab.count;
				sem_post(&user_count);
				int i;
				for(i = 0; i < tCount; i++)     // record the message into each active user
				{
					sem_wait(&user_each[i]);
					if(userTab.user[i].sd != -1)
						recordMessage(i, message, name, time);
					sem_post(&user_each[i]);
				}
									
				printf("%s, %s posts a message for currently connected users\n", time, name);
				     
				free(message);
				break;
			}
			
			case '5':                           // send a text message to all known users
			{
				char *message = (char *)malloc(sizeof(char) * MESSAGELENGTH);
				
				readStream(sd, message);
	
				sem_wait(&user_count);
				int tCount = userTab.count;
				sem_post(&user_count);
				int i;
				for(i = 0; i < tCount; i++)     // record the message into all users
				{
					sem_wait(&user_each[i]);
					recordMessage(i, message, name, time);
					sem_post(&user_each[i]);
				}
									
				printf("%s, %s posts a message for all known users\n", time, name);
				     
				free(message);
				break;
			}
			
			case '6':                           // get my messages
				sem_wait(&user_each[loc]);
				tmp = getMyMessage(loc);        // obtain my messages from known user table
				sem_post(&user_each[loc]);
				
				writeStream(sd, tmp);           // send to client
				
				free(tmp);
				printf("%s, %s gets messages\n", time, name);
				break;
			
			default:
				break;
		}
		readStream(sd, command);                // read next client request
	}
	
	// user exit
	sem_wait(&user_each[loc]);
	userTab.user[loc].sd = -1;    // update sd (non active)
	sem_post(&user_each[loc]);
	
	getTime(time);
	printf("%s, %s exits\n", time, name);
	free(name);
	
	close(sd);                    // close socket
}

/****************************************************************
* Func:   Find user location in known user table (userTab)      *
* Param:  char *name, user name                                 *
* Return: int, user location (known user)                       *
*         -1, unknown user                                      *
****************************************************************/
int userLoc(char *name)
{
	int i = 0;           // i means location

	while(i < userTab.count)
	{
		if(strcmp(userTab.user[i].name, name) == 0)
		{
			sem_post(&user_count);
			return i;    // known
		}
		i++;
	}

	return -1;           // unknown
}

/****************************************************************
* Func:   Record a message to recipient's message table         *
* Param:  int loc, location for recipient                       *
*         char *message, text to be recorded                    *
*         char *from, sender's name                             *
*         char *time, when is the sender send                   *
* Return: none                                                  *
****************************************************************/
void recordMessage(int loc, char *message, char *from, char *time)
{
	// parameter check
	if(loc >= USERSIZE || message == NULL)
		return;
	
	// 10 max message for a user
	int messageCount = userTab.user[loc].messageCount;
	if(messageCount >= MESSAGESIZE)
		return;
	
	userTab.user[loc].messageCount++;
	userTab.user[loc].from[messageCount] = (char *)malloc(sizeof(char) * (NAMESIZE+1));
	userTab.user[loc].message[messageCount] = (char *)malloc(sizeof(char) * (MESSAGELENGTH+1));
	userTab.user[loc].time[messageCount] = (char *)malloc(sizeof(char) * (19+1));
	
	strcpy(userTab.user[loc].from[messageCount], from);
	strcpy(userTab.user[loc].message[messageCount], message);
	strcpy(userTab.user[loc].time[messageCount], time);
}


/****************************************************************
* Func:   Record unknown user                                   *
* Param:  int sd, socket description                            *
*         char *name, unknown user name                         *
* Return: int, the location for unknown user                    *
****************************************************************/
int recordUser(int sd, char *name)
{
	int loc = userTab.count;
	if(loc >= USERSIZE)
		return -1;
	
	strcpy(userTab.user[userTab.count].name, name);
	userTab.user[userTab.count].sd = sd;
	userTab.count++;
	
	return loc;
}


/****************************************************************
* Func:   Package all known user name into a string             *
* Param:  none                                                  *
* Return: char*, known user name                                *
****************************************************************/
char* getKnownUserName()
{
	char *allName = (char *)malloc(sizeof(char) * (NAMESIZE * userTab.count));
	int i, offset = 0;
	
	for(i = 0; i < userTab.count; i++)
	{
		sprintf(allName+offset, "%s", userTab.user[i].name);
		offset += strlen(userTab.user[i].name);
		sprintf(allName+offset, "\n");
		offset++;
	}
	
	return allName;
}

/****************************************************************
* Func:   Package all active user name into a string            *
* Param:  none                                                  *
* Return: char*, active user name                               *
****************************************************************/
char* getActiveUserName()
{
	char *activeName = (char *)malloc(sizeof(char) * (NAMESIZE * userTab.count));
	int i, offset = 0;
	
	for(i = 0; i < userTab.count; i++)
	{
		if(userTab.user[i].sd != -1)
		{
			sprintf(activeName+offset, "%s", userTab.user[i].name);
			offset += strlen(userTab.user[i].name);
			sprintf(activeName+offset, "\n");
			offset++;
		}
	}
	
	return activeName;
}

/****************************************************************
* Func:   Obtain system time and transfer into specific format  *
* Param:  char *t, required time format                         *
* Return: none                                                  *
****************************************************************/
void getTime(char *t)
{
	time_t timep;
	struct tm *p_tm;
	timep = time(NULL);
	p_tm = localtime(&timep);
	
	if(p_tm->tm_hour <= 12)    // Format: MM/DD/YYYY HH:MM AM
		sprintf(t, "%d/%d/%d,%d:%d AM", p_tm->tm_mon+1, p_tm->tm_mday, p_tm->tm_year+1900, p_tm->tm_hour, p_tm->tm_min);
	else
		sprintf(t, "%d/%d/%d,%d:%d PM", p_tm->tm_mon+1, p_tm->tm_mday, p_tm->tm_year+1900, p_tm->tm_hour-12, p_tm->tm_min);
}

/****************************************************************
* Func:   Package all messages into a string                    *
* Param:  int loc, user location                                *
* Return: char*, all user messages                              *
****************************************************************/
char* getMyMessage(int loc)
{
	char *message = (char *)malloc(sizeof(char) * (MESSAGESIZE*(MESSAGELENGTH+NAMESIZE+20)));
	int i, offset = 0;
	
	if(userTab.user[loc].messageCount == 0)      // no message
	{
		message[0] = '\0';
		return message;
	}
	
	for(i = 0; i < userTab.user[loc].messageCount; i++)
	{
		// get messages
		sprintf(message+offset, "%s, %s, %s", userTab.user[loc].from[i], userTab.user[loc].time[i], userTab.user[loc].message[i]);
		offset += strlen(userTab.user[loc].from[i]) + strlen(userTab.user[loc].time[i]) + strlen(userTab.user[loc].message[i]) + 4;
		sprintf(message+offset, "\n");
		offset++;

		// clear messages
		free(userTab.user[loc].from[i]);
		free(userTab.user[loc].time[i]);
		free(userTab.user[loc].message[i]);
	}
	userTab.user[loc].messageCount = 0;
	
	return message;
}

/****************************************************************
* Func:   Initial server                                        *
* Param:  int port, port for the server                         *
* Return: int, socket description                               *
****************************************************************/
int serverInit(int port)
{
	int sd;
	char host[HOST_NAME_LIMIT];          // host name, max: 80 characters
	struct sockaddr_in sin;
	
	
	// create an internet domain stream stream socket
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)    // IPv4, TCP
	{
		perror("Error creating socket\n");
		exit(1);
	}
	
	// complete the socket structure
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;    // any address on this host
	sin.sin_port = htons(port);          // convert to network byte order
	
	// bind the socket to the address and port number
	if (bind(sd, (struct sockaddr *) &sin, sizeof(sin)) == -1)
	{
		perror("Error on bind call\n");
		exit(1);
	}
	
	// set queuesize of pending connections
	if (listen(sd, SOMAXCONN) == -1)
	{
		perror("Error on listen call\n");
		exit(1);
	}
	
	// announce server is running
	gethostname(host, HOST_NAME_LIMIT);
	printf("Server is running on %s: %d\n", host, port);
	
	// Initialize semaphores
	int i;
	if(sem_init(&user_count, 0, 1) == -1)
	{
		printf("Init user_count semaphore\n");
		exit(1);
	}
	for(i = 0; i < USERSIZE; i++)
	{
		if(sem_init(&user_each[i], 0, 1) == -1)
		{
			printf("Initial user_each semaphores\n");
			exit(1);
		}
	}
	
	return sd;
}