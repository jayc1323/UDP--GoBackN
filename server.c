
#include "server.h"
#include <sys/stat.h>

Packet* build_packets(char *file_name,long file_size,int *no);
void free_packet_array(Packet *packet_array,int num_packets);
void send_window(int filesize,int num,int start , int end , Packet *packet_array , bool *packets_sent,struct sockaddr *client_address,socklen_t client_len,SOCKET server_socket);

/*--------------------------------------------------------------*/
SOCKET setup_server_socket(char *port){

    printf("Configuring local address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *bind_address;
    getaddrinfo(0, port, &hints, &bind_address);


    printf("Creating socket...\n");
    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family,
            bind_address->ai_socktype, bind_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_listen)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        exit(1);
    }


   
    if (bind(socket_listen,
                bind_address->ai_addr, bind_address->ai_addrlen)) {
        fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
        exit(1);
    }
    
    freeaddrinfo(bind_address);
    return socket_listen;
}
/*-------------------------------------------------------------------------------------------*/
void handle_client(char *request,struct sockaddr *client_address,socklen_t client_len,SOCKET server_socket){

        //PARSE REQUEST INTO PACKET STRUCT
        int window_size = request[1];
         printf("Client request is %s\n",request);
        printf("Window size is %d\n",window_size);
        char file_name[20];
        strcpy(file_name,request+2);


        // CHECK IF FILE EXIST
         struct stat file_status;
         bool file_exists;
         long file_size;
        if((stat(file_name,&file_status))<0){

                   file_exists = false;
        }
        else{
                file_exists = true;
                file_size = file_status.st_size;

        }

         int num_packets;
        //CASE 1 : FILE EXISTS
        if(file_exists){

                printf("File : %s exists\n",file_name);
              Packet *packet_array = build_packets(file_name,file_size,&num_packets);
               printf("File divided into %d packets\n",num_packets);
                


              //START SENDING
            
                
                 bool transfer_complete = false;
                 bool *packets_sent = malloc(sizeof(bool)*num_packets);
              for(int b = 0; b < num_packets; b++){
                packets_sent[b] = false;
              }
              int timeout_count = 0;
              int window_start = 0;
              int window_end = (window_start) + (window_size - 1);

              while(!transfer_complete){
                 
                 
                 send_window(file_size,num_packets,window_start,window_end,packet_array,packets_sent,client_address,client_len,server_socket);
                 struct timeval timeout;
                 timeout.tv_sec = 3;
                 timeout.tv_usec = 0;
                  fd_set read_fds;
                FD_ZERO(&read_fds);
                FD_SET(server_socket, &read_fds);
                int s = select(server_socket + 1, &read_fds, NULL, NULL, &timeout);
                 
                 if(s>0){
                        char ack[2];
                        recvfrom(server_socket,ack,2,0,client_address,&client_len);
                        printf("Received ack for packet no %d\n",ack[1]);
                        int ack_no = ack[1];
                        window_start = ack_no + 1;
                        window_end = (window_start) + (window_size - 1);
                        if(window_start >= (num_packets)){
                                transfer_complete = true;
                        }
                        if(window_end >= (num_packets-1)){
                                window_end = (num_packets-1);
                        }
                        timeout_count = 0;


                        //slide window forward

                 }
                 else if(s==0){
                        timeout_count++;
                        for(int i = window_start;i<=window_end;i++){
                                packets_sent[i] = false;
                        }
                        if(timeout_count == 5){
                                printf("5 timeouts for same window..Ending transmission...\n");
                                break;
                        }
                 }
                 else{
                        fprintf(stderr,"select() failed..%d\n",GETSOCKETERRNO());
                        
                        break;
                 }


              }
              if((file_size % 512)==0){
              char last_packet[2];
              last_packet[0] = 2;
              last_packet[1] = num_packets;
              sendto(server_socket, last_packet, 2, 0, client_address, client_len);
              printf("Last packet of 2 byte sent\n");
              }

              printf("File transfer completed\n");
      
              free_packet_array(packet_array,num_packets);

               return;


              
             
              
        }


        //CASE 2 : FILE DNE
        //SEND ERROR MESSAG
        else{
                char error[1];
                error[0] = 4;
       
        sendto(server_socket, error, 1, 0,
                client_address, client_len);
         printf("File Does not Exist error msg sent to client\n");        

        }
        return;        
}
/*-----------------------------------------------------------------------------------------------*/
Packet* build_packets(char *file_name,long file_size,int *no){

             int num_packets;
             int last_packet_size;
             if((file_size % 512) == 0){
                //FILE SIZE DIVISIBLE BY PKT SIZE
                num_packets = (file_size/512);
                last_packet_size = 0;

             }
             else{
                num_packets = (file_size/512) + 1;
                last_packet_size = (file_size % 512);
             }
             Packet *packet_array = malloc(sizeof(Packet) * num_packets);
             if (packet_array == NULL) {
        perror("Failed to allocate memory for packets");
        return NULL;
    }
    
   
    FILE *file = fopen(file_name, "rb");
    if (file == NULL) {
        perror("Failed to open file");
        free(packet_array);
        return NULL;
    }
    
    for (int i = 0; i < num_packets; i++) {
        packet_array[i] = malloc(sizeof(struct packet));
        if (packet_array[i] == NULL) {
            perror("Failed to allocate memory for a packet");
            
            for (int j = 0; j < i; j++) free(packet_array[j]);
            free(packet_array);
            fclose(file);
            return NULL;
        }
        
        packet_array[i]->type = 2; 
        packet_array[i]->seq_no = i;

      
        int read_size = (i == num_packets - 1 && last_packet_size != 0) ? last_packet_size : 512;
        
       
        int bytes_read = fread(packet_array[i]->data, 1, read_size, file);
        if (bytes_read < read_size) {
            perror("Error reading file data");
            for (int j = 0; j <= i; j++) free(packet_array[j]);
            free(packet_array);
            fclose(file);
            return NULL;
        }
    }
    
   
    fclose(file);
    *no = num_packets;
    return packet_array;

 
       
}
/*-------------------------------------------------------------------------------------------*/
void free_packet_array(Packet *packet_array,int num_packets){

        for(int i = 0;i<num_packets;i++){
                free(packet_array[i]);
        }
        free(packet_array);
        return;

}
/*-----------------------------------------------------------------------------------------------*/

void send_window(int filesize,int num ,int start , int end , Packet *packet_array , bool *packets_sent,struct sockaddr *client_address,socklen_t client_len,SOCKET server_socket){

        printf("Sending window with start index %d and end index %d\n",start,end);
        for(int i = start;i<=end;i++){

                if(i >(num-1)){
                        continue;
                }

                if(!packets_sent[i]){
                        Packet p = packet_array[i];
                        int data_size = 512;
                        if(i == (num-1)){
                                if((filesize % 512)!=0){
                                        data_size = filesize % 512;
                                } 
                        }
                        char buffer[2+data_size];
                        buffer[0] = p->type;
                        buffer[1] = p->seq_no;
                        memcpy(buffer + 2, p->data, data_size);
                         sendto(server_socket, buffer, 2+data_size, 0, client_address, client_len);
                          printf("Packet with seq no %d sent\n",p->seq_no);
                          packets_sent[i] = true;
               

                }
        }

        return;

}