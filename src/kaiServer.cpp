#ifdef WIN32
#include <WinSock2.h>
#include <process.h>
#include <mmsystem.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#endif

#include <algorithm>
#include "kaiDefinition.h"
#include "kaiBuffer.h"
#include "kaiFunction.h"
#include "kaiMsg.h"
#include "kaiIDGenerator.h"
#include "kaiSocket.h"
#include "kaiServer.h"


#ifdef WIN32
void threadFunc(void* param)
#else
void* threadFunc(void* param)
#endif
{
	kaiServer* server = (kaiServer*)param;

	int sleepTime = 0;
	const int procTime = (int)(1000.f/server->_loopPerSec);
	int prevTime = 0;
	int elapsedTime;
	int curTime = 0;

	while (!server->_bStop)
	{
		server->_run();

		curTime = kaiGetCurrentSystemTime();
		elapsedTime = curTime - prevTime;
		prevTime = curTime;
		server->_sleepTime += procTime - elapsedTime;

		if(server->_sleepTime < 0)
			server->_sleepTime = 0;
		else if(server->_sleepTime > 0)
                {
#ifdef WIN32
			Sleep(server->_sleepTime);
#else
			timeval dt;
			dt.tv_usec = server->_sleepTime * 1000;
			dt.tv_sec = dt.tv_usec / 1000000;
			dt.tv_usec = dt.tv_usec % 1000000;
			select(1, NULL, NULL, NULL, &dt); 
#endif
                }
                
	}

#ifdef WIN32
	_endthread();
#else
	return 0;
#endif
}


kaiServer::kaiServer() : _serverSocket(), _curClientNum(0), _maxClientNum(0), _clientList(), _event(kaiNO_EVENT),
						_bTCP(true), _port(0), _bStop(false), _sleepTime(0)
{
#ifdef WIN32
	_hThread = (int)INVALID_HANDLE_VALUE;
#else
	_hThread = -1;
	_maxfd = 0;
#endif
#ifdef WIN32
	InitializeCriticalSection(&_cs);
#else
        pthread_mutex_init(&_lock, NULL);

#endif
}

kaiServer::~kaiServer()
{
	if(!_bStop)
		stop();
	for(ClientList::iterator iter = _clientList.begin(); iter != _clientList.end(); ++iter)
	{
		delete iter->second;
	}
	_clientList.clear();
	_clientListByID.clear();
#ifdef WIN32
	DeleteCriticalSection(&_cs);
#else
        pthread_mutex_destroy(&_lock);
#endif
	delete _idGen;
}

int kaiServer::create(unsigned short port, long event, bool bTCP)
{
	_bTCP = bTCP;
	_port = port;
	_event = event;
	_maxClientNum = 60;


	FD_ZERO(&_rset);
	_idGen = new kaiIDGenerator(_maxClientNum + 1);
	_timeout.tv_sec = 0;
	_timeout.tv_usec = 0;

	if (_bTCP)
	{
		int res;
		_serverSocket.create(true, kaiNO_EVENT);
		_serverSocket.setBlockingMode(kaiNON_BLOCKING_SOCKET);

		res = _serverSocket.bind(port);
		if (res == kaiFAIL)
			return kaiFAIL;

		res = _serverSocket.listen();
		if (res == kaiFAIL)
			return kaiFAIL;


		FD_SET(_serverSocket.getSocket(), &_rset);
#ifndef WIN32
		_fdSets.push_back(_serverSocket.getSocket());
		_maxfd = _serverSocket.getSocket();
#endif
	}

        return kaiSUCCESS;
}

unsigned short kaiServer::openUDPChannel()
{
	if (_bTCP)
		return 0;

	if (_curClientNum >= _maxClientNum)
		return 0;
	
	unsigned short port = _port++;

	kaiSocket* newSocket = new kaiSocket();
	newSocket->create(false);
	newSocket->bind(port);
	newSocket->setID(_idGen->getID());
	newSocket->setBlockingMode(kaiNON_BLOCKING_SOCKET);

	FD_SET(newSocket->getSocket(), &_rset);
#ifndef WIN32
	_fdSets.push_back(newSocket->getSocket());
	if (newSocket->getSocket() > _maxfd)
		_maxfd = newSocket->getSocket();
#endif
	_clientList.insert(make_pair((int)newSocket->getSocket(), newSocket));
	_clientListByID.insert(make_pair(newSocket->getID(), newSocket));
	_curClientNum++;
	
	return port;
}


void kaiServer::run(bool bThreaded /* = false */, int loopPerSec /* = 50 */)
{
	if (bThreaded)
	{
		_loopPerSec = loopPerSec;
#ifdef WIN32
		_hThread = _beginthread(threadFunc, 0, (void*) this);
#else
		int state = pthread_create((pthread_t*)&_hThread, 0, threadFunc, (void*)this);
#endif
	}
	else
	{
		int sleepTime = 0;
		const int procTime = (int)(1000.f/loopPerSec);
		int prevTime = 0;
		int elapsedTime;
		int curTime = 0;

		while (true)
		{	
			if (_bStop)
				break;
			_run();
			
			curTime = kaiGetCurrentSystemTime();
			elapsedTime = curTime - prevTime;
			prevTime = curTime;
			_sleepTime += procTime - elapsedTime;

			if(_sleepTime < 0)
				_sleepTime = 0;
			else if(_sleepTime > 0)
            {
#ifdef WIN32
				Sleep(_sleepTime);
#else
				timeval dt;
				dt.tv_usec = _sleepTime * 1000;
				dt.tv_sec = dt.tv_usec / 1000000;
				dt.tv_usec = dt.tv_usec % 1000000;
				select(1, NULL, NULL, NULL, &dt); 
#endif
            }
		}
	}
}

