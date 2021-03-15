/*This is the server used for the Computer Network final project.
Last modified: 12/01/2021
Author: Gaina Robert-Adrian (2E4)

The server will accepts clients and create a thread for each client connected.

Project name: Monitorizarea traficului (A) [Propunere Continental]
Descriere: Implementati un sistem care va fi capabil sa gestioneze traficul si sa ofere informatii virtuale soferilor. 
https://profs.info.uaic.ro/~computernetworks/ProiecteNet2020.php*/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sqlite3.h>

#define PORT 3333

int ALERT=0;
int client_list[200];
int connected[200];
int counter=0;

typedef struct thData
{
    int idThread; /* thread id*/
    int cl; /*socket descriptor*/      
} thData;

static void *treat(void *arg)
{
    struct thData tdL;
    tdL = *((struct thData *)arg);
    printf("[thread:%d] Waiting for command...\n", tdL.idThread);
    fflush(stdout);
    pthread_detach(pthread_self()); /* Mark that resources are automatically released*/
    raspunde((struct thData *)arg);
    close((intptr_t)arg);
    return (NULL);
}

int callback_login(int *notUsed, int argc, char **argv, char **columnNames) 
{
    if(argc==1)
    {
        (*notUsed)=1;
        return 1;
    }
   (*notUsed)=0; 
    return 0;
} /*Used for checking the existence of the user in the database*/

int callback(void *NotUsed, int argc, char **argv, char **azColName) 
{

    for (int i = 0; i < argc; i++) 
    {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }   
    printf("\n");  
    return 0;
} /* Function used for printing arguments of the interogations */

int callback_change(char* ans,int argc,char**argv,char**azColName)
{
    strcat(ans,":\nSpeed limit:");
    strcat(ans,argv[0]);
    strcat(ans,"KM/H\n\n");
    return 0;
} /*Speed limit formatter*/

int callback_get_id(char* ans,int argc,char** argv,char**azColname)
{
    strcpy(ans,argv[0]);
    return 0;
} /*Return cell-information statements*/

int callback_events(char* ans,int argc,char**argv,char**azColname)
{
    for (int i = 0; i < argc; i++) 
    { 
       strcat(ans,argv[i]);
        strcat(ans,"\n");
    }   
    strcat(ans,"\n");
    return 0;
}/*Formater for events/places*/

