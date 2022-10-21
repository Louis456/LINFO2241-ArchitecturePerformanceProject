# LINFO2241-ArchitecturePerformanceProject
Project for the course LINFO2241 Architecture and performance of computer systems at UCLouvain measuring the performance of a client-server application.

**targets** :
- *make* : make the client and server executables
- *make client* : make the client executable only
- *make server* : make the server executable only
- *make clean* : remove all object files and executables


You can then start the client with the following command :

```
./client [-k key_size] [-r request_rate] [-t duration] serverip:port
```

You can start the server with the following command :

```
./server [-j nb_thread] [-s size] [-p port]
```
