/*
	Things to add in main app:
	- Cam.
	- Mouse Delta.
	- Updated rect.
	- Updated math functions like mapRange.
	- move ScissorRect to screen to render from gui.
	- math log base
	- math round up 
	- round mod down
    - Just make sure to copy over math.cpp.
	- Vec4 (float color, float alpha)
	- Draw text functions use uchar now.
	- On WM_KILLFOCUS release all the keys.
	- Gui textbox clipboard.
	- Fixed getClipboard, added closeClipboard.



	* Add gradient.
	* Fix curser selection to go to nearest.
	* Multiple charts stacked.
	* Click on point and open chrome to youtube video.
	* Scale jpg with size.
	* Save title in video struct and maybe duration as well.
	* Clipboard messed up when app is running.
	* Sort playlist by date after download.
	* Cant copy in chrome when switching from app.
	- Change draggin in gui to match the winapi method.
    - Clamp window to monitor.
	- Dithering.
	- Date timeline.
	- Abstract timeline.
	- Mouse hover, show specific data.
	- Averages and statistics.
	- Debug gui stuff should disable app input when needed. (Mouse dragging, for example.)

	- Zoom stages for bottom timeline.
	- Combine 2 graphs again and do h scroll with side bars.
	- Unicode for comments.
	- Make middle mouse button on right panel open firefox with video.
	- Get playlists from channel.
	- Bottom Zoom glitchy.
	  - Viewport left and right of cam to big probably. Should add offset so cam goes from 0 to viewport width.
	- 101 not enough space for title, found on with 102. (Including zero byte.)
	  - They probably mean 100 characters of unicode, not ascii.
	- Download playlists fron channel.
	- Draw command dots.
	- Gui when scrollsection gets initialized, put in the current window height to avoid one frame jumping.
*/

// External.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <gl\gl.h>
// #include "external\glext.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#include "external\stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "external\stb_truetype.h"

#include "external\curl\curl.h"

typedef CURLcode curl_global_initFunction(long flags);
typedef CURL *curl_easy_initFunction(void);
typedef CURLcode curl_easy_setoptFunction(CURL *curl, CURLoption option, ...);
typedef CURLcode curl_easy_performFunction(CURL *curl);
typedef void curl_easy_cleanupFunction(CURL *curl);

curl_global_initFunction* curl_global_initX;
curl_easy_initFunction* curl_easy_initX;
curl_easy_setoptFunction* curl_easy_setoptX;
curl_easy_performFunction* curl_easy_performX;
curl_easy_cleanupFunction* curl_easy_cleanupX;



struct ThreadQueue;
struct GraphicsState;
struct DrawCommandList;
struct MemoryBlock;
struct DebugState;
struct Timer;
ThreadQueue* globalThreadQueue;
GraphicsState* globalGraphicsState;
DrawCommandList* globalCommandList;
MemoryBlock* globalMemory;
DebugState* globalDebugState;
Timer* globalTimer;

// Internal.

#include "rt_types.cpp"
#include "rt_timer.cpp"
#include "rt_misc.cpp"
#include "rt_math.cpp"
#include "rt_hotload.cpp"
#include "rt_misc_win32.cpp"
#include "rt_platformWin32.cpp"

#include "memory.cpp"
#include "openglDefines.cpp"
#include "userSettings.cpp"

#include "rendering.cpp"
#include "gui.cpp"

#include "entity.cpp"

#include "debug.cpp"

void loadCurlFunctions(HMODULE dll) {
	curl_global_initX = (curl_global_initFunction*)GetProcAddress(dll, "curl_global_init");
	curl_easy_initX = (curl_easy_initFunction*)GetProcAddress(dll, "curl_easy_init");
	curl_easy_setoptX = (curl_easy_setoptFunction*)GetProcAddress(dll, "curl_easy_setopt");
	curl_easy_performX = (curl_easy_performFunction*)GetProcAddress(dll, "curl_easy_perform");
	curl_easy_cleanupX = (curl_easy_cleanupFunction*)GetProcAddress(dll, "curl_easy_cleanup");
}

struct CurlData {
	char* buffer;
	int size;
};

size_t curlWrite(void *buffer, size_t size, size_t nmemb, void *userp) {
	CurlData* cd = (CurlData*)userp;
	memCpy(cd->buffer + cd->size, buffer, nmemb);
	cd->size += nmemb;
	cd->buffer[cd->size] = '\0';

	return nmemb;
}

int curlRequest(CURL* curlHandle, char* request, char* buffer) {
	CurlData cd = {buffer, 0};
	curl_easy_setoptX(curlHandle, CURLOPT_WRITEDATA, &cd);
	curl_easy_setoptX(curlHandle, CURLOPT_URL, request);

	int success = curl_easy_performX(curlHandle);
	myAssert(success == 0);

	return cd.size;
}

char* stringGetBetween(char* buffer, char* leftToken, char* rightToken, int* advance = 0) {
	char* string;

	int leftPos = strFindRight(buffer, leftToken);
	if(leftPos != -1) {
		int rightPos = strFind(buffer + leftPos, rightToken);
		int size = rightPos;
		string = getTString(size+1); 
		strClear(string);
		strCpy(string, buffer + leftPos, rightPos);
		if(advance) *advance = leftPos + rightPos + 1; 
	} else {
		string = getTString(1); strClear(string);
	}

	return string;
}

struct Date {
	int year, month, day, hours, minutes, seconds;
	i64 num;
};

i64 dateToInt(int year, int month, int day, int hour, int minute, int second) {
	i64 result = (i64)(year*(i64)31556926);
	result += (i64)(month*2629744);
	result += (i64)(day*86400);
	result += (i64)(hour*3600);
	result += (i64)(minute*60);
	result += (i64)(second);

	return result;
}

void intToDate(i64 date, int* year, int* month, int* day, int* hour, int* minute, int* second) {
	(*year) = date/((i64)31556926); date %= 31556926;
	(*month) = date/((i64)2629744); date %= 2629744;
	(*day) = date/86400; date %= 86400;
	(*hour) = date/3600; date %= 3600;
	(*minute) = date/60; date %= 60;
	(*second) = date;
}

void intToDate(i64 date, Date* d) {
	intToDate(date, &d->year, &d->month, &d->day, &d->hours, &d->minutes, &d->seconds);
}

Date stringToDate(char* s) {
	Date date;
	char* buffer = s;
	char* d = getTString(10); 

	strCpy(d, buffer, 4); buffer += 5; date.year = strToInt(d);
	strCpy(d, buffer, 2); buffer += 3; date.month = strToInt(d);
	strCpy(d, buffer, 2); buffer += 3; date.day = strToInt(d);
	strCpy(d, buffer, 2); buffer += 3; date.hours = strToInt(d);
	strCpy(d, buffer, 2); buffer += 3; date.minutes = strToInt(d);
	strCpy(d, buffer, 2); buffer += 3; date.seconds = strToInt(d);

	date.num = dateToInt(date.year, date.month, date.day, date.hours, date.minutes, date.seconds);

	return date;
}

// #pragma pack(push,1)
struct YoutubeVideo {
	char id[12];
	char title[101];
	char thumbnail[51];
	char dateString[25];
	Date date;

	int viewCount;
	int likeCount;
	int dislikeCount;
	int favoriteCount;
	int commentCount;
};
// #pragma pack(pop)

struct YoutubePlaylist {
	char id[40];
	char title[101];
	int count;
};

char* tempFile = "messageContent.txt";

char* curlPath = "C:\\Standalone\\curl\\curl.exe";
char* apiKey = "AIzaSyD-qRen5fSH7M3ePBey1RY0vRTyW0PKyLw";
char* youtubeApiPlaylistItems = "https://www.googleapis.com/youtube/v3/playlistItems";
char* youtubeApiPlaylist = "https://www.googleapis.com/youtube/v3/playlists";
char* youtubeApiVideos = "https://www.googleapis.com/youtube/v3/videos";
char* youtubeApiCommentThread = "https://www.googleapis.com/youtube/v3/commentThreads";

int maxDownloadCount = 40;

char* rocketBeansId = "UCQvTDmHza8erxZqDkjQ4bQQ";
char* moinmoinPlaylist = "PLsksxTH4pR3KZe3wbmAP2Tgn6rfhbDlBH";
char* moinmoinFile = "MoinMoin.playlist";

char* tempPlaylistFile = "temp.playlist";

// PLZ6-wZZqtEoGeqACJ1oK9H739inthE7X_
// PLsksxTH4pR3IGid8awp3UMNIPj1HoKWBS
// UUQvTDmHza8erxZqDkjQ4bQQ
// PLsksxTH4pR3KLj8__o-le4FlJtUbA9oHC


int getYoutubePlaylistSize(CURL* curlHandle, char* playlistId) {
	char* request = getTString(1000); strClear(request);
	strAppend(request, fillString("%s?key=%s", youtubeApiPlaylistItems, apiKey));
	strAppend(request, "&maxResults=1");
	strAppend(request, fillString("&playlistId=%s", playlistId));
	strAppend(request, "&part=contentDetails");

	char* message = getTString(1000);
	curlRequest(curlHandle, request, message);

	char* sizeString = stringGetBetween(message, "\"totalResults\": ", ",");
	int playlistSize = strLen(sizeString) > 0 ? strToInt(sizeString) : 0;

	return playlistSize;
}

char* downloadYoutubePlaylistVideoIds(CURL* curlHandle, char* playlistId, char* buffer, int count, char* pageToken = 0) {
	char* request = getTString(1000); strClear(request);
	strAppend(request, fillString("%s?key=%s", youtubeApiPlaylistItems, apiKey));
	strAppend(request, fillString("&maxResults=%i", count));
	strAppend(request, fillString("&playlistId=%s", playlistId));
	strAppend(request, "&part=contentDetails");
	if(pageToken) strAppend(request, fillString("&pageToken=%s", pageToken));

	curlRequest(curlHandle, request, buffer);

	char* nextPageToken = stringGetBetween(buffer, "\"nextPageToken\": \"", "\"");
	return nextPageToken;
}

struct VideoSnippet {
	bool selectedLoaded;
	char* selectedTopComments[10];
	int selectedCommentLikeCount[10];
	int selectedCommentReplyCount[10];
	int selectedCommentCount;
	Texture thumbnailTexture;
};

void downloadVideoSnippet(CURL* curlHandle, VideoSnippet* snippet, YoutubeVideo* vid) {
	char* buffer = (char*)getTMemory(megaBytes(1)); 

	// Download thumbnail and upload texture.
	{
		int size = curlRequest(curlHandle, vid->thumbnail, buffer);
		if(snippet->thumbnailTexture.id != -1) deleteTexture(&snippet->thumbnailTexture);
		loadTextureFromMemory(&snippet->thumbnailTexture, buffer, size, -1, INTERNAL_TEXTURE_FORMAT, GL_RGB, GL_UNSIGNED_BYTE);
	}

	// Get comments.
	{
		int commentCount = 10;
		snippet->selectedCommentCount = 0;

		char* request = getTString(kiloBytes(20)); strClear(request);
		strAppend(request, fillString("%s?key=%s", youtubeApiCommentThread, apiKey));
		strAppend(request, fillString("&videoId=%s", vid->id));

		strAppend(request, "&maxResults=%i", commentCount);
		strAppend(request, "&part=snippet");
		strAppend(request, "&order=relevance");
		strAppend(request, "&textFormat=plainText");

		int size = curlRequest(curlHandle, request, buffer);

		char* content;
		int advance;
		for(int i = 0; i < commentCount; i++) {
			content = stringGetBetween(buffer, "\"textOriginal\": \"", "\",", &advance); buffer += advance; // textDisplay
			int slen = strLen(content);
			char* s = getPString(slen + 1);
			strCpy(s, content);

			// Replace \n with real new lines.
			char* temp = s;
			int index = 0;
			for(;;) {
				int pos = strFind(temp, "\\n");
				if(index+pos > slen) break;
				if(pos == -1) break;
				temp[pos] = '\n';
				strErase(temp, pos+1, 1);
				index += pos+1;
				temp += pos+1;
			}
			// int l = strLen(s);
			// for(int i = 0; i < l; i++) {
			// 	if(s[i] == 'ö') s[i] = 'o';
			// 	else if(s[i] == 'ä') s[i] = 'a';
			// 	else if(s[i] == 'ü') s[i] = 'u';
			// }
			snippet->selectedTopComments[snippet->selectedCommentCount] = s;

			content = stringGetBetween(buffer, "\"likeCount\": ", ",", &advance); buffer += advance;
			snippet->selectedCommentLikeCount[snippet->selectedCommentCount] = strToInt(content);

			content = stringGetBetween(buffer, "\"totalReplyCount\": ", ",", &advance); buffer += advance;
			snippet->selectedCommentReplyCount[snippet->selectedCommentCount] = strToInt(content);

			snippet->selectedCommentCount++;
		}
	}
}

int downloadChannelPlaylistCount(CURL* curlHandle, char* channelId) {
	char* buffer = (char*)getTMemory(kiloBytes(2));

	char* request = getTString(kiloBytes(1)); strClear(request);
	strAppend(request, fillString("%s?key=%s", youtubeApiPlaylist, apiKey));
	strAppend(request, fillString("&channelId=%s", channelId));
	strAppend(request, "&maxResults=1");
	strAppend(request, "&part=id");
	curlRequest(curlHandle, request, buffer);

	int count = strToInt(stringGetBetween(buffer, "\"totalResults\": ", ","));
	return count;
}

