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

#define BUF_SIZE 262

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
	char buf[BUF_SIZE];
	int port;

	//Setup Timeout
	struct timeval tv;
	tv.tv_sec = 0;
	

	char status_string[10] = "yes";
	printf("Enter a port ");
	scanf("%d", &port);

  	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd<0){
	  printf("There was an arror creating the socket\n") ;
	  return 1;
	}
	

	
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
	unsigned int send_num = 0;
	vector<packet> packets;
	vector<unsigned int> acks;
	int count = 0;
	packet send_packet;
	bool first_time = true;
	int limit;
	unsigned int frame_count =0;
	unsigned int ack;
	unsigned int ack_num;
	int j=0;
	bool nack_flag = false;
	int data_size;

	while(1){
 	  printf("server running \n");
	  socklen_t len = sizeof(clientaddr);

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


	  //Open File
	  FILE *fp = fopen(filename,"r");

	  //Send error message if file DNE
	  if(fp == NULL){
	  	printf("Error opening file\n");
	  	//sendto(sockfd, buf, BUF_SIZE, 0,  (struct sockaddr*)&clientaddr, &len)
	  }

	  //Send file upon successful open
	  else{
	  	printf("Success opening file.\n");
	  

			//Send data in 256 byte chunks
	  		send_packet.frame_num = 0;
			while(1){
				if(nack_flag == false){
					//Keep track of frame number and window count
					frame_count++;
					send_packet.frame_num = htonl(frame_count);
					count++;

					//read from file and store data in data buffer
					data_size = fread(&send_packet.data, 1, CHUNK_SIZE, fp);

					//Copy Sequence frame number and data buffer into send buffer
					printf("Frame num: %u at \n", frame_count);
					memcpy(send_buf, &send_packet.frame_num, 4);
					memcpy(&send_buf[6], &send_packet.data, data_size);

					//Clear checksum and calculate
					//Function returns checksum in network byte order
					send_packet.checksum = 0;
					//print_buf(send_packet);
					uint16_t tmp = calc_checksum(&send_packet, data_size+6);
					send_packet.checksum = tmp;
					memcpy(&send_buf[4], &send_packet.checksum, data_size);

					//push buffer onto packet storage vector
					packets.push_back(send_packet);
				}

				//Send Packet if it has data
				uint16_t num = ntohl(send_packet.frame_num);
				if(data_size != 0){
					printf("Sending Packet %d containing %d bytes \n", num, data_size+6);
					sendto(sockfd, send_buf, data_size+6, 0,  (struct sockaddr*)&clientaddr, len);

				}
				else{
					printf("Error sending data\n");
					break;
				}

			
				//After 5 packets start looking for ACK's

				if(count >= 5){
					
					
					unsigned int packet_num = packets[0].frame_num;
					
					if(first_time){
						limit = 5;
						first_time = false;
					}
					else{
						limit = 1;
					}

					//Start Receiving ACK's
					//Set Timeout
					tv.tv_usec = 100000;
					if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
						perror("Error");
					}
					for(i=0; i<limit; i++){
						printf("Receiving ACK...\n");
						if(recvfrom(sockfd, &ack, BUF_SIZE, 0, (struct sockaddr*)&clientaddr, &len) < 0){
							//Timeout reached continue through loop
							continue;
						}	
						ack_num = ntohl(ack);						
						acks.push_back(ack);
						printf("ACK received for frame %d\n", ack_num);
					}

					//Clear Timeout
					tv.tv_usec = 0;
					if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
						perror("Error");
					}
					 
					//Verify that recieved ACK is for the first packet sent in the window
					int flag = 0;
					for(i=1; i<acks.size()+1; i++){
						//printf("Packet Num: %d  ACK num: %d\n", packet_num, acks[i-1]);
						if(acks[i-1] == packet_num){
							flag = i;
							ack_num = ntohl(acks[i-1]);
							break;
						}
					}
					if(flag != 0){
						//We got the ACK for this packet
						//remove packet from vector and continue to send next packet
						printf("Found the ACK for frame %d... sending next packet\n", ack_num);
						packets.erase(packets.begin());
						acks.erase(acks.begin()+flag-1);
						nack_flag = false;
					}
					else{
						//We did not get the correct ACK
						//resend packet from vector
						printf("Did not recieve ACK for correct frame... frame: %d\n", ack_num);
						
						memcpy(&send_buf, &packets[0], data_size+6);
						printf("Re-sending packet %d\n", ntohl(packets[0].frame_num));
						//sendto(sockfd, send_buf, BUF_SIZE, 0,  (struct sockaddr*)&clientaddr, len);
						nack_flag = true;
					}
				}
					bool ack_flag = false;
					if(data_size < 256){
					if(feof(fp)){
					int x;
						//Make sure client received final 5 packets
						for(i=0; i<packets.size(); i++){
							for(j=0; j<acks.size(); j++){
								if(packets[i].frame_num == acks[j]){
									ack_num = ntohl(acks[i]);
									printf("Received ACK for frame %d\n", ack_num);
									ack_flag = true;
									x=0;
								}						
							}
							if(!ack_flag){
								//resend missing packet
								printf("ACK missing... resending packet\n");
								memcpy(&send_buf, &packets[i], 262);
								if(i == packets.size()-1){
									sendto(sockfd, send_buf, data_size+6, 0,  (struct sockaddr*)&clientaddr, len);
								}
								else{
									sendto(sockfd, send_buf, BUF_SIZE, 0,  (struct sockaddr*)&clientaddr, len);
								}
								//receive missing ack
								//set Timeout
								tv.tv_usec = 100000;
								if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
									perror("Error");
								}
								if(recvfrom(sockfd, &ack, BUF_SIZE, 0, (struct sockaddr*)&clientaddr, &len) < 0){
									i--;
									x++;
									printf("x = %d\n", x);
									if(x>25){
										sendto(sockfd, 0, 15, 0,  (struct sockaddr*)&clientaddr, len);
										break;
									}
									else{
										continue;
									}
								}
								//Clear Timeout
								tv.tv_usec = 0;
								if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
									perror("Error");
								}
								ack_num = ntohl(ack);						
								acks.push_back(ack);
								printf("ACK received for frame %d\n", ack_num);	
								ack_flag = false;
							}
						}
						printf("End of File\n");
					}
					if(ferror(fp)){
						printf("Error Reading File\n");
					}
					break;
				}

				printf("\n");
			}
		}
		


		//Clear Strings
	 	memset(filename,0,strlen(filename));
	  	memset(line,0,strlen(line));
	  	memset(client_status,0,strlen(client_status));
	  	frame_num = 0;
	  	first_time = true;
	  	memset(&send_packet.frame_num, 0, 4);
	  	memset(&send_packet.data, 0, 256);
	  	packets.clear();
	  	acks.clear();
	  	frame_count = 0;
	  	count = 0;

	}

	return 0;
}