void raspunde(void *arg)
{
    int length, i = 0;
    struct thData tdL;
    int logged=0; 
    int extra=0;
    tdL = *((struct thData *)arg);
    while (1)
    {
        /* Reading client requuests */
        if (read(tdL.cl, &length, sizeof(int)) <= 0)
        {
            printf("[thread:%d]\n", tdL.idThread);
            printf("The client assigned for thread %d disconnected!\n", tdL.idThread);
            connected[tdL.idThread]=0;
            break; //Without this server crashes upon random client disconnect
        }
        char *command = malloc(length * sizeof(char));
        command[length]='\0';
        if (read(tdL.cl, command, length) <= 0)
        {
            printf("[thread:%d]\n", tdL.idThread);
            printf("The client assigned for thread %d disconnected!\n", tdL.idThread);
            connected[tdL.idThread]=0;
            break;
        }

        /*Here we will treat each command */
        char *answer=malloc(500);
        if (strncmp(command,"Quit",4)==0)
        {
            answer=strdup("Quitting...\n");
            connected[tdL.idThread]=0;
        }
        /* Quit */

        else if(logged==0)
        {
        if (strncmp(command, "Login: ", 7) == 0)
        {
            char* username=command+7;
            char interogate[256];
            strcpy(interogate,"SELECT user_name FROM users WHERE user_name='");
            strcat(interogate,username);
            interogate[strlen(interogate)-1]='\0';
            strcat(interogate,"'"); /* Prepare SQLite interogation for username search in database */
            sqlite3 *db;
            char* error_message;

            if(0!=sqlite3_open("traffic.db",&db))
            {
                printf("Error opening database: %s\n",sqlite3_errmsg(db));
                exit(-1);
            } /* Open the SQLite Database */

            int valid=0;
            int *p_valid=&valid; /* Method of reproducing transfer by reference in C++ */

            if(sqlite3_exec(db,interogate,callback_login,p_valid,&error_message)!=SQLITE_OK)
            {
                sqlite3_free(error_message);
            } /* sqlite3_exec executes the command interrogate in the db applying callback_login on each row returned. third parameter is used as the first in the callback function usually used for passing results out of exec*/   
            if(valid==1)
            {
                answer=strdup("Welcome, ");
                strcat(answer,username);
                logged=1;
            } /* The valid value is set on 1 if we find the user in the DB */
            else
            {
                answer=strdup("User not recognized\n");
            }
        }
        /* Login FORMAT: "Login: <username>"*/

        else
        {
            answer=strdup("You must be logged in to connect!\n");
        }
        /* Any other command is accesible ONLY after login. 
        However for safety measures ALERTS WILL STILL BE SEND TO CLIENTS STILL NOT CONNECTED*/

        }
        else if(logged==1)
        {
            if (strncmp(command, "Login: ", 7) == 0)
            {
                answer=strdup("You are already logged in!\n");
            }
            else if (strncmp(command, "Extra: ", 7) == 0)
            {
                printf(command+7);
                if(command[7]=='1')
                {
                    extra=1;
                    answer = strdup("Extra package 1 set![Street extra events]\n");
                }
                else if(command[7]=='2')
                {
                    extra=2;
                    answer=strdup("Extra packege 2 set![Neighbourhood extra events]\n");
                }
                else
                {
                    extra=0;
                    answer=strdup("Extra options were removed!\n");
                }    
            }   
            /* 3 Levels of informations:
            Level 0- Only speed regulations and alerts 
            Level 1- Events&Places recommandations from the current street
            Level 2- Events&Places recommandations from the current neighbourhood_id
            FORMAT: "Extra: <number>" */
            
            else  if (strncmp(command, "Change: ", 8) == 0)
            {
                char* change=command+8;
                if(change[0]=='S')
                {
                    char interogate[256];
                    strcpy(interogate,"SELECT street_id FROM map WHERE street_name='");
                    strcat(interogate,change+2);
                    interogate[strlen(interogate)-1]='\0';
                    strcat(interogate,"'");
                    /*Prepare the interogation for picking up the street_id for later use*/
                    sqlite3 *db;
                    char* error_message;
                    if(0!=sqlite3_open("traffic.db",&db))
                    {
                        printf("Error opening database: %s\n",sqlite3_errmsg(db));
                        exit(-1);
                    }
                    char street_id[10];//answer=strdup("Street change detected!\n");
                    if(sqlite3_exec(db,interogate,callback_get_id,&street_id,&error_message)!=SQLITE_OK)
                    {
                        sqlite3_free(error_message);
                    } 
                    char aux[2048];
                    strcpy(interogate,"SELECT speed_limit from speed WHERE street_id='");
                    strcat(interogate,street_id);
                    strcpy(aux,change+2);
                    aux[strlen(aux)-1]='\0';
                    strcat(interogate,"'");
                    /*Prepare the interogate for speed limit in the area*/
                    if(sqlite3_exec(db,interogate,callback_change,&aux,&error_message)!=SQLITE_OK)
                    {
                        sqlite3_free(error_message);
                    }   /*Speed limit selected*/
                    strcpy(interogate,"Select event_title,event_type,description FROM events  NATURAL JOIN map WHERE street_name='");
                    strcat(interogate,change+2);
                    interogate[strlen(interogate)-1]='\0';
                    strcat (interogate,"' AND (event_type='Accident' OR event_type='Blocaj')");
                    if(sqlite3_exec(db,interogate,callback_events,&aux,&error_message)!=SQLITE_OK)
                    {
                        sqlite3_free(error_message);
                    }
                    if(extra>=1)
                    {
                        strcpy(interogate,"Select event_title,event_type,description FROM events NATURAL JOIN map where street_id='");
                        strcat(interogate,street_id);
                        strcat(aux,"Extra on this street:\n"); 
                        strcat(interogate,"'");
                        if(sqlite3_exec(db,interogate,callback_events,&aux,&error_message)!=SQLITE_OK)
                        {
                            sqlite3_free(error_message);
                        } 
                        strcpy(interogate,"SELECT location_name,location_type,description FROM interest_points NATURAL JOIN map where street_id='");
                        strcat(interogate,street_id);
                        strcat(interogate,"'");
                        if(sqlite3_exec(db,interogate,callback_events,&aux,&error_message)!=SQLITE_OK)
                        {
                            sqlite3_free(error_message);
                        } 
                    }
                    if(extra==2)
                    {
                        strcpy(interogate,"Select neighbourhood_id FROM map WHERE street_id='");
                        strcat(interogate,street_id);
                        strcat(aux,"Extra in the neighbourhood:\n");
                        strcat(interogate,"'");
                        char neighbourhood_id[10];
                        if(sqlite3_exec(db,interogate,callback_get_id,&neighbourhood_id,&error_message)!=SQLITE_OK)
                        {
                            sqlite3_free(error_message);
                        }
                        printf("NID:%s\n",neighbourhood_id);
                        strcpy(interogate,"Select event_title,event_type,description FROM events NATURAL JOIN map where neighbourhood_id='");
                        strcat(interogate,neighbourhood_id);
                        strcat(interogate,"'");
                        if(sqlite3_exec(db,interogate,callback_events,&aux,&error_message)!=SQLITE_OK)
                        {
                            sqlite3_free(error_message);
                        } 
                        strcpy(interogate,"SELECT location_name,location_type,description FROM interest_points NATURAL JOIN map where neighbourhood_id='");
                        strcat(interogate,neighbourhood_id);
                        strcat(interogate,"'");
                        if(sqlite3_exec(db,interogate,callback_events,&aux,&error_message)!=SQLITE_OK)
                        {
                            sqlite3_free(error_message);
                        } 
                    }
                    /* Add extra informations based on preferences */
                    answer=strdup(aux);
                    street_id[0]='\0';
                    aux[0]='\0';
                } 
                else if(change[0]=='N')
                {
                    answer=strdup("Neighbourhood change detected!\n");
                }    
            }
            /*Change: This command is implemented to work assuming that the client is able to send
            the street name based on a geolocation API. 
            FORMAT: "Change: S:<street_name>" for requesting data about a street OR
                    "Change: N:<neighbourhood_name>" for requesting data about a neighbourhood*/

            else if (strncmp(command, "Alert: ", 7) == 0)
            {
                char type[20];
                int i=9,j=0;
                while(command[i]!='S'||command[i+1]!=':')
                {
                    type[j++]=command[i];
                    i++;
                }
                type[j-1]='\0';
                i+=2;
                j=0;
                char street[50];
                while(command[i]!='\0')
                {
                    street[j++]=command[i];
                    i++;
                }
                street[j-1]='\0';
                char interogate[256];
                strcpy(interogate,"SELECT street_id FROM map WHERE street_name='");
                strcat(interogate,street);
                strcat(interogate,"'");
                char street_id[20];
                sqlite3 *db;
                char* error_message;
                if(0!=sqlite3_open("traffic.db",&db))
                {
                    printf("Error opening database: %s\n",sqlite3_errmsg(db));
                    exit(-1);
                }
                if(sqlite3_exec(db,interogate,callback_get_id,&street_id,&error_message)!=SQLITE_OK)
                {
                    sqlite3_free(error_message);
                } /* Save street_id for future use based on street name given*/
                strcpy(interogate,"SELECT event_id FROM events WHERE event_type='");
                strcat(interogate,type);
                strcat(interogate,"' AND street_id='");
                strcat(interogate,street_id);
                strcat(interogate,"'");
                int found=0;
                int *p_found=&found;
                if(sqlite3_exec(db,interogate,callback_login,p_found,&error_message)!=SQLITE_OK)
                {
                    sqlite3_free(error_message);
                } /* Check if the alert was not already reported in the system*/
                if(found==1)
                {
                    answer=strdup("We already have this event recorded in our database!\n");
                }
                else if(found==0) /* Insert the new event in the database */
                {
                    char event_id[10];
                    if(sqlite3_exec(db,"SELECT COUNT(*) FROM events",callback_get_id,&event_id,&error_message)!=SQLITE_OK)
                    {
                        sqlite3_free(error_message);
                    } 
                    strcpy(interogate,"Select neighbourhood_id FROM map WHERE street_id='");
                    strcat(interogate,street_id);
                    strcat(interogate,"'");
                    char neighbourhood_id[10];
                    if(sqlite3_exec(db,interogate,callback_get_id,&neighbourhood_id,&error_message)!=SQLITE_OK)
                    {
                        sqlite3_free(error_message);
                    }
                    sprintf(event_id,"%d",1+atoi(event_id)); 
                    /* Get a unique event_id (EVENT ID IS THE PRIMARY KEY FOR THE TABLE*/
                    char insertion[256];
                    strcpy(insertion,"INSERT INTO events VALUES('");
                    strcat(insertion,event_id);strcat(insertion,"','");
                    strcat(insertion,type);strcat(insertion,"_");strcat(insertion,street);
                    strcat(insertion,"','");strcat(insertion,type);strcat(insertion,"','3','IMPORTANT','");
                    strcat(insertion,street_id);strcat(insertion,"','");strcat(insertion,neighbourhood_id);
                    strcat(insertion,"');");

                    if(sqlite3_exec(db,insertion,NULL,NULL,&error_message)!=SQLITE_OK)
                    {
                        printf("Error inserting into events %s\n",error_message);
                        sqlite3_free(error_message);
                    }
                    /* Event is inserted into the DB*/

                    answer = strdup("Thank you for submitting the alert!\n");
                  //               exit(0); 
                    strcpy(interogate,"SELECT event_title,description,street_name FROM events NATURAL JOIN map WHERE event_id='");
                    strcat(interogate,event_id);
                    strcat(interogate,"'");
                    char aux[500];
                    strcpy(aux,"IMPORTANT ALERT:\n"); 
                    if(sqlite3_exec(db,interogate,NULL,NULL,&error_message)!=SQLITE_OK)
                    {
                        printf("Error retaking alert %s\n",error_message);
                        sqlite3_free(error_message);
                    }
                    if(sqlite3_exec(db,interogate,callback_events,&aux,&error_message)!=SQLITE_OK)
                    {
                        printf("Error opening database: %s\n",sqlite3_errmsg(db));
                        sqlite3_free(error_message);
                    }/* Take the event from the database and prepare it for delivery to the clients */
                    for(int i=0;i<counter;++i)
                    {
                        if(connected[i]!=0&&i!=tdL.idThread)
                        {
                            int l = strlen(aux);
                            if (write(client_list[i], &l, sizeof(int)) <= 0)
                            {
                                printf("[thread %d] ", tdL.idThread);
                                perror("[thread] Can't write answer length.\n");
                            }
                            if (write(client_list[i], aux, l) <= 0)
                            {
                                printf("[thread %d] ", tdL.idThread);
                                perror("[thread] Can't write message  to client.\n");
                            }
                        }
                    }
                    /* Iterate through all the other clients and send the alert to them */
                }
            } 
            /*Alert system: This command allows the reporting of incidents on the road. 
            If an incident is not present in the database already it is inserted and sent to all the other users
            FORMAT: "Alert: T:<event_type> S:<street_name>"*/
            
            else if(strncmp(command,"Search: ",8)==0)
            {
                char* searchinput=command+8;
                char interogate[256];
                char aux[2048];
                sqlite3 *db;
                char* error_message;
                if(0!=sqlite3_open("traffic.db",&db))
                {
                    printf("Error opening database: %s\n",sqlite3_errmsg(db));
                    exit(-1);
                }
                if(searchinput[0]=='E')
                {
                    char interogate[256];
                    strcpy(interogate,"SELECT event_title,description,street_name FROM events NATURAL JOIN map WHERE event_title LIKE '%");
                    strcat(interogate,searchinput+2);
                    interogate[strlen(interogate)-1]='\0';
                    strcat(interogate,"%' OR description LIKE '%");
                    strcat(interogate,searchinput+2);
                    interogate[strlen(interogate)-1]='\0';
                    strcat(interogate,"%' OR event_type LIKE '%");
                    strcat(interogate,searchinput+2);
                    interogate[strlen(interogate)-1]='\0';
                    strcat(interogate,"%'");
                    strcpy(aux,"Results found:\n");
                    if(sqlite3_exec(db,interogate,callback_events,&aux,&error_message)!=SQLITE_OK)
                    {
                        printf("Error opening database: %s\n",sqlite3_errmsg(db));
                        sqlite3_free(error_message);
                    }
                    answer=strdup(aux);
                }
                else if(searchinput[0]=='P')
                {
                    char interogate[256];
                    strcpy(interogate,"SELECT location_name,description,street_name FROM interest_points NATURAL JOIN map WHERE location_name LIKE '%");
                    strcat(interogate,searchinput+2);
                    interogate[strlen(interogate)-1]='\0';
                    strcat(interogate,"%' OR description LIKE '%");
                    strcat(interogate,searchinput+2);
                    interogate[strlen(interogate)-1]='\0';
                    strcat(interogate,"%' OR location_type LIKE '%");
                    strcat(interogate,searchinput+2);
                    interogate[strlen(interogate)-1]='\0';
                    strcat(interogate,"%'");
                    strcpy(aux,"Results found:\n");
                    if(sqlite3_exec(db,interogate,callback_events,&aux,&error_message)!=SQLITE_OK)
                    {
                        printf("Error opening database: %s\n",sqlite3_errmsg(db));
                        sqlite3_free(error_message);
                    }
                    answer=strdup(aux);
                }
                else
                {
                    answer=strdup("Search syntax: Search: (<E:>|<P:>)<String>");
                }                
            }   
            /*Search: This command allows for searching additional information present on the map based
            on a pattern: (FI: Search: E:GAS)
            FORMAT: "Search: E:<pattern>" - For searching in the event table 
                OR  "Search: P:<pattern>" - For searching in the place table*/

        else
            answer = strdup("Command not recognized!\n");
        }

        int aux = strlen(answer);
        /*Return answer to the client*/
        if (write(tdL.cl, &aux, sizeof(int)) <= 0)
        {
            printf("[thread %d] ", tdL.idThread);
            perror("[thread] Can't write answer length.\n");
        }
        if (write(tdL.cl, answer, aux) <= 0)
        {
            printf("[thread %d] ", tdL.idThread);
            perror("[thread] Can't write message  to client.\n");
        }
        else
            printf("[thread %d] Message sent:\n %sTotal length:%d\n", tdL.idThread, answer, aux);
        if(connected[tdL.idThread]==0)
        {
            printf("The client assigned for thread %d disconnected!\n", tdL.idThread);
            close(tdL.cl);
            break;
        }
    free(answer);
    for(int i=0;i<strlen(command);++i)
    {
        command[i]=0;
    }
    free(command);
    length=0;
    }
}

