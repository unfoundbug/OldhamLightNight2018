#include "esp_err.h"

#include <sys/socket.h>

struct SUDPSocket
{
	int m_socketServer;
	int m_socketClient;
	struct sockaddr_in server_addr;
	int cliaddrlen;
	struct sockaddr_in client_addr;
};

esp_err_t UDPSocketStart(int port, struct SUDPSocket * sServer);
unsigned int UDPS_RecieveData(struct SUDPSocket * sServer, void* pData, unsigned int iBufferSize);