#ifdef WIN32
#include <WinSock2.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#endif

#include "kaiDefinition.h"
#include "kaiFunction.h"
#include "kaiBuffer.h"
#include "kaiMsg.h"
#include "kaiSocket.h"

kaiSocket::kaiSocket() : _event(0), _socket(INVALID_SOCKET), _recvBuffer(NULL), _sendBuffer(NULL), _userParam(-1), _bManualClose(false)
{

}

kaiSocket::~kaiSocket()
{
	if (_recvBuffer)
		delete _recvBuffer;
	if (_sendBuffer)
		delete _sendBuffer;

	if(_socket != INVALID_SOCKET)
		kaiCloseSocket(_socket);
}

void kaiSocket::setManualClose(bool b)
{
	_bManualClose = b;
}

void kaiSocket::setUserParam(int param)
{
	_userParam = param;
}

int kaiSocket::getUserParam() const
{
	return _userParam;
}

void kaiSocket::create(bool bTCP, long lEvent, unsigned buffersize, unsigned maxdatasize)
{	
	if (bTCP)
		_socket = kaiCreateSocket(kaiTCP);
	else
		_socket = kaiCreateSocket(kaiUDP);
		
	if (!_recvBuffer)
	{
		_recvBuffer = new kaiBuffer();
		_recvBuffer->init(buffersize, maxdatasize);		
	}

	if (!_sendBuffer)
	{
		_sendBuffer = new kaiBuffer();
		_sendBuffer->init(buffersize, maxdatasize);		
	}
	_event = lEvent;
}

int kaiSocket::send(const char* buf, int len)
{
	int sendSize = 0;

    do
    {
        sendSize = kaiSend(_socket, buf, len);
        if (sendSize == kaiFAIL)
            return kaiFAIL;
        buf += sendSize;
        len -= sendSize;
    } while (len > 0);

    if (_event&kaiON_SEND)
        onSend(sendSize);

    return len;
}

int kaiSocket::send(const kaiMsg& msg)
{
    return send(msg.buffer(), msg.size() + sizeof(kaiHeader));
}

int kaiSocket::send()
{
	unsigned size = _sendBuffer->getDataSize();
	int sendSize = 0;
	if (size > 0)
	{
		sendSize = kaiSend(_socket, _sendBuffer->bufferOut(), size);
		if (sendSize != kaiFAIL)
			_sendBuffer->pop(sendSize);
	}

	if ((_event&kaiON_SEND) && sendSize > 0)
		onSend(sendSize);

	return sendSize;
}

int kaiSocket::sendTo(const char* buf, int len, const char* addr, unsigned short port)
{
	sockaddr_in sock_addr;
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(port);
	sock_addr.sin_addr.s_addr = inet_addr(addr);

	int sendSize = kaiSendTo(_socket, buf, len, &sock_addr);

	if ((_event&kaiON_SEND) && sendSize > 0)
		onSend(sendSize);

	return sendSize;
}

int kaiSocket::sendTo(const kaiMsg& msg, const char* addr, unsigned short port)
{
	int sendSize = sendTo(msg.buffer(), msg.size() + sizeof(kaiHeader), addr, port);

	if ((_event&kaiON_SEND) && sendSize > 0)
		onSend(sendSize);

	return sendSize;
}

int kaiSocket::sendTo(const kaiMsg& msg)
{
	int sendSize = kaiSendTo(_socket, msg.buffer(), msg.size() + sizeof(kaiHeader), &_addr);

	if ((_event&kaiON_SEND) && sendSize > 0)
		onSend(sendSize);

	return sendSize;
}

int kaiSocket::sendTo(const char* addr, unsigned short port)
{
	unsigned size = _sendBuffer->getDataSize();
	int sendSize = 0;
	if (size > 0)
	{
		sockaddr_in sock_addr;
		sock_addr.sin_family = AF_INET;
		sock_addr.sin_port = htons(port);
		sock_addr.sin_addr.s_addr = inet_addr(addr);

		sendSize = kaiSendTo(_socket, _sendBuffer->bufferOut(), size, &sock_addr);	
		if (sendSize != kaiFAIL)
			_sendBuffer->pop(sendSize);
	}

	if ((_event&kaiON_SEND) && sendSize > 0)
		onSend(sendSize);

	return sendSize;
}