void kaiServer::stop()
{
	_bStop = true;
#ifdef WIN32
	if ((HANDLE)_hThread != INVALID_HANDLE_VALUE)
	{
		WaitForSingleObject((HANDLE)_hThread, INFINITE);
		_hThread = (int)INVALID_HANDLE_VALUE;
	}
#else
	if (_hThread != -1)
	{
		void* t_return;
		pthread_join((pthread_t)_hThread, &t_return);	
	}
#endif
}

void kaiServer::_closeClient()
{
#ifdef WIN32
	EnterCriticalSection(&_cs);
#else
	pthread_mutex_lock(&_lock);
#endif

	while(!_closeQueue.empty())
	{
		int id = _closeQueue.front();
		_closeQueue.pop();

		ClientList::iterator iter = _clientListByID.find(id);
		if(iter == _clientListByID.end())
			continue;

		kaiSocket* sock = iter->second;
		int key = (int)sock->getSocket();
		ClientList::iterator iter2 = _clientList.find(key);

		sock->close();
		if (_event&kaiON_CLOSE)
			onClose(sock->getID());

		FD_CLR(key, &_rset);

#ifndef WIN32
		vector<int>::iterator fd = find(_fdSets.begin(), _fdSets.end(), key);
		if (fd != _fdSets.end())
			_fdSets.erase(fd);

		if (key == _maxfd)
		{
			_maxfd = -1;
			for (int i = 0; i < _fdSets.size(); i++)
			{
				if (_fdSets[i] > _maxfd)
					_maxfd = _fdSets[i];
			}
		}
#endif

		if(iter2 != _clientList.end())
			_clientList.erase(iter2);
		_clientListByID.erase(iter);
		_idGen->releaseID(sock->getID());
		_curClientNum--;
		delete(sock);
	}
#ifdef WIN32
	LeaveCriticalSection(&_cs);
#else
	pthread_mutex_unlock(&_lock);
#endif
}

void kaiServer::closeClient(int id)
{
#ifdef WIN32
	EnterCriticalSection(&_cs);
#else
	pthread_mutex_lock(&_lock);
#endif

	_closeQueue.push(id);

#ifdef WIN32
	LeaveCriticalSection(&_cs);
#else
	pthread_mutex_unlock(&_lock);
#endif
}

void kaiServer::_run()
{
	_temp = _rset;
	int fdcnt;
	int fd;
	int fdmax;
#ifdef WIN32
	fdcnt = _rset.fd_count;
	fdmax = fdcnt;
#else
	fdcnt = _fdSets.size();
	fdmax = _maxfd +1;
#endif
	
	_timeout.tv_usec = 1000*_sleepTime;

	if (select(fdmax, &_temp, 0, 0, &_timeout) == SOCKET_ERROR)
	{
		printf("select error\n");
		return;
	}

	for (int idx = 0; idx < fdcnt; ++idx)
	{	
#ifdef WIN32
		fd = _rset.fd_array[idx];
#else
		fd = _fdSets[idx];
#endif

		if (FD_ISSET(fd, &_temp))
		{			
			if (fd == _serverSocket.getSocket())
			{
				if (_curClientNum >= _maxClientNum)
					continue;
				
				int id = _idGen->getID();
				kaiSocket* newSocket = new kaiSocket();
				newSocket->create(true, kaiNO_EVENT);
				newSocket->setID(id);
				_serverSocket.accept(*newSocket);
				newSocket->setBlockingMode(kaiNON_BLOCKING_SOCKET);
				newSocket->setManualClose(true);
					
				FD_SET(newSocket->getSocket(), &_rset);
#ifndef WIN32
				_fdSets.push_back(newSocket->getSocket());
				if (newSocket->getSocket() > _maxfd)
					_maxfd = newSocket->getSocket();
#endif
				_clientList.insert(make_pair((int)newSocket->getSocket(), newSocket));
				_clientListByID.insert(make_pair(newSocket->getID(), newSocket));
				_curClientNum++;
				
				if (_event&kaiON_ACCEPT)
					onAccept(newSocket->getID());
			}
			else
			{

				int key = fd;
				int id;
				ClientList::iterator iter = _clientList.find(key);
				if (iter == _clientList.end())
					continue;

				kaiSocket* sock = iter->second;
				id = sock->getID();

				int len = 0;
						
				bool breceived = false;
				if (_bTCP)
				{
					while(true)
					{
						len = sock->recv();
						if(len <= 0)
							break;
						breceived = true;	
					}
				}
				else
				{
					sockaddr_in addr;
					while(true)
					{
						len = sock->recvFrom(&addr);
						if(len <= 0)
							break;

						breceived = true;
					}
				}

				if (len <= 0 && !breceived)
				{			
					closeClient(id);
					continue;
				}

				if (_event&kaiON_MESSAGE)	
				{
					while (true)
					{
						kaiMsg msg;
						
						if (!sock->getMessage(msg))
							break;
						onMessage(sock->getID(), msg);
					}
				}
				else if (_event&kaiON_RECV)
				{
					void* msg;
					int len = 0;
					sock->getMessage(&msg, len);
					onReceive(sock->getID(), msg, len);

				}
			}
		}
	}
	_closeClient();
}

kaiSocket* kaiServer::findClient(int id)
{
	ClientList::iterator iter = _clientListByID.find(id);
	if (iter == _clientListByID.end())
		return NULL;

	return iter->second;
}
