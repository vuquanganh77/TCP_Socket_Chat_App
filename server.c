#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>

#define KEY 0x1A // XOR Key

struct client_info {
    int sockno;
    char ip[INET_ADDRSTRLEN];
};

int clients[100];
int n = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Ham ma hoa va giai ma XOR
void xor_crypt(char *input, char *output, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        output[i] = input[i] ^ KEY;
    }
}

void sendtoall(char *msg,int curr){
    int i;
    pthread_mutex_lock(&mutex);
    for(i = 0; i < n; i++) {
        if(clients[i] != curr) {
            // Ma hoa truoc khi gui
            xor_crypt(msg, msg, strlen(msg));

            if(send(clients[i], msg, strlen(msg), 0) < 0) {
                perror("Sending fail");
                continue;
            }
        }
    }
    pthread_mutex_unlock(&mutex);
}

void *recvmg(void *sock){
    struct client_info cl = *((struct client_info *)sock);
    char msg[500];
    char decrypted_msg[500];
    int len;
    int i;
    int j;
    while((len = recv(cl.sockno,msg,500,0)) > 0) {
        msg[len] = '\0';

        // Giai ma
        xor_crypt(msg, decrypted_msg, len);

        // Chuyen tiep ban ma den cac client
        sendtoall(decrypted_msg, cl.sockno);

        memset(msg, '\0', sizeof(msg));
        memset(decrypted_msg, '\0', sizeof(decrypted_msg));
    }
    pthread_mutex_lock(&mutex);
    printf("%s Disconnected\n", cl.ip);
    for(i = 0; i < n; i++) {
        if(clients[i] == cl.sockno) {
            j = i;
            while(j < n-1) {
                clients[j] = clients[j+1];
                j++;
            }
        }
    }
    n--;
    pthread_mutex_unlock(&mutex);
}

int main(int argc,char *argv[]){
    struct sockaddr_in my_addr,their_addr;
    int my_sock;
    int their_sock;
    socklen_t their_addr_size;
    int portno;
    pthread_t sendt, recvt;
    char msg[500];
    int len;
    struct client_info cl;
    char ip[INET_ADDRSTRLEN];

    if(argc > 2) {
        printf("Invalid arguments, (./server <port>)");
        exit(1);
    }
    portno = atoi(argv[1]);
    my_sock = socket(AF_INET,SOCK_STREAM,0);
    memset(my_addr.sin_zero,'\0',sizeof(my_addr.sin_zero));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(portno);
    my_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    their_addr_size = sizeof(their_addr);

    if(bind(my_sock,(struct sockaddr *)&my_addr,sizeof(my_addr)) != 0) {
        perror("Binding unsuccessful");
        exit(1);
    }

    if(listen(my_sock,5) != 0) {
        perror("Listening unsuccessful");
        exit(1);
    }

    while(1) {
        if((their_sock = accept(my_sock,(struct sockaddr *)&their_addr,&their_addr_size)) < 0) {
            perror("Accept unsuccessful");
            exit(1);
        }

        pthread_mutex_lock(&mutex);
        inet_ntop(AF_INET, (struct sockaddr *)&their_addr, ip, INET_ADDRSTRLEN);
        printf("%s connected\n",ip);
        cl.sockno = their_sock;
        strcpy(cl.ip, ip);
        clients[n] = their_sock;
        n++;
        pthread_create(&recvt, NULL, recvmg, &cl);
        pthread_mutex_unlock(&mutex);
    }
    return 0;
}
