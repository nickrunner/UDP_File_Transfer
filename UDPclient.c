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

typedef struct packet{
	unsigned int frame_num;
	uint16_t checksum;
	char data[256];
}packet;


uint16_t calc_checksum(void* vdata,size_t length) {
    // Cast the data pointer to one that can be indexed.
    char* data=(char*)vdata;

    // Initialise the accumulator.
    uint32_t acc=0xffff;

    // Handle complete 16-bit blocks.
    for (size_t i=0;i+1<length;i+=2) {
        uint16_t word;
        memcpy(&word,data+i,2);
        acc+=ntohs(word);
        if (acc>0xffff) {
            acc-=0xffff;
        }
    }

    // Handle any partial block at the end of the data.
    if (length&1) {
        uint16_t word=0;
        memcpy(&word,data+length-1,1);
        acc+=ntohs(word);
        if (acc>0xffff) {
            acc-=0xffff;
        }
    }

    // Return the checksum in network byte order.
    return htons(~acc);
}

void print_buf(packet p){
	int i, j;
	printf("\nFrame num: %d\n", p.frame_num);
	printf("checksum: %d\n", p.checksum);
	printf("\n\nDATA\n\n");
	for(i=0; i<CHUNK_SIZE; i++){
		printf("%02x ", (unsigned char)p.data[i]);
		if(i%16 == 0 && i != 0){
			printf("\n");
		}
	}
}


int main (int argc, char** argv){

	//SOCK_STREAM choses network layer
	//AF_INET specifies an internet socket
	int port;
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
	unsigned int received_frame = 0;
	unsigned int send_num;
	uint16_t checksum;
	packet rcv_packet;

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
			//increment frame count
			frame_count++;

			//receive data from server
			n = recvfrom(sockfd, buffer, CHUNK_SIZE+6, 0, (struct sockaddr*)&serveraddr, &len);
			printf("Received %d bytes of data\n", n);

			//Get data from packet
			memcpy(&rcv_packet.data, &buffer[6], 256);

			//Get frame number from packet
			memcpy(&rcv_packet.frame_num, &buffer, 4);

			//Get checksum from packet and store in variable
			memcpy(&rcv_packet.checksum, &buffer[4], 2);
			checksum = rcv_packet.checksum;

			//clear checksum and recalculate
			rcv_packet.checksum = 0;
			//print_buf(rcv_packet);
			uint16_t tmp = calc_checksum(&rcv_packet, 262);
			rcv_packet.checksum = tmp;

			received_frame = htonl(rcv_packet.frame_num);
			printf("Frame Count: %d   Received Frame: %d\n", frame_count, received_frame);

			
			
			//Handle Packet Corruption
			if(rcv_packet.checksum != checksum){
				printf("Received a corrupted packet\n");
				printf("Received Checksum: %d\nCalculated Checksum: %d\n", checksum, rcv_packet.checksum);
			}


			//Handle Data packet loss
			else if(received_frame > frame_count){
				printf("Did not receive the expected packet\n");
			}

			//Handle ACK packet loss
			else if(received_frame < frame_count){
				printf("Received a duplicate packet\n");
			}

			else{
				//Write to file
				printf("Writing to file\n");
				fwrite(&buffer[6], 1, n-6, fp);
				printf("Write success\n");

				//Send acknowledgement;
				sendto(sockfd, &rcv_packet.frame_num, 4, 0, (struct sockaddr*)&serveraddr, len);
				printf("Sending ACK for frame %d\n", received_frame);
			}

		}while( n >= CHUNK_SIZE);
		frame_count = 0;
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


