#include "server.h"

using namespace std;

//Actually allocate clients
vector<Client> Server::clients;

Server::Server() {

  //Initialize static mutex from MyThread
  MyThread::InitMutex();

  //For setsock opt (REUSEADDR)
  int yes = 1;

  //Init serverSock and start listen()'ing
  serverSock = socket(AF_INET, SOCK_STREAM, 0);
  memset(&serverAddr, 0, sizeof(sockaddr_in));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  serverAddr.sin_port = htons(PORT);

  //Avoid bind error if the socket was not close()'d last time;
  setsockopt(serverSock,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int));

  if(bind(serverSock, (struct sockaddr *) &serverAddr, sizeof(sockaddr_in)) < 0)
    cerr << "Failed to bind";

  listen(serverSock, 5);
}

/*
	AcceptAndDispatch();

	Main loop:
		Blocks at accept(), until a new connection arrives.
		When it happens, create a new thread to handle the new client.
*/
void Server::AcceptAndDispatch() {
  
  Client *c;
  MyThread *t;

  socklen_t cliSize = sizeof(sockaddr_in);

  while(1) {

          c = new Client();
	  t = new MyThread();

	  //Blocks here;
          c->sock = accept(serverSock, (struct sockaddr *) &clientAddr, &cliSize);

	  if(c->sock < 0) {
	    cerr << "Error on accept";
	  }
	  else {
	    t->Create((void *) Server::HandleClient, c);
	  }
  }
}

//Static
void *Server::HandleClient(void *args) {

  //Pointer to accept()'ed Client
  Client *c = (Client *) args;
  char buffer[256-25], message[256];
  int index;
  int n;

  //Add client in Static clients <vector> (Critical section!)
  MyThread::LockMutex((const char *) c->name);
  
    //Before adding the new client, calculate its id. (Now that we have the lock)
    cout << "\nBefore adding the new client:" << endl;
    c->SetId(Server::clients.size());
    sprintf(buffer, "Client n.%d", Server::clients.size());
		while (Server::FindClientName(buffer) == -1)
		{
			cout << "Client name already exists. Give it a random name" << endl;
			sprintf(buffer, "random name.%d", rand() % 100);
		}
    Server::clients.push_back(*c);
    c->SetName(buffer);
    cout << "Creating a client with name: " << c->name << ", id: " << c->id << " at index position: " << Server::FindClientIndex(c) << endl;
    cout << "New number of active clients: " << Server::clients.size() << endl;

  MyThread::UnlockMutex((const char *) c->name);

  while(1) {
    memset(buffer, 0, sizeof buffer);
    n = recv(c->sock, buffer, sizeof buffer, 0);

    //Client disconnected?
    if(n == 0) {
      cout << "Client " << c->name << " disconnected" << endl;
      close(c->sock);
      
      //Remove client in Static clients <vector> (Critical section!)
      MyThread::LockMutex((const char *) c->name);

        index = Server::FindClientIndex(c);
        cout << "Erasing user in position " << index << " whose name id is: " << Server::clients[index].id << endl;
        Server::clients.erase(Server::clients.begin() + index);
	cout << "New number of active clients: " << Server::clients.size() << endl;

      MyThread::UnlockMutex((const char *) c->name);

      break;
    }
    else if(n < 0) {
      cerr << "Error while receiving message from client: " << c->name << endl;
    }
    else {
      //Message received. Send to all clients.
      snprintf(message, sizeof message, "<%s>: %s", c->name, buffer); 
      cout << "Will send to all: " << message << endl;
      Server::SendToAll(message);
    }
  }

  //End thread
  return NULL;
}

void Server::SendToAll(char *message) {
  int n;

  //Acquire the lock
  MyThread::LockMutex("'SendToAll()'");
 
    for(size_t i=0; i<clients.size(); i++) {
      n = send(Server::clients[i].sock, message, strlen(message), 0);
      cout << n << " bytes sent." << endl;
    }
   
  //Release the lock  
  MyThread::UnlockMutex("'SendToAll()'");
}

void Server::ListClients() {
  cout << "Listing yet existing client names:" << endl;
  for(size_t i=0; i<clients.size(); i++) {
    cout << "ID: " << clients.at(i).id << ", name: "<< clients.at(i).name << endl;
  }
  cout << endl;
}

/*
  Should be called when vector<Client> clients is locked!
*/
int Server::FindClientIndex(Client *c) {
  for(size_t i=0; i<clients.size(); i++) {
    if((Server::clients[i].id) == c->id) return (int) i;
  }
  cerr << "Client id not found." << endl;
  return -1;
}

/*
  Should be called when vector<Client> clients is locked!
*/
int Server::FindClientName(char *c) {
	Server::ListClients();
	cout << "Looking for client name: " << c << endl;
  for(size_t i=0; i<clients.size(); i++) {
		if (strcmp (Server::clients[i].name,c) == 0)
		{
			return -1;
		}
  }
  cout << "Client name not found." << endl;
  return 0;
}
