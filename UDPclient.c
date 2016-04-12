//Nick Schrock
//CIS_457 Lab2 
//TCP Echo Client

#include <sys/socket.h>
#include <netinet/in.h> //network protocols 
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define CHUNK_SIZE 256

typedef struct eth_hdr{
	char dst_addr[6];
	char src_addr[6];
	uint16_t _type;
}eth_hdr;

typedef struct ip_hdr{
	uint8_t version: 4;
	uint8_t ihl: 4;
	uint8_t dscb;
	uint16_t total_length;
	uint16_t id;
	uint16_t frag_offset;
	uint8_t ttl;
	uint8_t protocol;
	uint16_t checksum;
	char src_addr[4];
	char dst_addr[4];
}ip_hdr;

void push_data(eth_hdr& eth, ip_hdr& ip, char* data, char* buf, int datalen){
	memset(buf, 0, 290);
	memcpy(buf, &eth, 14);
	memcpy(&buf[14], &ip, 20);
	memcpy(&buf[34], data, datalen);
}

void pull_data(eth_hdr& eth, ip_hdr& ip, char* data, char* buf, int datalen){
	memcpy(&eth, buf, 14);
	memcpy(&ip, &buf[14], 20);
	memcpy(data, &buf[34], datalen);
}

int main (int argc, char** argv){

	//SOCK_STREAM choses network layer
	//AF_INET specifies an internet socket
	int port;
	eth_hdr eth;
	ip_hdr ip;
	printf("Enter a port ");
	scanf("%d", &port);

	char ip_addr[5000];
	printf("Enter an IP address ");
	scanf("%s", ip_addr);

  	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd<0){
	  printf("There was an arror creating the socket\n") ;
	  return 1;
	}
	else{
		printf("Socket Created\n");
	}
	
	struct sockaddr_in serveraddr;
	serveraddr.sin_family = AF_INET;

	//specify port number and put them in the correct order
	//htns == host to network short
	serveraddr.sin_port = htons(port);
	//inet is loopback server... useful for client and server on same machine
	serveraddr.sin_addr.s_addr = inet_addr(ip_addr);

	char client_status[10] = "yes";
	char filename[5000];
	char file_status[100];
	char new_file[5000];
	char title[10] = "new_file.";
	char buffer[CHUNK_SIZE];
	int i, j = 0;
	unsigned int frame_count =0;
	unsigned char frame_num;

	socklen_t len = sizeof(serveraddr);

	while(strcmp(client_status, "yes") == 0){

		//Retrieve filename from user
	 	printf("Enter a filename to transfer: ");
		scanf("%s", filename);

		//Name file "new_file"
		for(i=0; i<strlen(title); i++){
			new_file[i] = title[i];
		}

		//Parse extension
		bool flag = false;
		for(i=0; i<sizeof(filename); i++){
			if(filename[i] == '.'){
				flag = true;
				strcat(new_file, &filename[i+1]);
			}
			
		}

		//Send filename to client
		sendto(sockfd, filename, strlen(filename),0, (struct sockaddr*)&serveraddr, len);
		printf("Sent from client: %s \n", filename);

		//Recieve file status from server
		//recv(sockfd, file_status, 100, 0);
		//printf("File Status: %s\n", file_status);

		//Make sure server found the file okay then start writing data
		//if(strcmp(file_status, "Error opening file.\n") != 0){

			//Create new file to store incoming data
			FILE *fp;
			fp = fopen(new_file, "w");
			printf("Creating new file\n");
			if(fp == NULL){
				printf("Error creating file\n");
			}

			int incoming_data = 0;

			//Clear buffer
			memset(buffer, 0, sizeof(buffer));
			int n;
			//Write each chunk of data to newly created file
			do{
				frame_count++;
				//incoming_data = read(sockfd, buffer, CHUNK_SIZE);
				n = recvfrom(sockfd, buffer, CHUNK_SIZE+4, 0, (struct sockaddr*)&serveraddr, &len);
				printf("Received: %d\n", n);
				memcpy(&frame_num, &buffer, 4);
				printf("Frame Count: %d   Received Frame: %d\n", frame_count, frame_num);
				printf("Writing to file\n");
				fwrite(&buffer[4], 1, n-4, fp);
				printf("Write success\n");

				//Send acknowledgement;
				sendto(sockfd, &frame_num, strlen(filename),0, (struct sockaddr*)&serveraddr, len);

			}while( n >= CHUNK_SIZE);

			if(incoming_data < 0){
				printf("Error reading data\n");
			}
		//}

		//Ask user if they would like to transfer another file
		printf("\n\nyes to transfer another file. \nno to exit\n");
		scanf("%s", client_status);
		send(sockfd, client_status, sizeof(client_status), 0);

		//Clear Strings
		memset(filename,0,strlen(filename));
		memset(file_status,0,strlen(file_status));
		memset(new_file,0,strlen(new_file));
	}
	return 0;


	 
}


