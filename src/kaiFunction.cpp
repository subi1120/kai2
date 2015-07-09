#ifdef WIN32
#include <WinSock2.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include "kaiDefinition.h"
#include "kaiFunction.h"

static unsigned int timebase = 0;
static int bTimeInit = false;


SOCKET kaiCreateSocket(int _type)
{
	int type;
	int protocol;
	SOCKET sock;
	
	if (_type == kaiTCP)
	{
		protocol = IPPROTO_TCP;
		type = SOCK_STREAM;
	}
	else
	{
		protocol = IPPROTO_UDP;
		type = SOCK_DGRAM;
	}
	if ((sock = socket(PF_INET, type, protocol)) == SOCKET_ERROR)
		return kaiINVALID_SOCKET;

	return sock;
}

SOCKET kaiBindSocket(unsigned short port, int type)
{
	SOCKET sock = kaiCreateSocket(type);
	if (sock == kaiINVALID_SOCKET)
		return kaiINVALID_SOCKET;
		
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		kaiCloseSocket(sock);
		return kaiINVALID_SOCKET;
	}	
	return sock;

}

int kaiConnectSocket(SOCKET sock, const char* addr, unsigned short port)
{
	sockaddr_in sock_addr;
	kaiMakeSockAddr(addr, port, &sock_addr);
	
	if (connect(sock, (sockaddr*)&sock_addr, sizeof(sock_addr)) == SOCKET_ERROR)
		return kaiFAIL;
	
	return kaiSUCCESS;
}

int kaiConnectSocket(SOCKET sock, const sockaddr_in* sock_addr)
{
	if (connect(sock, (sockaddr*)&sock_addr, sizeof(*sock_addr)) == SOCKET_ERROR)
		return kaiFAIL;

	return kaiSUCCESS;
}

SOCKET kaiListenSocket(unsigned short port)
{	

	SOCKET sock = kaiBindSocket(port, kaiTCP);
	if (sock == kaiINVALID_SOCKET)
		return kaiINVALID_SOCKET;

	if (listen(sock, 5) == SOCKET_ERROR)
	{
		kaiCloseSocket(sock);
		return kaiINVALID_SOCKET;
	}	
	return sock;

}

void kaiCloseSocket(SOCKET sock)
{
#ifdef WIN32
	closesocket(sock);
#else
	shutdown(sock, SHUT_RDWR);
#endif
}

void kaiMakeSockAddr(const char* addr, unsigned short port, sockaddr_in* sock_addr)
{
	sock_addr->sin_family = AF_INET;
	sock_addr->sin_port = htons(port);
	sock_addr->sin_addr.s_addr = inet_addr(addr);
}

void kaiSetBlockingMode(SOCKET sock, unsigned long blocking)
{
#ifdef WIN32
	ioctlsocket(sock, FIONBIO, &blocking);
#else
	ioctl(sock, FIONBIO, &blocking);
#endif
}

int kaiSetBroadcasting(SOCKET sock, int mode)
{	
	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&mode, sizeof(int)) == -1)
		return kaiFAIL;

	return kaiSUCCESS;
}

int kaiRecv(SOCKET sock, char* data, int len)
{
	// sangyobe (31/01/09)
	// when recv returns error with error code EWOULDBLOCK, return 0.
	// when the connection closed then returns 0, returns kaiFAIL for user to have chance to close the socket.

	int recv_len = 0;	
	if ((recv_len = recv(sock, data, len, 0)) == SOCKET_ERROR)
	{
#ifdef WIN32
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
			return kaiFAIL;
#else
		if (errno != EWOULDBLOCK)
			return kaiFAIL;
#endif
		recv_len = 0;
	}
	else if (recv_len == 0)
		return kaiFAIL;
	return recv_len;
}

int kaiSend(SOCKET sock, const char* data, int len)
{
	int send_len = 0;

	if ((send_len = send(sock, data, len, 0)) == SOCKET_ERROR)
	{
#ifdef WIN32
		if (WSAGetLastError() != WSAEWOULDBLOCK)
			return kaiFAIL;
#else
		if (errno != EWOULDBLOCK)
			return kaiFAIL;
#endif
		send_len = 0;
	}
	return send_len;;
}

int kaiRecvFrom(SOCKET sock, char* data, int len, sockaddr_in* from)
{
#ifdef WIN32
	int addrLen = sizeof(*from);
#else
        socklen_t addrLen = sizeof(*from);
#endif
	int recv_len = 0;
	
	if ((recv_len = recvfrom(sock, data, len, 0, (sockaddr*)from, &addrLen)) == SOCKET_ERROR)
	{
#ifdef WIN32
		if (WSAGetLastError() != WSAEWOULDBLOCK)
			return kaiFAIL;
#else
		if (errno != EWOULDBLOCK)
			return kaiFAIL;
#endif
		recv_len = 0;
	}	
	else if (recv_len == 0)
		kaiFAIL;

	return recv_len;
}

int kaiSendTo(SOCKET sock, const char* data, int len, const sockaddr_in* to)
{	
	int send_len = 0;

	if ((send_len = sendto(sock, data, len, 0, (sockaddr*)to, sizeof(*to))) == SOCKET_ERROR)
	{
#ifdef WIN32
		if (WSAGetLastError() != WSAEWOULDBLOCK)
			return kaiFAIL;
#else
		if (errno != EWOULDBLOCK)
			return kaiFAIL;
#endif
		send_len = 0;
	}
	return send_len;
}

int kaiBroadcast(SOCKET sock, int length, char* data, unsigned short port)
{
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = port;
	addr.sin_addr.s_addr = inet_addr("255.255.255.255");

	int send_len = 0;
	if ((send_len = sendto(sock, data, length, 0, (sockaddr*)&addr, sizeof(addr))) == SOCKET_ERROR)
	{
#ifdef WIN32
		if (WSAGetLastError() != WSAEWOULDBLOCK)
			return kaiFAIL;
#else
		if (errno != EWOULDBLOCK)
			return kaiFAIL;
#endif
	}	
	return send_len;
}


int kaiGetCurrentSystemTime()
{
#ifdef WIN32
	if (!bTimeInit)
	{
		bTimeInit = true;
		timebase = timeGetTime();
	}
	unsigned int curTime = timeGetTime() - timebase;
	return curTime;
#else
	struct timeval tp;
	struct timezone tzp;
	
	gettimeofday(&tp, &tzp);
	
	if (!bTimeInit)
	{
		bTimeInit = true;
		timebase = (unsigned int)tp.tv_sec * 1000 + (unsigned int)tp.tv_usec / 1000;
	}

	return ((unsigned int)tp.tv_sec * 1000 + (unsigned int)tp.tv_usec / 1000 - timebase);
#endif

}