void init_db()
{
    sqlite3* db;
    if(0!=sqlite3_open("traffic.db",&db))
    {
        printf("Error opening database: %s\n",sqlite3_errmsg(db));
        exit(-1);
    } /* Open the database*/
    char create_neighbourhood_query[256]=R"(create table if not exists
        neighbourhoods(
        neighbourhood_id TEXT PRIMARY KEY NOT NULL,
        neighbourhood_name TEXT NOT NULL
    );)";

    char create_map_query[256]=R"(create table if not exists 
    map(
        street_id TEXT PRIMARY KEY NOT NULL,
        street_name TEXT NOT NULL,
        neighbourhood_id TEXT NOT NULL,
        neighbourhood_name TEXT NOT NULL
    );)";
    char create_events_query[500]=R"(create table if not exists
    events(
        event_id TEXT PRIMARY KEY NOT NULL,
        event_title TEXT NOT NULL,
        event_type TEXT NOT NULL,
        extra TEXT NOT NULL,
        description TEXT NOT NULL,
        street_id TEXT NOT NULL,
        neighbourhood_id TEXT NOT NULL
    );)";
    char create_users_query[256]=R"(create table if not exists
    users(
        user_id TEXT PRIMARY KEY NOT NULL,
        user_name TEXT NOT NULL
    );)";
    char create_speed_query[256]=R"(create table if not exists
    speed(
        street_id TEXT PRIMARY KEY NOT NULL,
        speed_limit TEXT NOT NULL
    );)";
    char create_location_query[500]=R"(create table if not exists
    interest_points(
        location_id TEXT PRIMARY KEY NOT NULL,
        location_name TEXT NOT NULL,
        location_type TEXT NOT NULL,
        extra TEXT NOT NULL,
        description TEXT,
        street_id TEXT NOT NULL,
        neighbourhood_id TEXT NOT NULL
    );)";
    char* error_message;
     if(sqlite3_exec(db,create_neighbourhood_query,NULL,NULL,&error_message)!=SQLITE_OK)
    {
        printf("Error creating neigh table %s\n",error_message);
        sqlite3_free(error_message);
    }
    if(sqlite3_exec(db,create_map_query,NULL,NULL,&error_message)!=SQLITE_OK)
    {
        printf("Error creating map table %s\n",error_message);
        sqlite3_free(error_message);
    }
        if(sqlite3_exec(db,create_events_query,NULL,NULL,&error_message)!=SQLITE_OK)
    {
        printf("Error creating events table %s\n",error_message);
        sqlite3_free(error_message);
    }
    if(sqlite3_exec(db,create_users_query,NULL,NULL,&error_message)!=SQLITE_OK)
    {
        printf("Error creating users table %s\n",error_message);
        sqlite3_free(error_message);
    }
    if(sqlite3_exec(db,create_speed_query,NULL,NULL,&error_message)!=SQLITE_OK)
    {
        printf("Error creating speed table %s\n",error_message);
        sqlite3_free(error_message);
    }
    if(sqlite3_exec(db,create_location_query,NULL,NULL,&error_message)!=SQLITE_OK)
    {
        printf("Error creating location table %s\n",error_message);
        sqlite3_free(error_message);
    } /*Create tables for DB if not exist */

    /*R"(INSERT INTO neighbourhoods VALUES('1','Podu Ros'))";
    //R"(INSERT INTO neighbourhoods VALUES('2','Cantemir'))";
     char* insertion=R"(INSERT INTO neighbourhoods VALUES('3','Nicolina'))";
     if(sqlite3_exec(db,insertion,NULL,NULL,&SELECT event_title,description,street_name FROM events NATURAL JOIN map WHERE event_title LIKE '%a%' OR description LIKE '%a%')ES('12','Strada Clopotari','3','Nicolina'))";
    if(sqlite3_exec(db,insertion,NULL,NULL,&error_message)!=SQLITE_OK)
    {
        printf("Error inserting neighbourhoods tabspeedle %s\n",error_message);
        sqlite3_free(error_message);
    }*/
    /*char* insertion=R"(INSERT INTO speed VALUES('12','30'))";
     if(sqlite3_exec(db,insertion,NULL,NULL,&error_message)!=SQLITE_OK)
    {
        printf("Error inserting neighbourhoods table %s\n",error_message);
        sqlite3_free(error_message);
    }*/
    /*char* insertion=R"(INSERT INTO events VALUES(
        '3', answer=strdup("Street change detected!\n");
        'Proiectie Lumini',
        'Spectacol',
        '1',
        'Concurs proiectie lumini 15 Ianuarie',
        '3','1'))";
        if(sqlite3_exec(db,insertion,NULL,NULL,&error_message)!=SQLITE_OK)
    {
        printf("Error inserting neighbourhoods table %s\n",error_message);
        sqlite3_free(error_message);
    }
    char* insertion=R"(INSERT INTO users VALUES(
    '2','Admin'))";
     if(sqlite3_exec(db,insertion,NULL,NULL,&error_message)!=SQLITE_OK)
    {
        printf("Error inserting neighbourhoods table %s\n",error_message);
        sqlite3_free(error_message);
        if(sqlite3_exec(db,"SELECT COUNT(*) FROM events",callback,NULL,&error_message)!=SQLITE_OK)
    }
        if(sqlite3_exec(db,"SELECT * FROM events",callback,NULL,&error_message)!=SQLITE_OK)
    {
        printf("SELECT event_title,description,street_name FROM events NATURAL JOIN map WHERE event_title LIKE '%a%'rror inserting neighbourhoods table %s\n",error_message);
        sqlite3_free(error_message);
    }
    exit(0);*/
    sqlite3_close(db);
} /*Database initialization and easier modifications for test purposes*/

