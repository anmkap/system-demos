#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>

using namespace std;

int main(int argc, char *argv[])
{
	if(argc < 3)
	{
		fprintf(stderr, "Usage: client <port_number> <host_name>\n");
		exit(0);
	}

	struct sockaddr_in server_addr;
	struct hostent *server;
	int portno = atoi(argv[1]);
	server = gethostbyname(argv[2]);
	if(server == NULL)
	{
		fprintf(stderr, "No such host exists\n");
		exit(0);
	}

	server_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr,server->h_length);
	server_addr.sin_port = htons(portno);

	while(1)
	{
		string com,op,tar1,tar2;
		getline(cin,com);

		vector<int> idx(0);
		for(int i=0; i<com.size(); i++)
		{
			if (com.substr(i,1) == " ") idx.push_back(i);

		}

		for(int i=0; i<idx.size(); i++)
		{
			if (i == 0)
			{
				op = com.substr(0,idx.at(i));
				if (idx.size() == 1) tar1 = com.substr(idx.at(i)+1,com.size()-idx.at(i));
			}
			else 
			{
				tar1 = com.substr(idx.at(i-1)+1,idx.at(i)-idx.at(i-1)-1);
				tar2 = com.substr(idx.at(i)+1,com.size()-idx.at(i-1));
			}
		}

		if (idx.size() == 0) op = com;

		for (int i=0; i<op.size(); i++) op[i] = tolower(op[i]);
		for (int i=0; i<tar1.size(); i++) tar1[i] = tolower(tar1[i]);
		for (int i=0; i<tar2.size(); i++) tar2[i] = tolower(tar2[i]);

		int flg = 1;

		if (op == "create" or op == "query")
		{
			if (tar1 == "" or tar2 != "")
			{
				fprintf(stderr, "account ID cannot be NULL and amount has to be NULL.\n");
				flg = 0;
			}
			for (int i=0; i<tar1.size(); i++) 
			{
				if (tar1.substr(i,1) == "." and tar1.size()-i > 3)
				{
					fprintf(stderr, "invalid amount precision: maximum of 2 decimal places.\n");
					flg = 0;
				}	
			}
			if (tar1.size() >= 12)
			{
				fprintf(stderr, "argument is too large: has to be less or equal to 12 digits\n");
				flg = 0;
			}
		}
		else if (op == "update")
		{	
			if (tar1 == "" or tar2 == "" or tar2.size() > 12)
			{
				fprintf(stderr, "account ID and/or amount cannot be NULL and amount cannot be larger than 999,999,999.99\n");
				flg = 0;
			}
			for (int i=0; i<tar2.size(); i++) 
			{
				if (tar2.substr(i,1) == "." and tar2.size()-i > 3)
				{
					fprintf(stderr, "invalid amount precision: maximum of 2 decimal places.\n");
					flg = 0;
				}	
			}
			if (tar1.size() >= 12 or tar2.size() >= 12)
			{
				fprintf(stderr, "argument is too large: has to be less or equal to 12 digits\n");
				flg = 0;
			}
		}
		else if (op == "quit")
		{
			cout << "OK" << endl;
			exit(0);
		}
		else
		{
			fprintf(stderr, "illegal command: recognized commands are create, update, query, quit.\n");
			flg = 0;
		}

		if (flg == 1)
		{
			int socketfd = socket(AF_INET, SOCK_STREAM, 0);
			if(socketfd < 0)
			{
				fprintf(stderr, "Error creating socket\n");
				exit(0);
			}
		
			if(connect(socketfd,(struct sockaddr*) &server_addr,sizeof(server_addr)) < 0)
			{
				fprintf(stderr, "Error connecting\n");
				exit(0);				
			}
			char msg[32];
			strcpy(msg,op.c_str());
			strcpy(msg+8,tar1.c_str());
			strcpy(msg+20,tar2.c_str());
	
			int n = write(socketfd, msg, 32);
			if(n < 0)
			{
				fprintf(stderr, "Error writing to socket\n");
				exit(0);	
			}
			
			double repDub;
			string reply;
			char rcv[64];
			bzero(rcv,sizeof(rcv));
			n = read(socketfd, rcv, 64);
			if(n < 0)
			{
				fprintf(stderr, "Error writing to socket\n");
				exit(0);	
			}
			reply.assign(rcv,rcv+64);
			cout << reply << endl;
		}
	}
}