int kaiSocket::sendTo(const char* buf, int len, const sockaddr_in* lpSockAddr)
{
	int sendSize = kaiSendTo(_socket, buf, len, lpSockAddr);

	if ((_event&kaiON_SEND) && sendSize > 0)
		onSend(sendSize);

	return sendSize;

}
int kaiSocket::sendTo(const kaiMsg& msg, const sockaddr_in* lpSockAddr)
{
	int sendSize = kaiSendTo(_socket, msg.buffer(), msg.dataSize(), lpSockAddr);

	if ((_event&kaiON_SEND) && sendSize > 0)
		onSend(sendSize);

	return sendSize;
}

int kaiSocket::sendTo(const sockaddr_in* lpSockAddr)
{	
	unsigned size = _sendBuffer->getDataSize();
	int sendSize = 0;

	if (size > 0)
	{
		sendSize = kaiSendTo(_socket, _sendBuffer->bufferOut(), size, lpSockAddr);	
		if (sendSize != kaiFAIL)
			_sendBuffer->pop(sendSize);
	}

	if ((_event&kaiON_SEND) && sendSize > 0)
		onSend(sendSize);

	return sendSize;
}

int kaiSocket::recv()
{
	int recvSize = kaiRecv(_socket, _recvBuffer->bufferIn(), kaiMAX_MSG_SIZE);
	
	if (recvSize > 0)
	{
		_recvBuffer->put(recvSize);
	
		if (_event&kaiON_MESSAGE)
		{
			while (true)
			{
				kaiMsg msg;
				if (!getMessage(msg))
					break;
				onMessage(msg);
			}
		}
		else if (_event&kaiON_RECV)
		{
			void* data;
			int len;
			getMessage(&data, len);
			onReceive(data, len);	
		}
	}
	else if (recvSize == kaiFAIL && !_bManualClose)
	{
		close();
	}
	
	return recvSize;
}

int kaiSocket::recv(void* buf, int len)
{
	int recvSize = kaiRecv(_socket, (char*)buf, len);
	if (recvSize == kaiFAIL)
	{
		close();
	}
	return recvSize;
}

void kaiSocket::setBlockingMode(int blocking)
{
	kaiSetBlockingMode(_socket, blocking);
}

int kaiSocket::recvFrom(string& addr, unsigned short& port)
{	
	int recvSize = kaiRecvFrom(_socket, _recvBuffer->bufferIn(), kaiMAX_MSG_SIZE, &_addr);
	
	if (recvSize != kaiFAIL)
		_recvBuffer->put(recvSize);

	addr = inet_ntoa(_addr.sin_addr);
	port = ntohs(_addr.sin_port);

	if (_event&kaiON_MESSAGE)
	{
		while (true)
		{
			kaiMsg msg;
			if (!getMessage(msg))
				break;
			onMessage(msg);
		}
	}

	return recvSize;
}

int kaiSocket::recvFrom(void* buf, int len, string& addr, unsigned short& port)
{
	int recvSize = kaiRecvFrom(_socket, (char*)buf, len, &_addr);
	
	addr = inet_ntoa(_addr.sin_addr);
	port = ntohs(_addr.sin_port);

	return recvSize;
}

int kaiSocket::recvFrom(sockaddr_in* lpSockAddr)
{	
	int recvSize = kaiRecvFrom(_socket, _recvBuffer->bufferIn(), kaiMAX_MSG_SIZE, lpSockAddr);

	if (recvSize != kaiFAIL)
		_recvBuffer->put(recvSize);
		
	_addr = *lpSockAddr;

	if (_event&kaiON_MESSAGE)
	{
		while (true)
		{
			kaiMsg msg;
			if (!getMessage(msg))
				break;
			onMessage(msg);
		}
	} 
	else if (_event&kaiON_RECV)
	{
		void* data;
		int len;
		getMessage(&data, len);
		onReceive(data, len);	
	}

	return recvSize;
}

