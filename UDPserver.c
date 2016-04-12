//Nick Schrock
//CIS_475 Lab2
//TCP Echo Server

#include <sys/socket.h>
#include <netinet/in.h> //network protocols 
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <vector>

using namespace std;

#define CHUNK_SIZE 256

#define BUF_SIZE 290
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
	char buf[BUF_SIZE];
	int port;
	char status_string[10] = "yes";
	printf("Enter a port ");
	scanf("%d", &port);

  	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd<0){
	  printf("There was an arror creating the socket\n") ;
	  return 1;
	}
	fd_set sockets;
	FD_ZERO(&sockets);
	
	struct sockaddr_in serveraddr;
	struct sockaddr_in clientaddr;
	serveraddr.sin_family = AF_INET;

	//specify port number and put them in the correct order
	//htns == host to network short
	serveraddr.sin_port = htons(port);
	printf("server port set\nu");
	//inet is loopback server... useful for client and server on same machine
	//serveraddr.sin_addr.s_addr = INET+addr("127.0.0.1");
	serveraddr.sin_addr.s_addr = INADDR_ANY;
	printf("server address set\n");
	bind(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	printf("bound\n");
	int clientsocket  = 0; 

	char filename[5000];
	char file_error[100] = "Error opening file.\n";
	char file_success[100] = "File opened successfully\n";
	char client_status[5000];
	char line[5000];
	char send_buf[264];
	int flag, flag2 = 0;
	unsigned char frame_num = 0;
	unsinged char ack_num = 0;
	vector<char[264]> packets;
	int count = 0;

	eth_hdr eth;
	ip_hdr ip;

	while(1){
 	  printf("server running \n");
	  socklen_t len = sizeof(clientaddr);
	  
	  fd_set tmp_set = sockets;

	  printf("Ready to select socket\n");

	  printf("socket selected\n");

	  int i;


	  int n = recvfrom(sockfd, filename, BUF_SIZE, 0, (struct sockaddr*)&clientaddr, &len);


	 
	  //Recieve File name if first time through
	  if(flag == 0){
		  //recv(clientsocket, filename, 5000, 0);
	  	  //pull_data(eth, ip, filename, buf, n-34);
	  	  //memcpy(filename, buf,)
		  printf("Got from client: %s\n", filename);
		  printf("Preparing to open\n");
		  flag = 1;
		}

		//Only send file if client is connected
		if(flag2 == 0){

		  //Open File
		  FILE *fp = fopen(filename,"r");

		  //Send error message if file DNE
		  if(fp == NULL){
		  	printf("Error opening file\n");
		  	//sendto(sockfd, buf, BUF_SIZE, 0,  (struct sockaddr*)&clientaddr, &len)
		  }

		  //Send file upon successful open
		  else{
		  	printf("Success opening file. Sending status\n");
		  	//sendto(sockfd, buf, BUF_SIZE, 0,  (struct sockaddr*)&clientaddr, &len)
		  

				//Send data in 256 byte chunks
			  while(1){

			  	//Keep track of frame number and window count
			  	frame_num++;
			  	count++;

			  	//initialize data buffer to 0
			  	char buffer[CHUNK_SIZE]={0};

			  	//read from file and store data in data buffer
			  	int send_size = fread(buffer, 1, CHUNK_SIZE, fp);

			  	//Copy Sequence frame number and data buffer into send buffer
			  	printf("Frame num: %u at \n", frame_num);
			  	memcpy(send_buf, &frame_num, 4);
			  	memcpy(&send_buf[4], &buffer, 256);

			  	//expand size of send buffer and push buffer onto packet storage vector
			  	send_size += 4;
			  	packets.pushback(send_buf);

			  	//After 5 packets receive ACK from first packet
			  	unsigned char ack;
			  	if(count >= 5){
			  		recvfrom(sockfd, ack, BUF_SIZE, 0, (struct sockaddr*)&clientaddr, &len);
			  		//Verify that recieved ACK is for the first packet sent in the window
			  		if(ack == frame_num - 5){
			  			//We got the ACK for this packet
			  			//remove packet from vector and continue to send next packet
			  		}
			  		else{
			  			//We did not get the correct ACK
			  			//resend packet from vector
			  		}
			  	}

			  	if(send_size != 0){
			  		printf("Sending data: %d\n", send_size);
			  		sendto(sockfd, send_buf, send_size, 0,  (struct sockaddr*)&clientaddr, len);

			  	}
			  	else{
			  		printf("Error sending data\n");
			  		break;
			  	}

			  	if(send_size < 256){
			  		if(feof(fp)){
			  			//printf("size: \n", sizeof(send_size));
			  			printf("End of File\n");
			  		}
			  		if(ferror(fp)){
			  			printf("Error Reading File\n");
			  		}
			  		break;
			  	}

			  }
			}
		}

		//Shutdown server if client disconnected
		else{
			printf("\n\nShutting Down Server\n");
			break;
		}

		//Clear Strings
	 	memset(filename,0,strlen(filename));
	  	memset(line,0,strlen(line));
	  	memset(client_status,0,strlen(client_status));

	}

	return 0;
}


