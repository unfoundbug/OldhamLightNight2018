#include "UDPSocket.h"

#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_event_loop.h"
#include <sys/socket.h>

static unsigned int socklen = sizeof(struct sockaddr_in);

static const char *TAG = "UFB.UDPServer";

int get_socket_error_code(int socket)
{
    int result;
    u32_t optlen = sizeof(int);
    int err = getsockopt(socket, SOL_SOCKET, SO_ERROR, &result, &optlen);
    if (err == -1) {
        ESP_LOGE(TAG, "getsockopt failed:%s", strerror(err));
        return -1;
    }
    return result;
}

int show_socket_error_reason(int socket)
{
    int err = get_socket_error_code(socket);
    ESP_LOGW(TAG, "socket error %d %s", err, strerror(err));
    return err;
}

esp_err_t UDPSocketStart(int port, struct SUDPSocket * sServer)
{
	sServer->m_socketServer = socket(AF_INET, SOCK_DGRAM, 0);
	 if (sServer->m_socketServer < 0) {
    	show_socket_error_reason(sServer->m_socketServer);
	return ESP_FAIL;
    }
	struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sServer->m_socketServer, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    	show_socket_error_reason(sServer->m_socketServer);
		close(sServer->m_socketServer);	
		return ESP_FAIL;
    }
		
    return ESP_OK;
}

unsigned int UDPS_RecieveData(struct SUDPSocket * sServer, void* pData, unsigned int iBufferSize)
{
	int len = recvfrom(sServer->m_socketServer, pData, iBufferSize, 0, (struct sockaddr *)&sServer->client_addr, &socklen);
	uint16_t wDataChunk;
	if(len < 0)
	{
		
		return 0;
	}
	else
	{
		wDataChunk = ((uint8_t*)pData)[0];
		wDataChunk = wDataChunk << 8;
		wDataChunk |= ((uint8_t*)pData)[1];
		
		uint8_t byCSumCal=0;
		for(uint16_t i=0;i<wDataChunk; ++i)
			byCSumCal += ((uint8_t*)pData)[i+2];
		
		if(((uint8_t*)pData)[wDataChunk + 2] == byCSumCal)
			len = wDataChunk;
		else 
		{
			ESP_LOGE(TAG, "Bad checksum over %d bytes", wDataChunk);
			len = 0;
		}
		
		if (len == 0) {
			close(sServer->m_socketClient);
			sServer->m_socketClient = 0;
			show_socket_error_reason(sServer->m_socketClient);
		}
	}
	return len;
}