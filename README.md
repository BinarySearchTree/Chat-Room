# Chat-Room

The project utilize Sockets for communication between processes to demonstrate a message posting system.

The server will maintain messages posted by clients, which clients can retrieve and review.

Client:
1)	Accept a machine name and port number to connect to as command line arguments.
2)	Connect to the server.
3)	Prompt for and send the user’s name.
4)	Present the following menu of choices to the user:
a.	Display the names of all known users.
b.	Display the names of all currently connected users.
c.	Send a text message to a particular user.
d.	Send a text message to all currently connected users.
e.	Send a text message to all known users.
f.	Get my messages.
g.	Exit.
5)	Interact with the server to support the menu choices.
6)	Ask the user for the next choice or exit.


Server: 
1)	Accept a port number as a command line argument.
2)	Accept connections from clients.
3)	Create a new thread for each client.
4)	Store messages sent to each user.
5)	End by termination with control-C.


Server Thread:
1)	Accept and process requests from the client.
2)	Add the user’s name to the list of known users.
3)	Provide mutual exclusion protection for the data structure that stores the messages.
4)	Send only the minimal data needed to the client, not the menu or other UI text.

Other rules:
1)	Each client transaction should interact with the server.  Clients will not communicate directly with each other.
2)	Configuration:   your server should support multiple different clients at the same time, but should not allow the same user name to have more than one connection at the same time. 
3)	Authentication:   assume the client has privileges to use the system—do not require a password.
4)	Limits:  you can assume a maximum of 100 known users, and a maximum of 10 messages each, where each message is at most 80 characters long.
5)	Persistence:  when the server exits the messages it is storing are lost. They will not be saved to a file.  When a user gets their messages, those messages are removed from the server.
6)	Users:  a known user is any user who has connected during the server session, but may or may not be currently connected.  Also, a message sent to an unknown user makes them known. 
7)	Errors:  obvious errors should be caught and reported.  For example, an invalid menu choice. 
8)	Output:  your output should use the same wording and format as the sample output.

============================================================================

design.pdf contains data format that records messages exchanged between client and server.

=============================================================================

Step for compiling and executing the project:
- S1: Copy the source files into Linux System
- S2: Open at least two terminals (SSH for example)
- S3: Change the direct to source files
- S4: Compile: gcc server.c communicate.c -pthread -o server;
             gcc client.c communicate.c -o client
- S5: Run the program on different terminals: 
              ./server 2000 (for server, port can be specified randomly);
              ./client csgrads1.utdallas.edu 2000 (client, port should the same with server)
