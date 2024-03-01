#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>

#define KEY 0x1A // Khoa XOR

// Ham ma hoa va giai ma XOR
void xor_crypt(char *input, char *output, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        output[i] = input[i] ^ KEY;
    }
}

void *recvmg(void *sock){
    int their_sock = *((int *)sock);
    char msg[500];
    char decrypted_msg[500];
    int len;
    while((len = recv(their_sock, msg, 500, 0)) > 0) {
        msg[len] = '\0';
        
        // Giai ma
        xor_crypt(msg, decrypted_msg, len);

        fputs(decrypted_msg, stdout);
        memset(msg, '\0', sizeof(msg));
        memset(decrypted_msg, '\0', sizeof(decrypted_msg));
    }
}

int main(int argc, char *argv[]) {
    struct sockaddr_in their_addr;
    int my_sock;
    int their_sock;
    int their_addr_size;
    int portno;
    pthread_t sendt, recvt;
    char msg[500];
    char username[100];
    char res[600];
    char ip[INET_ADDRSTRLEN];
    int len;

    if(argc > 3) {
        printf("Too many arguments");
        exit(1);
    }
    portno = atoi(argv[2]);
    strcpy(username,argv[1]);
    my_sock = socket(AF_INET,SOCK_STREAM,0);
    memset(their_addr.sin_zero,'\0',sizeof(their_addr.sin_zero));
    their_addr.sin_family = AF_INET;
    their_addr.sin_port = htons(portno);
    their_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(connect(my_sock,(struct sockaddr *)&their_addr,sizeof(their_addr)) < 0) {
        perror("Connection not established");
        exit(1);
    }
    inet_ntop(AF_INET, (struct sockaddr *)&their_addr, ip, INET_ADDRSTRLEN);
    printf("Connected to %s, Start chatting\n",ip);
    pthread_create(&recvt, NULL, recvmg, &my_sock);
    while(fgets(msg, 500, stdin) > 0) {
        strcpy(res, username);
        strcat(res, ":");
        strcat(res, msg);

        // Ma hoa thong diep truoc khi gui
        xor_crypt(res, msg, strlen(res));

        len = write(my_sock, msg, strlen(res));
        if(len < 0) {
            perror("Message not sent");
            exit(1);
        }
        memset(msg, '\0', sizeof(msg));
        memset(res, '\0', sizeof(res));
    }
    pthread_join(recvt,NULL);
    close(my_sock);
    return 0;
}
