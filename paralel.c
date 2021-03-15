/*This is the client used for the Computer Network final project.
Last modified: 11/01/2021
Author: Gaina Robert-Adrian (2E4)

Parent-Child type

Project name: Monitorizarea traficului (A) [Propunere Continental]
Descriere: Implementati un sistem care va fi capabil sa gestioneze traficul si sa ofere informatii virtuale soferilor. 
https://profs.info.uaic.ro/~computernetworks/ProiecteNet2020.php*/
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>

int port; /* Connection port */
char buf[200];

int main(int argc, char *argv[])
{
  int sd; //Socket Descriptor
  struct sockaddr_in server; //Socket structure used for connection (IPv4)
  if (argc != 3)
  {
    printf("[CONNECT]: %s <server_adress> <port>\n", argv[0]);
    return -1;
  }
  port = atoi(argv[2]);
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) /*AF_INET=address family for IPV4 SOCK_STREAM=TCP socket socket(int domain,int type,int protocol)-creates and endpoint for comunication returning a file descriptor referring to that endpoint. Value=lowest fd not used*/
  {
    perror("[Client] Can't create socket!\n");
    exit(1);
  }
  
  server.sin_family = AF_INET; /* Type of adresses that can comunicate with the socket IPv4 family */
  server.sin_addr.s_addr = inet_addr(argv[1]); /* IP adress inet_addr(char) converts the char to the standard IPv4 notation*/
  server.sin_port = htons(port); /* Port for connection converts the short host byte order to network byte order*/

  if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) /*connect(sockfd,sockaddr*addr,socklent_t) - connects the socket referred by the sockfd to the adress specified by addr*/
  {
    perror("[Client] Can't connect to server\n");
    exit(2);
  }

  pid_t pid;
  if((pid=fork())==-1) /*Parent-Child Client. Parents: Sends command to the server, Child: Reads from the server*/
  {
    perror("[Client] Can't fork\n");
    exit(3);
  }
  if(pid)//Parent
  {
    printf("[Client] Add a command for server:\n");
    while (1)
    {
      memset(buf, 0, sizeof(buf)); /* Empty the buffer */
      fd_set readfds; /* fd_set a set of registers to be monitorized. In our case only stdin*/
      FD_SET(0, &readfds); /* FD_SET(int fd,fd_set set) Adds the fs to the set*/
      struct timeval tv; /* time structure with sec and microsec fields*/
      tv.tv_sec = 15;
      tv.tv_usec = 0;
      fflush(stdout);
      int ready;
      /*select(int nfds,fd_set *read,fd_set write,fd_set *except, struct time *time*/
      /*nfds=nr of file descriptors+1, then in order: fd for READ,WRITE and EXCEPTIONS, time structure*/
      if ((ready = select(1, &readfds, NULL, NULL, &tv)) < 0) /*Wait for at most tv_sec for the fd to be ready for reading operations*/
      {
         perror("[Client] Select error()\n");
         return -1;
      }
      else if (ready)
        read(0, buf, sizeof(buf));
      else
        strcpy(buf, "Change: S:Soseaua Nicolina\n"); /*Random street*/
    /* The select on the input descriptor allows us to either introduce a new manual request or 
  automatically send a request to the server if we have a "geolocation" system 
  The downside of this approach is that we only have a fixed period of time when we can indroduce the command. 
  If for instance we start writing at the 4th second (out of 15 allowed by the select timer) we will not be able to 
  finish it and we need to write it again. I need to find a method of checking if the client is writing in the terminal
  at a moment */
      int length = 0;
      while (buf[length] != '\0')
      {
        ++length;
      }
      char *street = malloc(length * sizeof(char));
      for (int i = 0; i < length; ++i)
      {
        street[i] = buf[i];
      }
      if (write(sd, &length, sizeof(int)) <= 0)
      {
        perror("[Client] Can't write to server!\n");
        close(sd);
        exit(3);
      }
      if (write(sd, street, length) <= 0)
      {
        perror("[Client] Can't write to server!\n");
        close(sd);
        exit(4);
      }
      if(strncmp(street,"Quit",4)==0)
      {
        wait(NULL);
        close(sd);
        exit(0);
      } 
    /*The quit command goes to the server in order to be sure that all data related to the client was
     cleared in the server. A faster implementation could send a clean command to the server and another one
     directly to the child
    Method: 
    1.Client sends a Quit request to server through parent process and waits for the child to end
    2.Server clears the information related to the client and sends a quit request to the client in the child process
    3.Cliereadynt receives the Quit signal in the child process. The child ends signaling the parent
    4.The parent process closes upon receiving the end signal from the child*/
  }
  }
  else//Child
  {
    while (1)
    { 
      /*Waiting for answers from server*/
      int answerSize;
      if (read(sd, &answerSize, sizeof(int)) < 0)
      {
        perror("[Client] Can't read from server.\n");
        close(sd);
        exit(5);
      }
      char *answer = malloc((answerSize + 1) * sizeof(char));
      if (read(sd, answer, answerSize) < 0)
      {
        perror("[Client] Can't read from server.\n");
        close(sd);
        exit(6);
      }
        printf("[Client]Answer from server:\n%s", answer);
      if(strncmp(answer,"Quit",4)==0)
      {
        close(sd);
        exit(0);
      }
      printf("[Client] Add a command for server:\n");
    } 
  }
}