int kaiSocket::recvFrom(void* buf, int len, sockaddr_in* lpSockAddr)
{
	int recvSize =  kaiRecvFrom(_socket, (char*)buf, len, lpSockAddr);
	_addr = *lpSockAddr;
	return recvSize;
}

void kaiSocket::close()
{
	if (_socket == INVALID_SOCKET)
		return; 

	kaiCloseSocket(_socket);
	if (_event & kaiON_CLOSE)
		onClose();
	_socket = INVALID_SOCKET;
	
	if(_recvBuffer)
		_recvBuffer->refresh();
	if(_sendBuffer)
		_sendBuffer->refresh();

	
}

int kaiSocket::connect(const char* addr, unsigned short port)
{
	return kaiConnectSocket(_socket, addr, port);	
}


int kaiSocket::listen()
{
	if (::listen(_socket, 16) == SOCKET_ERROR)
	{
		kaiCloseSocket(_socket);
		return kaiFAIL;
	}
	
	return kaiSUCCESS;
}

int kaiSocket::bind(unsigned short port)
{
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if (::bind(_socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		kaiCloseSocket(_socket);
		return kaiFAIL;
	}
	
	return kaiSUCCESS;
}

void kaiSocket::accept(kaiSocket& sock)
{
	sockaddr_in addr;
#ifdef WIN32
	int len = sizeof(addr);
#else
        socklen_t len = sizeof(addr);
#endif
	
	SOCKET newSock = ::accept(_socket, (sockaddr*)&addr, &len);
	sock.setSocket(newSock);
	
	if (_event&kaiON_ACCEPT)
		onAccept(sock);
}

void kaiSocket::setSocket(SOCKET sock)
{	
	if (_socket != INVALID_SOCKET)
		kaiCloseSocket(_socket);
	memset(&_socket, 0, sizeof(SOCKET));
	_socket = sock;
}

SOCKET kaiSocket::getSocket()
{
	return _socket;
}

int kaiSocket::getID() const
{
	return _id;
}

void kaiSocket::setID(int id)
{
	_id = id;
}

bool kaiSocket::getMessage(kaiMsg& msg)
{
	unsigned datasize = _recvBuffer->getDataSize();
	if (datasize < sizeof(kaiHeader))
		return false;

	kaiMsgSize size = 0;
	memcpy(&size, _recvBuffer->bufferOut()+Size_kaiMsgID, Size_kaiMsgSize);
	
	if (datasize < size+sizeof(kaiHeader))
		return false;

	kaiFlag flag;
	kaiMsgID id;

	memcpy(&id, _recvBuffer->bufferOut(), Size_kaiMsgID);
	memcpy(&flag, _recvBuffer->bufferOut()+6, Size_kaiFlag);
	
	msg.setBuffer((char*)_recvBuffer->bufferOut());
	msg.size(size);
	msg.flag(flag);
	msg.id(id);
	
	_recvBuffer->pop(size +  sizeof(kaiHeader));

	return true;
}

void kaiSocket::getMessage(void** data, int& len)
{
	len = _recvBuffer->getDataSize();
	*data = (void*)_recvBuffer->bufferOut();
	_recvBuffer->pop(len);
}

void kaiSocket::putMessage(const kaiMsg& msg)
{
	_sendBuffer->put(msg.buffer(), msg.dataSize());
}

void kaiSocket::putMessage(const void* data, int len)
{
	_sendBuffer->put(data, len);
}

void kaiSocket::reset()
{
	delete _recvBuffer;
	delete _sendBuffer;
	_recvBuffer = NULL;
	_sendBuffer = NULL;
	memset(&_socket, 0, sizeof(SOCKET));
}

void kaiSocket::setNoDelay()
{
	int val = 1;
	setsockopt(_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&val, sizeof(val));
}
