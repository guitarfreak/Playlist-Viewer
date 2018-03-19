#ifndef SHIPPING_MODE

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "rt_misc.cpp"
#include "rt_hotload.cpp"
#include "rt_misc_win32.cpp"

#define OPEN_CONSOLE 1

#include "curl.h"
typedef CURLcode curl_global_initFunction(long flags);
curl_global_initFunction* curl_global_initX;

#else 

#define OPEN_CONSOLE 0
#include "app.cpp"

#endif


#include "curl.h"
#define Libcurl_Dll_File "libcurl.dll"



int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCode) {

	if(OPEN_CONSOLE) {
		AllocConsole();
		freopen("conin$","r",stdin);
		freopen("conout$","w",stdout);
		freopen("conout$","w",stderr);
	}

	
#ifndef SHIPPING_MODE	

	HotloadDll hotloadDll;
	initDll(&hotloadDll, "app.dll", "appTemp.dll", "lock.tmp");

#endif 

	WindowsData wData = windowsData(instance, prevInstance, commandLine, showCode);

	HMODULE curlDll = LoadLibraryA(Libcurl_Dll_File);
	curl_global_initX = (curl_global_initFunction*)GetProcAddress(curlDll, "curl_global_init");
	curl_global_initX(CURL_GLOBAL_ALL);

	ThreadQueue threadQueue;
	threadInit(&threadQueue, 7);

	AppMemory appMemory = {};

    bool firstFrame = true;
    bool secondFrame = false;
    bool isRunning = true;
    while(isRunning) {

    	bool reload = false;
		
	#ifndef SHIPPING_MODE
		
		#ifndef RELEASE_BUILD	
		if(threadQueueFinished(&threadQueue)) reload = updateDll(&hotloadDll);
	 	#endif 

	 	platform_appMain = (appMainType*)getDllFunction(&hotloadDll, "appMain");
	    platform_appMain(firstFrame, reload, &isRunning, wData, &threadQueue, &appMemory);

	#else 

	    appMain(firstFrame, reload, &isRunning, wData, &threadQueue, &appMemory);

	#endif

        if(firstFrame) firstFrame = false;
    }

	return 0;
}