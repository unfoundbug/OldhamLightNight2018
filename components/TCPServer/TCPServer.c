#include "TCPServer.h"

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

static const char *TAG = "UFB.TCPServer";

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
int show_socket_error_reason(const char *str, int socket)
{
    int err = get_socket_error_code(socket);

    if (err != 0) {
        ESP_LOGW(TAG, "%s socket error %d %s", str, err, strerror(err));
    }

    return err;
}
int check_working_socket(int socket)
{
    int ret;
    ESP_LOGD(TAG, "check socket");
    ret = get_socket_error_code(socket);
    if (ret != 0) {
        ESP_LOGW(TAG, "server socket error %d %s", ret, strerror(ret));
    }
    if (ret == ECONNRESET) {
        return ret;
    }
    return 0;
}

esp_err_t TCPS_StartServer(int port, struct STCPServer * sServer)
{
	  ESP_LOGI(TAG, "server socket....port=%d\n", port);
    sServer->m_socketServer = socket(AF_INET, SOCK_STREAM, 0);
    if (sServer->m_socketServer < 0) {
        show_socket_error_reason("create_server", sServer->m_socketServer);
        return ESP_FAIL;
    }

    sServer->server_addr.sin_family = AF_INET;
    sServer->server_addr.sin_port = htons(port);
    sServer->server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sServer->m_socketServer, (struct sockaddr *)&sServer->server_addr, sizeof(sServer->server_addr)) < 0) {
        show_socket_error_reason("bind_server", sServer->m_socketServer);
        close(sServer->m_socketServer);
        return ESP_FAIL;
    }
    if (listen(sServer->m_socketServer, 5) < 0) {
        show_socket_error_reason("listen_server", sServer->m_socketServer);
        close(sServer->m_socketServer);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Server running");
    return ESP_OK;
}

bool TCPS_AcceptSocket(struct STCPServer * sServer)
{
	bool bResult = false;
	if(sServer->m_socketClient > 0)
		if(check_working_socket(sServer->m_socketClient))
			return true;

	sServer->m_socketClient = accept(sServer->m_socketServer, (struct sockaddr *)&sServer->client_addr, &socklen);
	    if (sServer->m_socketClient < 0) {
	        show_socket_error_reason("accept_server", sServer->m_socketClient);
	        return false;
	    }
	return true;
}

unsigned int TCPS_SendData(struct STCPServer * sServer, void* pData, unsigned int iDataSize);

unsigned int TCPS_RecieveData(struct STCPServer * sServer, void* pData, unsigned int iBufferSize)
{
    unsigned int len = 0;
	uint8_t byDataSize;
	uint8_t byChecksum;
	uint8_t byCSumCal=0;
    if(sServer->m_socketClient > 0)
    {
		len = recv(sServer->m_socketClient, &byDataSize, 1, 0);
		len = 0;
		
		while(len != byDataSize)
			len += recv(sServer->m_socketClient, pData+len, byDataSize-len, 0);
		
		len = recv(sServer->m_socketClient, &byChecksum, 1, 0);
		
		for(uint8_t i=0;i<byDataSize; ++i)
			byCSumCal += ((uint8_t*)pData)[i];
		
		if(byCSumCal == byChecksum)
			len = byDataSize;
		else 
		{
			ESP_LOGE(TAG, "Bad checksum over %d bytes", byDataSize);
			len = 0;
		}
		
		if (len == 0) {
			close(sServer->m_socketClient);
			sServer->m_socketClient = 0;
			show_socket_error_reason("recv_data", sServer->m_socketClient);
		}
    }
	return len;
}