void downloadChannelPlaylists(CURL* curlHandle, YoutubePlaylist* playlists, int* playlistCount, char* channelId, char* buffer, int count) {
	char* request = getTString(kiloBytes(20)); strClear(request);

	char* pageToken = 0;
	for(int i = 0; i < count; i += maxDownloadCount) {
		int dCount = i + maxDownloadCount > count ? count-i : maxDownloadCount;

		strClear(request);
		strAppend(request, fillString("%s?key=%s", youtubeApiPlaylist, apiKey));
		strAppend(request, fillString("&channelId=%s", channelId));
		strAppend(request, fillString("&maxResults=%i", dCount));
		if(pageToken != 0) strAppend(request, fillString("&pageToken=%s", pageToken));
		strAppend(request, "&part=snippet");
		curlRequest(curlHandle, request, buffer);

		char* nextPageToken = stringGetBetween(buffer, "\"nextPageToken\": \"", "\"");
		pageToken = nextPageToken;

		int index = i;
		int advance = 0;
		for(;;) {
			char* s = stringGetBetween(buffer, "\"id\": \"", "\"", &advance); buffer += advance;
			if(strLen(s) == 0) break;

			strCpy(playlists[index].id, s);

			s = stringGetBetween(buffer, "\"title\": \"", "\"", &advance); buffer += advance;
			strCpy(playlists[index].title, s);
			index++;
		}

		// Get playlist video count.
		{
			strClear(request);
			strAppend(request, fillString("%s?key=%s", youtubeApiPlaylist, apiKey));
			strAppend(request, "&part=contentDetails");
			strAppend(request, "&id=");
			for(int index = 0; index < dCount; index++) {
				strAppend(request, fillString("%s,", playlists[i+index].id));
			}
			curlRequest(curlHandle, request, buffer);

			int index = i;
			int advance = 0;
			for(;;) {
				char* s = stringGetBetween(buffer, "\"itemCount\": ", "\n", &advance); buffer += advance;
				if(strLen(s) == 0) break;

				playlists[index].count = strToInt(s);
				index++;
			}
		}
	}

	*playlistCount = count;
}



struct AppData {
	// General.

	SystemData systemData;
	Input input;
	WindowSettings wSettings;
	GraphicsState graphicsState;

	f64 dt;
	f64 time;
	int frameCount;

	DrawCommandList commandList2d;
	DrawCommandList commandList3d;

	bool updateFrameBuffers;

	// 

	Vec2i cur3dBufferRes;
	int msaaSamples;
	Vec2i fboRes;
	bool useNativeRes;

	float aspectRatio;
	float fieldOfView;
	float nearPlane;
	float farPlane;

	// Window.

	bool resizeMode;
	bool dragMode;
	int dragOffsetX, dragOffsetY;
	int resizeOffsetX, resizeOffsetY;

	// App.

	CURL* curlHandle;
	HMODULE curlDll;

	YoutubeVideo videos[1000];
	int videoCount;
	GraphCam cam, camLikes;
	int heightMoveMode;

	int selectedVideo;
	VideoSnippet videoSnippet;

	char* urlString;
	char* urlFile;
	int urlVideoCount;

	bool startDownload;
	bool startLoadFile;
	bool startSaveFile;

	char* channelId;
	int playlistDownloadCount;

	YoutubePlaylist playlists[200];
	int playlistCount;
};



void debugMain(DebugState* ds, AppMemory* appMemory, AppData* ad, bool reload, bool* isRunning, bool init, ThreadQueue* threadQueue);

