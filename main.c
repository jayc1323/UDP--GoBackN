/*------------------------------
 A3 Jay Choudhary 
 main.c

*/
#include "server.h"
#include <time.h>
/*--------------------------------------------------------------------------------------*/

// void write_random_bytes_to_file(const char *filename, size_t num_bytes) {
    
//     FILE *file = fopen(filename, "wb");
//     if (!file) {
//         perror("Error opening file");
//         return;
//     }

   
//     srand((unsigned int)time(NULL));

//     for (size_t i = 0; i < num_bytes; i++) {
        
//         unsigned char random_byte = (unsigned char)(rand() % 256);
//         fwrite(&random_byte, sizeof(unsigned char), 1, file);
//     }

//     fclose(file);
// }
/*------------------------------------------------------------------------------------------------------------------------*/

int main(int argc,char* argv[]){
        if(argc !=2 ){
                fprintf(stderr,"Usage : ./a.out Port no\n");
                return 1;
        }
        char *port_no = argv[1];

        SOCKET server = setup_server_socket(port_no);

       //write_random_bytes_to_file("test_file.txt", 1240);
        
    
    while(1){
         printf("Server ready to receive messages..\n");
    struct sockaddr_storage client_address;
    socklen_t client_len = sizeof(client_address);
    char *read = malloc(1024);

    int bytes_received = recvfrom(server,
            read, 1024,
            0,
            (struct sockaddr*) &client_address, &client_len);

    printf("Received (%d bytes): %s\n",
            bytes_received,read);
    if(bytes_received <=2){
        //PROBABLY AN ACK FROM A CLIENT WHOSE FILE TRANSFER WAS COMPLETED
         free(read);
        continue;
    }        
    handle_client(read,(struct sockaddr*) &client_address,client_len,server);        
    free(read);



    }


    CLOSESOCKET(server);
    return 0;

}
