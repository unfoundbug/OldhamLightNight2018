#include "esp_err.h"

#include <sys/socket.h>

struct STCPServer
{
	int m_socketServer;
	int m_socketClient;
	struct sockaddr_in server_addr;
	int cliaddrlen;
	struct sockaddr_in client_addr;
};

esp_err_t TCPS_StartServer(int port, struct STCPServer * sServer);

bool TCPS_AcceptSocket(struct STCPServer * sServer);

bool TCPS_ClientConnected(struct STCPServer * sServer);

unsigned int TCPS_SendData(struct STCPServer * sServer, void* pData, unsigned int iDataSize);

unsigned int TCPS_RecieveData(struct STCPServer * sServer, void* pData, unsigned int iBufferSize);