#pragma optimize( "", off )
extern "C" APPMAINFUNCTION(appMain) {

	if(init) {

		// Init memory.

		SYSTEM_INFO info;
		GetSystemInfo(&info);

		char* baseAddress = (char*)gigaBytes(8);
	    VirtualAlloc(baseAddress, gigaBytes(40), MEM_RESERVE, PAGE_READWRITE);

		ExtendibleMemoryArray* pMemory = &appMemory->extendibleMemoryArrays[appMemory->extendibleMemoryArrayCount++];
		initExtendibleMemoryArray(pMemory, megaBytes(512), info.dwAllocationGranularity, baseAddress);

		ExtendibleBucketMemory* dMemory = &appMemory->extendibleBucketMemories[appMemory->extendibleBucketMemoryCount++];
		initExtendibleBucketMemory(dMemory, megaBytes(1), megaBytes(512), info.dwAllocationGranularity, baseAddress + gigaBytes(16));

		MemoryArray* tMemory = &appMemory->memoryArrays[appMemory->memoryArrayCount++];
		initMemoryArray(tMemory, megaBytes(30), baseAddress + gigaBytes(33));



		MemoryArray* pDebugMemory = &appMemory->memoryArrays[appMemory->memoryArrayCount++];
		initMemoryArray(pDebugMemory, megaBytes(50), 0);

		MemoryArray* tMemoryDebug = &appMemory->memoryArrays[appMemory->memoryArrayCount++];
		initMemoryArray(tMemoryDebug, megaBytes(30), 0);

		ExtendibleMemoryArray* debugMemory = &appMemory->extendibleMemoryArrays[appMemory->extendibleMemoryArrayCount++];
		initExtendibleMemoryArray(debugMemory, megaBytes(512), info.dwAllocationGranularity, baseAddress + gigaBytes(34));
	}

	// Setup memory and globals.

	MemoryBlock gMemory = {};
	gMemory.pMemory = &appMemory->extendibleMemoryArrays[0];
	gMemory.tMemory = &appMemory->memoryArrays[0];
	gMemory.dMemory = &appMemory->extendibleBucketMemories[0];
	gMemory.pDebugMemory = &appMemory->memoryArrays[1];
	gMemory.tMemoryDebug = &appMemory->memoryArrays[2];
	gMemory.debugMemory = &appMemory->extendibleMemoryArrays[1];
	globalMemory = &gMemory;

	DebugState* ds = (DebugState*)getBaseMemoryArray(gMemory.pDebugMemory);
	AppData* ad = (AppData*)getBaseExtendibleMemoryArray(gMemory.pMemory);
	GraphicsState* gs = &ad->graphicsState;

	Input* input = &ad->input;
	SystemData* systemData = &ad->systemData;
	HWND windowHandle = systemData->windowHandle;
	WindowSettings* ws = &ad->wSettings;

	globalThreadQueue = threadQueue;
	globalGraphicsState = &ad->graphicsState;
	globalDebugState = ds;
	globalTimer = ds->timer;

	// Init.

	if(init) {

		// @AppInit.

		//
		// DebugState.
		//

		getPMemoryDebug(sizeof(DebugState));
		*ds = {};
		ds->assets = getPArrayDebug(Asset, 100);

		ds->inputCapacity = 600;
		ds->recordedInput = (Input*)getPMemoryDebug(sizeof(Input) * ds->inputCapacity);

		ds->timer = getPStructDebug(Timer);
		globalTimer = ds->timer;
		int timerSlots = 50000;
		ds->timer->bufferSize = timerSlots;
		ds->timer->timerBuffer = (TimerSlot*)getPMemoryDebug(sizeof(TimerSlot) * timerSlots);

		ds->savedBufferMax = 20000;
		ds->savedBufferIndex = 0;
		ds->savedBufferCount = 0;
		ds->savedBuffer = (GraphSlot*)getPMemoryDebug(sizeof(GraphSlot) * ds->savedBufferMax);


		ds->gui = getPStructDebug(Gui);
		// gui->init(rectCenDim(vec2(0,1), vec2(300,800)));
		// gui->init(rectCenDim(vec2(1300,1), vec2(300,500)));
		ds->gui->init(rectCenDim(vec2(1300,1), vec2(300, ws->currentRes.h)), 0);

		// ds->gui->cornerPos = 

		ds->gui2 = getPStructDebug(Gui);
		// ds->gui->init(rectCenDim(vec2(1300,1), vec2(400, ws->currentRes.h)), -1);
		ds->gui2->init(rectCenDim(vec2(1300,1), vec2(300, ws->currentRes.h)), 3);

		ds->input = getPStructDebug(Input);
		// ds->showMenu = false;
		ds->showMenu = false;
		ds->showStats = false;
		ds->showConsole = false;
		ds->showHud = true;
		// ds->guiAlpha = 0.95f;
		ds->guiAlpha = 1;

		for(int i = 0; i < arrayCount(ds->notificationStack); i++) {
			ds->notificationStack[i] = getPStringDebug(DEBUG_NOTE_LENGTH+1);
		}

		TIMER_BLOCK_NAMED("Init");

		//
		// AppData.
		//

		getPMemory(sizeof(AppData));
		*ad = {};
		
		initSystem(systemData, ws, windowsData, vec2i(1920*0.75f, 1080*0.75f), true, true, true, 1);
		windowHandle = systemData->windowHandle;

		loadFunctions();

		const char* extensions = wglGetExtensionsStringEXT();

		wglSwapIntervalEXT(1);
		int fps = wglGetSwapIntervalEXT();

		initInput(&ad->input);

		// Init curl.

		ad->curlDll = LoadLibraryA("libcurl.dll");
		loadCurlFunctions(ad->curlDll);

		//
		// Setup shaders and uniforms.
		//

		loadShaders();

		//
		// Setup Textures.
		//

		for(int i = 0; i < TEXTURE_SIZE; i++) {
			Texture tex;
			uchar buffer [] = {255,255,255,255 ,255,255,255,255 ,255,255,255,255, 255,255,255,255};
			loadTexture(&tex, buffer, 2,2, 1, INTERNAL_TEXTURE_FORMAT, GL_RGBA, GL_UNSIGNED_BYTE);

			addTexture(tex);
		}

		// for(int i = 0; i < TEXTURE_SIZE; i++) {
		// 	Texture tex;
		// 	loadTextureFromFile(&tex, texturePaths[i], -1, INTERNAL_TEXTURE_FORMAT, GL_RGBA, GL_UNSIGNED_BYTE);
		// 	addTexture(tex);
		// }

		//
		// Setup Meshs.
		//

		uint vao = 0;
		glCreateVertexArrays(1, &vao);
		glBindVertexArray(vao);

		for(int i = 0; i < MESH_SIZE; i++) {
			Mesh* mesh = getMesh(i);

			MeshMap* meshMap = meshArrays +i;

			glCreateBuffers(1, &mesh->bufferId);
			glNamedBufferData(mesh->bufferId, meshMap->size, meshMap->vertexArray, GL_STATIC_DRAW);
			mesh->vertCount = meshMap->size / sizeof(Vertex);
		}

		// 
		// Samplers.
		//

		gs->samplers[SAMPLER_NORMAL] = createSampler(16.0f, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
		// gs->samplers[SAMPLER_NORMAL] = createSampler(16.0f, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST);

		gs->samplers[SAMPLER_VOXEL_1] = createSampler(16.0f, GL_REPEAT, GL_REPEAT, GL_NEAREST, GL_NEAREST_MIPMAP_LINEAR);
		gs->samplers[SAMPLER_VOXEL_2] = createSampler(16.0f, GL_REPEAT, GL_REPEAT, GL_NEAREST, GL_NEAREST_MIPMAP_LINEAR);
		gs->samplers[SAMPLER_VOXEL_3] = createSampler(16.0f, GL_REPEAT, GL_REPEAT, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);

		//
		//
		//

		ad->fieldOfView = 60;
		ad->msaaSamples = 4;
		ad->fboRes = vec2i(0, 120);
		ad->useNativeRes = true;
		ad->nearPlane = 0.1f;
		ad->farPlane = 3000;
		ad->dt = 1/(float)60;

		//
		// FrameBuffers.
		//

		{
			for(int i = 0; i < FRAMEBUFFER_SIZE; i++) {
				FrameBuffer* fb = getFrameBuffer(i);
				initFrameBuffer(fb);
			}

			attachToFrameBuffer(FRAMEBUFFER_2d, FRAMEBUFFER_SLOT_COLOR, GL_RGBA8, 0, 0);

			attachToFrameBuffer(FRAMEBUFFER_DebugMsaa, FRAMEBUFFER_SLOT_COLOR, GL_RGBA8, 0, 0, ad->msaaSamples);
			attachToFrameBuffer(FRAMEBUFFER_DebugNoMsaa, FRAMEBUFFER_SLOT_COLOR, GL_RGBA8, 0, 0);

			ad->updateFrameBuffers = true;
		}

		//

		curl_global_initX(CURL_GLOBAL_ALL);
		ad->curlHandle = curl_easy_initX();
		curl_easy_setoptX(ad->curlHandle, CURLOPT_WRITEFUNCTION, curlWrite);
		// curl_easy_setoptX(curlHandle, CURLOPT_WRITEDATA, &internal_struct);

		ad->videoSnippet.thumbnailTexture.id = -1;

		ad->urlString = (char*)getPMemory(100); strClear(ad->urlString);
		strCpy(ad->urlString, moinmoinPlaylist);

		ad->urlFile = (char*)getPMemory(100); strClear(ad->urlFile);
		strCpy(ad->urlFile, tempPlaylistFile);

		ad->selectedVideo = -1;

		ad->channelId = (char*)getPMemory(100); strClear(ad->channelId);
		strCpy(ad->channelId, rocketBeansId);
	}

	// @AppStart.

	TIMER_BLOCK_BEGIN_NAMED(reload, "Reload");

	if(reload) {
		loadFunctions();
		SetWindowLongPtr(systemData->windowHandle, GWLP_WNDPROC, (LONG_PTR)mainWindowCallBack);

		if(HOTRELOAD_SHADERS) {
			loadShaders();
		}

		loadCurlFunctions(ad->curlDll);
		curl_easy_setoptX(ad->curlHandle, CURLOPT_WRITEFUNCTION, curlWrite);
	}

	TIMER_BLOCK_END(reload);

	// Update timer.
	{
		if(init) {
			ds->lastTimeStamp = timerInit();
			ds->dt = 1/(float)60;
		} else {
			ds->dt = timerUpdate(ds->lastTimeStamp, &ds->lastTimeStamp);
			ds->time += ds->dt;
		}
	}

	clearTMemory();

	// Allocate drawCommandlist.

	int clSize = kiloBytes(1000);
	drawCommandListInit(&ad->commandList3d, (char*)getTMemory(clSize), clSize);
	drawCommandListInit(&ad->commandList2d, (char*)getTMemory(clSize), clSize);
	globalCommandList = &ad->commandList3d;


	// Update input.
	{
		TIMER_BLOCK_NAMED("Input");
		updateInput(ds->input, isRunning, windowHandle);

		ad->input = *ds->input;
		if(ds->console.isActive) {
			memSet(ad->input.keysPressed, 0, sizeof(ad->input.keysPressed));
			memSet(ad->input.keysDown, 0, sizeof(ad->input.keysDown));
		}

		ad->dt = ds->dt;
		ad->time = ds->time;

		ad->frameCount++;
	}

	// Handle recording.
	{
		if(ds->recordingInput) {
			memCpy(ds->recordedInput + ds->inputIndex, &ad->input, sizeof(Input));
			ds->inputIndex++;
			if(ds->inputIndex >= ds->inputCapacity) {
				ds->recordingInput = false;
			}
		}

		if(ds->playbackInput && !ds->playbackPause) {
			ad->input = ds->recordedInput[ds->playbackIndex];
			ds->playbackIndex = (ds->playbackIndex+1)%ds->inputIndex;
			if(ds->playbackIndex == 0) ds->playbackSwapMemory = true;
		}
	} 

	if(ds->playbackPause) goto endOfMainLabel;

	if(ds->playbackBreak) {
		if(ds->playbackBreakIndex == ds->playbackIndex) {
			ds->playbackPause = true;
		}
	}

    if(input->keysPressed[KEYCODE_ESCAPE]) {
    	*isRunning = false;
    }



	if(input->mouseButtonPressed[1]) {
		POINT p;
		GetCursorPos(&p);
		ScreenToClient(windowHandle, &p);
		ad->dragOffsetX = p.x; // Border size.
		ad->dragOffsetY = p.y;

		RECT r;
		GetWindowRect(windowHandle, &r);
		ad->resizeOffsetX = (r.right - r.left) - p.x;
		ad->resizeOffsetY = (r.bottom - r.top) - p.y;

		if(input->keysDown[KEYCODE_CTRL]) {
			ad->resizeMode = true;
		} else {
			ad->dragMode = true;
		}
	} 

	if(input->mouseButtonReleased[1]) {
		ad->resizeMode = false;
		ad->dragMode = false;
	}

	bool updateWindow = false;
	int updateWindowX = 0;
	int updateWindowY = 0;
	int updateWidth = 0;
	int updateHeight = 0;

	if(ad->resizeMode) {
		POINT ps;
		GetCursorPos(&ps);
		ScreenToClient(windowHandle, &ps);

		RECT r;
		GetWindowRect(windowHandle, &r);

		int newWidth = clampMin(ps.x + ad->resizeOffsetX, 300);
		int newHeight = clampMin(ps.y + ad->resizeOffsetY, 300);

		updateWindow = true;

		updateWindowX = r.left;
		updateWindowY = r.top;
		updateWidth = newWidth;
		updateHeight = newHeight;
		
		ws->currentRes.w = newWidth;
		ws->currentRes.h = newHeight;
		ws->aspectRatio = ws->currentRes.x / (float)ws->currentRes.y;

		ad->updateFrameBuffers = true;
	}

	if(ad->dragMode) {
		POINT p;
		GetCursorPos(&p);

		RECT r;
		GetWindowRect(windowHandle, &r);

		MoveWindow(windowHandle, p.x - ad->dragOffsetX, p.y - ad->dragOffsetY, r.right-r.left, r.bottom-r.top, true);
	}

	if(!updateWindow) {
		if(windowSizeChanged(windowHandle, ws)) {
			if(!windowIsMinimized(windowHandle)) {
				updateResolution(windowHandle, ws);
				ad->updateFrameBuffers = true;
			}
		}
	}

	if(ad->updateFrameBuffers) {
		TIMER_BLOCK_NAMED("Upd FBOs");

		ad->updateFrameBuffers = false;
		ad->aspectRatio = ws->aspectRatio;
		
		ad->fboRes.x = ad->fboRes.y*ad->aspectRatio;

		if(ad->useNativeRes) ad->cur3dBufferRes = ws->currentRes;
		else ad->cur3dBufferRes = ad->fboRes;

		Vec2i s = ad->cur3dBufferRes;
		Vec2 reflectionRes = vec2(s);

		setDimForFrameBufferAttachmentsAndUpdate(FRAMEBUFFER_2d, ws->currentRes.w, ws->currentRes.h);
		setDimForFrameBufferAttachmentsAndUpdate(FRAMEBUFFER_DebugMsaa, ws->currentRes.w, ws->currentRes.h);
		setDimForFrameBufferAttachmentsAndUpdate(FRAMEBUFFER_DebugNoMsaa, ws->currentRes.w, ws->currentRes.h);
	}
	


	TIMER_BLOCK_BEGIN_NAMED(openglInit, "Opengl Init");

	// Opengl Debug settings.
	{
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

		const int count = 1;
		GLenum sources;
		GLenum types;
		GLuint ids;
		GLenum severities;
		GLsizei lengths;

		int bufSize = 1000;
		char* messageLog = getTString(bufSize);
		memSet(messageLog, 0, bufSize);

		uint fetchedLogs = 1;
		while(fetchedLogs = glGetDebugMessageLog(count, bufSize, &sources, &types, &ids, &severities, &lengths, messageLog)) {
			if(severities == GL_DEBUG_SEVERITY_NOTIFICATION) continue;

			if(severities == GL_DEBUG_SEVERITY_HIGH) printf("HIGH: \n");
			else if(severities == GL_DEBUG_SEVERITY_MEDIUM) printf("MEDIUM: \n");
			else if(severities == GL_DEBUG_SEVERITY_LOW) printf("LOW: \n");
			else if(severities == GL_DEBUG_SEVERITY_NOTIFICATION) printf("NOTE: \n");

			printf("\t%s \n", messageLog);
		}
	}

	// Clear all the framebuffers and window backbuffer.
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(0,0,0,0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		bindFrameBuffer(FRAMEBUFFER_2d);
		glClearColor(0,0,0,0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		bindFrameBuffer(FRAMEBUFFER_DebugMsaa);
		glClearColor(0,0,0,0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		bindFrameBuffer(FRAMEBUFFER_DebugNoMsaa);
		glClearColor(0,0,0,0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

	// Setup opengl.
	{
		// glEnable(GL_DITHER);

		// glDepthRange(-1.0,1.0);
		glFrontFace(GL_CW);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glDisable(GL_SCISSOR_TEST);
		// glEnable(GL_LINE_SMOOTH);
		// glEnable(GL_POLYGON_SMOOTH);
		// glDisable(GL_POLYGON_SMOOTH);
		// glDisable(GL_SMOOTH);
		
		glEnable(GL_TEXTURE_2D);
		// glEnable(GL_ALPHA_TEST);
		// glAlphaFunc(GL_GREATER, 0.9);
		// glDisable(GL_LIGHTING);
		// glDepthFunc(GL_LESS);
		// glClearDepth(1);
		// glDepthMask(GL_TRUE);
		glEnable(GL_MULTISAMPLE);
		// glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquation(GL_FUNC_ADD);

		// glViewport(0,0, ad->cur3dBufferRes.x, ad->cur3dBufferRes.y);
	}

	TIMER_BLOCK_END(openglInit);




	// @AppLoop.

	if(ad->startDownload) { 
	// if(init) { 
		ad->startDownload = false;

		char* playlist = ad->urlString;
		char* playlistFile = ad->urlFile;
		int count = ad->urlVideoCount;

		// maxDownloadCount = 40;
		// playlist = "ULhcJGOokZyFk";
		// playlistFile = "temp.playlist";
		// count = 50;

		YoutubeVideo* vids = getTArray(YoutubeVideo, count);

		char* pageToken = 0;
		char* buffer = (char*)getTMemory(megaBytes(3)); 
		for(int i = 0; i < count; i += maxDownloadCount) {

			int dCount = maxDownloadCount;
			if(i + dCount > count) {
				dCount = count - i;
			}

			pageToken = downloadYoutubePlaylistVideoIds(ad->curlHandle, playlist, buffer, dCount, pageToken);

			// Get Video ids.
			{
				int index = i;
				int advance = 0;
				for(;;) {
					int reverseIndex = count-1 - index;

					char* s = stringGetBetween(buffer, "\"videoId\": \"", "\"", &advance);
					if(strLen(s) == 0) break;

					strCpy(vids[reverseIndex].id, s);
					buffer += advance;

					s = stringGetBetween(buffer, "\"videoPublishedAt\": \"", "\"", &advance);

					strCpy(vids[reverseIndex].dateString, s);
					buffer += advance;
					index++;
				}
			}
			
			// Get Statistics.
			{
				char* request = getTString(kiloBytes(20)); strClear(request);
				strAppend(request, fillString("%s?key=%s", youtubeApiVideos, apiKey));
				strAppend(request, "&part=statistics");
				strAppend(request, "&id=");

				for(int videoIndex = i; videoIndex < i+dCount; videoIndex++) {
					int reverseIndex = count-1 - videoIndex;
					strAppend(request, fillString("%s,", vids[reverseIndex].id));
				}

				curlRequest(ad->curlHandle, request, buffer);

				int index = i;
				int advance = 0;
				for(;;) {
					int reverseIndex = count-1 - index;

					char* s = stringGetBetween(buffer, "\"viewCount\": \"", "\"", &advance); buffer += advance;
					if(strLen(s) == 0) break;

					vids[reverseIndex].viewCount = strToInt(s);
					s = stringGetBetween(buffer, "\"likeCount\": \"", "\"", &advance); buffer += advance;
					vids[reverseIndex].likeCount = strToInt(s);
					s = stringGetBetween(buffer, "\"dislikeCount\": \"", "\"", &advance); buffer += advance;
					vids[reverseIndex].dislikeCount = strToInt(s);
					s = stringGetBetween(buffer, "\"favoriteCount\": \"", "\"", &advance); buffer += advance;
					vids[reverseIndex].favoriteCount = strToInt(s);
					s = stringGetBetween(buffer, "\"commentCount\": \"", "\"", &advance); buffer += advance;
					vids[reverseIndex].commentCount = strToInt(s);
					index++;	
				}
			}

			// Get title.
			{
				char* request = getTString(kiloBytes(20)); strClear(request);
				strAppend(request, fillString("%s?key=%s", youtubeApiVideos, apiKey));
				strAppend(request, "&part=snippet");
				strAppend(request, "&id=");

				for(int videoIndex = i; videoIndex < i+dCount; videoIndex++) {
					int reverseIndex = count-1 - videoIndex;
					strAppend(request, fillString("%s,", vids[reverseIndex].id));
				}

				curlRequest(ad->curlHandle, request, buffer);

				int index = i;
				int advance = 0;
				for(;;) {
					int reverseIndex = count-1 - index;

					char* s = stringGetBetween(buffer, "\"title\": \"", "\"", &advance); buffer += advance;
					if(strLen(s) == 0) break;

					// Title
					strCpy(vids[reverseIndex].title, s);

					// Thumbnail.
					// default, medium, high, standard, maxres.
					int pos = strFind(buffer, "\"medium\":"); buffer += pos;
					s = stringGetBetween(buffer, "\"url\": \"", "\",", &advance); buffer += advance;
					strCpy(vids[reverseIndex].thumbnail, s);

					// Skip the second "title" from localization data.
					char* throwAway = stringGetBetween(buffer, "\"title\": \"", "\"", &advance); buffer += advance;

					index++;
				}


			}
			
		}

		for(int i = 0; i < count; i++) {
			vids[i].date = stringToDate(vids[i].dateString);
		}

		// Save to file.
		FILE* file = fopen(playlistFile, "wb");
		fwrite(&count, sizeof(int), 1, file);
		fwrite(vids, count*sizeof(YoutubeVideo), 1, file);
		fclose(file);
	}

	if(ad->startSaveFile) {
		// Save to file.
		FILE* file = fopen(ad->urlFile, "wb");
		fwrite(&ad->videoCount, sizeof(int), 1, file);
		fwrite(ad->videos, ad->videoCount*sizeof(YoutubeVideo), 1, file);
		fclose(file);
	}

	if(init || ad->startLoadFile) {
	// if(ad->startLoadFile) {
		ad->startLoadFile = false;
		char* filePath = init?tempPlaylistFile:ad->urlFile;

		FILE* file = fopen(filePath, "rb");
		if(file) {
			fread(&ad->videoCount, sizeof(int), 1, file);
			fread(&ad->videos, ad->videoCount*sizeof(YoutubeVideo), 1, file);
			fclose(file);

			if(ad->videoCount == 0) {
				graphCamInit(&ad->cam,      0, 10000, 0, 1000);
				graphCamInit(&ad->camLikes, 0, 10000, 0, 1000);
			} else {

				// Sort videos.
				// if(false)
				{
					int size = ad->videoCount;
					for(int off = 0; off < size-1; off++) {
						bool sw = false;

						for(int i = 0; i < size-1 - off; i++) {
							if(ad->videos[i+1].date.num < ad->videos[i].date.num) {
								// swap(&off, &size);
								swapGeneric(YoutubeVideo, ad->videos[i+1], ad->videos[i]);
								sw = true;
							}
						}

						if(!sw) break;
					}
				}

				Statistic statViews, statLikes, statDates;
				{
					beginStatistic(&statViews);
					beginStatistic(&statLikes);
					beginStatistic(&statDates);

					for(int i = 0; i < ad->videoCount; i++) {
						YoutubeVideo* video = ad->videos + i;

						updateStatistic(&statViews, video->viewCount);
						updateStatistic(&statLikes, video->likeCount);
						updateStatistic(&statDates, video->date.num);
					}
				}

				if(statDates.min == statDates.max) {
					graphCamInit(&ad->cam,      statDates.min - 10000, statDates.min + 10000, 0, statViews.max*1.5f);
					graphCamInit(&ad->camLikes, statDates.min - 10000, statDates.min + 10000, 0, statLikes.max*1.5f);
				} else {
					graphCamInit(&ad->cam,      statDates.min, statDates.max, 0, statViews.max*1.5f);
					graphCamInit(&ad->camLikes, statDates.min, statDates.max, 0, statLikes.max*1.5f);
				}
			}

			ad->selectedVideo = -1;

		} 
	}

	{
		globalCommandList = &ad->commandList2d;

		int fontSize = 20;
		Font* font = getFont(FONT_CALIBRI, fontSize);
		Vec2i res = ws->currentRes;
		float zoomSpeed = 1.2f;


		Rect screenRect = rect(0, -res.h, res.w, 0);
		float sidePanelWidth = rectGetW(screenRect)*0.25f;

		// Rect rChart = screenRect;
		Rect rChart = ad->selectedVideo!=-1 ? rect(screenRect.min, screenRect.max - vec2(sidePanelWidth,0)) : screenRect;
		Rect rGraph = rChart;
		rGraph.min += vec2(100,font->height*2);
		Rect rLeftText = rect(rChart.left,rGraph.bottom,rGraph.left,rChart.top);
		Rect rBottomText = rect(rGraph.left,rChart.bottom,rChart.right,rGraph.bottom);
		Rect infoRect = rect(screenRect.min + vec2(rectGetW(screenRect)-sidePanelWidth, 0), screenRect.max);

		float graphHeight = rectGetH(rGraph);
		rGraph.min.y += graphHeight/2;
		rLeftText.min.y += graphHeight/2;

		Rect rGraph2 = rect(rectGetUL(rBottomText), rectGetDR(rGraph));
		Rect rLeftText2 = rect(rChart.left, rBottomText.top, rGraph2.left, rLeftText.bottom);

		float g = 0.15f;
		dcRect(screenRect, vec4(0.4f,g,g,1));
		Vec4 graphColor2 = vec4(0.1f, 0.1f, 0.2f, 1);
		Vec4 graphColor1 = vec4(0.2f, 0.1f, 0.4f, 1);
		dcPrimitive2d(rGraph.min, rectGetUL(rGraph), rGraph.max, rectGetDR(rGraph), 
		              graphColor1, graphColor2, graphColor2, graphColor1);

		dcPrimitive2d(rGraph2.min, rectGetUL(rGraph2), rGraph2.max, rectGetDR(rGraph2), 
		              graphColor1, graphColor2, graphColor2, graphColor1);

		// dcRect(rGraph2, vec4(0,1,0,1));
		// dcRect(rLeftText2, vec4(0,1,1,1));

		{
			GraphCam* cam = &ad->cam;
			GraphCam* camLikes = &ad->camLikes;

			graphCamSetViewPort(cam, rGraph);
			graphCamSetViewPort(camLikes, rGraph2);

			if(input->mouseWheel != 0) {
				if(input->keysDown[KEYCODE_SHIFT]) {
					if(pointInRect(input->mousePosNegative, cam->viewPort))
						graphCamScaleToPos(cam, 0, zoomSpeed, -input->mouseWheel, zoomSpeed, input->mousePosNegative);
					if(pointInRect(input->mousePosNegative, camLikes->viewPort))
						graphCamScaleToPos(camLikes, 0, zoomSpeed, -input->mouseWheel, zoomSpeed, input->mousePosNegative);
					
				} else {
					graphCamScaleToPos(cam, -input->mouseWheel, zoomSpeed, 0, zoomSpeed, input->mousePosNegative);
					graphCamScaleToPos(camLikes, -input->mouseWheel, zoomSpeed, 0, zoomSpeed, input->mousePosNegative);
				}
			}

			graphCamSizeClamp(cam, 60*60*4, 1000);
			graphCamSizeClamp(camLikes, 60*60*4, 100);

			if(input->mouseButtonPressed[0]) {
				if(pointInRect(input->mousePosNegative, cam->viewPort)) ad->heightMoveMode = 0;
				else if(pointInRect(input->mousePosNegative, camLikes->viewPort)) ad->heightMoveMode = 1;
			}

			if(input->mouseButtonReleased[0]) {
				ad->heightMoveMode = -1;
			}

			if(input->mouseDelta != vec2(0,0)) {
				if(ad->heightMoveMode == 0) {
					graphCamTrans(cam, 0, input->mouseDelta.y);
				} else if(ad->heightMoveMode == 1) {
					graphCamTrans(camLikes, 0, input->mouseDelta.y);
				}

				if(input->mouseButtonDown[0]) {
					graphCamTrans(cam, -input->mouseDelta.x, 0);
					graphCamTrans(camLikes, -input->mouseDelta.x, 0);
				}
			}

			graphCamPosClamp(cam);
			graphCamPosClamp(camLikes);
		}

		GraphCam* cam = &ad->cam;

		// Draw left text.
		{
			Vec4 mainColor = vec4(0.9f,1);
			Vec4 semiColor = vec4(0.6f,1);
			Vec4 horiLinesColor = vec4(1,0.03f);
			float markLength = 10;
			float fontMargin = 4;
			float div = 10;
			int subDiv = 4;
			float splitSizePixels = font->height*3;

			dcState(STATE_LINEWIDTH, 2);
			dcEnable(STATE_SCISSOR);

			{
				GraphCam* cam = &ad->cam;
				Rect scaleRect = rLeftText;
				float splitSize = splitSizePixels * (cam->h / rectGetH(cam->viewPort));
				float stepSize = pow(div, roundUpFloat(logBase(splitSize, div)));
				// float subSplitSize = font->height * (cam->h / rectGetH(cam->viewPort));
				int i = 0;
				float p = roundModDown(cam->bottom, stepSize);
				while(p < cam->top + stepSize) {
					float y = graphCamMapY(cam, p);

					// Horizontal line.
					dcScissor(scissorRectScreenSpace(cam->viewPort, res.h));
					dcLine2d(vec2(scaleRect.right, y), vec2(cam->viewPort.right, y), horiLinesColor); 
					dcScissor(scissorRectScreenSpace(scaleRect, res.h));

					// Base markers.
					dcLine2dOff(vec2(scaleRect.right, y), vec2(-markLength,0), mainColor); 
					dcText(fillString("%i.",(int)p), font, vec2(scaleRect.right - markLength - fontMargin, y), mainColor, vec2i(1,0));

					// Semi markers.
					for(int i = 1; i < subDiv; i++) {
						float y = graphCamMapY(cam, p+i*(stepSize/subDiv));
						dcLine2dOff(vec2(scaleRect.right, y), vec2(-markLength*0.5f,0), semiColor); 
						dcText(fillString("%i.",(int)(stepSize/subDiv)), font, vec2(scaleRect.right - markLength*0.5f - fontMargin, y), semiColor, vec2i(1,0));
					}

					p += stepSize;
				}
			}

			{
				GraphCam* cam = &ad->camLikes;
				Rect scaleRect = rLeftText2;
				float splitSize = splitSizePixels * (cam->h / rectGetH(cam->viewPort));
				float stepSize = pow(div, roundUpFloat(logBase(splitSize, div)));
				// float subSplitSize = font->height * (cam->h / rectGetH(cam->viewPort));
				int i = 0;
				float p = roundModDown(cam->bottom, stepSize);
				while(p < cam->top + stepSize) {
					float y = graphCamMapY(cam, p);

					// Horizontal line.
					dcScissor(scissorRectScreenSpace(cam->viewPort, res.h));
					dcLine2d(vec2(scaleRect.right, y), vec2(cam->viewPort.right, y), horiLinesColor); 
					dcScissor(scissorRectScreenSpace(scaleRect, res.h));

					// Base markers.
					dcLine2dOff(vec2(scaleRect.right, y), vec2(-markLength,0), mainColor); 
					dcText(fillString("%i.",(int)p), font, vec2(scaleRect.right - markLength - fontMargin, y), mainColor, vec2i(1,0));

					// Semi markers.
					for(int i = 1; i < subDiv; i++) {
						float y = graphCamMapY(cam, p+i*(stepSize/subDiv));
						dcLine2dOff(vec2(scaleRect.right, y), vec2(-markLength*0.5f,0), semiColor); 
						dcText(fillString("%i.",(int)(stepSize/subDiv)), font, vec2(scaleRect.right - markLength*0.5f - fontMargin, y), semiColor, vec2i(1,0));
					}

					p += stepSize;
				}
			}

			dcDisable(STATE_SCISSOR);
		}

		// Draw bottom text.
		{
			Vec4 mainColor = vec4(0.9f,1);
			Vec4 semiColor = vec4(0.6f,1);
			Vec4 horiLinesColor = vec4(1,1,1,0.03f);
			float markLength = font->height-3;
			float fontMargin = 0;
			float div = 10;
			float splitSizePixels = 200;

			dcState(STATE_LINEWIDTH, 2);
			dcEnable(STATE_SCISSOR);

			Rect scaleRect = rBottomText;
			double splitSize = splitSizePixels * (cam->w / rectGetW(cam->viewPort));
			double stepSize = pow(div, roundUpFloat(logBase(splitSize, div)));
			// double subSplitSize = font->height * (cam->h / rectGetH(cam->viewPort));
			int i = 0;
			double p = roundModDown(cam->left, stepSize);
			while(p < cam->right + stepSize) {
				dcScissor(scissorRectScreenSpace(rect(rGraph2.min, rGraph.max), res.h));

				// Vertical lines.
				{
					double x = graphCamMapX(cam, p);
					dcLine2d(vec2(x, scaleRect.bottom),vec2(x, rGraph.top), horiLinesColor); 
				}

				dcScissor(scissorRectScreenSpace(scaleRect, res.h));

				// Base markers.
				{
					double x = graphCamMapX(cam, p);
					dcLine2dOff(vec2(x, scaleRect.top), vec2(0,-markLength), mainColor); 

					Date d;
					intToDate(p, &d);
					char* dateString = fillString("%i..%i..%i", d.day, d.month, d.year);
					dcText(dateString, font, vec2(x, scaleRect.top - markLength - fontMargin), mainColor, vec2i(0,1));
				}

				// Semi markers.
				{
					// int d = pow(2, roundFloat(logBase(stepSize/subSplitSize, 2)));
					int d = 4;
					for(int i = 0; i < d; i++) {
						double x = graphCamMapX(cam, p+i*(stepSize/d) + (stepSize/d)/2);

						Date date;
						intToDate(stepSize/d, &date);
						char* dateString;
						if(date.day != 0) dateString = fillString("%id", date.day);
						else if(date.hours != 0) dateString = fillString("%ih", date.hours);
						else if(date.minutes != 0) dateString = fillString("%imin", date.minutes);
						else if(date.seconds != 0) dateString = fillString("%isec", date.seconds);
						else dateString = fillString("0");

						dcText(dateString, font, vec2(x, scaleRect.top), semiColor, vec2i(0,1));

						if(i == 0) continue;
						{
							double x = graphCamMapX(cam, p+i*(stepSize/d));
							dcLine2dOff(vec2(x, scaleRect.top), vec2(0, -markLength*0.5f), semiColor); 
						}
					}
				}

				p += stepSize;
			}

			dcDisable(STATE_SCISSOR);
		}

		YoutubeVideo* vids = ad->videos;
		int vidCount = ad->videoCount;

		// Mouse hover.
		{
			GraphCam* cam = &ad->cam;
			for(int i = 0; i < vidCount; i++) {
				bool lastElement = i < vidCount-1;

				if(lastElement)
					if(vids[i+1].date.num < cam->left || vids[i].date.num > cam->right) continue;

				float x = graphCamMapX(cam, vids[i].date.num) + 1;
				float x2 = lastElement?graphCamMapX(cam, vids[i+1].date.num) + 1 : x + 1000000;

				if(input->mousePos.x < x + (x2-x)/2) {
					if(input->mousePos.x >= ad->camLikes.viewPort.left)
						dcLine2d(vec2(x, ad->camLikes.viewPort.bottom), vec2(x,ad->cam.viewPort.top), vec4(1,0,0,0.2f));

					Font* font = getFont(FONT_CALIBRI, 20);
					Vec4 c = vec4(1.0f,0.5f,0,1);
					Vec2 p = rChart.max - vec2(5);
					dcText(fillString("%s", ad->videos[i].title), font, p, c, vec2i(1,1)); p.y -= font->height;
					dcText(fillString("%s", ad->videos[i].dateString), font, p, c, vec2i(1,1)); p.y -= font->height;
					dcText(fillString("%i", i), font, p, c, vec2i(1,1)); p.y -= font->height;

					if(input->mouseButtonPressed[2]) {
						if(i != ad->selectedVideo) {
							ad->selectedVideo = i;
							downloadVideoSnippet(ad->curlHandle, &ad->videoSnippet, ad->videos + ad->selectedVideo);
						}
					}

					break;
				}
			}
		}

		dcState(STATE_LINEWIDTH, 2);
		dcEnable(STATE_SCISSOR);

		for(int graphIndex = 0; graphIndex < 2; graphIndex++) {
			GraphCam* cam;
			if(graphIndex == 0) cam = &ad->cam;
			else cam = &ad->camLikes;

			dcScissor(scissorRectScreenSpace(cam->viewPort, res.h));

			for(int i = 0; i < vidCount-1; i++) {
				if(vids[i+1].date.num < cam->left || vids[i].date.num > cam->right) continue;

				float x = graphCamMapX(cam, vids[i].date.num) + 1;
				float x2 = graphCamMapX(cam, vids[i+1].date.num) + 1;

				if(graphIndex == 0) {
					dcLine2d(vec2(x, graphCamMapY(cam, vids[i].viewCount)), vec2(x2, graphCamMapY(cam, vids[i+1].viewCount)), vec4(0,0.7f,1,1));
				} else if(graphIndex == 1) {
					dcLine2d(vec2(x, graphCamMapY(cam, vids[i].likeCount)), vec2(x2, graphCamMapY(cam, vids[i+1].likeCount)), vec4(0.0f,0.7f,1,1));
					dcLine2d(vec2(x, graphCamMapY(cam, vids[i].dislikeCount)), vec2(x2, graphCamMapY(cam, vids[i+1].dislikeCount)), vec4(0.7f,0.3f,1,1));

					double div1 = (double)vids[i].likeCount/(vids[i].likeCount + vids[i].dislikeCount);
					double div2 = (double)vids[i+1].likeCount/(vids[i+1].likeCount + vids[i+1].dislikeCount);
					dcLine2d(vec2(x, mapRangeDouble(div1,0,1,cam->viewPort.bottom,cam->viewPort.top)), vec2(x2, mapRangeDouble(div2,0,1,cam->viewPort.bottom,cam->viewPort.top)), vec4(0.9f,0.5f,1,1));
				}

				// dcLine2d(vec2(x, graphCamMapY(cam, vids[i].commentCount)), vec2(x2, graphCamMapY(cam, vids[i+1].commentCount)), vec4(1,0.3f,0.8f,1));
				// dcLine2d(vec2(x, graphCamMapY(cam, vids[i].favoriteCount)), vec2(x2, graphCamMapY(cam, vids[i+1].favoriteCount)), vec4(1,1,1,1));
				// dcLine2d(vec2(x, graphCamMapY(cam, vids[i].commentCount)), vec2(x2, graphCamMapY(cam, vids[i+1].commentCount)), vec4(1,0.3f,0.8f,1));
			}
		}

		dcDisable(STATE_SCISSOR);

		Font* titleFont = getFont(FONT_CALIBRI, 30);
		// Vec2 tp0 = vec2(rectGetCen(rGraph).x, rGraph.top - 10);
		// Vec2 tp1 = vec2(rectGetCen(rGraph2).x, rGraph2.top - 10);

		Vec2 tp0 = rectGetUL(rGraph) + vec2(20,-10);
		Vec2 tp1 = rectGetUL(rGraph2) + vec2(20,-10);;

		dcText("Views", titleFont, tp0, vec4(0.9f,0.9f,0.9f,1), vec2i(-1,1), 0, 2, vec4(0,0,0,1));
		dcText("Likes", titleFont, tp1, vec4(0.9f,0.9f,0.9f,1), vec2i(-1,1), 0, 2, vec4(0,0,0,1));

		if(ad->selectedVideo != -1) {
			// Graph selection line.
			{
				float x = graphCamMapX(cam, ad->videos[ad->selectedVideo].date.num) + 1;
				dcLine2d(vec2(x, ad->camLikes.viewPort.bottom), vec2(x,ad->cam.viewPort.top), vec4(1,0,0,0.1f));
			}

			// infoRect = rectExpand(infoRect, -vec2(20,20));
			dcRect(infoRect, vec4(0,0.9f));

			float border = 20;
			infoRect = rectExpand(infoRect, -vec2(border,border));
			Font* font = getFont(FONT_CALIBRI, 20);
			Font* font2 = getFont(FONT_SOURCESANS_PRO, 20);
			float xMid = rectGetCen(infoRect).x;
			float width = rectGetW(infoRect);
			float yPos = infoRect.top;

			dcEnable(STATE_SCISSOR);
			dcScissor(scissorRectScreenSpace(infoRect, res.h));

			YoutubeVideo* sv = ad->videos + ad->selectedVideo;
			VideoSnippet* sn = &ad->videoSnippet;

			dcText(fillString("Index: %i, VideoId: %s", ad->selectedVideo, sv->id), font, vec2(xMid, yPos), vec4(1,1), vec2i(0,1));
			yPos -= font->height;

			// dcRect(rectCenDim(xMid, yPos - sn->thumbnailHeight/2, sn->thumbnailWidth, sn->thumbnailHeight), rect(0,0,1,1), vec4(1,1), sn->thumbnailTexture.id);
			Rect imageRect = rect(infoRect.left, yPos - rectGetW(infoRect)*(sn->thumbnailTexture.dim.h/(float)sn->thumbnailTexture.dim.w), infoRect.right, yPos);
			dcRect(imageRect, rect(0,0,1,1), vec4(1,1), sn->thumbnailTexture.id);
			yPos -= rectGetH(imageRect);

			dcText(sv->title, font, vec2(xMid, yPos), vec4(1,1), vec2i(0,1), width);
			yPos -= getTextHeight(sv->title, font, vec2(xMid, yPos), width);
			yPos -= font->height;

			char* date = fillString("Date: %i..%i..%i", sv->date.day, sv->date.month, sv->date.year);
			char* time = fillString("Time: %i:%i:%i.", sv->date.hours, sv->date.minutes, sv->date.seconds);
			char* views = fillString("Views: %i.", sv->viewCount);
			char* likes = fillString("Likes/Dislikes: %i | %i -> %f", sv->likeCount, sv->dislikeCount, sv->likeCount / (float)(sv->likeCount+sv->dislikeCount));
			char* comments = fillString("Comments: %i", sv->commentCount);
			char* favorites = fillString("Favorites: %i", sv->favoriteCount);
			char* s = fillString("%s, %s\n%s\n%s\n%s\n%s", date, time, views, likes, comments, favorites);
			dcText(s, font, vec2(infoRect.left, yPos), vec4(1,1), vec2i(-1,1));
			yPos -= getTextHeight(s, font);
			yPos -= font->height;

			dcText("Comments", font, vec2(xMid, yPos), vec4(1,1), vec2i(0,1));
			yPos -= font->height;

			for(int i = 0; i < sn->selectedCommentCount; i++) {
				dcText(sn->selectedTopComments[i], font2, vec2(infoRect.left, yPos), vec4(1,1), vec2i(-1,1), width);
				// drawText(sn->selectedTopComments[i], font2, vec2(infoRect.left, yPos), vec4(1,1), vec2i(-1,1), width);
				yPos -= getTextHeight(sn->selectedTopComments[i], font2, vec2(xMid, yPos), width);
				dcText(fillString("Likes: %i, Replies: %i", sn->selectedCommentLikeCount[i], sn->selectedCommentReplyCount[i]), font2, vec2(infoRect.right, yPos), vec4(0.7f,1), vec2i(1,1), width);
				yPos -= font2->height;
				// yPos -= font2->height*0.5f;
			}

			dcDisable(STATE_SCISSOR);
		}

		// Border.
		float w = 0.8f;
		Vec4 borderColor = vec4(w,w,w,1);
		dcLine2d(vec2(screenRect.left+1, screenRect.bottom), vec2(screenRect.left+1, screenRect.top), borderColor);
		dcLine2d(vec2(screenRect.left, screenRect.top), vec2(screenRect.right, screenRect.top), borderColor);
		dcLine2d(vec2(screenRect.right, screenRect.top), vec2(screenRect.right, screenRect.bottom), borderColor);
		dcLine2d(vec2(screenRect.right, screenRect.bottom+1), vec2(screenRect.left, screenRect.bottom+1), borderColor);
	}


	endOfMainLabel:

	debugMain(ds, appMemory, ad, reload, isRunning, init, threadQueue);

	// Render.
	{
		TIMER_BLOCK_NAMED("Render");

		bindShader(SHADER_QUAD);
		glDisable(GL_DEPTH_TEST);

		ortho(rect(0, -ws->currentRes.h, ws->currentRes.w, 0));
		glViewport(0,0, ws->currentRes.x, ws->currentRes.y);

		bindFrameBuffer(FRAMEBUFFER_2d);

		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
		glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);

		bindFrameBuffer(FRAMEBUFFER_DebugMsaa);
		executeCommandList(&ad->commandList2d);

		double timeStamp = timerInit();
			executeCommandList(&ds->commandListDebug, false, reload);
		static double tempTime = 0;
		tempTime += ds->dt;
		if(tempTime >= 1) {
			ds->debugRenderTime = timerUpdate(timeStamp);
			tempTime = 0;
		}

		blitFrameBuffers(FRAMEBUFFER_DebugMsaa, FRAMEBUFFER_DebugNoMsaa, ws->currentRes, GL_COLOR_BUFFER_BIT, GL_LINEAR);


		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquation(GL_FUNC_ADD);

		bindFrameBuffer(FRAMEBUFFER_2d);
		drawRect(rect(0, -ws->currentRes.h, ws->currentRes.w, 0), vec4(1,1,1,ds->guiAlpha), rect(0,1,1,0), 
		         getFrameBuffer(FRAMEBUFFER_DebugNoMsaa)->colorSlot[0]->id);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquation(GL_FUNC_ADD);


		#if USE_SRGB 
			glEnable(GL_FRAMEBUFFER_SRGB);
		#endif 

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		drawRect(rect(0, -ws->currentRes.h, ws->currentRes.w, 0), vec4(1,1), rect(0,1,1,0), 
		         getFrameBuffer(FRAMEBUFFER_2d)->colorSlot[0]->id);

		#if USE_SRGB
			glDisable(GL_FRAMEBUFFER_SRGB);
		#endif
	}

	// Swap window background buffer.
	{
		TIMER_BLOCK_NAMED("Swap");
		if(updateWindow) MoveWindow(windowHandle, updateWindowX, updateWindowY, updateWidth, updateHeight, true);
		swapBuffers(&ad->systemData);
		glFinish();

		if(init) {
			showWindow(windowHandle);
			GLenum glError = glGetError(); printf("GLError: %i\n", glError);
		}
	}

	// debugMain(ds, appMemory, ad, reload, isRunning, init, threadQueue);

	// debugUpdatePlayback(ds, appMemory);

	// @AppEnd.
}



void debugMain(DebugState* ds, AppMemory* appMemory, AppData* ad, bool reload, bool* isRunning, bool init, ThreadQueue* threadQueue) {
	// @DebugStart.

	#define PVEC3(v) v.x, v.y, v.z
	#define PVEC2(v) v.x, v.y

	globalMemory->debugMode = true;

	i64 timeStamp = timerInit();

	Input* input = ds->input;
	WindowSettings* ws = &ad->wSettings;

	clearTMemoryDebug();

	ExtendibleMemoryArray* debugMemory = &appMemory->extendibleMemoryArrays[1];
	ExtendibleMemoryArray* pMemory = globalMemory->pMemory;

	int clSize = megaBytes(2);
	drawCommandListInit(&ds->commandListDebug, (char*)getTMemoryDebug(clSize), clSize);
	globalCommandList = &ds->commandListDebug;


	ds->gInput = { input->mousePos, input->mouseWheel, input->mouseButtonPressed[0], input->mouseButtonDown[0], 
					input->keysPressed[KEYCODE_ESCAPE], input->keysPressed[KEYCODE_RETURN], input->keysPressed[KEYCODE_SPACE], input->keysPressed[KEYCODE_BACKSPACE], input->keysPressed[KEYCODE_DEL], input->keysPressed[KEYCODE_HOME], input->keysPressed[KEYCODE_END], 
					input->keysPressed[KEYCODE_LEFT], input->keysPressed[KEYCODE_RIGHT], input->keysPressed[KEYCODE_UP], input->keysPressed[KEYCODE_DOWN], 
					input->keysDown[KEYCODE_SHIFT], input->keysDown[KEYCODE_CTRL], 
					input->keysPressed[KEYCODE_X], input->keysPressed[KEYCODE_C], input->keysPressed[KEYCODE_V], 
					input->inputCharacters, input->inputCharacterCount};

	if(input->keysPressed[KEYCODE_F6]) ds->showMenu = !ds->showMenu;
	if(input->keysPressed[KEYCODE_F7]) ds->showStats = !ds->showStats;
	if(input->keysPressed[KEYCODE_F8]) ds->showHud = !ds->showHud;

	// Recording update.
	{
		if(ds->playbackSwapMemory) {
			threadQueueComplete(threadQueue);
			ds->playbackSwapMemory = false;

			pMemory->index = ds->snapShotCount-1;
			pMemory->arrays[pMemory->index].index = ds->snapShotMemoryIndex;

			for(int i = 0; i < ds->snapShotCount; i++) {
				memCpy(pMemory->arrays[i].data, ds->snapShotMemory[i], pMemory->slotSize);
			}
		}
	}

	if(ds->showMenu) {
		int fontSize = 20;

		bool initSections = false;

		Gui* gui = ds->gui;
		gui->start(ds->gInput, getFont(FONT_CALIBRI, fontSize), ws->currentRes);

		static int sectionMode = 0;

		gui->label("App", 1, gui->colors.sectionColor, vec4(0,0,0,1));

		gui->div(0,0,0);
		if(gui->button("Options", (int)(sectionMode == 0) + 1)) sectionMode = 0;
		if(gui->button("Videos", (int)(sectionMode == 1) + 1)) sectionMode = 1;
		if(gui->button("Playlists", (int)(sectionMode == 2) + 1)) sectionMode = 2;

		if(sectionMode == 0) {
			gui->div(0.1f,0,0,0.1f);
			if(gui->button("<-")) {
				int newSelection = clampMin(ad->selectedVideo - 1, 0);
				if(newSelection != ad->selectedVideo) {
					ad->selectedVideo = newSelection;
					downloadVideoSnippet(ad->curlHandle, &ad->videoSnippet, ad->videos + ad->selectedVideo);
				}
			};			

			if(gui->button("Open in Chrome.")) {
				shellExecuteNoWindow(fillString("chrome https://www.youtube.com/watch?v=%s", ad->videos[ad->selectedVideo].id), false);
			}
			if(gui->button("Open in Firefox.")) {
				shellExecuteNoWindow(fillString("C:\\Program Files (x86)\\Mozilla Firefox\\firefox.exe https://www.youtube.com/watch?v=%s", ad->videos[ad->selectedVideo].id), false);
			}

			if(gui->button("->")) {
				int newSelection = clampMax(ad->selectedVideo + 1, ad->videoCount - 1);
				if(newSelection != ad->selectedVideo) {
					ad->selectedVideo = newSelection;
					downloadVideoSnippet(ad->curlHandle, &ad->videoSnippet, ad->videos + ad->selectedVideo);
				}
			};

			gui->div(vec2(0.2f,0));
			gui->label("Url: ", 0);
			gui->textBoxChar(ad->urlString, strLen(ad->urlString), 100);

			gui->div(vec2(0.2f,0));
			gui->label("File: ", 0);
			gui->textBoxChar(ad->urlFile, strLen(ad->urlFile), 100);

			gui->div(0.2f,0.2f,0);
			gui->label("Count: ", 0);
			gui->textBoxInt(&ad->urlVideoCount);
			if(gui->button("Get video count from url.")) {
				int count = getYoutubePlaylistSize(ad->curlHandle, ad->urlString);
				ad->urlVideoCount = count;
			}

			gui->div(0,0,0);
			if(gui->button("Download and Save.")) ad->startDownload = true;
			if(gui->button("Save to file.")) ad->startSaveFile = true;
			if(gui->button("Load from file.")) ad->startLoadFile = true;

			gui->empty();
			{
				gui->div(vec2(0.2f,0));
				gui->label("Channel Id:", 0);
				gui->textBoxChar(ad->channelId);

				gui->div(0.2f,0.2f,0);
				gui->label("Count: ", 0);
				gui->textBoxInt(&ad->playlistDownloadCount);
				if(gui->button("Get playlist count.")) {
					ad->playlistDownloadCount = downloadChannelPlaylistCount(ad->curlHandle, ad->channelId);;
				}
				
				if(gui->button("Load Playlist from Channel")) {
					char* buffer = (char*)getTMemory(megaBytes(3));
					downloadChannelPlaylists(ad->curlHandle, ad->playlists, &ad->playlistCount, ad->channelId, buffer, ad->playlistDownloadCount);
				}

				if(ad->playlistCount > 0) {
					gui->label("Playlists");

					float leftPad = 20;
					for(int i = 0; i < ad->playlistCount; i++) {
						YoutubePlaylist* playlist = ad->playlists + i;

						if(gui->button(fillString("%i: %s. (%i.)", i, playlist->title, playlist->count), 0, 0)) {
							strCpy(ad->urlString, playlist->id);
							ad->urlVideoCount = playlist->count;
						}
					}
				}
			}
		}

		if(sectionMode == 1) {
			gui->label(fillString("VideoCount: %i.", ad->videoCount), 0);

			float leftPad = 20;
			for(int i = 0; i < ad->videoCount; i++) {
				YoutubeVideo* video = ad->videos + i;

				gui->label(fillString("Video %i:", i), 0);

				gui->div(vec2(leftPad,0)); gui->empty(); gui->label(fillString("Title: %s", video->title), 0);
				gui->div(vec2(leftPad,0)); gui->empty(); gui->label(fillString("Thumbnail: %s", video->thumbnail), 0);
				gui->div(vec2(leftPad,0)); gui->empty(); gui->label(fillString("Id: %s", video->id), 0);
				gui->div(vec2(leftPad,0)); gui->empty(); gui->label(fillString("Date: %s", video->dateString), 0);
				gui->div(vec2(leftPad,0)); gui->empty(); gui->label(fillString("ViewCount: %i.", video->viewCount), 0);
				gui->div(vec2(leftPad,0)); gui->empty(); gui->label(fillString("likeCount: %i.", video->likeCount), 0);
				gui->div(vec2(leftPad,0)); gui->empty(); gui->label(fillString("DislikeCount: %i.", video->dislikeCount), 0);
				gui->div(vec2(leftPad,0)); gui->empty(); gui->label(fillString("FavoriteCount: %i.", video->favoriteCount), 0);
				gui->div(vec2(leftPad,0)); gui->empty(); gui->label(fillString("CommentCount: %i.", video->commentCount), 0);

			}
		}

		if(sectionMode == 2) {
			float leftPad = 20;
			for(int i = 0; i < ad->playlistCount; i++) {
				YoutubePlaylist* playlist = ad->playlists + i;

				gui->label(fillString("Playlist %i: ", i), 0);

				gui->div(vec2(leftPad,0)); gui->empty(); gui->label(fillString("Id: %s", playlist->id), 0);
				gui->div(vec2(leftPad,0)); gui->empty(); gui->label(fillString("title: %s", playlist->title), 0);
				gui->div(vec2(leftPad,0)); gui->empty(); gui->label(fillString("count: %i.", playlist->count), 0);
			}
		}

		static bool sectionGuiRecording = false;
		// if(gui->beginSection("Recording", &sectionGuiRecording)) {
		if(false) {

			bool noActiveThreads = threadQueueFinished(threadQueue);

			gui->div(vec2(0,0));
			gui->label("Active Threads:");
			gui->label(fillString("%i", !noActiveThreads));

			gui->div(vec2(0,0));
			gui->label("Max Frames:");
			gui->label(fillString("%i", ds->inputCapacity));

			gui->div(vec2(0,0));
			if(gui->switcher("Record", &ds->recordingInput)) {
				if(ds->playbackInput || !noActiveThreads) ds->recordingInput = false;

				if(ds->recordingInput) {
					if(threadQueueFinished(threadQueue)) {

						ds->snapShotCount = pMemory->index+1;
						ds->snapShotMemoryIndex = pMemory->arrays[pMemory->index].index;
						for(int i = 0; i < ds->snapShotCount; i++) {
							if(ds->snapShotMemory[i] == 0) 
								ds->snapShotMemory[i] = (char*)malloc(pMemory->slotSize);

							memCpy(ds->snapShotMemory[i], pMemory->arrays[i].data, pMemory->slotSize);
						}


						ds->recordingInput = true;
						ds->inputIndex = 0;
					}
				}
			}
			gui->label(fillString("%i", ds->inputIndex));


			if(ds->inputIndex > 0 && !ds->recordingInput) {
				char* s = ds->playbackInput ? "Stop Playback" : "Start Playback";

				if(gui->switcher(s, &ds->playbackInput)) {
					if(ds->playbackInput) {
						threadQueueComplete(threadQueue);
						ds->playbackIndex = 0;

						pMemory->index = ds->snapShotCount-1;
						pMemory->arrays[pMemory->index].index = ds->snapShotMemoryIndex;

						for(int i = 0; i < ds->snapShotCount; i++) {
							memCpy(pMemory->arrays[i].data, ds->snapShotMemory[i], pMemory->slotSize);
						}
					} else {
						ds->playbackPause = false;
						ds->playbackBreak = false;
					}
				}

				if(ds->playbackInput) {
					gui->div(vec2(0,0));

					gui->switcher("Pause/Resume", &ds->playbackPause);

					int cap = ds->playbackIndex;
					gui->slider(&ds->playbackIndex, 0, ds->inputIndex - 1);
					ds->playbackIndex = cap;

					gui->div(vec3(0.25f,0.25f,0));
					if(gui->button("Step")) {
						ds->playbackBreak = true;
						ds->playbackPause = false;
						ds->playbackBreakIndex = (ds->playbackIndex + 1)%ds->inputIndex;
					}
					gui->switcher("Break", &ds->playbackBreak);
					gui->slider(&ds->playbackBreakIndex, 0, ds->inputIndex - 1);
				}
			}

		// } gui->endSection();
		}

		static bool sectionGuiSettings = initSections;
		// if(gui->beginSection("GuiSettings", &sectionGuiSettings)) {
		if(false) {
			guiSettings(gui);
		// } gui->endSection();
		}

		static bool sectionSettings = initSections;
		// if(gui->beginSection("Settings", &sectionSettings)) {
		if(false) {
			gui->div(vec2(0,0)); if(gui->button("Compile")) shellExecute("C:\\Projects\\Hmm\\code\\buildWin32.bat");
								 if(gui->button("Up Buffers")) ad->updateFrameBuffers = true;
			gui->div(vec2(0,0)); gui->label("FoV", 0); gui->slider(&ad->fieldOfView, 1, 180);
			gui->div(vec2(0,0)); gui->label("MSAA", 0); gui->slider(&ad->msaaSamples, 1, 8);
			gui->switcher("Native Res", &ad->useNativeRes);
			gui->div(0,0,0); gui->label("FboRes", 0); gui->slider(&ad->fboRes.x, 150, ad->cur3dBufferRes.x); gui->slider(&ad->fboRes.y, 150, ad->cur3dBufferRes.y);
			gui->div(0,0,0); gui->label("NFPlane", 0); gui->slider(&ad->nearPlane, 0.01, 2); gui->slider(&ad->farPlane, 1000, 5000);
		// } gui->endSection();
		}




		static bool sectionTest = false;
		// if(gui->beginSection("Test", &sectionTest)) {
		if(false) {
			static int scrollHeight = 200;
			static int scrollElements = 13;
			static float scrollVal = 0;
			gui->div(vec2(0,0)); gui->slider(&scrollHeight, 0, 2000); gui->slider(&scrollElements, 0, 100);

			gui->beginScroll(scrollHeight, &scrollVal); {
				for(int i = 0; i < scrollElements; i++) {
					gui->button(fillString("Element: %i.", i));
				}
			} gui->endScroll();

			static int textCapacity = 50;
			static char* text = getPArray(char, textCapacity);
			static bool DOIT = true;
			if(DOIT) {
				DOIT = false;
				strClear(text);
				strCpy(text, "This is a really long sentence!");
			}
			gui->div(vec2(0,0)); gui->label("Text Box:", 0); gui->textBoxChar(text, 0, textCapacity);

			static int textNumber = 1234;
			gui->div(vec2(0,0)); gui->label("Int Box:", 0); gui->textBoxInt(&textNumber);

			static float textFloat = 123.456f;
			gui->div(vec2(0,0)); gui->label("Float Box:", 0); gui->textBoxFloat(&textFloat);

		// } gui->endSection();
		}

		gui->end();
	}

	ds->timer->timerInfoCount = __COUNTER__;

	int fontHeight = 18;
	Timer* timer = ds->timer;
	int cycleCount = arrayCount(ds->timings);

	bool threadsFinished = threadQueueFinished(threadQueue);

	int bufferIndex = timer->bufferIndex;

	// Save const strings from initialised timerinfos.
	{
		int timerCount = timer->timerInfoCount;
		for(int i = 0; i < timerCount; i++) {
			TimerInfo* info = timer->timerInfos + i;

			// Set colors.
			float ss = i%(timerCount/2) / ((float)timerCount/2);
			float h = i < timerCount/2 ? 0.1f : -0.1f;
			Vec3 color = vec3(0,0,0);
			hslToRgb(color.e, 360*ss, 0.5f, 0.5f+h);

			vSet3(info->color, color.r, color.g, color.b);


			if(!info->initialised || info->stringsSaved) continue;
			char* s;
			
			s = info->file;
			info->file = getPStringDebug(strLen(s) + 1);
			strCpy(info->file, s);

			s = info->function;
			info->function = getPStringDebug(strLen(s) + 1);
			strCpy(info->function, s);

			s = info->name;
			info->name = getPStringDebug(strLen(s) + 1);
			strCpy(info->name, s);

			info->stringsSaved = true;
		}
	}

	if(ds->setPause) {
		ds->lastCycleIndex = ds->cycleIndex;
		ds->cycleIndex = mod(ds->cycleIndex-1, arrayCount(ds->timings));

		ds->timelineCamSize = -1;
		ds->timelineCamPos = -1;

		ds->setPause = false;
	}
	if(ds->setPlay) {
		ds->cycleIndex = ds->lastCycleIndex;
		ds->setPlay = false;
	}

	Timings* timings = ds->timings[ds->cycleIndex];
	Statistic* statistics = ds->statistics[ds->cycleIndex];

	int cycleIndex = ds->cycleIndex;
	int newCycleIndex = (ds->cycleIndex + 1)%cycleCount;

	// Timer update.
	{

		if(!ds->noCollating) {
			zeroMemory(timings, timer->timerInfoCount*sizeof(Timings));
			zeroMemory(statistics, timer->timerInfoCount*sizeof(Statistic));

			ds->cycleIndex = newCycleIndex;

			// Collate timing buffer.

			// for(int threadIndex = 0; threadIndex < threadQueue->threadCount; threadIndex++) 
			{
				// GraphSlot* graphSlots = ds->graphSlots[threadIndex];
				// int index = ds->graphSlotCount[threadIndex];

				for(int i = ds->lastBufferIndex; i < bufferIndex; ++i) {
					TimerSlot* slot = timer->timerBuffer + i;
					
					int threadIndex = threadIdToIndex(threadQueue, slot->threadId);

					if(slot->type == TIMER_TYPE_BEGIN) {
						int index = ds->graphSlotCount[threadIndex];

						GraphSlot graphSlot;
						graphSlot.threadIndex = threadIndex;
						graphSlot.timerIndex = slot->timerIndex;
						graphSlot.stackIndex = index;
						graphSlot.cycles = slot->cycles;
						ds->graphSlots[threadIndex][index] = graphSlot;

						ds->graphSlotCount[threadIndex]++;
					} else {
						ds->graphSlotCount[threadIndex]--;
						int index = ds->graphSlotCount[threadIndex];
						if(index < 0) index = 0; // @Hack, to keep things running.

						ds->graphSlots[threadIndex][index].size = slot->cycles - ds->graphSlots[threadIndex][index].cycles;
						ds->savedBuffer[ds->savedBufferIndex] = ds->graphSlots[threadIndex][index];
						ds->savedBufferIndex = (ds->savedBufferIndex+1)%ds->savedBufferMax;
						ds->savedBufferCount = clampMax(ds->savedBufferCount + 1, ds->savedBufferMax);


						Timings* timing = timings + ds->graphSlots[threadIndex][index].timerIndex;
						timing->cycles += ds->graphSlots[threadIndex][index].size;
						timing->hits++;
					}
				}

				// ds->graphSlotCount[threadIndex] = index;
			}

			// ds->savedBufferCounts[cycleIndex] = savedBufferCount;

			for(int i = 0; i < timer->timerInfoCount; i++) {
				Timings* t = timings + i;
				t->cyclesOverHits = t->hits > 0 ? (u64)(t->cycles/t->hits) : 0; 
			}

			for(int timerIndex = 0; timerIndex < timer->timerInfoCount; timerIndex++) {
				Statistic* stat = statistics + timerIndex;
				beginStatistic(stat);

				for(int i = 0; i < arrayCount(ds->timings); i++) {
					Timings* t = &ds->timings[i][timerIndex];
					if(t->hits == 0) continue;

					updateStatistic(stat, t->cyclesOverHits);
				}

				endStatistic(stat);
				if(stat->count == 0) stat->avg = 0;
			}
		}
	}

	ds->lastBufferIndex = bufferIndex;

	if(threadsFinished) {
		timer->bufferIndex = 0;
		ds->lastBufferIndex = 0;
	}

	assert(timer->bufferIndex < timer->bufferSize);

	if(init) {
		ds->lineGraphCamSize = 700000;
		ds->lineGraphCamPos = 0;
		ds->mode = 0;
		ds->lineGraphHeight = 30;
		ds->lineGraphHighlight = 0;
	}

	//
	// Draw timing info.
	//

	if(ds->showStats) 
	{
		static int highlightedIndex = -1;
		Vec4 highlightColor = vec4(1,1,1,0.05f);

		// float cyclesPerFrame = (float)((3.5f*((float)1/60))*1024*1024*1024);
		float cyclesPerFrame = (float)((3.5f*((float)1/60))*1000*1000*1000);
		fontHeight = 20;
		Vec2 textPos = vec2(550, -fontHeight);
		int infoCount = timer->timerInfoCount;

		Gui* gui = ds->gui2;
		gui->start(ds->gInput, getFont(FONT_CALIBRI, fontHeight), ws->currentRes);

		gui->label("App Statistics", 1, gui->colors.sectionColor, vec4(0,0,0,1));

		float sectionWidth = 120;
		float headerDivs[] = {sectionWidth,sectionWidth,sectionWidth,0,80,80};
		gui->div(headerDivs, arrayCount(headerDivs));
		if(gui->button("Data", (int)(ds->mode == 0) + 1)) ds->mode = 0;
		if(gui->button("Line graph", (int)(ds->mode == 1) + 1)) ds->mode = 1;
		if(gui->button("Timeline", (int)(ds->mode == 2) + 1)) ds->mode = 2;
		gui->empty();
		gui->label(fillString("%fms", ds->debugTime*1000), 1);
		gui->label(fillString("%fms", ds->debugRenderTime*1000), 1);

		gui->div(vec2(0.2f,0));
		if(gui->switcher("Freeze", &ds->noCollating)) {
			if(ds->noCollating) ds->setPause = true;
			else ds->setPlay = true;
		}
		gui->slider(&ds->cycleIndex, 0, cycleCount-1);

		if(ds->mode == 0)
		{
			int barWidth = 1;
			int barCount = arrayCount(ds->timings);
			float sectionWidths[] = {0,0.2f,0,0,0,0,0,0, barWidth*barCount};
			// float sectionWidths[] = {0.1f,0,0.1f,0,0.05f,0,0,0.1f, barWidth*barCount};

			char* headers[] = {"File", "Function", "Description", "Cycles", "Hits", "C/H", "Avg. Cycl.", "Total Time", "Graphs"};
			gui->div(sectionWidths, arrayCount(sectionWidths));

			float textSectionEnd;
			for(int i = 0; i < arrayCount(sectionWidths); i++) {
				// @Hack: Get the end of the text region by looking at last region.
				if(i == arrayCount(sectionWidths)-1) textSectionEnd = gui->getCurrentRegion().max.x;

				Vec4 buttonColor = vec4(gui->colors.regionColor.rgb, 0.5f);
				if(gui->button(headers[i], 0, 1, buttonColor, vec4(0,0,0,1))) {
					if(abs(ds->graphSortingIndex) == i) ds->graphSortingIndex *= -1;
					else ds->graphSortingIndex = i;
				}
			}

			SortPair* sortList = getTArrayDebug(SortPair, infoCount+1);
			{
				for(int i = 0; i < infoCount+1; i++) sortList[i].index = i;

		   			 if(abs(ds->graphSortingIndex) == 3) for(int i = 0; i < infoCount+1; i++) sortList[i].key = timings[i].cycles;
		   		else if(abs(ds->graphSortingIndex) == 4) for(int i = 0; i < infoCount+1; i++) sortList[i].key = timings[i].hits;
		   		else if(abs(ds->graphSortingIndex) == 5) for(int i = 0; i < infoCount+1; i++) sortList[i].key = timings[i].cyclesOverHits;
		   		else if(abs(ds->graphSortingIndex) == 6) for(int i = 0; i < infoCount+1; i++) sortList[i].key = statistics[i].avg;
		   		else if(abs(ds->graphSortingIndex) == 7) for(int i = 0; i < infoCount+1; i++) sortList[i].key = timings[i].cycles/cyclesPerFrame;

		   		bool sortDirection = true;
		   		if(ds->graphSortingIndex < 0) sortDirection = false;

		   		if(valueBetween(abs(ds->graphSortingIndex), 3, 7)) 
					bubbleSort(sortList, infoCount, sortDirection);
			}

			for(int index = 0; index < infoCount; index++) {
				int i = sortList[index].index;

				TimerInfo* tInfo = timer->timerInfos + i;
				Timings* timing = timings + i;

				if(!tInfo->initialised) continue;

				gui->div(sectionWidths, arrayCount(sectionWidths)); 

				// if(highlightedIndex == i) {
				// 	Rect r = gui->getCurrentRegion();
				// 	Rect line = rect(r.min, vec2(textSectionEnd,r.min.y + fontHeight));
				// 	dcRect(line, highlightColor);
				// }

				gui->label(fillString("%s", tInfo->file + 21),0);
				if(gui->button(fillString("%s", tInfo->function),0, 0, vec4(gui->colors.regionColor.rgb, 0.2f))) {
					char* command = fillString("%s %s:%i", editor_executable_path, tInfo->file, tInfo->line);
					shellExecuteNoWindow(command);
				}
				gui->label(fillString("%s", tInfo->name),0);
				gui->label(fillString("%i64.c", timing->cycles),2);
				gui->label(fillString("%i64.", timing->hits),2);
				gui->label(fillString("%i64.c", timing->cyclesOverHits),2);
				gui->label(fillString("%i64.c", (i64)statistics[i].avg),2); // Not a i64 but whatever.
				gui->label(fillString("%.3f%%", ((float)timing->cycles/cyclesPerFrame)*100),2);

				// Bar graphs.
				dcState(STATE_LINEWIDTH, barWidth);

				gui->empty();
				Rect r = gui->getCurrentRegion();
				float rheight = gui->getDefaultHeight();
				float fontBaseOffset = 4;

				float xOffset = 0;
				for(int statIndex = 0; statIndex < barCount; statIndex++) {
					Statistic* stat = statistics + i;
					u64 coh = ds->timings[statIndex][i].cyclesOverHits;

					float height = mapRangeClamp(coh, stat->min, stat->max, 1, rheight);
					Vec2 rmin = r.min + vec2(xOffset, fontBaseOffset);
					float colorOffset = mapRange(coh, stat->min, stat->max, 0, 1);
					// dcRect(rectMinDim(rmin, vec2(barWidth, height)), vec4(colorOffset,1-colorOffset,0,1));
					dcLine2d(rmin, rmin+vec2(0,height), vec4(colorOffset,1-colorOffset,0,1));

					xOffset += barWidth;
				}
			}
		}

		// Timeline graph.
		if(ds->mode == 2 && ds->noCollating)
		{
			float lineHeightOffset = 1.2;

			gui->empty();
			Rect cyclesRect = gui->getCurrentRegion();
			gui->heightPush(1.5f);
			gui->empty();
			Rect headerRect = gui->getCurrentRegion();
			gui->heightPop();

			float lineHeight = fontHeight * lineHeightOffset;

			gui->heightPush(3*lineHeight +  2*lineHeight*(threadQueue->threadCount-1));
			gui->empty();
			Rect bgRect = gui->getCurrentRegion();
			gui->heightPop();

			float graphWidth = rectGetDim(bgRect).w;

			int swapTimerIndex = 0;
			for(int i = 0; i < timer->timerInfoCount; i++) {
				if(!timer->timerInfos[i].initialised) continue;

				if(strCompare(timer->timerInfos[i].name, "Swap")) {
					swapTimerIndex = i;
					break;
				}
			}

			int recentIndex = mod(ds->savedBufferIndex-1, ds->savedBufferMax);
			int oldIndex = mod(ds->savedBufferIndex - ds->savedBufferCount, ds->savedBufferMax);
			GraphSlot recentSlot = ds->savedBuffer[recentIndex];
			GraphSlot oldSlot = ds->savedBuffer[oldIndex];
			double cyclesLeft = oldSlot.cycles;
			double cyclesRight = recentSlot.cycles + recentSlot.size;
			double cyclesSize = cyclesRight - cyclesLeft;

			// Setup cam pos and zoom.
			if(ds->timelineCamPos == -1 && ds->timelineCamSize == -1) {
				ds->timelineCamSize = (recentSlot.cycles + recentSlot.size) - oldSlot.cycles;
				ds->timelineCamPos = oldSlot.cycles + ds->timelineCamSize/2;
			}

			if(gui->input.mouseWheel) {
				float wheel = gui->input.mouseWheel;

				float offset = wheel < 0 ? 1.1f : 1/1.1f;
				if(!input->keysDown[KEYCODE_SHIFT] && input->keysDown[KEYCODE_CTRL]) 
					offset = wheel < 0 ? 1.2f : 1/1.2f;
				if(input->keysDown[KEYCODE_SHIFT] && input->keysDown[KEYCODE_CTRL]) 
					offset = wheel < 0 ? 1.4f : 1/1.4f;

				double oldZoom = ds->timelineCamSize;
				ds->timelineCamSize *= offset;
				clampDouble(&ds->timelineCamSize, 1000, cyclesSize);
				double diff = ds->timelineCamSize - oldZoom;

				float zoomOffset = mapRange(input->mousePos.x, bgRect.min.x, bgRect.max.x, -0.5f, 0.5f);
				ds->timelineCamPos -= diff*zoomOffset;
			}


			Vec2 dragDelta = vec2(0,0);
			gui->drag(bgRect, &dragDelta, vec4(0,0,0,0));

			ds->timelineCamPos -= dragDelta.x * (ds->timelineCamSize/graphWidth);
			clampDouble(&ds->timelineCamPos, cyclesLeft + ds->timelineCamSize/2, cyclesRight - ds->timelineCamSize/2);


			double camPos = ds->timelineCamPos;
			double zoom = ds->timelineCamSize;
			double orthoLeft = camPos - zoom/2;
			double orthoRight = camPos + zoom/2;


			// Header.
			{
				dcRect(cyclesRect, gui->colors.sectionColor);
				Vec2 cyclesDim = rectGetDim(cyclesRect);

				dcRect(headerRect, vec4(1,1,1,0.1f));
				Vec2 headerDim = rectGetDim(headerRect);

				{
					float viewAreaLeft = mapRangeDouble(orthoLeft, cyclesLeft, cyclesRight, cyclesRect.min.x, cyclesRect.max.x);
					float viewAreaRight = mapRangeDouble(orthoRight, cyclesLeft, cyclesRight, cyclesRect.min.x, cyclesRect.max.x);

					float viewSize = viewAreaRight - viewAreaLeft;
					float viewMid = viewAreaRight + viewSize/2;
					float viewMinSize = 2;
					if(viewSize < viewMinSize) {
						viewAreaLeft = viewMid - viewMinSize*0.5;
						viewAreaRight = viewMid + viewMinSize*0.5;
					}

					dcRect(rect(viewAreaLeft, cyclesRect.min.y, viewAreaRight, cyclesRect.max.y), vec4(1,1,1,0.03f));
				}

				float g = 0.7f;
				float heightMod = 0.0f;
				double div = 4;
				double divMod = (1/div) + 0.05f;

				double timelineSection = div;
				while(timelineSection < zoom*divMod*(ws->currentRes.h/(graphWidth))) {
					timelineSection *= div;
					heightMod += 0.1f;
				}

				clampMax(&heightMod, 1);

				dcState(STATE_LINEWIDTH, 3);
				double startPos = roundModDouble(orthoLeft, timelineSection) - timelineSection;
				double pos = startPos;
				while(pos < orthoRight + timelineSection) {
					double p = mapRangeDouble(pos, orthoLeft, orthoRight, bgRect.min.x, bgRect.max.x);

					// Big line.
					{
						float h = headerDim.h*heightMod;
						dcLine2d(vec2(p,headerRect.min.y), vec2(p,headerRect.min.y + h), vec4(g,g,g,1));
					}

					// Text
					{
						Vec2 textPos = vec2(p,cyclesRect.min.y + cyclesDim.h/2);
						float percent = mapRange(pos, cyclesLeft, cyclesRight, 0, 100);
						int percentInterval = mapRangeDouble(timelineSection, 0, cyclesSize, 0, 100);

						char* s;
						if(percentInterval > 10) s = fillString("%i%%", (int)percent);
						else if(percentInterval > 1) s = fillString("%.1f%%", percent);
						else if(percentInterval > 0.1) s = fillString("%.2f%%", percent);
						else s = fillString("%.3f%%", percent);

						float tw = getTextDim(s, gui->font).w;
						if(valueBetween(bgRect.min.x, textPos.x - tw/2, textPos.x + tw/2)) textPos.x = bgRect.min.x + tw/2;
						if(valueBetween(bgRect.max.x, textPos.x - tw/2, textPos.x + tw/2)) textPos.x = bgRect.max.x - tw/2;

						dcText(s, gui->font, textPos, gui->colors.textColor, vec2i(0,0), 0, 1, gui->colors.shadowColor);
					}

					pos += timelineSection;
				}
				dcState(STATE_LINEWIDTH, 1);

				pos = startPos;
				timelineSection /= div;
				heightMod *= 0.6f;
				int index = 0;
				while(pos < orthoRight + timelineSection) {

					// Small line.
					if((index%(int)div) != 0) {
						double p = mapRangeDouble(pos, orthoLeft, orthoRight, bgRect.min.x, bgRect.max.x);
						float h = headerDim.h*heightMod;
						dcLine2d(vec2(p,headerRect.min.y), vec2(p,headerRect.min.y + h), vec4(g,g,g,1));
					}

					// Cycle text.
					{
						float pMid = mapRangeDouble(pos - timelineSection/2, orthoLeft, orthoRight, bgRect.min.x, bgRect.max.x);
						Vec2 textPos = vec2(pMid,headerRect.min.y + headerDim.h/3);

						double cycles = timelineSection;
						char* s;
						if(cycles < 1000) s = fillString("%ic", (int)cycles);
						else if(cycles < 1000000) s = fillString("%.1fkc", cycles/1000);
						else if(cycles < 1000000000) s = fillString("%.1fmc", cycles/1000000);
						else if(cycles < 1000000000000) s = fillString("%.1fbc", cycles/1000000000);
						else s = fillString("INF");

						dcText(s, gui->font, textPos, gui->colors.textColor, vec2i(0,0), 0, gui->settings.textShadow, gui->colors.shadowColor);
					}

					pos += timelineSection;
					index++;

				}
			}

			dcState(STATE_LINEWIDTH, 1);

			bool mouseHighlight = false;
			Rect hRect;
			Vec4 hc;
			char* hText;
			GraphSlot* hSlot;

			Vec2 startPos = rectGetUL(bgRect);
			startPos -= vec2(0, lineHeight);

			int firstBufferIndex = oldIndex;
			int bufferCount = ds->savedBufferCount;
			for(int threadIndex = 0; threadIndex < threadQueue->threadCount; threadIndex++) {

				// Horizontal lines to distinguish thread bars.
				if(threadIndex > 0) {
					Vec2 p = startPos + vec2(0,lineHeight);
					float g = 0.8f;
					dcLine2d(p, vec2(bgRect.max.x, p.y), vec4(g,g,g,1));
				}

				for(int i = 0; i < bufferCount; ++i) {
					GraphSlot* slot = ds->savedBuffer + ((firstBufferIndex+i)%ds->savedBufferMax);
					if(slot->threadIndex != threadIndex) continue;

					Timings* t = timings + slot->timerIndex;
					TimerInfo* tInfo = timer->timerInfos + slot->timerIndex;

					if(slot->cycles + slot->size < orthoLeft || slot->cycles > orthoRight) continue;


					double barLeft = mapRangeDouble(slot->cycles, orthoLeft, orthoRight, bgRect.min.x, bgRect.max.x);
					double barRight = mapRangeDouble(slot->cycles + slot->size, orthoLeft, orthoRight, bgRect.min.x, bgRect.max.x);

					// Draw vertical line at swap boundaries.
					if(slot->timerIndex == swapTimerIndex) {
						float g = 0.8f;
						dcLine2d(vec2(barRight, bgRect.min.y), vec2(barRight, bgRect.max.y), vec4(g,g,g,1));
					}

					// Bar min size is 1.
					if(barRight - barLeft < 1) {
						double mid = barLeft + (barRight - barLeft)/2;
						barLeft = mid - 0.5f;
						barRight = mid + 0.5f;
					}

					float y = startPos.y+slot->stackIndex*-lineHeight;
					Rect r = rect(vec2(barLeft,y), vec2(barRight, y + lineHeight));

					float cOff = slot->timerIndex/(float)timer->timerInfoCount;
					Vec4 c = vec4(tInfo->color[0], tInfo->color[1], tInfo->color[2], 1);

					if(gui->getMouseOver(gui->input.mousePos, r)) {
						mouseHighlight = true;
						hRect = r;
						hc = c;

						hText = fillString("%s %s (%i.c)", tInfo->function, tInfo->name, slot->size);
						hSlot = slot;
					} else {
						float g = 0.1f;
						gui->drawRect(r, vec4(g,g,g,1));

						bool textRectVisible = (barRight - barLeft) > 1;
						if(textRectVisible) {
							if(barLeft < bgRect.min.x) r.min.x = bgRect.min.x;
							Rect textRect = rect(r.min+vec2(1,1), r.max-vec2(1,1));

							gui->drawTextBox(textRect, fillString("%s %s (%i.c)", tInfo->function, tInfo->name, slot->size), c, 0, rectGetDim(textRect).w);
						}
					}

				}

				if(threadIndex == 0) startPos.y -= lineHeight*3;
				else startPos.y -= lineHeight*2;

			}

			if(mouseHighlight) {
				if(hRect.min.x < bgRect.min.x) hRect.min.x = bgRect.min.x;

				float tw = getTextDim(hText, gui->font).w + 2;
				if(tw > rectGetDim(hRect).w) hRect.max.x = hRect.min.x + tw;

				float g = 0.8f;
				gui->drawRect(hRect, vec4(g,g,g,1));

				Rect textRect = rect(hRect.min+vec2(1,1), hRect.max-vec2(1,1));
				gui->drawTextBox(textRect, hText, hc);
			}

			gui->div(0.1f, 0); 
			gui->div(0.1f, 0); 

			if(gui->button("Reset")) {
				ds->timelineCamSize = (recentSlot.cycles + recentSlot.size) - oldSlot.cycles;
				ds->timelineCamPos = oldSlot.cycles + ds->timelineCamSize/2;
			}

			gui->label(fillString("Cam: %i64., Zoom: %i64.", (i64)ds->timelineCamPos, (i64)ds->timelineCamSize));
		}
		



		// Line graph.
		if(ds->mode == 1)
		{
			dcState(STATE_LINEWIDTH, 1);

			// Get longest function name string.
			float timerInfoMaxStringSize = 0;
			int cycleCount = arrayCount(ds->timings);
			int timerCount = ds->timer->timerInfoCount;
			for(int timerIndex = 0; timerIndex < timerCount; timerIndex++) {
				TimerInfo* info = &timer->timerInfos[timerIndex];
				if(!info->initialised) continue;

				Statistic* stat = &ds->statistics[cycleIndex][timerIndex];
				if(stat->avg == 0) continue;

				char* text = strLen(info->name) > 0 ? info->name : info->function;
				timerInfoMaxStringSize = max(getTextDim(text, gui->font).w, timerInfoMaxStringSize);
			}

			// gui->div(0.2f, 0);
			gui->slider(&ds->lineGraphHeight, 1, 60);
			// gui->empty();

			gui->heightPush(gui->getDefaultHeight() * ds->lineGraphHeight);
			gui->div(vec3(timerInfoMaxStringSize + 2, 0, 120));
			gui->empty(); Rect rectNames = gui->getCurrentRegion();
			gui->empty(); Rect rectLines = gui->getCurrentRegion();
			gui->empty(); Rect rectNumbers = gui->getCurrentRegion();
			gui->heightPop();

			float rTop = rectLines.max.y;
			float rBottom = rectLines.min.y;

			Vec2 dragDelta = vec2(0,0);
			gui->drag(rectLines, &dragDelta, vec4(0,0,0,0.2f));

			float wheel = gui->input.mouseWheel;
			if(wheel) {
				float offset = wheel < 0 ? 1.1f : 1/1.1f;
				if(!input->keysDown[KEYCODE_SHIFT] && input->keysDown[KEYCODE_CTRL]) 
					offset = wheel < 0 ? 1.2f : 1/1.2f;
				if(input->keysDown[KEYCODE_SHIFT] && input->keysDown[KEYCODE_CTRL]) 
					offset = wheel < 0 ? 1.4f : 1/1.4f;

				float heightDiff = ds->lineGraphCamSize;
				ds->lineGraphCamSize *= offset;
				ds->lineGraphCamSize = clampMin(ds->lineGraphCamSize, 0.00001f);
				heightDiff -= ds->lineGraphCamSize;

				float mouseOffset = mapRange(input->mousePosNegative.y, rBottom, rTop, -0.5f, 0.5f);
				ds->lineGraphCamPos += heightDiff * mouseOffset;
			}

			ds->lineGraphCamPos -= dragDelta.y * ((ds->lineGraphCamSize)/(rTop - rBottom));
			clampMin(&ds->lineGraphCamPos, ds->lineGraphCamSize/2.05f);

			float orthoTop = ds->lineGraphCamPos + ds->lineGraphCamSize/2;
			float orthoBottom = ds->lineGraphCamPos - ds->lineGraphCamSize/2;

			// Draw numbers.
			{
				gui->scissorPush(rectNumbers);

				float y = 0;
				float length = 10;

				float div = 10;
				float timelineSection = div;
				float splitMod = (1/div)*0.2f;
				while(timelineSection < ds->lineGraphCamSize*splitMod*(ws->currentRes.h/(rTop-rBottom))) timelineSection *= div;

				float start = roundMod(orthoBottom, timelineSection) - timelineSection;

				float p = start;
				while(p < orthoTop) {
					p += timelineSection;
					y = mapRange(p, orthoBottom, orthoTop, rBottom, rTop);

					dcLine2d(vec2(rectNumbers.min.x, y), vec2(rectNumbers.min.x + length, y), vec4(1,1,1,1)); 
					dcText(fillString("%i64.c",(i64)p), gui->font, vec2(rectNumbers.min.x + length + 4, y), vec4(1,1,1,1), vec2i(-1,0));
				}

				gui->scissorPop();
			}

			for(int timerIndex = 0; timerIndex < timerCount; timerIndex++) {
				TimerInfo* info = &timer->timerInfos[timerIndex];
				if(!info->initialised) continue;

				Statistic* stat = &ds->statistics[cycleIndex][timerIndex];
				if(stat->avg == 0) continue;

				float statMin = mapRange(stat->min, orthoBottom, orthoTop, rBottom, rTop);
				float statMax = mapRange(stat->max, orthoBottom, orthoTop, rBottom, rTop);
				if(statMax < rBottom || statMin > rTop) continue;

				Vec4 color = vec4(info->color[0], info->color[1], info->color[2], 1);

				float yAvg = mapRange(stat->avg, orthoBottom, orthoTop, rBottom, rTop);
				char* text = strLen(info->name) > 0 ? info->name : info->function;
				float textWidth = getTextDim(text, gui->font, vec2(rectNames.max.x - 2, yAvg)).w;

				gui->scissorPush(rectNames);
				Rect tr = getTextLineRect(text, gui->font, vec2(rectNames.max.x - 2, yAvg), vec2i(1,-1));
				if(gui->buttonUndocked(text, tr, 2, gui->colors.panelColor)) ds->lineGraphHighlight = timerIndex;
				gui->scissorPop();

				Rect rectNamesAndLines = rect(rectNames.min, rectLines.max);
				gui->scissorPush(rectNamesAndLines);
				dcLine2d(vec2(rectLines.min.x - textWidth - 2, yAvg), vec2(rectLines.max.x, yAvg), color);
				gui->scissorPop();

				gui->scissorPush(rectLines);

				if(timerIndex == ds->lineGraphHighlight) dcState(STATE_LINEWIDTH, 3);
				else dcState(STATE_LINEWIDTH, 1);

				bool firstEmpty = ds->timings[0][timerIndex].cyclesOverHits == 0;
				Vec2 p = vec2(rectLines.min.x, 0);
				if(firstEmpty) p.y = yAvg;
				else p.y = mapRange(ds->timings[0][timerIndex].cyclesOverHits, orthoBottom, orthoTop, rBottom, rTop);
				for(int i = 1; i < cycleCount; i++) {
					Timings* t = &ds->timings[i][timerIndex];

					bool lastElementEmpty = false;
					if(t->cyclesOverHits == 0) {
						if(i != cycleCount-1) continue;
						else lastElementEmpty = true;
					}

					float y = mapRange(t->cyclesOverHits, orthoBottom, orthoTop, rBottom, rTop);
					float xOff = rectGetDim(rectLines).w/(cycleCount-1);
					Vec2 np = vec2(rectLines.min.x + xOff*i, y);

					if(lastElementEmpty) np.y = yAvg;

					dcLine2d(p, np, color);
					p = np;
				}

				dcState(STATE_LINEWIDTH, 1);

				gui->scissorPop();
			}

			gui->empty();
			Rect r = gui->getCurrentRegion();
			Vec2 rc = rectGetCen(r);
			float rw = rectGetDim(r).w;

			// Draw color rectangles.
			float width = (rw/timerCount)*0.75f;
			float height = fontHeight*0.8f;
			float sw = (rw-(timerCount*width))/(timerCount+1);

			for(int i = 0; i < timerCount; i++) {
				TimerInfo* info = &timer->timerInfos[i];

				Vec4 color = vec4(info->color[0], info->color[1], info->color[2], 1);
				Vec2 pos = vec2(r.min.x + sw+width/2 + i*(width+sw), rc.y);
				dcRect(rectCenDim(pos, vec2(width, height)), color);
			}

		}

		gui->end();

	}

	//
	// Dropdown Console.
	//

	{
		Console* con = &ds->console;

		if(init) {
			con->init(ws->currentRes.y);
		}

		bool smallExtension = input->keysPressed[KEYCODE_F5] && !input->keysDown[KEYCODE_CTRL];
		bool bigExtension = input->keysPressed[KEYCODE_F5] && input->keysDown[KEYCODE_CTRL];

		con->update(ds->input, vec2(ws->currentRes), ad->dt, smallExtension, bigExtension);

		// Execute commands.

		if(con->commandAvailable) {
			con->commandAvailable = false;

			char* comName = con->comName;
			char** args = con->comArgs;
			char* resultString = "";
			bool pushResult = true;

			if(strCompare(comName, "add")) {
				int a = strToInt(args[0]);
				int b = strToInt(args[1]);

				resultString = fillString("%i + %i = %i.", a, b, a+b);

			} else if(strCompare(comName, "addFloat")) {
				float a = strToFloat(args[0]);
				float b = strToFloat(args[1]);

				resultString = fillString("%f + %f = %f.", a, b, a+b);

			} else if(strCompare(comName, "print")) {
				resultString = fillString("\"%s\"", args[0]);

			} else if(strCompare(comName, "cls")) {
				con->clearMainBuffer();
				pushResult = false;

			} else if(strCompare(comName, "doNothing")) {

			} else if(strCompare(comName, "setGuiAlpha")) {
				ds->guiAlpha = strToFloat(args[0]);

			} else if(strCompare(comName, "exit")) {
				*isRunning = false;

			}
			if(pushResult) con->pushToMainBuffer(resultString);
		}

		con->updateBody();

	}

	// Notifications.
	{
		// Update notes.
		int deletionCount = 0;
		for(int i = 0; i < ds->notificationCount; i++) {
			ds->notificationTimes[i] -= ds->dt;
			if(ds->notificationTimes[i] <= 0) {
				deletionCount++;
			}
		}

		// Delete expired notes.
		if(deletionCount > 0) {
			for(int i = 0; i < ds->notificationCount-deletionCount; i++) {
				ds->notificationStack[i] = ds->notificationStack[i+deletionCount];
				ds->notificationTimes[i] = ds->notificationTimes[i+deletionCount];
			}
			ds->notificationCount -= deletionCount;
		}

		// Draw notes.
		float fontSize = 24;
		Font* font = getFont(FONT_CALIBRI, fontSize);
		Vec4 color = vec4(1,0.5f,0,1);

		float y = -fontSize/2;
		for(int i = 0; i < ds->notificationCount; i++) {
			char* note = ds->notificationStack[i];
			dcText(note, font, vec2(ws->currentRes.w/2, y), color, vec2i(0,0), 0, 2);
			y -= fontSize;
		}
	}

	if(ds->showHud) {
		int fontSize = 19;
		int pi = 0;
		// Vec4 c = vec4(1.0f,0.2f,0.0f,1);
		Vec4 c = vec4(1.0f,0.4f,0.0f,1);
		Vec4 c2 = vec4(0,0,0,1);
		Font* font = getFont(FONT_CONSOLAS, fontSize);
		int sh = 1;
		Vec2 offset = vec2(6,6);
		Vec2i ali = vec2i(1,1);

		Vec2 tp = vec2(ad->wSettings.currentRes.x, 0) - offset;

		static f64 timer = 0;
		static int fpsCounter = 0;
		static int fps = 0;
		timer += ds->dt;
		fpsCounter++;
		if(timer >= 1.0f) {
			fps = fpsCounter;
			fpsCounter = 0;
			timer = 0;
		}

		// dcText(fillString("Fps  : %i", fps), font, tp, c, ali, 0, sh, c2); tp.y -= fontSize;
		// dcText(fillString("BufferIndex: %i",    ds->timer->bufferIndex), font, tp, c, ali, 0, sh, c2); tp.y -= fontSize;
		// dcText(fillString("LastBufferIndex: %i",ds->lastBufferIndex), font, tp, c, ali, 0, sh, c2); tp.y -= fontSize;

		for(int i = 0; i < ds->infoStackCount; i++) {
			dcText(fillString("%s", ds->infoStack[i]), font, tp, c, ali, 0, sh, c2); tp.y -= fontSize;
		}
		ds->infoStackCount = 0;
	}


	if(*isRunning == false) {
		guiSave(ds->gui, 2, 0);
		if(globalDebugState->gui2) guiSave(globalDebugState->gui2, 2, 3);
	}

	// Update debugTime every second.
	static f64 tempTime = 0;
	tempTime += ds->dt;
	if(tempTime >= 1) {
		ds->debugTime = timerUpdate(timeStamp);
		tempTime = 0;
	}
}

#pragma optimize( "", on ) 
