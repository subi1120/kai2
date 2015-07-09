#include "kai.h"


bool kaiInit = false;


#ifdef WIN32
int kaiInitializeWinSock(void)
{
	WORD versionRequested;
	WSADATA wsaData;

	versionRequested = MAKEWORD(2, 0);
	int error = WSAStartup(versionRequested, &wsaData);
	if (error)
	{
		return 1;
	}
	else
	{
		if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 0)
		{
			WSACleanup();
			return 1;
		}
	}

	return 0;
}
#endif

int kaiInitialize()
{
	if (kaiInit)
		return 0;

	kaiInit = true;

#ifdef WIN32
	return kaiInitializeWinSock();
#else
	return 0;
#endif

}

void kaiShutdown()
{
	if (!kaiInit)
		return;

	kaiInit = false;
#ifdef WIN32
	WSACleanup();
#endif
}

