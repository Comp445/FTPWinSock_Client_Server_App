/*a small file Server
Usage: suppose Server is running on sd1.encs.concordia.ca and server is running on sd2.encs.concordia.ca
.Also suppose there is a file called test.txt on the server.
In the Server,issuse "Server sd2.encs.concordia.ca test.txt size" and you can get the size of the file.
In the Server,issuse "Server sd2.encs.concordia.ca test.txt time" and you can get creation time of the file
*/

#pragma once
#pragma comment( linker, "/defaultlib:ws2_32.lib" ) //This is your basic WINSOCK shell
#include <winsock.h>
#include <windows.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <process.h>
#include <iostream>
#include <string.h>
#include <fstream>
#include "Thread.h"
#include "server.h"
#include "string"
#include <windows.h>

using namespace std;

struct PACKET
{
    char data[1300];
	bool FIRST_PACKET_OF_STREAM;
    bool LAST_PACKET_OF_STREAM;
};

union 
{
	struct sockaddr generic;
	struct sockaddr_in ca_in;}ca;
	int calen=sizeof(ca); 

//port data types
#define REQUEST_PORT 0x7070
int port=REQUEST_PORT;

//buffer data types
char szbuffer[2000];
char *buffer;
int ibufferlen;
int ibytesrecv;
int ibytessent;

//socket data types
SOCKET s;
SOCKET s1;
SOCKADDR_IN sa;      // filled by bind
SOCKADDR_IN sa1;     // fill with server info, IP, port

//file buffer data
PACKET * packet_collection;
int amount_of_packets;

//host data types
char localhost[11];
HOSTENT *hp;

//wait variables
int nsa1;
int r,infds=1, outfds=0;
struct timeval timeout;
const struct timeval *tp=&timeout;
fd_set readfds;

//others
HANDLE test;
DWORD dwtest;

