# Traffic_Monitoring
---------------------------------------
Final Project for the Computer Networks Course

1 Introduction
---------------------------------------
Traffic decongestion and fluidization is a modern problem that can be attenuated with the help of navigation software. This paper serves as the technical report of the application named Traffic monitoring, a project recommendation from Continental for the Computer Network course at Faculty of Computer Science. The scope of this application is being able to monitorize the traffic upon a certain area by providing informations to the drivers. In addition to that, the drivers can report temporary incidents on the road for the others or modify informations in the system.

2 Application architecture
---------------------------------------
The application will be based upon the Client-Server model with a concurrent server. In this way, we are avoiding the starvation problem that appears when using an iterative server with an increased number of clients. 

The project implementation is based on the Transmission Control Protocol (TCP). This connection oriented method was chosen in order to assure a safe transport of data between the clients and the server. For this type of application we must be sure that we are not losing any data packets during the transfer nor receiving them in a different order. In addition to this, it is more reliable to keep a constant connection between the clients and the server in order to send traffic updates to the participants. Therefore, the TCP protocol will be used for implementing the application. 

The concurrency pattern used will be threading. For each client the server will create a new thread that will execute the commands required. Threading was chosen over forking because of the common memory zone present in the technique, allowing the server to send information faster to the clients. 

The information transmitted between the client and the server will be done using sockets. Each client will create a parent and a child process after the connection is established. From this point the client-server communication is assured by the parent process while the server-client is managed by the chil. (Write to server in parent read from child). 

All the traffic details will be stored into a SQLite database. This database can be modified over time by the clients and each update of it will be sent to the connected clients.

More details in the PDF file. 