int main()
{
    struct sockaddr_in server; //Socket structure used for connection (IPv4) (server)
    struct sockaddr_in from; //Socket structure used for connection (IPv4) (client)
    int sd;
    int pid;
    pthread_t th[1024];
    int i = 0;
    init_db();
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) /*AF_INET=address family for IPV4 SOCK_STREAM=TCP socket socket(int domain,int type,int protocol)-creates and endpoint for comunication returning a file descriptor referring to that endpoint. Value=lowest fd not used*/
    {
        perror("[Server] Can't create socket!\n");
        exit(1);
    }
    int on = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)); /* int setsockopt(int sockfd, int level, int optname,const void *optval, socklen_t optlen); sockfd=socket descriptor, level: SOL_SOCKET API LEVEL, optname= ALLOW REUSE OF LOCAL ADRESS*/
    bzero(&server, sizeof(server)); // Clean structures
    bzero(&from, sizeof(from)); //Clean structures

    server.sin_family = AF_INET; /* Type of adresses that can comunicate with the socket IPv4 family */
    server.sin_addr.s_addr = htonl(INADDR_ANY); /* Accept any local adress*/
    server.sin_port = htons(PORT); /*Define the port where we accept connections*/

    if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) /*Bind the adress specified in the server structure to the socket descriptor*/
    {
        perror("[Server] Can't bind!\n");
        exit(2);
    }
    if (listen(sd, 5) == -1) /* sd listens for connections. Max backlog is 2nd argument*/
    {
        perror("[Server] Can't listen.\n");
        exit(3);
    }
    while (1)
    {
        thData *td;
        int length = sizeof(from);
        printf("[server] Waiting at port %d\n", PORT);
        fflush(stdout);
        int client;
        if ((client = accept(sd, (struct sockaddr *)&from, &length)) < 0) /* Extract the first connection request from sd, creates a new socket returning its fd*/
        {
            perror("[Server] Error at accept!\n");
            exit(4);
        }
        connected[counter]=1;
        client_list[counter++]=client;
        int idThread;
        int cl;
        td = (struct thData *)malloc(sizeof(struct thData));
        td->idThread = i++;
        td->cl = client;
        pthread_create(&th[i], NULL, &treat, td); /* 1-location of storing the ID of the new thread, 2- Atributes for thread Start a new thread that executes the 3rd argument. The 3rd argument is a func that has as a parameter the 4th*/
    }
}