TcpServer::TcpServer()
{
	WSADATA wsadata;
	if (WSAStartup(0x0202,&wsadata)!=0)
		TcpThread::err_sys("Starting WSAStartup() error\n");
	
	//Display name of local host
	if(gethostname(servername,HOSTNAME_LENGTH)!=0) //get the hostname
		TcpThread::err_sys("Get the host name error,exit");
	
	printf("Server: %s waiting to be contacted for time/size request...\n",servername);
	
	
	//Create the server socket
	if ((serverSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		TcpThread::err_sys("Create socket error,exit");
	
	//Fill-in Server Port and Address info.
	ServerPort=REQUEST_PORT;
	memset(&ServerAddr, 0, sizeof(ServerAddr));      /* Zero out structure */
	ServerAddr.sin_family = AF_INET;                 /* Internet address family */
	ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);  /* Any incoming interface */
	ServerAddr.sin_port = htons(ServerPort);         /* Local port */
	
	//Bind the server socket
    if (bind(serverSock, (struct sockaddr *) &ServerAddr, sizeof(ServerAddr)) < 0)
		TcpThread::err_sys("Bind socket error,exit");
	
	//Successfull bind, now listen for Server requests.
	if (listen(serverSock, MAXPENDING) < 0)
		TcpThread::err_sys("Listen socket error,exit");
}

TcpServer::~TcpServer()
{
	WSACleanup();
}


void TcpServer::start()
{
	for (;;) /* Run forever */
	{
		/* Set the size of the result-value parameter */
		clientLen = sizeof(ServerAddr);

		/* Wait for a Server to connect */
		if ((clientSock = accept(serverSock, (struct sockaddr *) &ClientAddr, 
			&clientLen)) < 0)
			TcpThread::err_sys("Accept Failed ,exit");
		
        /* Create a Thread for this new connection and run*/
		TcpThread * pt=new TcpThread(clientSock,serverSock);
		pt->start();
	}
}

//////////////////////////////TcpThread Class //////////////////////////////////////////
void TcpThread::err_sys(char * fmt,...)
{     
	perror(NULL);
	va_list args;
	va_start(args,fmt);
	fprintf(stderr,"error: ");
	vfprintf(stderr,fmt,args);
	fprintf(stderr,"\n");
	va_end(args);
	exit(1);
}
unsigned long TcpThread::ResolveName(char name[])
{
	struct hostent *host;            /* Structure containing host information */
	
	if ((host = gethostbyname(name)) == NULL)
		err_sys("gethostbyname() failed");
	
	/* Return the binary, network byte ordered address */
	return *((unsigned long *) host->h_addr_list[0]);
}

/*
msg_recv returns the length of bytes in the msg_ptr->buffer,which have been recevied successfully.
*/
int TcpThread::msg_recv(int sock,Msg * msg_ptr)
{
	int rbytes,n;
	
	for(rbytes=0;rbytes<MSGHDRSIZE;rbytes+=n)
		if((n=recv(sock,(char *)msg_ptr+rbytes,MSGHDRSIZE-rbytes,0))<=0)
			err_sys("Recv MSGHDR Error");
		
		for(rbytes=0;rbytes<msg_ptr->length;rbytes+=n)
			if((n=recv(sock,(char *)msg_ptr->buffer+rbytes,msg_ptr->length-rbytes,0))<=0)
				err_sys( "Recevier Buffer Error");
			
			return msg_ptr->length;
}

/* msg_send returns the length of bytes in msg_ptr->buffer,which have been sent out successfully
*/
int TcpThread::msg_send(int sock,Msg * msg_ptr)
{
	int n;
	if((n=send(sock,(char *)msg_ptr,MSGHDRSIZE+msg_ptr->length,0))!=(MSGHDRSIZE+msg_ptr->length))
		err_sys("Send MSGHDRSIZE+length Error");
	return (n-MSGHDRSIZE);
	
}

void TcpThread::run() 
{

	WSADATA wsadata;

		try
		{        		 
			if (WSAStartup(0x0202,&wsadata)!=0)  
				cout<<"Error in starting WSAStartup()\n";
			
			else
			{
				buffer="WSAStartup was suuccessful\n";   
				WriteFile(test,buffer,sizeof(buffer),&dwtest,NULL); 

				/* display the wsadata structure */
				cout<< endl
					<< "wsadata.wVersion "       << wsadata.wVersion       << endl
					<< "wsadata.wHighVersion "   << wsadata.wHighVersion   << endl
					<< "wsadata.szDescription "  << wsadata.szDescription  << endl
					<< "wsadata.szSystemStatus " << wsadata.szSystemStatus << endl
					<< "wsadata.iMaxSockets "    << wsadata.iMaxSockets    << endl
					<< "wsadata.iMaxUdpDg "      << wsadata.iMaxUdpDg      << endl;
			}  

			//Display info of local host
			gethostname(localhost,10);
			cout<<"ftpd_tcp starting at host: "<<localhost<< endl;

			if((hp=gethostbyname(localhost)) == NULL) 
			{
				cout << "gethostbyname() cannot get local host info?"
					<< WSAGetLastError() << endl; 
				exit(1);
			}

			//Successfull bind, now listen for client requests.
			if(listen(SERVER_SOCKET,10) == SOCKET_ERROR)
				throw "couldn't  set up listen on socket";
			else cout << "Listen was successful" << endl;

			FD_ZERO(&readfds);

			int close_command=0;
			//wait loop

			cout << "waiting to be contacted for transferring files..." << endl;

				FD_SET(SERVER_SOCKET,&readfds);  //always check the listener

				if(!(outfds=select(infds,&readfds,NULL,NULL,tp))) {}

				else if (outfds == SOCKET_ERROR) throw "failure in Select";

				else if (FD_ISSET(s,&readfds))  cout << "got a connection request" << endl; 

				//Fill in szbuffer from accepted request.
				if((ibytesrecv = recv(CLIENT_SOCKET,szbuffer,128,0)) == SOCKET_ERROR) //CLIENT SENDS THIS SERVER A REQUEST TYPE.
					throw "Receive error in server program\n";

				std::string msg_converted( reinterpret_cast< char const* >(szbuffer) ); 

				if(msg_converted == "get") //THE REQUEST IS A GET-TYPE
				{
						char response_msg[128];
						sprintf_s(response_msg,"ok");

						if(send(CLIENT_SOCKET,response_msg,128,0)==SOCKET_ERROR) //SERVER SENDS A OK CONFIRMATION TO CLIENT.
							throw "Send error in server program\n";

						char filenamebuffer[128];
						if((ibytesrecv = recv(CLIENT_SOCKET,filenamebuffer,128,0)) == SOCKET_ERROR)  //CLIENT SENDS FILE-NAME TO SERVER.
							throw "Receive error in server program\n";

						std::string file_name_converted( reinterpret_cast< char const* >(filenamebuffer) ); //should be receiving the filename

						//------------------------------------------------------------------------------------------------------------------
						//PROCESS FILE AND SEND IT BACK....
						//------------------------------------------------------------------------------------------------------------------
						ifstream file (file_name_converted, ios::in|ios::binary|ios::ate);
						if (file.is_open())
						{
								int filesize = file.tellg();
								char * memblock = new char [filesize];
								file.seekg (0, ios::beg);
								file.read (memblock, filesize);
								file.close();

								//CREATE PACKETS BASED ON SIZE OF FILE
								amount_of_packets = ceil((filesize/1300.0));
								int position_of_buffer = 0;
								packet_collection = new PACKET[amount_of_packets];
								int packet_number=0;
								int byte_in_packet=0;
								int dummy=0;

								for(packet_number=0; packet_number< amount_of_packets; packet_number++)
								{
									for(byte_in_packet=0; byte_in_packet<1300; ++byte_in_packet)
									{
										if(position_of_buffer < filesize)
										{
											memcpy (&packet_collection[packet_number].data[byte_in_packet], &memblock[position_of_buffer++],1);
											if(byte_in_packet==1299)
												memcpy (&packet_collection[packet_number].data[byte_in_packet+1], "\0" ,1); //LAST CHAR MUST BE A END
											continue;
										}
									}
								}
								delete[] memblock; //get rid of the in-memory buffer storage.
						}

						else 
								throw "Unable to open file";
						//------------------------------------------------------------------------------------------------------------------
						//------------------------------------------------------------------------------------------------------------------
						//------------------------------------------------------------------------------------------------------------------
						//NOW AFTER BREAKING THE BINARY FILE IN PARTS, WE WILL NOW SEND IT IN PIECES. 
						// 1. SEND NUMBER OF PACKETS TO SEND.
						// 2. CLIENT SAYS "start"
						// 3. SERVER STARTS SENDING PACKET. ONE BY ONE.
						// 4. CLIENT RECEIVES 1 DATA PIECE.
						// 5. CLIENT SENDS A RESPONSE MSG.
						// 5. 5.1 - IF RESPONSE == "ACCEPTED", THEN SEND ANOTHER PACKET
						//    5.2 - IF RESPONSE == "FAILED", THEN SEND THE SAME PACKET OVER AGAIN. (BUT TCP ENSURES SUCCESS, SO NOT SURE IF WE WILL DO STEP 5.2)
						// 4. REPEAT STEP 5 UNTIL LAST PACKET.
						sprintf_s(response_msg,"ok");

						char int_to_char[128];
						itoa (amount_of_packets,int_to_char,10); //convert int to char array
								
						if(send(CLIENT_SOCKET,int_to_char,128,0)==SOCKET_ERROR) //SERVER SENDS THE TOTAL # OF DATA CHUNKS TO SEND.
								throw "Send error in server program\n";

						char start_command[128];
						if(recv(CLIENT_SOCKET,start_command,128,0) == SOCKET_ERROR) // WAIT FOR 'start' command from client
								throw "get data failed 1\n";

						std::string start_command_string_form( reinterpret_cast< char const* >(start_command) );

						if(start_command_string_form=="start")
						{
								// in a for loop, we send each packet, piece by piece.
								for(int packet_counter=0; packet_counter < amount_of_packets; packet_counter++)
								{
										if(send(CLIENT_SOCKET,packet_collection[packet_counter].data,1300,0)==SOCKET_ERROR) //SERVER SENDS data.
												throw "Send error in server program\n";

										char receive_msg[128];

										if((ibytesrecv = recv(CLIENT_SOCKET,receive_msg,128,0)) == SOCKET_ERROR) // WAIT FOR 'received' msg from client
												throw "get data failed 2\n";
								}
						}											
				}

				else if(msg_converted == "put")
				{
						//TODO
				}

		} //end of try 

		//Display needed error message.
		catch(char* str)
		{ 
			cerr<<str<<WSAGetLastError()<<endl;
			int test;
			cin >> test;
		}

		//close Client socket
		closesocket(CLIENT_SOCKET);		

		cout << "SERVER IS DONE" << endl;
		/* When done uninstall winsock.dll (WSACleanup()) and exit */ 
		WSACleanup();
}


int main(void)
{
	
	TcpServer ts;
	ts.start();
	
	return 0;
}


