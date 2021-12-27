#include "thread.h"
#include "socketserver.h"
#include <algorithm>
#include <stdlib.h>
#include <time.h>
#include <list>
#include <vector>
#include <thread>

using namespace Sync;


class SocketThread : public Thread
{
private:
    // Reference to our connected socket.
    Socket &socket;

    // A byte array for the data we are receiving and sending.
    ByteArray data;

	// Global indicator of number of chat rooms.
	int chatRoomNum;

	// The port our server is running on.
	int port;

	//reference to number of Rooms
	int& numberRooms;

    // Reference to termination.
    bool& terminate;

    // Holder for SocketThread pointers.
    std::vector<SocketThread*> &socketThreadsHolder;

    
public:
	SocketThread(Socket& socket, std::vector<SocketThread*> &clientSockThr, bool &terminate, int port, int &numberRooms) :
		socket(socket), socketThreadsHolder(clientSockThr), terminate(terminate), port(port), numberRooms(numberRooms)
	{}

    ~SocketThread()
    {
		this->terminationEvent.Wait();
	}

    Socket& GetSocket()
    {
        return socket;
    }

    const int GetChatRoom()
    {
        return chatRoomNum;
    }

    virtual long ThreadMain()
    {
		// Parse the integer value of the port number to a string.
		std::string stringPort = std::to_string(port);

		try {
			// Attempt to gather bytestream data.
			socket.Read(data);

			std::string chatRoomString = data.ToString();
			chatRoomString = chatRoomString.substr(1, chatRoomString.size() - 1);
			chatRoomNum = std::stoi(chatRoomString);
			std::cout << "Current chat room number from socket.Read(): " << chatRoomNum << std::endl;

			while(!terminate) {
				int socketResult = socket.Read(data);
				
				std::string recv = data.ToString();
				if(recv == "shutdown") {

					ByteArray send("#close");
					socket.Write(send);
					// Iterator method to select and erase this socket thread.
					socketThreadsHolder.erase(std::remove(socketThreadsHolder.begin(), socketThreadsHolder.end(), this), socketThreadsHolder.end());

					std::cout<< "A client is shutting off from our server. Erase client!" << std::endl;
					break;
				}

				// A forward slash is appended as the first character to change the chat room number.
				if (recv[0] == '/') {
					// Split the received string.
					std::string stringChat = recv.substr(1, recv.size() - 1);
				
					// Parse the integer after the forward slash character, representing the chat room number.
					chatRoomNum = std::stoi(stringChat);
					std::cout << "A client just joined room " << chatRoomNum << std::endl;
					continue;
				}

				if (recv == "#") {
					numberRooms += 1;
					std::cout << "Created New Room: " << numberRooms <<std::endl;
					chatRoomNum = numberRooms;
					std::cout << "A client just joined room " << chatRoomNum << std::endl;
					std::string ret = ("newroom " + std::to_string(numberRooms));
					for (int i = 0; i < socketThreadsHolder.size(); i++) {
						SocketThread *clientSocketThread = socketThreadsHolder[i];
						Socket &clientSocket = clientSocketThread->GetSocket();
						ByteArray send(ret);
						clientSocket.Write(send);
					}
					continue;
				}

				for (int i = 0; i < socketThreadsHolder.size(); i++) {
					SocketThread *clientSocketThread = socketThreadsHolder[i];
					if (clientSocketThread->GetChatRoom() == chatRoomNum)
					{
						Socket &clientSocket = clientSocketThread->GetSocket();
						ByteArray sendBa(recv);
						clientSocket.Write(sendBa);
					}
				}
			}
		} 
		// Catch string-thrown exceptions (e.g. stoi())
		catch(std::string &s) {
			std::cout << s << std::endl;
		}
		// Catch thrown exceptions and distinguish them in console.
		catch(std::exception &e){
			std::cout << "A client has abruptly quit their messenger app!" << std::endl;
		}
		std::cout << "A client has left!" << std::endl;
		return 1;
	}
};

class ServerThread : public Thread
{
private:
	// Reference to socket server.
    SocketServer &server;

	// std::vector<Socket*> socketsHolder;
    std::vector<SocketThread*> socketThrHolder;

	// Given socket port number.
	int port;

	// Given chats number.
	int numberRooms;

	// Flag for termination.
    bool terminate = false;
    
public:
    ServerThread(SocketServer& server, int numberRooms, int port)
    : server(server), numberRooms(numberRooms), port(port)
    {}

    ~ServerThread()
    {
        // Close the client sockets.
        for (auto thread : socketThrHolder)
        {
            try
            {
                // Close the socket.
                Socket& toClose = thread->GetSocket();
                toClose.Close();
            }
            catch (...)
            {
                // This will catch all exceptions.
            }
        }
		std::vector<SocketThread*>().swap(socketThrHolder);
        terminate = true;
    }

    virtual long ThreadMain()
    {
        while (true)
        {
            try {
				// Stringify port number.
                std::string stringPortNum = std::to_string(port);
                std::cout << "FlexWait/Natural blocking call on client!" <<std::endl;

                // Wait for a client socket connection
                Socket sock = server.Accept();

				// Front-end receives number of chats through socket.
                std::string allChats = std::to_string(numberRooms) + '\n';

				// Byte array conversion for number of chats.
                ByteArray allChats_conv(allChats); 

				// Send number of chats.
                sock.Write(allChats_conv);
                Socket* newConnection = new Socket(sock);

                // Pass a reference to this pointer into a new socket thread.
                Socket &socketReference = *newConnection;
                socketThrHolder.push_back(new SocketThread(socketReference, std::ref(socketThrHolder), terminate, port, numberRooms));
            }
			// Catch string-thrown exception.
            catch (std::string error)
            {
                std::cout << "ERROR: " << error << std::endl;
				// Exit thread function.
                return 1;
            }
			// In case of unexpected shutdown.
			catch (TerminationException terminationException)
			{
				std::cout << "Server has shut down!" << std::endl;
				// Exit with exception thrown.
				return terminationException;
			}
        }
		return 1;
    }
};

int main(void) {
	try {
		int port = 3000;

		// Admin sets value of number of chat rooms for the server.
		int rooms = 20;

		std::cout << "SE 3316 Server" << std::endl 
			<<"Type done to quit the server..." << std::endl;

		// Create our server.
		SocketServer server(port);

		// Need a thread to perform sever operations.
		ServerThread st(server, rooms, port);

		// This will wait for input to shutdown the server
		FlexWait cinWaiter(1, stdin);
		cinWaiter.Wait();
		std::cin.get();

		// Cleanup, including exiting clients, when the user presses enter

		// Shut down and clean up the server
		server.Shutdown();

		std::cout << "Sayonara!" << std::endl;
	}
	catch(std::string error) {
		std::cout << error << std::endl;
	} catch (TerminationException term) {
		std::cout << "Shutdown" << std::endl;
	}
}