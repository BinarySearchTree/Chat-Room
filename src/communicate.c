/**********************************************************************************************
***********************************************************************************************
**  Created by Yiheng Gao, 4/16/2018, for project 3                                          **
**                                                                                           **
**  read/write for network communication using sockets                                       **
**                                                                                           **
**  Functions:                                                                               **
**    - External:                                                                            **
**        int writeSream(int sd, char *content);      // write to sd                         **
**        int readStream(int sd, char *content);      // read from sd                        **
**                                                                                           **
***********************************************************************************************
**********************************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define MAX_MESSAGE 80			// 80 characters max for a message


extern int writeSream(int sd, char *content);
extern int readStream(int sd, char *content);


/****************************************************************
* Func:   Write to sd                                           *
*         format: 3 chars for length of the content             *
*                 rest chars would be the content itself        *
* Param:  int sd, socket description                            *
*         char *content, content to be write                    *
* Return: 0 success                                             *
****************************************************************/
int writeStream(int sd, char *content)
{
	int count = 0;
	
	// create package
	int len = strlen(content);		
	char *message = (char *)malloc(sizeof(char) * (len + 3 + 1));	  // last one should be '\0'
	sprintf(message, "%3d%s", len, content);
	
	// send package
	while((count += write(sd, message, strlen(message))) != strlen(message));
	
	free(message);
	
	return 0;
}


/****************************************************************
* Func:   Read from sd                                          *
* Param:  int sd, socket description                            *
*         char *content, content to be written                  *
* Return: 0 success                                             *
****************************************************************/
int readStream(int sd, char *content)
{
	char len_char[3];
	int len;
	int len_count = 0, count = 0;

	// get the length of the message
	while((len_count += read(sd, len_char, 3)) != 3);        // read first 3 character
	len = atoi(len_char);                                    // get length of the message
	
	// read the content
	while((count += read(sd, content, len)) != len);
	content[len] = '\0';			                         // add EOF for string
	
	return 0;
}