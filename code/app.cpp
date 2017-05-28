/*
====================================================================

	ToDo:
	- Filter -> percentage of views.
	- Standard deviation graph.
	- Avg. line of number videos of released in a month or so.
	- Two modes for windows border or self made border.
	- Make UI look more 3D instead of plain flat.
	- Fix layout system.
	- Show download speed.
	- Clamp by view count, like count and so on.
	- Detect double click.
	- Make panel its own window.
	- Make system to send multiple requests to save time.
	- Put in bold and italic font modes.

	- Put in server timeout.
	- Runs poorly when aero is enabled.
	- Get settings file working again.
	- Total cleanup of the code.
	- Text box settings add align left right text padding/offset.

	Done Today: 
	- Text edit offset. Text edit cursor hover. Started putting in settings sub menu. Code deletion. Bold and italic fonts. 

	Bugs:
	- Crash when server takes too long to respond.

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

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#include "external\stb_image_write.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "external\stb_rect_pack.h"

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
MemoryBlock* globalMemory;
DebugState* globalDebugState;
Timer* globalTimer;

// Internal.

#ifndef RELEASE_BUILD
	#define TIMER_BLOCKS_ENABLED
#endif 

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

float globalScreenHeight;
#include "rendering.cpp"
#include "gui.cpp"

#include "debug.cpp"


#define Graph_Zoom_Min 24*60*60
#define Graph_Zoom_MinNoDate 10
#define Window_Min_Size_X 500
#define Window_Min_Size_Y 500
#define Side_Panel_Min_Width 200

#define Graph_Min_Height 0.1f
#define Cam_Min_Height_0 1000
#define Cam_Min_Height_1 100
#define Cam_Min_Height_2 0.1
#define Cam_Min_Height_3 10

#define Graph_Zoom_Speed 1.2f

#define Cubic_Curve_Segment_Mod 20
#define Cubic_Curve_Segment_Min 4

#define App_Font_Folder "..\\data\\Fonts\\"
#define Windows_Font_Folder "C:\\Windows\\Fonts\\"
#define Default_Font "calibri.ttf"
#define Playlist_Folder "..\\playlists\\"

#define App_Folder "..\\app\\"
#define App_Settings_File "..\\app\\appSettings.txt"
#define App_Save_File "..\\app\\appSave.txt"
#define Screenshot_File "..\\app\\screenshot.png"

#define Panel_Min_X 300
#define Panel_Min_Y 300

#define Screenshot_Min_X 500
#define Screenshot_Min_Y 500

#define Line_Graph_Count 4




void loadCurlFunctions(HMODULE dll) {
	curl_global_initX = (curl_global_initFunction*)GetProcAddress(dll, "curl_global_init");
	curl_easy_initX = (curl_easy_initFunction*)GetProcAddress(dll, "curl_easy_init");
	curl_easy_setoptX = (curl_easy_setoptFunction*)GetProcAddress(dll, "curl_easy_setopt");
	curl_easy_performX = (curl_easy_performFunction*)GetProcAddress(dll, "curl_easy_perform");
	curl_easy_cleanupX = (curl_easy_cleanupFunction*)GetProcAddress(dll, "curl_easy_cleanup");
}

struct CurlRequestData {
	CURL* curlHandle;
	char* request;
	
	char* buffer;
	int size;
	int maxSize;

	i64 bytesDownloaded;
	i64 bytesTotal;

	bool abort;
	bool active;
	bool finished;
};

void curlRequestDataInit(CurlRequestData* requestData, char* request) {
	requestData->request = request;
	requestData->size = 0;
	requestData->abort = false;
	requestData->finished = false;
}

int curlProgress(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
	CurlRequestData* requestData = (CurlRequestData*)clientp;

	requestData->bytesDownloaded = dlnow;
	requestData->bytesTotal = dltotal;

	if(requestData->abort) {
		return -1;
	}

	return 0;
}

size_t curlWriteThreaded(void *buffer, size_t size, size_t nmemb, void *userp) {
	CurlRequestData* cd = (CurlRequestData*)userp;

	myAssert(cd->size + nmemb < cd->maxSize);
	memCpy(cd->buffer + cd->size, buffer, nmemb);
	cd->size += nmemb;
	cd->buffer[cd->size] = '\0';

	return nmemb;
}

void curlRequest(void* data) {
	CurlRequestData* request = (CurlRequestData*)data;

	request->active = true;

	curl_easy_setoptX(request->curlHandle, CURLOPT_WRITEDATA, request);
	curl_easy_setoptX(request->curlHandle, CURLOPT_URL, request->request);
	curl_easy_setoptX(request->curlHandle, CURLOPT_WRITEFUNCTION, curlWriteThreaded);

	curl_easy_setoptX(request->curlHandle, CURLOPT_NOPROGRESS, 0);
	curl_easy_setoptX(request->curlHandle, CURLOPT_XFERINFODATA, request);
	curl_easy_setoptX(request->curlHandle, CURLOPT_XFERINFOFUNCTION, curlProgress);

	curl_easy_setoptX(request->curlHandle, CURLOPT_TCP_FASTOPEN, 1);
	curl_easy_setoptX(request->curlHandle, CURLOPT_TCP_KEEPALIVE, 1);

	int success = curl_easy_performX(request->curlHandle);

	if(request->abort) request->abort = false;

	bool fail = (success != CURLE_ABORTED_BY_CALLBACK && success != 0);
	myAssert(!fail);

	request->active = false;
	request->finished = true;
}

void curlRequestDataInitAdd(ThreadQueue* threadQueue, CurlRequestData* requestData, char* request) {
	curlRequestDataInit(requestData, request);
	threadQueueAdd(threadQueue, curlRequest, requestData);
}



bool isLeapYear(int year, bool addFrontDigits = true) {
	if(addFrontDigits) year += 2000;

	if(year % 4 != 0) return false;
	else if(year % 100 != 0) return true;
	else if(year % 400 != 0) return false;
	else return true;
}

int getYearDayCount(int year) {
	return isLeapYear(year)?366:365;
}

int getMonthDayCount(int month, int year) {
	static int monthDayCount[] = {31,28,31,30,31,30,31,31,30,31,30,31};

	int result = monthDayCount[month-1];
	if(isLeapYear(year) && month == 2) result++;

	return result;
}

struct Date {
	char y, m, d;
	char h, mins, secs;
	i64 n;
};

i64 dateEncode(char year, char month, char day, char hour, char minute, char second) {

	i64 yearSeconds = 0;
	for(int i = 0; i < year; i++) yearSeconds += (i64)getYearDayCount(i)*86400;

	i64 monthSeconds = 0;
	for(int i = 0; i < month-1; i++) monthSeconds += (i64)getMonthDayCount(i+1, year)*86400;

	i64 result = 0;
	result += yearSeconds;
	result += monthSeconds;
	if(day > 0) result += (i64)(day-1)*86400;
	result += (i64)hour*3600;
	result += (i64)minute*60;
	result += (i64)second;

	return result;
}

void dateDecode(i64 date, char* year, char* month, char* day, char* hour, char* minute, char* second) {
	i64 n = date;

	int tempYear = 0;
	for(;;) {
		i64 yearSeconds = (i64)getYearDayCount(tempYear)*86400;
		if(n >= yearSeconds) {
			tempYear++;
			n -= yearSeconds;
		} else break;
	}

	int tempMonth = 0;
	for(;;) {
		i64 monthSeconds = (i64)getMonthDayCount(tempMonth+1, tempYear)*86400;

		if(n >= monthSeconds) {
			tempMonth++;
			n -= monthSeconds;
		} else break;
	}

	(*year) = tempYear;
	(*month) = tempMonth+1;
	(*day) = n/86400 + 1; n %= 86400;
	(*hour) = n/3600; n %= 3600;
	(*minute) = n/60; n %= 60;
	(*second) = n;
}

void dateEncode(Date* d) {
	d->n = dateEncode(d->y, d->m, d->d, d->h, d->mins, d->secs);
}

void dateDecode(Date* d) {
	dateDecode(d->n, &d->y, &d->m, &d->d, &d->h, &d->mins, &d->secs);
}

Date dateDecode(i64 n) {
	Date d;
	dateDecode(n, &d.y, &d.m, &d.d, &d.h, &d.mins, &d.secs);
	return d;
}

Date stringToDate(char* s) {
	// Example: 2017-02-01T12:03:23.000Z

	Date date;
	char* buffer = s;
	char* d = getTString(10); 

	strCpy(d, buffer, 4); buffer += 5; date.y = strToInt(d) - 2000;
	strCpy(d, buffer, 2); buffer += 3; date.m = strToInt(d);
	strCpy(d, buffer, 2); buffer += 3; date.d = strToInt(d);
	strCpy(d, buffer, 2); buffer += 3; date.h = strToInt(d);
	strCpy(d, buffer, 2); buffer += 3; date.mins = strToInt(d);
	strCpy(d, buffer, 2); buffer += 3; date.secs = strToInt(d);

	dateEncode(&date);

	return date;
}

Date stringDurationToDate(char* s) {
	// Example: P1DT1S or PT1H32M5S
	
	Date date = {};
	char* buffer = s;
	char* d = getTString(10); 

	// We assume "PT" at start.
	char* b = s+2; 
	while(*b != '\0') {
		int pos = parseNumber(b);
		char ns[4]; strCpy(ns, b, pos);
		int n = strToInt(ns);
		b += pos;

		char t = *b;
		if(t == 'H') date.h = n;
		else if(t == 'M') date.mins = n;
		else if(t == 'S') date.secs = n;
		b++;
	}

	dateEncode(&date);

	return date;
}

// Mode 0 = years, Mode 1 = months, Mode 2 = days, Mode 3 = hours.
void dateIncrement(Date* date, int mode) {
	if(mode == 0) {
		date->y++;
	} else if(mode == 1) {
		date->m++;
		if(date->m > 12) {
			date->m = 1;
			dateIncrement(date, mode-1);
		}
	} else if(mode == 2) {
		date->d++;
		int daysOfMonth = getMonthDayCount(date->m, date->y);
		if(date->d > daysOfMonth) {
			date->d = 1;
			dateIncrement(date, mode-1);
		}
	} else if(mode == 3) {
		date->h++;
		if(date->h >= 24) {
			date->h = 0;
			dateIncrement(date, mode-1);
		}
	}
}

// Really?
void dateIncrement(Date* date, int count, int mode) {
	for(int i = 0; i < count; i++) dateIncrement(date, mode);
}

i64 monthsToInt(float months) {
	i64 result = (i64)(months*2629744.0f);
	return result;
}



#pragma pack(push,1)
struct YoutubeVideo {
	char id[12];
	char title[151];
	char thumbnail[51];
	char dateString[25]; // 2017-02-01T12:03:23.000Z
	Date date;
	Date duration;

	int viewCount;
	int likeCount;
	int dislikeCount;
	int favoriteCount;
	int commentCount;
};
#pragma pack(pop)

struct YoutubePlaylist {
	char id[40];
	char title[151];
	int count;
	int maxCount;
};

struct SearchResult {
	char title[151];
	char id[40];
	int count;
	char allPlaylistId[40];
};

struct VideoSnippet {
	bool selectedLoaded;
	char* selectedTopComments[10];
	int selectedCommentLikeCount[10];
	int selectedCommentReplyCount[10];
	int selectedCommentCount;
	Texture thumbnailTexture;
};

#define Thumbnail_Size_X 480
#define Thumbnail_Size_Y 360

#define Max_Download_Count 40
#define Max_Playlist_Download_Count 10
#define Search_Result_Count 10
#define Page_Token_Size 10

inline double videoGetLikesDiff(YoutubeVideo* vid) {
	return divZero(vid->dislikeCount, (vid->likeCount + vid->dislikeCount));
}

#define Youtube_Api_Key "AIzaSyD-qRen5fSH7M3ePBey1RY0vRTyW0PKyLw"
#define Youtube_Api_Playlist_Items "https://www.googleapis.com/youtube/v3/playlistItems"
#define Youtube_Api_Playlist "https://www.googleapis.com/youtube/v3/playlists"
#define Youtube_Api_Videos "https://www.googleapis.com/youtube/v3/videos"
#define Youtube_Api_Comment_Thread "https://www.googleapis.com/youtube/v3/commentThreads"
#define Youtube_Api_Channel "https://www.googleapis.com/youtube/v3/channels"
#define Youtube_Api_Search "https://www.googleapis.com/youtube/v3/search"

char* requestPlaylistItems(char* playlistId, int maxResults, char* pageToken = 0) {
	return fillString("%s?key=%s&maxResults=%i&playlistId=%s&part=contentDetails%s", 
					  Youtube_Api_Playlist_Items, Youtube_Api_Key, maxResults, playlistId, pageToken?fillString("&pageToken=%s",pageToken):"");
}
char* requestCommentThread(char* videoId, int maxResults) {
	return fillString("%s?key=%s&maxResults=%i&videoId=%s&part=snippet&order=relevance&=textFormat=plainText", 
						Youtube_Api_Comment_Thread, Youtube_Api_Key, maxResults, videoId);
}
char* requestPlaylist(char* idType, char* id, char* part, int maxResults, char* pageToken = 0) {
	return fillString("%s?key=%s&maxResults=%i&%s=%s&part=%s%s", 
								Youtube_Api_Playlist, Youtube_Api_Key, maxResults, idType, id, part, pageToken?fillString("&pageToken=%s",pageToken):"");
}
char* requestChannel(char* idType, char* id, char* part, int maxResults) {
	return fillString("%s?key=%s&maxResults=%i&%s=%s&part=%s", 
								Youtube_Api_Channel, Youtube_Api_Key, maxResults, idType, id, part);
}
char* requestVideos(char* id, char* part) {
	return fillString("%s?key=%s&id=%s&part=%s", 
								Youtube_Api_Videos, Youtube_Api_Key, id, part);
}
char* requestSearch(char* type, char* searchString, int maxResults) {
	return fillString("%s?key=%s&maxResults=%i&part=snippet&type=%s&q=%s", 
								Youtube_Api_Search, Youtube_Api_Key, maxResults, type, searchString);
}



char* getPlaylistFilePath(char* playlistId) {
	return fillString("%s%s.playlist", Playlist_Folder, playlistId);
}

void savePlaylistToFile(YoutubePlaylist* playlist, YoutubeVideo* videos, int videoCount, int maxVideoCount, int oldVideoCount, char* pageToken) {
	char* filePath = fillString("%s%s.playlist", Playlist_Folder, playlist->id);

	FILE* file;
	if(oldVideoCount == 0) {
		file = fopen(filePath, "wb");
	} else if(oldVideoCount > 0) {
		file = fopen(filePath, "rb+");
		fseek(file, 0, SEEK_SET);
	} else if(oldVideoCount == -1) {
		file = fopen(filePath, "rb+");
		fseek(file, 0, SEEK_SET);
	}

	fwrite(&playlist->title, memberSize(YoutubePlaylist, title), 1, file);
	fwrite(&playlist->id, memberSize(YoutubePlaylist, id), 1, file);
	int currentCount = videoCount + oldVideoCount;
	if(oldVideoCount != -1) {
		fwrite(&currentCount, sizeof(int), 1, file);
	} else {
		oldVideoCount = 0;
		fseek(file, sizeof(int), SEEK_CUR);
	}
	fwrite(&maxVideoCount, sizeof(int), 1, file);
	fwrite(pageToken, Page_Token_Size, 1, file);

	fseek(file, (maxVideoCount-videoCount-oldVideoCount)*sizeof(YoutubeVideo), SEEK_CUR);
	fwrite(videos, videoCount*sizeof(YoutubeVideo), 1, file);

	fclose(file);
}

bool loadPlaylistFromFile(YoutubePlaylist* playlist, YoutubeVideo* videos, int* videoCount, int* maxVideoCount = 0, char* pageToken = 0) {
	char* filePath = fillString("%s%s.playlist", Playlist_Folder, playlist->id);
	FILE* file = fopen(filePath, "rb");
	if(file) {
		fread(&playlist->title, memberSize(YoutubePlaylist, title), 1, file);
		fread(&playlist->id, memberSize(YoutubePlaylist, id), 1, file);
		fread(videoCount, sizeof(int), 1, file);

		int pos;
		pos = ftell(file);

		int tempMaxCount;
		if(maxVideoCount == 0) fread(&tempMaxCount, sizeof(int), 1, file);
		else fread(&maxVideoCount, sizeof(int), 1, file);
		pos = ftell(file);
		if(pageToken == 0) fseek(file, Page_Token_Size, SEEK_CUR);
		else fread(&pageToken, Page_Token_Size, 1, file);
		pos = ftell(file);

		if(maxVideoCount == 0) fseek(file, (tempMaxCount-(*videoCount))*sizeof(YoutubeVideo), SEEK_CUR);
		else fseek(file, ((*maxVideoCount)-(*videoCount))*sizeof(YoutubeVideo), SEEK_CUR);
		pos = ftell(file);

		fread(videos, (*videoCount)*sizeof(YoutubeVideo), 1, file);
		pos = ftell(file);
		fclose(file);

		return true;
	}

	return false;
}

bool loadPlaylistHeaderFromFile(YoutubePlaylist* playlist, char* fileName, int* maxVideoCount = 0, char* pageToken = 0) {
	char* filePath = fillString("%s%s", Playlist_Folder, fileName);
	FILE* file = fopen(filePath, "rb");
	if(file) {
		fread(&playlist->title, memberSize(YoutubePlaylist, title), 1, file);
		fread(&playlist->id, memberSize(YoutubePlaylist, id), 1, file);
		fread(&playlist->count, sizeof(int), 1, file);

		int maxCount;
		fread(&maxCount, sizeof(int), 1, file);
		playlist->maxCount = maxCount;
		if(maxVideoCount != 0) {
			*maxVideoCount = maxCount;
		}
		if(pageToken == 0) fseek(file, Page_Token_Size, SEEK_CUR);
		else fread(pageToken, Page_Token_Size, 1, file);

		fclose(file);

		return true;
	}

	return false;
}

#pragma pack(push,1)
struct PlaylistFileHeader {
	char title[151];
	char id[40];
	int count;
	int maxCount;
	char pageToken[Page_Token_Size];
};
#pragma pack(pop)

PlaylistFileHeader loadPlaylistHeaderFromFileX(char* playlistId) {
	PlaylistFileHeader header;

	FILE* file = fopen(getPlaylistFilePath(playlistId), "rb+");
	if(file) {
		fseek(file, 0, SEEK_SET);
		fread(&header, sizeof(PlaylistFileHeader), 1, file);
		fclose(file);
	}

	return header;
}

void loadPlaylistFolder(YoutubePlaylist* playlists, int* playlistCount) {
	char* folderPath = fillString("%s*",Playlist_Folder);
	WIN32_FIND_DATA findData; 
	HANDLE folderHandle = FindFirstFile(folderPath, &findData);

	if(INVALID_HANDLE_VALUE != folderHandle) {
		(*playlistCount) = 0;
		for(;;) {
			bool result = FindNextFile(folderHandle, &findData);
			if(!result) break;

			char* fileName = findData.cFileName;

			int pos = strFind(fileName, ".playlist");
			if(pos != -1) {
				loadPlaylistHeaderFromFile(&playlists[*playlistCount], fileName);
				(*playlistCount)++;
			}
		}
	}
}

void removePlaylistFile(YoutubePlaylist* playlist) {
	char* filePath = fillString("%s%s.playlist", Playlist_Folder, playlist->id);
	remove(filePath);
}



struct LineGraph {
	double* points[Line_Graph_Count];
	int count;
};

void calculateAverages(YoutubeVideo* videos, int videoCount, LineGraph* lineGraph, float timeSpanInMonths, float widthInMonths, double minMaxWidth, LineGraph* countLineGraph) {
	TIMER_BLOCK();

	double avgStartX = videos[0].date.n;
	double avgWidth = monthsToInt(widthInMonths);
	double avgTimeSpan = monthsToInt(timeSpanInMonths);

	int statCount = (minMaxWidth/avgWidth) + 2;
	lineGraph->count = statCount;
	countLineGraph->count = statCount;
	for(int i = 0; i < Line_Graph_Count; i++) {
		if(lineGraph->points[i] != 0) free(lineGraph->points[i]);
		lineGraph->points[i] = (double*)malloc(statCount*sizeof(double));

		if(countLineGraph->points[i] != 0) free(countLineGraph->points[i]);
		countLineGraph->points[i] = (double*)malloc(statCount*sizeof(double));
	}

	Statistic* stats[Line_Graph_Count];
	for(int i = 0; i < Line_Graph_Count; i++) stats[i] = getTArray(Statistic, statCount);

	for(int i = 0; i < statCount; i++) {
		for(int gi = 0; gi < Line_Graph_Count; gi++) beginStatistic(stats[gi] + i);
	}

	for(int i = 1; i < videoCount; i++) {
		YoutubeVideo* vid = videos + i;
		double timeX = vid->date.n;

		for(int si = 0; si < statCount; si++) {
			double avgMidX = avgStartX + (si*avgWidth);
			if((avgMidX - avgTimeSpan/2) <= timeX && timeX <= (avgMidX + avgTimeSpan/2)) {

				updateStatistic(&stats[0][si], vid->viewCount);
				updateStatistic(&stats[1][si], vid->likeCount + vid->dislikeCount);
				updateStatistic(&stats[2][si], videoGetLikesDiff(vid));
				updateStatistic(&stats[3][si], vid->commentCount);
			}
		}
	}

	for(int i = 0; i < statCount; i++) {
		for(int gi = 0; gi < Line_Graph_Count; gi++) {
			endStatistic(stats[gi] + i);
			lineGraph->points[gi][i] = stats[gi][i].count > 0 ? stats[gi][i].avg : 0;

			countLineGraph->points[gi][i] = stats[gi][i].count;
		}
	}
}

struct DownloadInfo {
	CURL* curlHandle;
	char* buffer;
	char* tempBuffer;

	char* stringBuffer;

	YoutubeVideo* vids;
	char* playlistId;
	int totalCount;
	int count;
	char pageToken[Page_Token_Size];

	int lastVideoCount;
	int maxVideoCount;
	bool continuedDownload;

	int progressIndex;
	bool finishedDownload;
};



enum GuiFocus {
	Gui_Focus_MLeft = 0,
	Gui_Focus_MRight,
	Gui_Focus_MMiddle,
	Gui_Focus_MWheel,
	Gui_Focus_MPos,

	Gui_Focus_Size,
};

struct TextEditVars {
	int cursorIndex;
	int markerIndex;
	Vec2 scrollOffset;

	bool cursorChanged;
};

struct TextBoxSettings {
	Font* font;
	float shadow;
	Vec4 textColor;
	Vec4 shadowColor;

	Vec4 backgroundColor;
	float roundedCorner;

	Vec4 borderColor;
};

struct TextEditSettings {
	TextBoxSettings textBoxSettings;
	char* textBuffer;

	bool wrapping;
	bool singleLine;

	float cursorWidth;

	char* defaultText;

	Vec4 colorSelection;
	Vec4 colorCursor;

	float textOffset;
};

struct ScrollRegionSettings {
	Vec2 border;
	float scrollBarWidth;
	Vec2 sliderMargin;
	float sliderRounding;

	float sliderSize;
	float sliderSizeMin;

	float scrollAmount;

	bool scrollWithBackground;
	bool scrollWithSlider;
	bool scrollWithMouseWheel;

	bool fixedHeight;

	Vec4 sliderColor;
	Vec4 scrollBarColor;
	Vec4 backgroundColor;
	Vec4 borderColor;
};

struct SliderSettings {
	TextBoxSettings textBoxSettings;

	float size;
	float minSize;
	float lineWidth;
	float rounding;
	float heightOffset;

	Vec4 color;
	Vec4 lineColor;
};

struct LayoutData {
	Rect region;
	Vec2 pos;
	Vec2 dim;
	float defaultHeight;
	float yPadding;
	float yPaddingExtra;
};

struct NewGui {
	int id;

	int activeId;
	int gotActiveId;
	int wasActiveId;
	
	int hotId[Gui_Focus_Size];
	int contenderId[Gui_Focus_Size];
	int contenderIdZ[Gui_Focus_Size];

	Input* input;
	WindowSettings* windowSettings;

	// Temp vars for convenience.

	Vec2 mouseAnchor, mouseAnchor2;
	int mode;

	char editText[100];
	int editInt;
	float editFloat;
	int editMaxSize;

	TextEditVars editVars;

	//

	int zLevel;

	TextBoxSettings textBoxSettings;
	TextBoxSettings buttonSettings;
	TextEditSettings editSettings;
	SliderSettings sliderSettings;
	ScrollRegionSettings scrollSettings;

	Rect scissor;
	Rect scissorStack[10];
	int scissorStackIndex;

	LayoutData* ld;
	LayoutData layoutStack[10];
	int layoutStackIndex;
};

void newGuiBegin(NewGui* gui, Input* input = 0) {
	gui->id = 1;
	gui->gotActiveId = 0;
	gui->wasActiveId = 0;

	for(int i = 0; i < Gui_Focus_Size; i++) {
		gui->contenderId[i] = 0;
		gui->contenderIdZ[i] = 0;
	}

	if(gui->activeId != 0) {
		for(int i = 0; i < Gui_Focus_Size; i++) gui->hotId[i] = 0;
	}

	gui->input = input;

	gui->scissor = rectCenDim(0,0,10000000,10000000);
	gui->scissorStack[0] = gui->scissor;
	gui->scissorStackIndex = 0;
	gui->layoutStackIndex = 0;
}

void newGuiEnd(NewGui* gui) {
	for(int i = 0; i < Gui_Focus_Size; i++) {
		gui->hotId[i] = gui->contenderId[i];
	}
}

int newGuiIncrementId(NewGui* gui) {
	return gui->id++;
}

int newGuiCurrentId(NewGui* gui) {
	return gui->id-1;
}

bool newGuiIsHot(NewGui* gui, int id = 0, int focus = 0) {
	return (id==0?newGuiCurrentId(gui):id) == gui->hotId[focus];
}

bool newGuiIsActive(NewGui* gui, int id = 0) {
	return (id==0?newGuiCurrentId(gui):id) == gui->activeId;
}

bool newGuiIsHotOrActive(NewGui* gui, int id = 0, int focus = 0) {
	return newGuiIsHot(gui, id, focus) || newGuiIsActive(gui, id);
}

bool newGuiGotActive(NewGui* gui, int id = 0) {
	return (id==0?newGuiCurrentId(gui):id) == gui->gotActiveId;
}

bool newGuiWasActive(NewGui* gui, int id = 0) {
	return (id==0?newGuiCurrentId(gui):id) == gui->wasActiveId;
}

bool newGuiIsWasHotOrActive(NewGui* gui, int id = 0, int focus = 0) {
	return newGuiIsHot(gui, id, focus) || newGuiIsActive(gui, id) || newGuiWasActive(gui, id);
}

void newGuiClearActive(NewGui* gui) {
	gui->activeId = 0;
}

void newGuiSetNotActive(NewGui* gui, int id) {
	newGuiClearActive(gui);
	gui->wasActiveId = id;
}

bool newGuiSomeoneActive(NewGui* gui) {
	return gui->activeId != 0;
}

void newGuiSetNotActiveWhenActive(NewGui* gui, int id) {
	if(newGuiIsActive(gui, id)) newGuiClearActive(gui);
}

void newGuiSetActive(NewGui* gui, int id, bool input, int focus = 0) {
	if(input && newGuiIsHot(gui, id, focus)) {
		gui->activeId = id;
		gui->gotActiveId = id;
	}
}

void newGuiSetHot(NewGui* gui, int id, float z, int focus = 0) {
	if(!newGuiSomeoneActive(gui)) {
		if(z > gui->contenderIdZ[focus]) {
			gui->contenderId[focus] = id;
			gui->contenderIdZ[focus] = z;
		} else {
			if(z == gui->contenderIdZ[focus]) {
				gui->contenderId[focus] = max(id, gui->contenderId[focus]);
			}
		}
	}
}

void newGuiSetHotAll(NewGui* gui, int id, float z) {
	for(int i = 0; i < Gui_Focus_Size; i++) {
		newGuiSetHot(gui, id, z, i);
	}
}

void newGuiSetHotAll(NewGui* gui, float z) {
	return newGuiSetHotAll(gui, newGuiIncrementId(gui), z);
}

void newGuiSetHotAllMouseOver(NewGui* gui, int id, Rect r, float z) {
	if(pointInRectEx(gui->input->mousePosNegative, r)) return newGuiSetHotAll(gui, id, z);
}

void newGuiSetHotAllMouseOver(NewGui* gui, Rect r, float z) {
	return newGuiSetHotAllMouseOver(gui, newGuiIncrementId(gui), r, z);
}

void newGuiSetHotMouseOver(NewGui* gui, int id, Vec2 mousePos, Rect r, float z, int focus = 0) {
	if(pointInRectEx(mousePos, r)) {
		newGuiSetHot(gui, id, z, focus);
	}
}

int newGuiInputFromFocus(Input* input, int focus, bool press = true) {
	switch(focus) {
		case Gui_Focus_MLeft: return press?input->mouseButtonPressed[0]:input->mouseButtonReleased[0];
		case Gui_Focus_MRight: return press?input->mouseButtonPressed[1]:input->mouseButtonReleased[1];
		case Gui_Focus_MMiddle: return press?input->mouseButtonPressed[2]:input->mouseButtonReleased[2];

		case Gui_Focus_MWheel: return input->mouseWheel != 0;
		default: return -1;
	}
}

void newGuiSetCursor(NewGui* gui, LPCSTR cursorType) {
	setCursor(gui->windowSettings, cursorType);
}


int newGuiButtonAction(NewGui* gui, int id, Rect r, float z, Vec2 mousePos, bool input, int focus = 0) {
	newGuiSetActive(gui, id, input, focus);
	if(newGuiIsActive(gui, id)) newGuiSetNotActive(gui, id);
	newGuiSetHotMouseOver(gui, id, mousePos, r, z, focus);

	return id;
}
int newGuiButtonAction(NewGui* gui, Rect r, float z, Vec2 mousePos, bool input, int focus = 0) {
	return newGuiButtonAction(gui, newGuiIncrementId(gui), r, z, mousePos, input, focus);
}
int newGuiButtonAction(NewGui* gui, int id, Rect r, float z, int focus = 0) {
	bool input = newGuiInputFromFocus(gui->input, focus, true);
	return newGuiButtonAction(gui, id, r, z, gui->input->mousePosNegative, input, focus);
}
int newGuiButtonAction(NewGui* gui, Rect r, float z, int focus = 0) {
	return newGuiButtonAction(gui, newGuiIncrementId(gui), r, z, focus);
}

bool newGuiGoButtonAction(NewGui* gui, Rect r, float z, int focus = 0) {
	int id = newGuiButtonAction(gui, r, z, focus);
	bool gotActive = newGuiGotActive(gui, id);
	return gotActive;
}
bool newGuiGoButtonAction(NewGui* gui, int id, Rect r, float z, int focus = 0) {
	id = newGuiButtonAction(gui, id, r, z, focus);
	bool gotActive = newGuiGotActive(gui, id);
	return gotActive;
}

int newGuiDragAction(NewGui* gui, int id, Rect r, float z, Vec2 mousePos, bool input, bool inputRelease, int focus = 0) {
	newGuiSetActive(gui, id, input, focus);
	if(newGuiIsActive(gui, id) && inputRelease) newGuiSetNotActive(gui, id);
	newGuiSetHotMouseOver(gui, id, mousePos, r, z, focus);

	return id;
}
int newGuiDragAction(NewGui* gui, Rect r, float z, Vec2 mousePos, bool input, bool inputRelease, int focus = 0) {
	return newGuiDragAction(gui, newGuiIncrementId(gui), r, z, mousePos, input, inputRelease, focus);
}
int newGuiDragAction(NewGui* gui, Rect r, float z, int focus = 0) {
	bool input = newGuiInputFromFocus(gui->input, focus, true);
	bool inputRelease = newGuiInputFromFocus(gui->input, focus, false);
	return newGuiDragAction(gui, newGuiIncrementId(gui), r, z, gui->input->mousePosNegative, input, inputRelease, focus);
}

int newGuiGoDragAction(NewGui* gui, Rect r, float z, bool input, bool inputRelease, int focus = 0, bool screenMouse = false) {
	Vec2 mousePos = screenMouse ? gui->input->mousePosNegativeScreen : gui->input->mousePosNegative;
	int id = newGuiDragAction(gui, r, z, mousePos, input, inputRelease, focus);
	if(newGuiGotActive(gui, id)) return 1;
	if(newGuiIsActive(gui, id)) return 2;
	if(newGuiWasActive(gui, id)) return 3;
	
	return 0;
}
int newGuiGoDragAction(NewGui* gui, Rect r, float z, int focus = 0, bool screenMouse = false) {
	bool input = newGuiInputFromFocus(gui->input, focus, true);
	bool inputRelease = newGuiInputFromFocus(gui->input, focus, false);
	return newGuiGoDragAction(gui, r, z, input, inputRelease, focus, screenMouse);
}

bool newGuiGoMousePosAction(NewGui* gui, Rect r, float z) {
	newGuiSetHotMouseOver(gui, newGuiIncrementId(gui), gui->input->mousePosNegative, r, z, Gui_Focus_MPos);
	bool hot = newGuiIsHot(gui, newGuiCurrentId(gui), Gui_Focus_MPos);
	return hot;
}


Vec4 newGuiHotActiveColorMod(bool isHot, bool isActive) {
	Vec4 colorMod = vec4(0,0,0,0);
	if(isHot) colorMod = vec4(0.08f,0); 
	if(isActive) colorMod = vec4(0.17f,0); 

	return colorMod;
}
// We assume you got an id first before calling this.
Vec4 newGuiColorModId(NewGui* gui, int id, int focus = 0) {
	return newGuiHotActiveColorMod(newGuiIsHot(gui, id, focus), newGuiIsActive(gui, id));
}
Vec4 newGuiColorModBId(NewGui* gui, int id, int focus = 0) {
	return newGuiHotActiveColorMod(newGuiIsHot(gui, id, focus), newGuiGotActive(gui, id));
}
Vec4 newGuiColorMod(NewGui* gui, int focus = 0) {
	return newGuiColorModId(gui, newGuiCurrentId(gui), focus);
}
Vec4 newGuiColorModB(NewGui* gui, int focus = 0) {
	return newGuiColorModBId(gui, newGuiCurrentId(gui), focus);
}



TextBoxSettings textBoxSettings(Font* font, Vec4 backgroundColor, Vec4 textColor, float shadow, Vec4 shadowColor, float roundedCorners, Vec4 borderColor) {
	return {font, shadow, textColor, shadowColor, backgroundColor, roundedCorners, borderColor};
}
TextBoxSettings textBoxSettings(Font* font, Vec4 backgroundColor, Vec4 textColor, float shadow, Vec4 shadowColor) {
	return textBoxSettings(font, backgroundColor, textColor, shadow, shadowColor, 0, vec4(0,0));
}
TextBoxSettings textBoxSettings(Font* font, Vec4 backgroundColor, Vec4 textColor) {
	return {font, 0, textColor, vec4(0,0), backgroundColor, 0};
}
TextBoxSettings textBoxSettings(Font* font, Vec4 textColor) {
	return textBoxSettings(font, vec4(0,0), textColor);
}



void textEditBox(char* text, int textMaxSize, Font* font, Rect textRect, Input* input, Vec2i align, TextEditSettings tes, TextEditVars* tev);


int newGuiGoTextEdit(NewGui* gui, Rect textRect, float z, void* var, Font* font, TextEditSettings editSettings, TextEditVars* editVars, int maxTextSize, int mode) {
	Input* input = gui->input;

	maxTextSize = min(maxTextSize, arrayCount(gui->editText));

	bool leftMouse = input->mouseButtonPressed[0] && !pointInRectEx(input->mousePosNegative, textRect);
	bool enter = input->keysPressed[KEYCODE_RETURN];
	bool escape = input->keysPressed[KEYCODE_ESCAPE];

	bool releaseEvent = leftMouse || enter || escape;

	int event = newGuiGoDragAction(gui, textRect, z, input->mouseButtonPressed[0], releaseEvent, Gui_Focus_MLeft);

	if(event == 1) {
		gui->editVars.scrollOffset = vec2(0,0);
		if(mode == 0)      strCpy(gui->editText, (char*)var);
		else if(mode == 1) strCpy(gui->editText, fillString("%i", *((int*)var)));
		else if(mode == 2) strCpy(gui->editText, fillString("%f", *((float*)var)));
	}

	// if(event == 3 && (leftMouse || enter)) {
	if(event == 3 && (enter)) {
		if(mode == 0)      strCpy((char*)var, gui->editText);
		else if(mode == 1) *((int*)var) = strToInt(gui->editText);
		else if(mode == 2) *((float*)var) = strToFloat(gui->editText);
	}

	if(event == 1 || event == 2) {
		textEditBox(gui->editText, maxTextSize, font, textRect, input, vec2i(-1,1), editSettings, editVars);
	}

	if(event == 3 && (escape || leftMouse)) event = 4;

	return event;
}
int newGuiGoTextEdit(NewGui* gui, Rect textRect, float z, char* text, Font* font, TextEditSettings editSettings, int maxTextSize) {
	return newGuiGoTextEdit(gui, textRect, z, text, font, editSettings, &gui->editVars, maxTextSize, 0);
}
int newGuiGoTextEdit(NewGui* gui, Rect textRect, float z, int* number, Font* font, TextEditSettings editSettings) {
	return newGuiGoTextEdit(gui, textRect, z, number, font, editSettings, &gui->editVars, arrayCount(gui->editText), 1);
}
int newGuiGoTextEdit(NewGui* gui, Rect textRect, float z, float* number, Font* font, TextEditSettings editSettings) {
	return newGuiGoTextEdit(gui, textRect, z, number, font, editSettings, &gui->editVars, arrayCount(gui->editText), 2);
}



Rect newGuiCalcSlider(float value, Rect br, float size, float min, float max, bool vertical) {
	if(vertical) {
		float sliderPos = mapRange(value, min, max, br.min.x + size/2, br.max.x - size/2);
		Rect slider = rectCenDim(sliderPos, rectCen(br).y, size, rectH(br));
		return slider;
	} else {
		float sliderPos = mapRange(value, min, max, br.min.y + size/2, br.max.y - size/2);
		Rect slider = rectCenDim(rectCen(br).x, sliderPos, rectW(br), size);
		return slider;
	}
}

float newGuiSliderGetValue(Vec2 sliderCenter, Rect br, float size, float min, float max, bool vertical) {
	if(vertical) {
		float sliderValue = mapRangeClamp(sliderCenter.x, br.min.x + size/2, br.max.x - size/2, min, max);
		return sliderValue;
	} else {
		float sliderValue = mapRangeClamp(sliderCenter.y, br.min.y + size/2, br.max.y - size/2, min, max);
		return sliderValue;
	}
}

void newGuiDiv(float width, float* c, int size, float offset = 0) {
	float dynamicSum = 0;
	int flowCount = 0;
	float staticSum = 0;
	int staticCount = 0;
	for(int i = 0; i < size; i++) {
		float val = c[i];
		
		if(val == 0) flowCount++; 			 // float element
		else if(val <= 1) dynamicSum += val; // dynamic element
		else { 								 // static element
			staticSum += val;
			staticCount++;
		}
	}

	if(flowCount) {
		float flowVal = abs(dynamicSum-1)/(float)flowCount;
		for(int i = 0; i < size; i++)
			if(c[i] == 0) c[i] = flowVal;
	}

	float totalWidth = width - staticSum - offset*(size-1);
	float sum = 0;
	for(int i = 0; i < size; i++) {
		if(sum > width) {
			c[i] = 0;
			continue;
		}

		float val = c[i];
		if(val <= 1) {
			if(totalWidth > 0) c[i] = val * totalWidth;
			else c[i] = 0;
		} else c[i] = val;

		float added = c[i] + offset;
		if(sum + added > width) {
			c[i] = width - sum;
			sum += added;
		}
		else sum += added;
	}
}

void newGuiRectsFromWidths(Rect r, float* widths, int size, Rect* rects, float offset = 0) {
	float yPos = rectCen(r).y;
	float h = rectH(r);
	float sum = 0;
	for(int i = 0; i < size; i++) {
		float xPos = r.left + sum + widths[i]/2;
		rects[i] = rectCenDim(xPos, yPos, widths[i], h);

		sum += widths[i] + offset;
	}
}

//

bool getRectScissor(Rect* scissor, Rect r) {
	*scissor = rectIntersect(*scissor, r);
	if(rectEmpty(*scissor)) return false;
	return true;
}

Rect getRectScissor(Rect scissor, Rect r) {
	return rectIntersect(scissor, r);
}

Rect scissorTestIntersect(Rect scissor, Rect r) {
	scissorTestScreen(rectIntersect(scissor, r));
	return r;
}




void drawTextBox(Rect r, char* text, Vec2i align, Rect scissor, TextBoxSettings settings) {
	// scissorTestScreen(rectExpand(scissor, vec2(2,2)));
	scissorTestScreen(scissor);

	if(settings.backgroundColor.a != 0) {
		drawRectRounded(r, settings.backgroundColor, settings.roundedCorner);
	}
	glLineWidth(0.5f);
	if(settings.borderColor.a != 0) {
		drawRectRoundedOutlined(r, settings.backgroundColor, settings.borderColor, settings.roundedCorner);
	}

	float xPos = rectCen(r).x + (rectW(r)/2)*align.x;
	float yPos = rectCen(r).y + (rectH(r)/2)*align.y;

	if(align.x == -1) xPos += 2;
	if(align.x == 1) xPos -= 2;

	scissorTestScreen(rectExpand(getRectScissor(r, scissor), vec2(-3,-3)));
	// scissorTestScreen(getRectScissor(r, scissor));

	if(settings.shadow == 0) drawText(text, settings.font, vec2(xPos, yPos), settings.textColor, align);
	else drawText(text, settings.font, vec2(xPos, yPos), settings.textColor, align, 0, settings.shadow, settings.shadowColor);

	scissorTestScreen(scissor);
}
void drawTextBox(Rect r, char* text, Vec2i align, TextBoxSettings settings) {
	drawTextBox(r, text, align, rectExpand(r, vec2(1,1)), settings);
}
void drawTextBox(Rect r, char* text, Rect scissor, TextBoxSettings settings) {
	drawTextBox(r, text, vec2i(0,0), scissor, settings);
}
void drawTextBox(Rect r, char* text, TextBoxSettings settings) {
	drawTextBox(r, text, vec2i(0,0), rectExpand(r, vec2(1,1)), settings);
}


void drawTextEditBox(char* text, Rect textRect, bool active, Rect scissor, TextEditVars editVars, TextEditSettings editSettings) {
	TextBoxSettings textSettings = editSettings.textBoxSettings;

	Vec2 startPos = rectL(textRect) + vec2(editSettings.textOffset,0);
	if(active) startPos += editVars.scrollOffset;

	scissorTestScreen(scissor);
	drawRect(textRect, textSettings.backgroundColor);
	if(textSettings.borderColor.a != 0) {
		glLineWidth(0.5f);
		drawRectOutline(textRect, textSettings.borderColor);
	}

	// scissorTestScreen(rectExpand(scissor, vec2(-1,-1)));
	scissorTestScreen(rectExpand(getRectScissor(textRect, scissor), vec2(-3,-3)));
	// scissorTestScreen(getRectScissor(textRect, scissor));

	if(active) text = editSettings.textBuffer;

	Vec2i align = vec2i(-1,0);
	if(active) {
		// Selection.
		drawTextSelection(text, textSettings.font, startPos, editVars.cursorIndex, editVars.markerIndex, editSettings.colorSelection, align);
	}

	if(!strEmpty(editSettings.defaultText) && strEmpty(text) && !active) 
		drawText(editSettings.defaultText, textSettings.font, rectCen(textRect), textSettings.textColor, vec2i(0,0), 0, textSettings.shadow, textSettings.shadowColor);
	else 
		drawText(text, textSettings.font, startPos, textSettings.textColor, align, 0, textSettings.shadow, textSettings.shadowColor);

	if(active) {
		// Cursor.
		Vec2 cPos = textIndexToPos(text, textSettings.font, startPos, editVars.cursorIndex, align);
		Rect cRect = rectCenDim(cPos, vec2(editSettings.cursorWidth, textSettings.font->height));
		if(editVars.cursorIndex == 0) cRect = rectTrans(cRect, vec2(editSettings.cursorWidth/2));
		drawRect(cRect, editSettings.colorCursor);
	}

	scissorTestScreen(scissor);
}
void drawTextEditBox(int number, Rect textRect, bool active, Rect scissor, TextEditVars editVars, TextEditSettings editSettings) {
	drawTextEditBox(fillString("%i", number), textRect, active, scissor, editVars, editSettings);
}
void drawTextEditBox(float number, Rect textRect, bool active, Rect scissor, TextEditVars editVars, TextEditSettings editSettings) {
	drawTextEditBox(fillString("%f", number), textRect, active, scissor, editVars, editSettings);
}


void drawSlider(void* val, bool type, Rect br, Rect sr, Rect scissor, SliderSettings settings) {
	scissorTestScreen(scissor);

	TextBoxSettings textSettings = settings.textBoxSettings;

	rectExpand(&sr, vec2(0,-settings.heightOffset*2));

	if(textSettings.backgroundColor.a > 0) drawRect(br, textSettings.backgroundColor);
	if(settings.lineColor.a > 0 && settings.lineWidth > 0) {
		glLineWidth(settings.lineWidth);
		drawLine(rectL(br), rectR(br), settings.lineColor);
	}

	if(settings.rounding > 0) drawRectRounded(sr, settings.color, settings.rounding);
	else drawRect(sr, settings.color);

	if(textSettings.borderColor.a != 0) drawRectOutline(br, textSettings.borderColor);

	// scissorTestScreen(rectExpand(scissor, vec2(-1,-1)));
	scissorTestScreen(getRectScissor(br, scissor));

	char* text = type == 0 ? fillString("%f", *((float*)val)) : fillString("%i", *((int*)val)) ;
	drawText(text, textSettings.font, rectCen(br), textSettings.textColor, vec2i(0,0), 0, textSettings.shadow, textSettings.shadowColor);
}
void drawSlider(float val, Rect br, Rect sr, Rect scissor, SliderSettings settings) {
	return drawSlider(&val, 0, br, sr, scissor, settings);
}
void drawSlider(int val, Rect br, Rect sr, Rect scissor, SliderSettings settings) {
	return drawSlider(&val, 1, br, sr, scissor, settings);
}

//

void newGuiQuickTextBox(NewGui* gui, Rect r, char* t, Vec2i align = vec2i(0,0), TextBoxSettings* settings = 0) {
	if(rectEmpty(getRectScissor(gui->scissor, r))) return;

	TextBoxSettings set = settings == 0 ? gui->textBoxSettings : *settings;
	drawTextBox(r, t, align, gui->scissor, set);
}

bool newGuiQuickButton(NewGui* gui, Rect r, char* text, Vec2i align = vec2i(0,0), TextBoxSettings* settings = 0) {

	Rect intersection = getRectScissor(gui->scissor, r);
	bool active = newGuiGoButtonAction(gui, intersection, gui->zLevel);
	if(rectEmpty(intersection)) return false;

	TextBoxSettings set = settings == 0 ? gui->buttonSettings : *settings;
	set.backgroundColor += newGuiColorModB(gui);
	drawTextBox(r, text, align, gui->scissor, set);

	return active;
}

bool newGuiQuickSlider(NewGui* gui, Rect r, float* val, float min, float max, SliderSettings* settings = 0) {

	SliderSettings set = settings == 0 ? gui->sliderSettings : *settings;
	Rect slider = newGuiCalcSlider(*val, r, set.size, min, max, true);

	int event = newGuiGoDragAction(gui, slider, gui->zLevel);
	if(rectEmpty(getRectScissor(gui->scissor, r))) return false;

	if(event == 1) gui->mouseAnchor = gui->input->mousePosNegative - rectCen(slider);
	if(event > 0) {
		*val = newGuiSliderGetValue(gui->input->mousePosNegative - gui->mouseAnchor, r, set.size, min, max, true);
		slider = newGuiCalcSlider(*val, r, set.size, min, max, true);
	}

	set.color += newGuiColorMod(gui);
	drawSlider(*val, r, slider, gui->scissor, set);

	if(event > 3) return true;
	return false;
}
bool newGuiQuickSlider(NewGui* gui, Rect r, int* val, int min, int max, SliderSettings* settings = 0) {

	float floatVal = *val;

	SliderSettings set = settings == 0 ? gui->sliderSettings : *settings;
	Rect slider = newGuiCalcSlider(floatVal, r, set.size, min, max, true);

	int event = newGuiGoDragAction(gui, slider, gui->zLevel);
	if(rectEmpty(getRectScissor(gui->scissor, r))) return false;

	if(event == 1) gui->mouseAnchor = gui->input->mousePosNegative - rectCen(slider);
	if(event > 0) {
		floatVal = newGuiSliderGetValue(gui->input->mousePosNegative - gui->mouseAnchor, r, set.size, min, max, true);
		floatVal = roundInt(floatVal);
		slider = newGuiCalcSlider(floatVal, r, set.size, min, max, true);
	}

	*val = floatVal;

	set.color += newGuiColorMod(gui);
	drawSlider(*val, r, slider, gui->scissor, set);

	if(event > 3) return true;
	return false;
}

// varType: 0 = char, 1 = int, 2 = float
bool newGuiQuickTextEditAllVars(NewGui* gui, Rect r, void* data, int varType, int maxSize, TextEditSettings* editSettings = 0) {
	Rect intersect = getRectScissor(gui->scissor, r);

	TextEditSettings set = editSettings == 0 ? gui->editSettings : *editSettings;
	TextBoxSettings textSettings = set.textBoxSettings;

	char* charData = (char*)data;
	int* intData = (int*)data;
	float* floatData = (float*)data;

	int event;
	if(varType == 0) event = newGuiGoTextEdit(gui, intersect, gui->zLevel, charData, textSettings.font, set, maxSize);
	else if(varType == 1) event = newGuiGoTextEdit(gui, intersect, gui->zLevel, intData, textSettings.font, set);
	else event = newGuiGoTextEdit(gui, intersect, gui->zLevel, floatData, textSettings.font, set);

	if(rectEmpty(intersect)) return false;

	if(event == 0) set.textBoxSettings.backgroundColor += newGuiColorMod(gui);
	if(varType == 0) drawTextEditBox(charData, r, event > 0, gui->scissor, gui->editVars, set);
	else if(varType == 1) drawTextEditBox(*intData, r, event > 0, gui->scissor, gui->editVars, set);
	else drawTextEditBox(*floatData, r, event > 0, gui->scissor, gui->editVars, set);

	if(newGuiIsWasHotOrActive(gui)) newGuiSetCursor(gui, IDC_IBEAM);

	if(event == 3) return true;

	return false;
}
bool newGuiQuickTextEdit(NewGui* gui, Rect r, char* data, int maxSize, TextEditSettings* editSettings = 0) {
	return newGuiQuickTextEditAllVars(gui, r, data, 0, maxSize, editSettings);
}
bool newGuiQuickTextEdit(NewGui* gui, Rect r, int* data, TextEditSettings* editSettings = 0) {
	return newGuiQuickTextEditAllVars(gui, r, data, 1, 0, editSettings);
}
bool newGuiQuickTextEdit(NewGui* gui, Rect r, float* data, TextEditSettings* editSettings = 0) {
	return newGuiQuickTextEditAllVars(gui, r, data, 2, 0, editSettings);
}




struct ScrollRegionValues {
	bool hasScrollBar;
	Rect scissor;
	Vec2 pos;
	Rect region;
};

void newGuiQuickScroll(NewGui* gui, Rect r, float height, float* scrollValue, ScrollRegionValues* scrollValues, ScrollRegionSettings* settings = 0) {
	ScrollRegionSettings set = settings == 0 ? gui->scrollSettings : *settings;

	clampMin(&height, 0);

	float scrollRegionHeight = rectH(r);

	float itemsHeight = height - scrollRegionHeight;
	clampMin(&itemsHeight, 0);
	bool hasScrollbar = itemsHeight > 0;

	Rect scrollRegion = r;
	if(!hasScrollbar && !set.fixedHeight) rectSetB(&scrollRegion, scrollRegion.top - height);
	float scrollBarWidth = hasScrollbar?set.scrollBarWidth:0;

	Rect itemRegion = rectSetR(scrollRegion, scrollRegion.right - scrollBarWidth);
	if(hasScrollbar) rectAddR(&itemRegion, set.sliderMargin.x);
	Rect scrollBarRegion = rectSetL(scrollRegion, scrollRegion.right - scrollBarWidth);

	scissorTestScreen(rectExpand(gui->scissor, vec2(-3,-3)));

	// if(!hasScrollbar) rectSetB(&scrollRegion, scrollRegion.top-height);
	drawRect(scrollRegion, set.backgroundColor);
	if(set.scrollBarColor.a != 0) drawRect(scrollBarRegion, set.scrollBarColor);
	if(set.borderColor.a != 0) {
		glLineWidth(0.5f);
		drawRectOutline(scrollRegion, set.borderColor);
	}

	float sliderSize = set.sliderSize;
	if(sliderSize == 0) sliderSize = (rectH(scrollBarRegion) / (rectH(scrollRegion)+itemsHeight)) * rectH(scrollBarRegion);
	clampMin(&sliderSize, set.sliderSizeMin);

	if(hasScrollbar) {
		// Scroll with mousewheel.
		{
			if(newGuiGoButtonAction(gui, getRectScissor(gui->scissor, scrollRegion), gui->zLevel, Gui_Focus_MWheel)) {
				float scrollAmount = set.scrollAmount;
				if(gui->input->keysDown[KEYCODE_SHIFT]) scrollAmount *= 2;

				(*scrollValue) = clamp((*scrollValue) + -gui->input->mouseWheel*(scrollAmount/itemsHeight), 0, 1);
			}
		}

		Rect slider;
		int sliderId;
		// Scroll with handle.
		{
			slider = newGuiCalcSlider(1-(*scrollValue), scrollBarRegion, sliderSize, 0, 1, false);
			int event = newGuiGoDragAction(gui, getRectScissor(gui->scissor, slider), gui->zLevel);
			sliderId = newGuiCurrentId(gui);
			if(event == 1) gui->mouseAnchor = gui->input->mousePosNegative - rectCen(slider);
			if(event > 0) {
				(*scrollValue) = 1-newGuiSliderGetValue(gui->input->mousePosNegative - gui->mouseAnchor, scrollBarRegion, sliderSize, 0, 1, false);
				slider = newGuiCalcSlider(1-(*scrollValue), scrollBarRegion, sliderSize, 0, 1, false);
			}
		}

		// Scroll with background drag.
		if(set.scrollWithBackground)
		{
			int event = newGuiGoDragAction(gui, getRectScissor(gui->scissor, itemRegion), gui->zLevel);
			if(event == 1) gui->mouseAnchor.y = gui->input->mousePosNegative.y - ((*scrollValue)*itemsHeight);
			if(event > 0) {
				(*scrollValue) = (gui->input->mousePosNegative.y - gui->mouseAnchor.y)/itemsHeight;
				clamp(&(*scrollValue), 0, 1);

				slider = newGuiCalcSlider(1-(*scrollValue), scrollBarRegion, sliderSize, 0, 1, false);
			}
		}

		rectExpand(&slider, -set.sliderMargin*2);
		if(hasScrollbar) drawRectRounded(slider, set.sliderColor + newGuiColorModId(gui, sliderId), set.sliderRounding);
	}

	set.border.x = clampMin(set.border.x, 1);
	set.border.y = clampMin(set.border.y, 1);

	itemRegion = rectExpand(itemRegion, -set.border*2);

	scrollValues->hasScrollBar = hasScrollbar;
	scrollValues->scissor = getRectScissor(gui->scissor, itemRegion);
	scrollValues->region = itemRegion;
	// scrollValues->pos = vec2(itemRegion.left, r.top + (*scrollValue)*itemsHeight);
	scrollValues->pos = vec2(itemRegion.left, itemRegion.top + (*scrollValue)*itemsHeight);

	// scissorTestScreen(rectExpand(scissor, vec2(-1,-3)));
}

TextEditSettings textEditSettings(TextBoxSettings textSettings, char* textBuffer, bool wrapping, bool singleLine, float cursorWidth, Vec4 colorSelection, Vec4 colorCursor, float textOffset) {
	return {textSettings, textBuffer, wrapping, singleLine, cursorWidth, "", colorSelection, colorCursor, textOffset};
}

void textEditBox(char* text, int textMaxSize, Font* font, Rect textRect, Input* input, Vec2i align, TextEditSettings tes, TextEditVars* tev) {

	if(tes.singleLine) tes.wrapping = false;

	// Vec2 startPos = rectTL(textRect) + tev->scrollOffset;
	Vec2 startPos = rectTL(textRect) + tev->scrollOffset + vec2(tes.textOffset,0);
	int wrapWidth = tes.wrapping ? rectDim(textRect).w : 0;

	int cursorIndex = tev->cursorIndex;
	int markerIndex = tev->markerIndex;

	int mouseIndex = textMouseToIndex(text, font, startPos, input->mousePosNegative, align, wrapWidth);

	if(input->mouseButtonPressed[0]) {
		if(pointInRect(input->mousePosNegative, textRect)) {
			markerIndex = mouseIndex;
		}
	}

	if(input->mouseButtonDown[0]) {
		cursorIndex = mouseIndex;
	}



	bool left = input->keysPressed[KEYCODE_LEFT];
	bool right = input->keysPressed[KEYCODE_RIGHT];
	bool up = input->keysPressed[KEYCODE_UP];
	bool down = input->keysPressed[KEYCODE_DOWN];

	bool a = input->keysPressed[KEYCODE_A];
	bool x = input->keysPressed[KEYCODE_X];
	bool c = input->keysPressed[KEYCODE_C];
	bool v = input->keysPressed[KEYCODE_V];
	
	bool home = input->keysPressed[KEYCODE_HOME];
	bool end = input->keysPressed[KEYCODE_END];
	bool backspace = input->keysPressed[KEYCODE_BACKSPACE];
	bool del = input->keysPressed[KEYCODE_DEL];
	bool enter = input->keysPressed[KEYCODE_RETURN];
	bool tab = input->keysPressed[KEYCODE_TAB];

	bool ctrl = input->keysDown[KEYCODE_CTRL];
	bool shift = input->keysDown[KEYCODE_SHIFT];

	// Main navigation and things.

	int startCursorIndex = cursorIndex;

	if(ctrl && backspace) {
		shift = true;
		left = true;
	}

	if(ctrl && del) {
		shift = true;
		right = true;
	}

	if(!tes.singleLine) {
		if(up || down) {
			float cursorYOffset;
			if(up) cursorYOffset = font->height;
			else if(down) cursorYOffset = -font->height;

			Vec2 cPos = textIndexToPos(text, font, startPos, cursorIndex, align, wrapWidth);
			cPos.y += cursorYOffset;
			int newCursorIndex = textMouseToIndex(text, font, startPos, cPos, align, wrapWidth);
			cursorIndex = newCursorIndex;
		}
	}


	if(left) {
		if(ctrl) {
			if(cursorIndex > 0) {
				while(text[cursorIndex-1] == ' ') cursorIndex--;

				if(cursorIndex > 0)
			 		cursorIndex = strFindBackwards(text, ' ', cursorIndex-1);
			}
		} else {
			bool isSelected = cursorIndex != markerIndex;
			if(isSelected && !shift) {
				if(cursorIndex < markerIndex) {
					markerIndex = cursorIndex;
				} else {
					cursorIndex = markerIndex;
				}
			} else {
				if(cursorIndex > 0) cursorIndex--;
			}
		}
	}

	if(right) {
		if(ctrl) {
			while(text[cursorIndex] == ' ') cursorIndex++;
			if(cursorIndex <= strLen(text)) {
				cursorIndex = strFindOrEnd(text, ' ', cursorIndex+1);
				if(cursorIndex != strLen(text)) cursorIndex--;
			}
		} else {
			bool isSelected = cursorIndex != markerIndex;
			if(isSelected && !shift) {
				if(cursorIndex > markerIndex) {
					markerIndex = cursorIndex;
				} else {
					cursorIndex = markerIndex;
				}
			} else {
				if(cursorIndex < strLen(text)) cursorIndex++;
			}
		}
	}

	if(tes.singleLine) {
		if(home) {
			cursorIndex = 0;
		}

		if(end) {
			cursorIndex = strLen(text);
		}
	}

	if((startCursorIndex != cursorIndex) && !shift) {
		markerIndex = cursorIndex;
	}

	if(ctrl && a) {
		cursorIndex = 0;
		markerIndex = strLen(text);
	}

	bool isSelected = cursorIndex != markerIndex;

	if((ctrl && x) && isSelected) {
		c = true;
		del = true;
	}

	if((ctrl && c) && isSelected) {
		char* selection = textSelectionToString(text, cursorIndex, markerIndex);
		setClipboard(selection);
	}

	if(enter) {
		if(tes.singleLine) {
			// strClear(text);
			// cursorIndex = 0;
			// markerIndex = 0;
		} else {
			input->inputCharacters[input->inputCharacterCount++] = '\n';
		}
	}

	if(backspace || del || (input->inputCharacterCount > 0) || (ctrl && v)) {
		if(isSelected) {
			int delIndex = min(cursorIndex, markerIndex);
			int delAmount = abs(cursorIndex - markerIndex);
			strRemoveX(text, delIndex, delAmount);
			cursorIndex = delIndex;
		}

		markerIndex = cursorIndex;
	}

	if(ctrl && v) {
		char* clipboard = (char*)getClipboard();
		int clipboardSize = strLen(clipboard);
		if(clipboardSize + strLen(text) < textMaxSize) {
			strInsert(text, cursorIndex, clipboard);
			cursorIndex += clipboardSize;
			markerIndex = cursorIndex;
		}
		closeClipboard();
	}

	// Add input characters to input buffer.
	if(input->inputCharacterCount > 0) {
		if(input->inputCharacterCount + strLen(text) < textMaxSize) {
			strInsert(text, cursorIndex, input->inputCharacters, input->inputCharacterCount);
			cursorIndex += input->inputCharacterCount;
			markerIndex = cursorIndex;
		}
	}

	if(backspace && !isSelected) {
		if(cursorIndex > 0) {
			strRemove(text, cursorIndex);
			cursorIndex--;
		}
		markerIndex = cursorIndex;
	}

	if(del && !isSelected) {
		if(cursorIndex+1 <= strLen(text)) {
			strRemove(text, cursorIndex+1);
		}
		markerIndex = cursorIndex;
	}

	// Scrolling.
	{
		Vec2 cursorPos = textIndexToPos(text, font, startPos, cursorIndex, align, wrapWidth);

		float leftEnd = textRect.left + tes.textOffset;
		float rightEnd = textRect.right - tes.textOffset;
		if(		cursorPos.x < leftEnd) tev->scrollOffset.x += leftEnd - cursorPos.x;
		else if(cursorPos.x > rightEnd) tev->scrollOffset.x += rightEnd - cursorPos.x;

		clampMax(&tev->scrollOffset.x, 0);
		
		float ch = font->height;
		if(!tes.singleLine) {
			if(		cursorPos.y - ch/2 < textRect.min.y) tev->scrollOffset.y += textRect.min.y - (cursorPos.y - ch/2);
			else if(cursorPos.y + ch/2 > textRect.max.y) tev->scrollOffset.y += textRect.max.y - (cursorPos.y + ch/2);
		
			clampMin(&tev->scrollOffset.y, 0);
		}
	}


	tev->cursorChanged = (tev->cursorIndex != cursorIndex || tev->markerIndex != markerIndex);

	tev->cursorIndex = cursorIndex;
	tev->markerIndex = markerIndex;

	// // Cursor.
	// // tev->cursorTime += tes.dt * tes.cursorSpeed;
	// // Vec4 cmod = vec4(0,cos(tev->cursorTime)*tes.cursorColorMod - tes.cursorColorMod,0,0);
}

struct GuiWindowSettings {
	float borderSize;
	float cornerSize;
	Vec2 minDim;
	Vec2 maxDim;
	Rect insideRect;

	bool movable;
	bool resizableX;
	bool resizableY;

	bool mouseScreenCoordinates;
};

bool newGuiWindowUpdate(NewGui* gui, Rect* r, float z, GuiWindowSettings settings) {
	Rect region = *r;
	Rect sr = settings.insideRect;
	bool insideClamp = !rectEmpty(sr);
	if(!insideClamp) sr = rectCenDim(vec2(0,0), vec2(FLT_MAX, FLT_MAX)); // So we don't clamp.

	if(settings.maxDim == vec2(0,0)) settings.maxDim = vec2(FLT_MAX,FLT_MAX);

	bool screenMouse = settings.mouseScreenCoordinates;

	float w = settings.borderSize;
	Vec2 p = screenMouse?gui->input->mousePosNegativeScreen:gui->input->mousePosNegative;

	int uiEvent = 0;
	bool changeCursor = false;
	Vec2i resizeAlign;

	bool move = false;


	if((settings.resizableY || settings.resizableX) || settings.movable) {
		int event = newGuiGoDragAction(gui, region, z, Gui_Focus_MLeft, screenMouse);
		int eventRightClick = newGuiGoDragAction(gui, region, z, Gui_Focus_MRight, screenMouse);
		event = max(event, eventRightClick);
		if(event == 1) {
			gui->mode = gui->input->keysDown[KEYCODE_CTRL];

			if(!gui->mode) {
				POINT p; 
				GetCursorPos(&p);
				Vec2 mp = vec2(p.x, -p.y);

				// gui->mouseAnchor = input->mousePosNegative - rectTL(region);
				gui->mouseAnchor = mp - rectTL(region);
				gui->mouseAnchor2 = rectDim(region);
			}
		}

		if(event > 0) {
			if(gui->mode) {
				uiEvent = event;
				resizeAlign = vec2i(1,-1);
			} else {
				move = true;

				POINT p; 
				GetCursorPos(&p);
				Vec2 mp = vec2(p.x, -p.y);

				// Vec2 pos = input->mousePosNegative - gui->mouseAnchor;
				Vec2 pos = mp - gui->mouseAnchor;

				if(insideClamp) {
					clamp(&pos.x, sr.left, sr.right - gui->mouseAnchor2.w);
					clamp(&pos.y, sr.bottom + gui->mouseAnchor2.h, sr.top);
				}
				region = rectTLDim(pos, gui->mouseAnchor2);
			}
		}
	}

	float cornerSize = settings.cornerSize;
    for(int x = -1; x < 2; x++) {
    	for(int y = -1; y < 2; y++) {
    		if(x == 0 && y == 0) continue;

    		Vec2i align = vec2i(x,y);
    		Vec2 dim = vec2(align.x==0?rectW(region)-cornerSize*2+2:w+1, align.y==0?rectH(region)-cornerSize*2+2:w+1);
    		Rect r = rectAlignDim(region, align, dim);

    		int event;
    		bool corner = abs(x) == 1 && abs(y) == 1;
    		if(corner) {
    			r = rectAlignDim(region, align, vec2(w+1,cornerSize));
	    		event = newGuiGoDragAction(gui, r, z, Gui_Focus_MLeft, screenMouse);
    			r = rectAlignDim(region, align, vec2(cornerSize,w+1));
    			gui->id--;
				event = newGuiGoDragAction(gui, r, z, Gui_Focus_MLeft, screenMouse);
    		} else {
	    		event = newGuiGoDragAction(gui, r, z, Gui_Focus_MLeft, screenMouse);
    		}

    		if(event > 0) {
    			uiEvent = event;
    			resizeAlign = align;
    		}
			if(newGuiIsWasHotOrActive(gui)) {
    			changeCursor = true;
    			resizeAlign = align;
			}
    	}
    }

    if(!move) {
        if(uiEvent == 1) {
        	if(resizeAlign.x == -1) gui->mouseAnchor.x = p.x - region.left;
        	if(resizeAlign.x ==  1) gui->mouseAnchor.x = p.x - region.right;
        	if(resizeAlign.y ==  1) gui->mouseAnchor.y = p.y - region.top;
        	if(resizeAlign.y == -1) gui->mouseAnchor.y = p.y - region.bottom;
        }

        if(uiEvent > 0) {
			if(resizeAlign.x == -1) region.left = (p - gui->mouseAnchor).x;
			else if(resizeAlign.x == 1) region.right = (p - gui->mouseAnchor).x;

			if(settings.resizableY) {
				if(resizeAlign.y == -1) region.bottom = (p - gui->mouseAnchor).y;
				else if(resizeAlign.y == 1) region.top = (p - gui->mouseAnchor).y;
			}
        }

    	if(changeCursor) {
    		if(resizeAlign == vec2i(-1,-1) || resizeAlign == vec2i(1,1)) newGuiSetCursor(gui, IDC_SIZENESW);
    		if(resizeAlign == vec2i(-1,1) || resizeAlign == vec2i(1,-1)) newGuiSetCursor(gui, IDC_SIZENWSE);
    		if(resizeAlign == vec2i(-1,0) || resizeAlign == vec2i(1, 0)) newGuiSetCursor(gui, IDC_SIZEWE);
    		if(resizeAlign == vec2i(0,-1) || resizeAlign == vec2i(0, 1)) newGuiSetCursor(gui, IDC_SIZENS);
    	}

    	// if(uiEvent > 0 && insideClamp) {
    	if(uiEvent > 0) {
    		Vec2 minDim = settings.minDim;
    		Vec2 maxDim = settings.maxDim;

			if(settings.resizableX) {
    			if(resizeAlign.x == -1) region.left = clamp(region.left, max(sr.left, region.right - maxDim.x), region.right - minDim.x);
    			if(resizeAlign.x ==  1) region.right = clamp(region.right, region.left + minDim.x, min(sr.right, region.left + maxDim.x));
			}

			if(settings.resizableY) {
	    		if(resizeAlign.y == -1) region.bottom = clamp(region.bottom, max(sr.bottom, region.top - maxDim.y), region.top - minDim.y);
	    		if(resizeAlign.y ==  1) region.top = clamp(region.top, region.bottom + minDim.y, min(sr.top, region.bottom + maxDim.y));
    		}
    	}
    }


    // If window is resizing clamp panel.
    if(insideClamp)
    {
    	Vec2 dim = rectDim(region);
    	if(region.left < sr.left) rectTrans(&region, vec2(sr.left - region.left, 0));
    	if(region.right > sr.right) rectTrans(&region, vec2(sr.right - region.right, 0));
    	if(region.bottom < sr.bottom) rectTrans(&region, vec2(0, sr.bottom - region.bottom));
    	if(region.top > sr.top) rectTrans(&region, vec2(0, sr.top - region.top));

    	if(rectH(region) > rectH(sr)) {
    		region.top = sr.top;
    		region.bottom = sr.bottom;
    	}
    	if(rectW(region) > rectW(sr)) {
    		region.left = sr.left;
    		region.right = sr.right;
    	}
    }

    *r = region;

    if(move || uiEvent) return true;

    return false;
}

// @GuiAutomation

LayoutData layoutData(Rect region, Vec2 pos, float defaultHeight, float yPadding, float yPaddingExtra) {
	LayoutData ld = {region, pos, rectDim(region), defaultHeight, yPadding, yPaddingExtra};
	return ld;
}
LayoutData layoutData(Rect region, float defaultHeight, float yPadding, float yPaddingExtra) {
	LayoutData ld = {region, rectTL(region), rectDim(region), defaultHeight, yPadding, yPaddingExtra};
	return ld;
}

Rect newGuiLRectAdv(NewGui* gui, float height) {
	Rect r = rectTLDim(gui->ld->pos, vec2(gui->ld->dim.w, height));
	gui->ld->pos.y -= height + gui->ld->yPadding;
	return r;
}
Rect newGuiLRectAdv(NewGui* gui) { 
	return newGuiLRectAdv(gui, gui->ld->defaultHeight); 
}
void newGuiLAdv(NewGui* gui, float height) { 
	gui->ld->pos.y -= height + gui->ld->yPadding;
}

LayoutData* newGuiLayoutPush(NewGui* gui, LayoutData ld) {
	gui->layoutStackIndex++;
	gui->layoutStack[gui->layoutStackIndex] = ld;
	gui->ld = &gui->layoutStack[gui->layoutStackIndex];
	return gui->ld;
}
LayoutData* newGuiLayoutPush(NewGui* gui, Rect region) {
	LayoutData newLd = *gui->ld;
	newLd.region = region;
	newLd.pos = rectTL(newLd.region);
	newLd.dim = rectDim(region);
	return newGuiLayoutPush(gui, newLd);
}
LayoutData* newGuiLayoutPop(NewGui* gui) {
	gui->layoutStackIndex--;
	gui->ld = &gui->layoutStack[gui->layoutStackIndex];
	return gui->ld;
}

void newGuiScissorPush(NewGui* gui, Rect scissor) {
	gui->scissor = rectIntersect(gui->scissor, scissor);
	gui->scissorStack[gui->scissorStackIndex+1] = gui->scissor;
	gui->scissorStackIndex++;

	scissorTestScreen(gui->scissor);
}
void newGuiScissorPop(NewGui* gui) {
	gui->scissorStackIndex--;
	gui->scissor = gui->scissorStack[gui->scissorStackIndex];

	scissorTestScreen(gui->scissor);
}



bool downloadStepNext(int* downloadMode, int* modeIndex, CurlRequestData* requestData, bool waitForRequest = true) {
	if((*downloadMode) == (*modeIndex)++) {

		if((*modeIndex) == 1) {
			(*downloadMode)++;
			if(requestData->active) requestData->abort = true;
			return true;
		} else if((*modeIndex) == 2 && !requestData->abort) {
			(*downloadMode)++;
			requestData->finished = false;
			return true;
		} else if((*modeIndex) > 2 && ((waitForRequest&&requestData->finished) || !waitForRequest)) {
			(*downloadMode)++;
			requestData->finished = false;
			// if(lastStep) (*downloadMode) = 0;

			return true;
		}
	}

	return false;
}

enum DownloadMode {
	Download_Mode_Snippet = 1,
	Download_Mode_Search,
	Download_Mode_ChannelPlaylists,
	Download_Mode_Videos,
};

struct DownloadVideosData {
	FILE* file;

	YoutubeVideo vids[Max_Download_Count];
	char pageToken[Page_Token_Size];
	int lastCount;
	int maxCount;

	int i;
	int count;
	int dCount;
};

struct DownloadModeData {
	int downloadStep;
	int downloadMode;
	int newDownloadMode;

	int downloadProgress;
	int downloadMax;

	bool switchMode;

	DownloadVideosData videosModeData;
};

void downloadModeSet(DownloadModeData* modeData, int newMode, bool* appIsBusy) {
	modeData->newDownloadMode = newMode;
	modeData->switchMode = true;
	*appIsBusy = true;
}

void downloadModeAbort(DownloadModeData* modeData, bool* appIsBusy) { downloadModeSet(modeData, 0, appIsBusy); }

inline void downloadModeProgress(DownloadModeData* modeData, int progress, int max) { 
	modeData->downloadProgress = progress;
	modeData->downloadMax = max;
}

inline void downloadModeFinish(DownloadModeData* modeData) { modeData->downloadMode = 0; }
inline bool downloadModeActive(DownloadModeData* modeData) { return modeData->downloadMode > 0; }



union AppColors {
	struct {
		Vec4 graphBackgroundBottom;
		Vec4 graphBackgroundTop;
		Vec4 graphLine1;
		Vec4 graphLine2;
		Vec4 graphLineAvg;
		Vec4 font;
		Vec4 font2;
		Vec4 font3;
		Vec4 fontShadow;
		Vec4 background;
		Vec4 buttons;
		Vec4 sidePanel;
		Vec4 likes;
		Vec4 dislikes;
		Vec4 comments;
		Vec4 commentScroll;
		Vec4 sidePanelButtons;
		Vec4 editBackground;
		Vec4 editSelection;
		Vec4 editCursor;
		Vec4 mouseHover;
		Vec4 outlines;
		Vec4 graphMarkers;
		Vec4 graphSubMarkers;
		Vec4 windowBorder;
	};

	Vec4 e[25];
};

char* appColorsStrings[] = {
	"graphBackgroundBottom",
	"graphBackgroundTop",
	"graphLine1",
	"graphLine2",
	"graphLineAvg",
	"font1",
	"font2",
	"font3",
	"fontShadow",
	"background",
	"buttons",
	"sidePanel",
	"likes",
	"dislikes",
	"comments",
	"commentScroll",
	"sidePanelButtons",
	"editBackground",
	"editSelection",
	"editCursor",
	"mouseHover",
	"outlines",
	"graphMarkers",
	"graphSubMarkers",
	"windowBorder",
};

union AppSettings {
	struct {
		char* font;
		char* fontComment;
		char* fontTitle;
		char* fontPanel;

		int fontSize;
		int fontCommentSize;
		int fontTitleSize;
		int fontPanelSize;
		int fontShadow;

		int marginBottomText;
		int marginLeftText;
		int marginSidePanel;
		int marginComments;
		int marginButtons;
		int marginButtonsHeight;
		int marginFilters;
		int marginSlider;
		int commentScrollbarWidth;
		int resizeRegionSize;
		int titleOffset;

		int windowBorder;
		int windowTitleHeight;
	};

	struct {
		char* fonts[APP_FONT_COUNT];
		int e[17];
	};
};

char* appSettingsStrings[] = {
	"font",
	"fontComment",
	"fontTitle",
	"fontPanel",
	"fontSize",
	"fontCommentSize",
	"fontTitleSize",
	"fontPanelSize",
	"fontShadow",
	"marginBottomText",
	"marginLeftText",
	"marginSidePanel",
	"marginComments",
	"marginButtons",
	"marginButtonsHeight",
	"marginFilters",
	"marginSlider",
	"commentScrollbarWidth",
	"resizeRegionSize",
	"titleOffset",
	"windowBorder",
	"windowTitleHeight",
};

void appWriteSettings(char* filePath, AppSettings* as, AppColors* ac) {
	char* text;
	int maxString;
	int offset;

	FILE* file = fopen(filePath, "wb");

	maxString = 0;
	for(int i = 0; i < arrayCount(appSettingsStrings); i++) {
		maxString = max(maxString, strLen(appSettingsStrings[i]));
	}


	text = "Notes \r\n";
	fwrite(text, strLen(text), 1, file);

	text = "- Settings are hotloaded. \r\n";
	fwrite(text, strLen(text), 1, file);
	text = "- App and windows font folders are checked for fonts. \r\n";
	fwrite(text, strLen(text), 1, file);
	text = "- Delete this file to get back to the default settings. \r\n";
	fwrite(text, strLen(text), 1, file);

	text = "\r\n";
	fwrite(text, strLen(text), 1, file);

	text = "\\\\ ============ App Settings. ============ \\\\\r\n \r\n";
	fwrite(text, strLen(text), 1, file);


	offset = maxString + 2;
	for(int ai = 0; ai < arrayCount(appSettingsStrings); ai++) {
		text = fillString("%s:", appSettingsStrings[ai]);
		fwrite(text, strLen(text), 1, file);

		char space = ' ';
		int a = offset-strlen(text);
		for(int i = 0; i < a; i++) fwrite(&space, 1, 1, file);
	
		if(ai < APP_FONT_COUNT)	text = fillString("\"%s\"\r\n", as->fonts[ai]);
		else text = fillString("%i\r\n", as->e[ai-APP_FONT_COUNT]);

		fwrite(text, strLen(text), 1, file);
	}



	maxString = 0;
	for(int i = 0; i < arrayCount(appColorsStrings); i++) {
		maxString = max(maxString, strLen(appColorsStrings[i]));
	}

	text = "\r\n\\\\ ============ App Colors. ============ \\\\\r\n \r\n";
	fwrite(text, strLen(text), 1, file);

	offset = maxString + 2;
	for(int i = 0; i < arrayCount(appColorsStrings); i++) {
		char* text1 = fillString("%s:", appColorsStrings[i]);
		char* text2 = fillString("{%f, %f, %f, %f}\r\n", PVEC4(ac->e[i]));

		fwrite(text1, strLen(text1), 1, file);

		char space = ' ';
		int a = offset-strlen(text1);
		for(int i = 0; i < a; i++) fwrite(&space, 1, 1, file);
		
		fwrite(text2, strLen(text2), 1, file);
	}

	fclose(file);
}

void appReadSettings(char* filePath, AppSettings* as, AppColors* ac) {
	int fSize = fileSize(filePath);
	char* buffer = getTString(fSize)+1;
	readFileToBuffer(buffer, filePath);

	char* value = getTString(20);

	for(int i = 0; i < arrayCount(appSettingsStrings); i++) {
		int pos = strFind(buffer, ':');
		buffer += pos;

		buffer = eatSpaces(buffer);

		int size = 0;
		if(i < APP_FONT_COUNT) {
			int size = parseString(buffer);
			as->fonts[i] = getPString(size-1);
			strCpy(as->fonts[i], buffer+1, size-2);
		} else {
			int size = parseNumber(buffer);
			strCpy(value, buffer, size);
			as->e[i-APP_FONT_COUNT] = strToInt(value);
		}

		buffer += size;
	}

	for(int i = 0; i < arrayCount(appColorsStrings); i++) {
		int pos = strFind(buffer, ':');
		buffer += pos;

		Vec4 c;
		buffer += strFind(buffer, '{');

		buffer--;
		for(int j = 0; j < 4; j++) {
			buffer++;
			buffer = eatSpaces(buffer);

			int size = parseNumber(buffer);
			strCpy(value, buffer, size);
			c.e[j] = strToFloat(value);

			buffer += size;
			buffer = eatSpaces(buffer);
		}

		ac->e[i] = c;
	}
}

void appSettingsDefault(AppSettings* as, AppColors* ac) {
		ac->graphBackgroundBottom = vec4(0.2f, 0.1f, 0.4f, 1);
		ac->graphBackgroundTop    = vec4(0.1f, 0.1f, 0.2f, 1);
		ac->graphLine1            = vec4(0,0.7f,1,1);
		ac->graphLine2            = vec4(0.9f,0.5f,1,1);
		ac->graphLineAvg          = vec4(0,0.9f,0.3f,0.8f);
		ac->font2                 = vec4(0.6f,1);
		ac->font3                 = vec4(1.0f,0.5f,0,1);
		ac->fontShadow            = vec4(0,1);
		ac->background            = vec4(0.45f, 0.15f, 0.15f, 1);
		ac->buttons               = vec4(0.52,0.25,0.20,1);
		ac->sidePanel             = vec4(0.13f,0.02f,0.02f, 1.0f);
		ac->likes                 = vec4(0.2f,0.2f,0.5f,1);
		ac->dislikes              = vec4(0.5f,0.2f,0.2f,1);
		ac->comments              = vec4(0.2f,0.1f,0,1);
		ac->commentScroll         = vec4(0.3f,0.15f,0,1);
		ac->sidePanelButtons      = vec4(0.3f,0.1f,0,1);
		ac->editBackground        = vec4(0.33,0.11,0.11,1);
		ac->editSelection         = vec4(0.24,0.41,0.59,1); 
		ac->editCursor            = vec4(0.2f,0.7f,0,1);
		ac->mouseHover            = vec4(0.8f,0,0.2f,0.2f);
		ac->outlines              = vec4(0,1);
		ac->graphMarkers          = vec4(1,0.05f);
		ac->graphSubMarkers       = vec4(1,0.01f);
		ac->windowBorder          = ac->background - vec4(0.2f,0.1f,0,0);

		as->font = "OpenSans-Bold.ttf";
		as->fontComment = "SourceSansPro-Regular.ttf";
		as->fontTitle = "OpenSans-Bold.ttf";
		as->fontPanel = "calibri.ttf";
		as->fontSize = 22;
		as->fontCommentSize = 23;
		as->fontTitleSize = 30;
		as->fontPanelSize = 20;
		as->fontShadow = 2;

		as->marginBottomText = -3;
		as->marginLeftText = 2;
		as->marginSidePanel = 10;
		as->marginComments = 10;
		as->marginButtons = 4;
		as->marginButtonsHeight = 4;
		as->marginFilters = 5;
		as->marginSlider = 7;
		as->commentScrollbarWidth = 20;
		as->resizeRegionSize = 5;
		as->titleOffset = 10;
		as->windowBorder = 5;
		as->windowTitleHeight = as->fontSize;
}

struct AppTempSettings {
	Rect windowRect;
	float sidePanelWidth;
	float graphOffsets[Line_Graph_Count+1];
	int playlistFolderIndex;

	Rect panelRect;
};

void appWriteTempSettings(char* filePath, AppTempSettings* at) {
	writeDataToFile((char*)at, sizeof(AppTempSettings), filePath);
}

void appReadTempSettings(char* filePath, AppTempSettings* at) {
	readDataFile((char*)at, filePath);
}

void appTempDefault(AppTempSettings* at, Rect monitor) {
	Rect r = monitor;
	Vec2 center = vec2(rectCenX(r), (r.top - r.bottom)/2);
	Vec2 dim = vec2(rectW(r), -rectH(r));
	at->windowRect = rectCenDim(center, dim*0.85f);

	at->sidePanelWidth = rectW(at->windowRect)*0.25f;

	at->graphOffsets[0] = 0.0f;
	at->graphOffsets[1] = 0.4f;
	at->graphOffsets[2] = 0.6f;
	at->graphOffsets[3] = 0.8f;
	at->graphOffsets[4] = 1.0f;

	at->playlistFolderIndex = 0;

	at->panelRect = rectTLDim(100,-100,400,600);
}



enum JSonType {
	JSON_STRING = 0,
	JSON_NUMBER,
	JSON_BOOL,
	JSON_OBJECT,
	JSON_OBJECT_PAIR,
	JSON_ARRAY,
	JSON_NULL,
};

struct JSonValue {
	int type;

	union {
		char* string;
		float number;
		bool b;

		struct {
			char* string;
			JSonValue* value;
		};

		struct {
			JSonValue* array; 
			int size;
		};
	};
};

struct JSonNode {
	JSonValue value;
	JSonNode* next;
};

enum JSonTokenType {
	JSON_TOKEN_BRACE_OPEN = 0,
	JSON_TOKEN_BRACE_CLOSE,
	JSON_TOKEN_BRACKET_OPEN,
	JSON_TOKEN_BRACKET_CLOSE,
	JSON_TOKEN_COLON,
	JSON_TOKEN_COMMA,
	JSON_TOKEN_STRING,
	JSON_TOKEN_NUMBER,
	JSON_TOKEN_TRUE,
	JSON_TOKEN_FALSE,
	JSON_TOKEN_NULL,
};

struct Token {
	int type;
	char* data;
	int size;
};

Token jsonGetToken(char** buffer, bool advance = true) {
	Token token;

	char* b = *buffer;
	b = eatWhiteSpaces(b);

	char* startAddress = b;

	token.data = b;
	token.size = 1;

	char c = b[0];
	b++;
	switch(c) {
		case '{': token.type = JSON_TOKEN_BRACE_OPEN; break;
		case '}': token.type = JSON_TOKEN_BRACE_CLOSE; break;
		case '[': token.type = JSON_TOKEN_BRACKET_OPEN; break;
		case ']': token.type = JSON_TOKEN_BRACKET_CLOSE; break;
		case ':': token.type = JSON_TOKEN_COLON; break;
		case ',': token.type = JSON_TOKEN_COMMA; break;
		case 't': token.type = JSON_TOKEN_TRUE; token.size = 4; b+=3; break;
		case 'f': token.type = JSON_TOKEN_FALSE; token.size = 5; b+=4; break;
		case 'n': token.type = JSON_TOKEN_NULL; token.size = 4; b+=3; break;

		case '\"': {
			token.data = b;
			while(*b != '\"') {
				if(*b == '\\') {
					b++;
					if(*b == 'u') b += 5;
					else b++;
				} else {
					// Assuming utf-8
					b += unicodeGetSize((uchar*)b);
				}
			}
			b++;

			token.type = JSON_TOKEN_STRING;
			token.size = b - startAddress - 2;
		} break;

		default: {
			b--;
			if(*b == '-') b++;
			if(*b == '0') b++;
			else while(charIsDigit(*b)) b++;
			if(*b == '.') {
				b++;
				while(charIsDigit(*b)) b++;
			}
			if(*b == 'e' || *b == 'E') {
				b++;
				if(*b == '+' || *b == '-') b++;
				while(charIsDigit(*b)) b++;
			}

			token.type = JSON_TOKEN_NUMBER;
			token.size = b - startAddress;
		} break;
	}

	if(advance) {
		*buffer = b;
	}

	return token;
}

int jsonPeekToken(char** buffer) {
	Token token = jsonGetToken(buffer, false);
	return token.type;
}

struct JSonParseInfo {
	char* buffer;
	Token* tokens;
	int size;
};

JSonValue* jsonParseValue(char** buffer);

JSonValue* jsonParseString(char** buffer) {
	Token token = jsonGetToken(buffer);
	myAssert(token.type == JSON_TOKEN_STRING);

	JSonValue* value = getTStruct(JSonValue);
	value->type = JSON_STRING;

	// Transform json string to c string.
	value->string = getTString(token.size+1);
	char* t = token.data;
	char* s = value->string;
	int size = 0;
	for(int i = 0; i < token.size; i++) {
		char c = t[i];
		if(c == '\\') {
			switch(t[i+1]) {
				case '\"': s[size] = '\"'; break;
				case '\\': s[size] = '\\'; break;
				case '/' : s[size] = '/' ; break;
				case 'b': s[size] = '\b'; break;
				case 'f': s[size] = '\f'; break;
				case 'n': s[size] = '\n'; break;
				case 'r': s[size] = '\r'; break;
				case 't': s[size] = '\t'; break;
				case 'u': {
					s[size]   = t[i+1];
					s[size+1] = t[i+2];
					s[size+2] = t[i+3];
					s[size+3] = t[i+4];
					size += 3;
					i += 3-1;
				} break;
			}
			size++;
			i += 2-1;
		} else {
			s[size] = c;
			size++;
		}
	}
	s[size] = '\0';

	return value;
}

JSonValue* jsonParseArray(char** buffer) {
	myAssert(jsonGetToken(buffer).type == JSON_TOKEN_BRACKET_OPEN);

	JSonNode* node = 0;
	int listSize = 0;

	if(jsonPeekToken(buffer) != JSON_TOKEN_BRACKET_CLOSE) {
		for(;;) {
			JSonNode* n = getTStruct(JSonNode);
			n->next = 0;

			n->value = *(jsonParseValue(buffer));

			if(node == 0) node = n;
			else {
				JSonNode* it = node;
				while(it->next != 0) it = it->next;
				it->next = n;
			}
			listSize++;

			if(jsonPeekToken(buffer) != JSON_TOKEN_COMMA) break;
			myAssert(jsonGetToken(buffer).type == JSON_TOKEN_COMMA);
		}	
	}
	myAssert(jsonGetToken(buffer).type == JSON_TOKEN_BRACKET_CLOSE);

	JSonValue* value = getTStruct(JSonValue);
	value->type = JSON_ARRAY;
	value->size = listSize;
	value->array = getTArray(JSonValue, listSize);
	JSonNode* it = node;
	for(int i = 0; i < listSize; i++) {
		value->array[i] = it->value;
		it = it->next;
	}

	return value;
}

JSonValue* jsonParseObject(char** buffer) {
	myAssert(jsonGetToken(buffer).type == JSON_TOKEN_BRACE_OPEN);

	JSonNode* node = 0;
	int listSize = 0;

	if(jsonPeekToken(buffer) != JSON_TOKEN_BRACE_CLOSE) {
		for(;;) {
			JSonNode* n = getTStruct(JSonNode);
			n->next = 0;

			JSonValue* value = &n->value;
			value->type = JSON_OBJECT_PAIR;

			Token token = jsonGetToken(buffer);
			myAssert(token.type == JSON_TOKEN_STRING);
			// Transform string before copying.
			value->string = getTString(token.size+1);
			strCpy(value->string, token.data, token.size);

			myAssert(jsonGetToken(buffer).type == JSON_TOKEN_COLON);

			value->value = jsonParseValue(buffer);

			if(node == 0) node = n;
			else {
				JSonNode* it = node;
				while(it->next != 0) it = it->next;
				it->next = n;
			}
			listSize++;

			if(jsonPeekToken(buffer) != JSON_TOKEN_COMMA) break;
			myAssert(jsonGetToken(buffer).type == JSON_TOKEN_COMMA);
		}	
	}
	myAssert(jsonGetToken(buffer).type == JSON_TOKEN_BRACE_CLOSE);

	JSonValue* value = getTStruct(JSonValue);
	value->type = JSON_OBJECT;
	value->size = listSize;
	value->array = getTArray(JSonValue, listSize);
	JSonNode* it = node;
	for(int i = 0; i < listSize; i++) {
		value->array[i] = it->value;
		it = it->next;
	}

	return value;
}

JSonValue* jsonParseValue(char** buffer) {

	int tokenType = jsonPeekToken(buffer);
	if(tokenType == JSON_TOKEN_BRACE_OPEN) {
		JSonValue* value = jsonParseObject(buffer);
		return value;
	} else if(tokenType == JSON_TOKEN_BRACKET_OPEN) {
		JSonValue* value = jsonParseArray(buffer);
		return value;
	} else if(tokenType == JSON_TOKEN_STRING) {
		JSonValue* value = jsonParseString(buffer);
		return value;
	} else if(tokenType == JSON_TOKEN_TRUE || tokenType == JSON_TOKEN_FALSE) {
		jsonGetToken(buffer);
		JSonValue* value = getTStruct(JSonValue);
		value->type = JSON_BOOL;
		value->b = tokenType == JSON_TOKEN_TRUE ? true : false;
		return value;
	} else if(tokenType == JSON_TOKEN_NUMBER) {
		Token token = jsonGetToken(buffer);
		JSonValue* value = getTStruct(JSonValue);
		value->type = JSON_NUMBER;
		value->number = strToFloat(token.data);
		return value;
	} else if(tokenType == JSON_TOKEN_NULL) {
		jsonGetToken(buffer);
		JSonValue* value = getTStruct(JSonValue);
		value->type = JSON_NULL;
		return value;
	}

	return 0;
}

void jsonPrint(JSonValue* value) {
	if(value->type == JSON_STRING) {
		printf("\"%s\"", value->string);
	} else if(value->type == JSON_NUMBER) {
		printf("%f", value->number);
	} else if(value->type == JSON_BOOL) {
		printf("\"%i\"", value->b);
	} else if(value->type == JSON_NULL) {
		printf("null");
	} else if(value->type == JSON_OBJECT) {
		printf("{\n");
		for(int i = 0; i < value->size; i++) {
			printf("\"%s\": ", value->array[i].string);
			jsonPrint(value->array[i].value);
			printf(", \n");
		}
		printf("}\n");
	} else if(value->type == JSON_ARRAY) {
		printf("[\n");
		for(int i = 0; i < value->size; i++) {
			jsonPrint(value->array + i);
			printf(", \n");
		}
		printf("]\n");
	} 
}

JSonValue* jsonGetMemberSingle(JSonValue* object, char* name) {
	JSonValue* array = object->array;
	JSonValue* member = 0;
	for(int i = 0; i < object->size; i++) {
		if(strCompare(array[i].string, name)) {
			member = array[i].value;
			break;
		}
	}

	return member;
}

JSonValue* jsonGetMember(JSonValue* object, char* name, char* name2 = 0, char* name3 = 0) {
	JSonValue* member = jsonGetMemberSingle(object, name);
	if(member && name2) member = jsonGetMemberSingle(member, name2);
	if(member && name3) member = jsonGetMemberSingle(member, name3);

	return member;
}

int jsonGetInt(JSonValue* object, char* name, char* name2 = 0, char* name3 = 0) {
	JSonValue* member = jsonGetMember(object, name, name2, name3);
	if(member) return roundInt(member->number);
	else return 0; // Bad idea?
}

char* jsonGetString(JSonValue* object, char* name, char* name2 = 0, char* name3 = 0) {
	JSonValue* member = jsonGetMember(object, name, name2, name3);
	if(member) return member->string;
	else return 0;
}

#define For_JsonArray(value) for(JSonValue* it = value->array; it != value->array+value->size; it++) 



#define For_Layout(layout) \
	for(Layout* l = layout->list; l != 0; l = l->next)

struct Layout {
	Vec2i align;
	Rect r;

	Layout* next;
	Layout* list;

	Vec2 minDim;
	Vec2 maxDim;
	Vec2 dim;

	Vec2 finalDim;

	Vec2 padding;
	Vec2 borderPadding;
	bool vAxis;
};

Layout* layoutAlloc(Layout node) {
	Layout* newNode = getTStruct(Layout);
	*newNode = node;

	return newNode;
}

Layout layout(Vec2 dim, bool vAxis = false, Vec2i align = vec2i(0,0), Vec2 padding = vec2(0,0), Vec2 borderPadding = vec2(0,0)) {
	Layout node = {};
	node.dim = dim;
	node.align = align;
	node.vAxis = vAxis;
	node.borderPadding = borderPadding;
	node.padding = padding;

	return node;
}

Layout layout(Rect region, bool vAxis = false, Vec2i align = vec2i(0,0), Vec2 padding = vec2(0,0), Vec2 borderPadding = vec2(0,0)) {
	Layout node = {};
	node.r = region;
	node.align = align;
	node.vAxis = vAxis;
	node.borderPadding = borderPadding;
	node.padding = padding;

	return node;
}
void layout(Layout* node, Rect region, bool vAxis = false, Vec2i align = vec2i(0,0), Vec2 padding = vec2(0,0), Vec2 borderPadding = vec2(0,0)) {
	*node = layout(region, vAxis, align, padding, borderPadding);
}

Rect layoutGetRect(Layout* node) {
	return rectExpand(node->r, -node->borderPadding*2);
}

Vec2 layoutGetDim(Layout* node) {
	return rectDim(layoutGetRect(node));
}

Rect layoutInc(Layout** node) {
	Rect r = (*node)->r;
	(*node) = (*node)->next;
	return r;
}

void layoutAdd(Layout* node, Layout* newNode, bool addEnd = true) {
	Layout* list = node->list;

	if(list == 0) node->list = newNode;
	else {
		if(addEnd) {
			while(list->next != 0) list = list->next;
			list->next = newNode;
		} else {
			newNode->next = list;
			node->list = newNode;
		}
	}
}

Layout* layoutAdd(Layout* node, Layout nn, bool addEnd = true) {
	Layout* newNode = getTStruct(Layout);
	*newNode = nn;

	layoutAdd(node, newNode, addEnd);
	return newNode;
}

void layoutCalcSizes(Layout* mainNode) {
	Layout* n;
	bool vAxis = mainNode->vAxis;
	Rect mainRect = layoutGetRect(mainNode);

	if(rectZero(mainRect)) return;

	int size = 0;
	n = mainNode->list;
	while(n != 0) {
		size++;
		n = n->next;
	}

	float dim = vAxis==0? rectW(mainRect) : rectH(mainRect);
	float dim2 = vAxis==1? rectW(mainRect) : rectH(mainRect);
	float offset = mainNode->padding.e[vAxis];
	float totalWidth = dim - offset*(size-1);

	float dynamicSum = 0;
	int flowCount = 0;
	float staticSum = 0;
	int staticCount = 0;

	float widthWithoutFloats = 0;

	n = mainNode->list;
	while(n != 0) {
		float val = n->dim.e[vAxis];
			
		if(val < 0) {}
		else if(val == 0) flowCount++;
		else if(val <= 1) widthWithoutFloats += val*totalWidth;
		else widthWithoutFloats += val;

		val = n->dim.e[(vAxis+1)%2];
		if(val == 0) n->dim.e[(vAxis+1)%2] = dim2;
		else if(val <= 1) n->dim.e[(vAxis+1)%2] = val * dim2;

		n = n->next;
	}


	float flowVal = flowCount>0 ? (totalWidth-widthWithoutFloats)/flowCount : 0;
	flowVal = clampMin(flowVal, 0);
	n = mainNode->list;
	while(n != 0) {
		n->finalDim = n->dim;

		float val = n->dim.e[vAxis];

		if(val < 0) n->finalDim.e[vAxis] = 0;
		else if(val == 0) n->finalDim.e[vAxis] = flowVal;
		else if(val <= 1) n->finalDim.e[vAxis] = val*totalWidth;

		clampMin(&n->finalDim.e[vAxis], 0);

		if(n->minDim.x != 0) clampMin(&n->finalDim.x, n->minDim.x);
		if(n->maxDim.x != 0) clampMax(&n->finalDim.x, n->maxDim.x);
		if(n->minDim.y != 0) clampMin(&n->finalDim.y, n->minDim.y);
		if(n->maxDim.y != 0) clampMax(&n->finalDim.y, n->maxDim.y);

		n = n->next;
	}

}

void layoutCalcRects(Layout* mainNode) {
	Rect mainRect = layoutGetRect(mainNode);
	if(rectZero(mainRect)) return;

	bool vAxis = mainNode->vAxis;
	Layout* node;

	Vec2 boundingDim = vec2(0,0);
	node = mainNode->list;
	while(node != 0) {
		boundingDim.e[vAxis] += node->finalDim.e[vAxis] + mainNode->padding.e[vAxis];
		boundingDim.e[(vAxis+1)%2] = max(boundingDim.e[(vAxis+1)%2], node->finalDim.e[(vAxis+1)%2]);

		node = node->next;
	}
	boundingDim.e[vAxis] -= mainNode->padding.e[vAxis];

	Vec2i align = mainNode->align;
	Vec2 currentPos = rectAlign(mainRect, align);

	if(vAxis == false) {
		currentPos.x -= boundingDim.x * (align.x+1)/2;
		currentPos.y += boundingDim.y * (-align.y)/2;

		node = mainNode->list;
		while(node != 0) {
			Vec2 p = currentPos;
			p.y -= node->finalDim.y/2;

			node->r = rectBLDim(p, node->finalDim);
			currentPos.x += node->finalDim.x + mainNode->padding.x;

			node = node->next;
		}
	} else {
		currentPos.y -= boundingDim.y * (align.y-1)/2;
		currentPos.x += boundingDim.x * (-align.x)/2;

		node = mainNode->list;
		while(node != 0) {
			Vec2 p = currentPos;
			p.x -= node->finalDim.x/2;

			node->r = rectTLDim(p, node->finalDim);
			currentPos.y -= node->finalDim.y + mainNode->padding.y;

			node = node->next;
		}
	}

}

void layoutCalc(Layout* mainNode, bool recursive = true) {
	layoutCalcSizes(mainNode);
	layoutCalcRects(mainNode);

	if(recursive) {
		Layout* node = mainNode->list;
		while(node != 0) {
			if(node->list != 0) {
				layoutCalc(node, true);			
			}

			node = node->next;
		}
	}
}

Layout* layoutQuickRow(Layout* node, Rect region, float s1, float s2 = -1, float s3 = -1, float s4 = -1) {
	node->r = region;
	node->list = 0;
	layoutAdd(node, layout(!node->vAxis ? vec2(s1,0) : vec2(0,s1)));
	if(s2 != -1) layoutAdd(node, layout(!node->vAxis ? vec2(s2,0) : vec2(0,s2)));
	if(s3 != -1) layoutAdd(node, layout(!node->vAxis ? vec2(s3,0) : vec2(0,s3)));
	if(s4 != -1) layoutAdd(node, layout(!node->vAxis ? vec2(s4,0) : vec2(0,s4)));

	layoutCalc(node);

	return node->list;
}



union NewAppColors {
	struct {
		Vec4 font;
		Vec4 font2;
		Vec4 graphFont;
		Vec4 fontShadow;

		Vec4 windowBorder;
		Vec4 background;
		Vec4 button;
		Vec4 uiBackground;
		Vec4 edge;
		Vec4 editCursor;
		Vec4 editSelection;

		Vec4 graphData1;
		Vec4 graphData2;
		Vec4 graphData3;
		Vec4 graphBackgroundBottom;
		Vec4 graphBackgroundTop;
		Vec4 graphMark;
		Vec4 graphSubMark;
	};

	Vec4 e[20];
};

char* newAppColorsStrings[] = {
	"font",
	"font2",
	"graphFont",
	"fontShadow",
	"windowBorder",
	"background",
	"button",
	"uiBackground",
	"edge",
	"editCursor",
	"editSelection",
	"graphData1",
	"graphData2",
	"graphData3",
	"graphBackgroundBottom",
	"graphBackgroundTop",
	"graphMark",
	"graphSubMark",
};

struct NewAppSettings {
	char* font;
	char* fontBold;
	char* fontItalic;

	int fontHeight;
	float fontShadow;
	int graphTitleFontHeight;
	float graphFontShadow;

	float windowHeightMod;
	float heightMod;
	float textPaddingMod;

	float rounding;

	int windowBorder;
	int border;
	int padding;
};

struct AppData {
	// General.

	SystemData systemData;
	Input input;
	WindowSettings wSettings;
	GraphicsState graphicsState;

	f64 dt;
	f64 time;
	int frameCount;
	i64 swapTime;

	bool updateFrameBuffers;

	int msaaSamples;

	// Window.

	Rect clientRect;
	Vec2i frameBufferSize;
	bool screenShotMode;

	// App.

	bool reloadSettings;
	FILETIME settingsFileLastWriteTime;

	AppColors appColors;
	AppSettings appSettings;

	NewAppColors newAppColors;
	NewAppSettings newAppSettings;

	bool appIsBusy;

	Font* font;
	Font* fontTitle;

	CURL* curlHandle;
	HMODULE curlDll;

	GraphCam cams[10];
	int camCount;

	int heightMoveMode;
	bool widthMove;

	float leftTextWidth;

	float graphOffsets[Line_Graph_Count+1];
	float sidePanelWidth;
	float sidePanelMax;

	//

	DownloadInfo dInfo;
	CurlRequestData requestData;
	DownloadModeData modeData;

	//

	NewGui newGui;
	NewGui* gui;

	Rect panelRect;
	bool panelActive;

	TextBoxSettings labelSettings;
	TextBoxSettings buttonSettings;
	TextBoxSettings uiButtonSettings;
	TextBoxSettings scrollButtonSettings;
	TextEditSettings editSettings;
	SliderSettings sliderSettings;
	ScrollRegionSettings scrollSettings;
	ScrollRegionSettings commentScrollSettings;

	//

	int playlistFolderIndex;
	YoutubePlaylist playlist;
	YoutubeVideo* videos;
	int maxVideoCount;

	VideoSnippet videoSnippet;
	Vec2 hoveredPoint;
	int hoveredVideo;
	float hoveredVideoStat;
	float commentScrollValue;

	Vec2 selectedPoint;
	int selectedVideo;

	YoutubePlaylist downloadPlaylist;

	bool startScreenShot;
	char screenShotFilePath[50];
	Vec2i screenShotDim;

	char* searchString;
	char* searchStringPlaylists;
	char* searchStringChannels;

	bool startDownload;
	bool update;
	bool startLoadFile;

	char* userName;
	char* channelId;
	int playlistDownloadCount;

	YoutubePlaylist* playlists;
	int playlistCount;

	SearchResult* searchResults;
	int searchResultCount;
	int searchResultMaxCount;

	int lastSearchMode;

	YoutubePlaylist playlistFolder[50];
	int playlistFolderCount;

	float statTimeSpan;
	float statWidth;

	Statistic stats[Line_Graph_Count];
	LineGraph averagesLineGraph;
	LineGraph releaseCountLineGraph;

	char exclusiveFilter[50];
	char inclusiveFilter[50];

	int graphDrawMode;

	bool sortByDate;
	int sortStat;

	Vec2 clampFilter[Line_Graph_Count];
};



void debugMain(DebugState* ds, AppMemory* appMemory, AppData* ad, bool reload, bool* isRunning, bool init, ThreadQueue* threadQueue);

// #pragma optimize( "", off )
#pragma optimize( "", on )
extern "C" APPMAINFUNCTION(appMain) {

	i64 startupTimer = timerInit();

	if(init) {

		// Init memory.

		SYSTEM_INFO info;
		GetSystemInfo(&info);

		char* baseAddress = (char*)gigaBytes(8);
	    VirtualAlloc(baseAddress, gigaBytes(40), MEM_RESERVE, PAGE_READWRITE);

		ExtendibleMemoryArray* pMemory = &appMemory->extendibleMemoryArrays[appMemory->extendibleMemoryArrayCount++];
		initExtendibleMemoryArray(pMemory, megaBytes(50), info.dwAllocationGranularity, baseAddress);

		ExtendibleBucketMemory* dMemory = &appMemory->extendibleBucketMemories[appMemory->extendibleBucketMemoryCount++];
		initExtendibleBucketMemory(dMemory, megaBytes(1), megaBytes(50), info.dwAllocationGranularity, baseAddress + gigaBytes(16));

		MemoryArray* tMemory = &appMemory->memoryArrays[appMemory->memoryArrayCount++];
		initMemoryArray(tMemory, megaBytes(30), baseAddress + gigaBytes(33));



		MemoryArray* pDebugMemory = &appMemory->memoryArrays[appMemory->memoryArrayCount++];
		initMemoryArray(pDebugMemory, megaBytes(50), 0);

		MemoryArray* tMemoryDebug = &appMemory->memoryArrays[appMemory->memoryArrayCount++];
		initMemoryArray(tMemoryDebug, megaBytes(30), 0);

		ExtendibleMemoryArray* debugMemory = &appMemory->extendibleMemoryArrays[appMemory->extendibleMemoryArrayCount++];
		initExtendibleMemoryArray(debugMemory, megaBytes(30), info.dwAllocationGranularity, baseAddress + gigaBytes(34));
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
		// ds->assets = getPArrayDebug(Asset, 100);

		#ifndef RELEASE_BUILD
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
		#else 
			ds->timer = getPStructDebug(Timer);
			ds->timer->bufferSize = 1;
			globalTimer = ds->timer;
		#endif

		TIMER_BLOCK_NAMED("Init");

		ds->gui = getPStructDebug(Gui);
		// gui->init(rectCenDim(vec2(0,1), vec2(300,800)));
		// gui->init(rectCenDim(vec2(1300,1), vec2(300,500)));
		ds->gui->init(rectCenDim(vec2(1300,1), vec2(300, ws->currentRes.h)), 0);

		// ds->gui->cornerPos = 

		ds->gui2 = getPStructDebug(Gui);
		// ds->gui->init(rectCenDim(vec2(1300,1), vec2(400, ws->currentRes.h)), -1);
		ds->gui2->init(rectCenDim(vec2(1300,1), vec2(300, ws->currentRes.h)), 3);

		ds->input = getPStructDebug(Input);
		// ds->showMenu = true;
		ds->showMenu = false;
		ds->showStats = false;
		ds->showConsole = false;
		ds->showHud = true;
		// ds->guiAlpha = 0.95f;
		ds->guiAlpha = 1;
		ds->noCollating = true;

		#ifdef RELEASE_BUILD
			ds->noCollating = true;
		#else 
			ds->noCollating = false;
		#endif

		for(int i = 0; i < arrayCount(ds->notificationStack); i++) {
			ds->notificationStack[i] = getPStringDebug(DEBUG_NOTE_LENGTH+1);
		}

		//
		// AppData.
		//

		TIMER_BLOCK_BEGIN_NAMED(initAppData, "Init AppData");

		getPMemory(sizeof(AppData));
		*ad = {};
		
		initSystem(systemData, ws, windowsData, vec2i(1920*0.85f, 1080*0.85f), true, true, false, 1);
		windowHandle = systemData->windowHandle;

	    printf("%Opengl Version: %s\n", (char*)glGetString(GL_VERSION));

		loadFunctions();

		const char* extensions = wglGetExtensionsStringEXT();

		wglSwapIntervalEXT(1);
		int fps = wglGetSwapIntervalEXT();

		initInput(&ad->input);

		// Init curl.

		ad->curlDll = LoadLibraryA("libcurl.dll");
		loadCurlFunctions(ad->curlDll);

		updateResolution(windowHandle, ws);
		globalScreenHeight = ws->currentRes.h;

		TIMER_BLOCK_END(initAppData);


		//
		// Setup Textures.
		//

		TIMER_BLOCK_BEGIN_NAMED(initGraphics, "Init Graphics");

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

		// 
		// Samplers.
		//

		gs->samplers[SAMPLER_NORMAL] = createSampler(16.0f, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
		// gs->samplers[SAMPLER_NORMAL] = createSampler(16.0f, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST);

		//
		//
		//

		ad->msaaSamples = 4;
		ad->dt = 1/(float)60;

		ad->graphicsState.zOrder = 0;

		//
		// FrameBuffers.
		//

		{
			for(int i = 0; i < FRAMEBUFFER_SIZE; i++) {
				FrameBuffer* fb = getFrameBuffer(i);
				initFrameBuffer(fb);
			}

			attachToFrameBuffer(FRAMEBUFFER_2dMsaa, FRAMEBUFFER_SLOT_COLOR, GL_RGBA16F, 0, 0, ad->msaaSamples);
			attachToFrameBuffer(FRAMEBUFFER_2dMsaa, FRAMEBUFFER_SLOT_DEPTH, GL_DEPTH_COMPONENT32F, 0, 0, ad->msaaSamples);
			attachToFrameBuffer(FRAMEBUFFER_2dNoMsaa, FRAMEBUFFER_SLOT_COLOR, GL_RGBA16F, 0, 0);

			attachToFrameBuffer(FRAMEBUFFER_DebugMsaa, FRAMEBUFFER_SLOT_COLOR, GL_RGBA8, 0, 0, ad->msaaSamples);
			attachToFrameBuffer(FRAMEBUFFER_DebugNoMsaa, FRAMEBUFFER_SLOT_COLOR, GL_RGBA8, 0, 0);

			// attachToFrameBuffer(FRAMEBUFFER_ScreenShot, FRAMEBUFFER_SLOT_COLOR, GL_SRGB8_ALPHA8, 0, 0);
			attachToFrameBuffer(FRAMEBUFFER_ScreenShot, FRAMEBUFFER_SLOT_COLOR, GL_SRGB8, 0, 0);

			ad->updateFrameBuffers = true;



			// ad->frameBufferSize = vec2i(2560, 1440);
			ad->frameBufferSize = ws->biggestMonitorSize;
			Vec2i fRes = ad->frameBufferSize;

			setDimForFrameBufferAttachmentsAndUpdate(FRAMEBUFFER_2dMsaa, fRes.w, fRes.h);
			setDimForFrameBufferAttachmentsAndUpdate(FRAMEBUFFER_2dNoMsaa, fRes.w, fRes.h);
			setDimForFrameBufferAttachmentsAndUpdate(FRAMEBUFFER_DebugMsaa, fRes.w, fRes.h);
			setDimForFrameBufferAttachmentsAndUpdate(FRAMEBUFFER_DebugNoMsaa, fRes.w, fRes.h);

			// setDimForFrameBufferAttachmentsAndUpdate(FRAMEBUFFER_ScreenShot, fRes.w, fRes.h);
		}

		TIMER_BLOCK_END(initGraphics);

		//

		TIMER_BLOCK_BEGIN_NAMED(initRest, "Init Rest");


		HANDLE fileChangeHandle  = FindFirstChangeNotification(App_Folder, false, FILE_NOTIFY_CHANGE_LAST_WRITE);
		if(fileChangeHandle == INVALID_HANDLE_VALUE) printf("Could not set folder change notification.\n");
		systemData->folderHandles[systemData->folderHandleCount++] = fileChangeHandle;
		ad->settingsFileLastWriteTime = getLastWriteTime(App_Settings_File);

		// fileChangeHandle = FindFirstChangeNotification(Playlist_Folder, false, FILE_NOTIFY_CHANGE_LAST_WRITE);
		fileChangeHandle = FindFirstChangeNotification(Playlist_Folder, false, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE);
		if(fileChangeHandle == INVALID_HANDLE_VALUE) printf("Could not set folder change notification.\n");
		systemData->folderHandles[systemData->folderHandleCount++] = fileChangeHandle;


		globalGraphicsState->fontFolders[globalGraphicsState->fontFolderCount++] = getPStringCpy(App_Font_Folder);
		globalGraphicsState->fontFolders[globalGraphicsState->fontFolderCount++] = getPStringCpy(Windows_Font_Folder);


		ad->curlHandle = curl_easy_initX();

		int maxVideoCount = 5000;
		ad->maxVideoCount = maxVideoCount;
		ad->videos = (YoutubeVideo*)malloc(sizeof(YoutubeVideo)*maxVideoCount);
		ad->videoSnippet.thumbnailTexture.id = -1;
		ad->selectedVideo = -1;

		ad->channelId = (char*)getPMemory(100); strClear(ad->channelId);
		ad->userName = (char*)getPMemory(100); strClear(ad->userName);
		ad->searchString = (char*)getPMemory(100); strClear(ad->searchString);
		ad->searchStringPlaylists = (char*)getPMemory(100); strClear(ad->searchStringPlaylists);
		ad->searchStringChannels = (char*)getPMemory(100); strClear(ad->searchStringChannels);

		ad->statTimeSpan = 3.0f;
		ad->statWidth = 1.0f;

		strClear(ad->screenShotFilePath);
		strCpy(ad->screenShotFilePath, Screenshot_File);
		ad->screenShotDim = vec2i(5000, 1000);

		ad->leftTextWidth = 0;
		ad->camCount = 4;

		ad->graphOffsets[0] = 0.0f;
		ad->graphOffsets[1] = 0.4f;
		ad->graphOffsets[2] = 0.6f;
		ad->graphOffsets[3] = 0.8f;
		ad->graphOffsets[4] = 1.0f;

		ad->sidePanelWidth = ws->currentRes.w*0.25f;
		ad->sidePanelMax = 0.5f;

		// Make test file.
		if(false) 
		{
			// int count = 10000;
			int count = 100000;
			// int count = 1000;

			YoutubePlaylist playlist = {"PLAYLIST_ID", "PLAYLIST_TITLE", count};

			Date d = {5, 1, 1, 0, 0, 0, 0};
			dateEncode(&d);

			clearTMemory();
			// YoutubeVideo* videos = getTArray(YoutubeVideo, count);
			YoutubeVideo* videos = (YoutubeVideo*)malloc(sizeof(YoutubeVideo)*count);
			for(int i = 0; i < count; i++) {
				// d.n += randomInt(20*60*60,50*60*60);
				d.n += randomInt(20*60,2*60*60);
				dateDecode(&d);

				char* dateString = fillString("%i %i %i %i:%i:%i", (int)d.y, (int)d.m, (int)d.d, (int)d.h, (int)d.mins, (int)d.secs);
				// char* dateString = "";

	            videos[i] = {"Video_Id", "Video_Title", "Video_Thumbnail_Url", "", 
	                          d, 
	                          randomInt(5000,15000), 
	                          randomInt(100,2000), 
	                          randomInt(10,200), 
	                          randomInt(10,100), 
	                          randomInt(10,100) };

	           	strCpy(videos[i].dateString, dateString);
			}

			savePlaylistToFile(&playlist, videos, count, count, 0, "PageToken");

			free(videos);
		}
		
		ad->graphDrawMode = 0;
		ad->sortByDate = true;
		ad->sortStat = -1;

		ad->requestData.curlHandle = ad->curlHandle;
		ad->requestData.maxSize = megaBytes(1);
		ad->requestData.buffer = (char*)getPMemory(ad->requestData.maxSize);

		TIMER_BLOCK_END(initRest);

		ad->panelActive = false;

		ad->searchResultMaxCount = 300;
		ad->searchResults = getPArray(SearchResult, ad->searchResultMaxCount);



		// ad->panelActive = true;
		// strCpy(ad->downloadPlaylist.title, "Royal Beef");
		// strCpy(ad->downloadPlaylist.id, "PLsksxTH4pR3IX9CL91UVp-6S9mFBmnlgF");
		// ad->downloadPlaylist.count = 18;

		// strCpy(ad->downloadPlaylist.title, "Rocket Beans TV");
		// strCpy(ad->downloadPlaylist.id, "UUQvTDmHza8erxZqDkjQ4bQQ");
		// ad->downloadPlaylist.count = 6438;


		// char* filePath = fillString("%s\\%s.playlist", Playlist_Folder, ad->downloadPlaylist.id);
		// remove(filePath);
	}



	// @AppStart.

	TIMER_BLOCK_BEGIN_NAMED(reload, "Reload");

	if(reload) {
		loadFunctions();
		SetWindowLongPtr(systemData->windowHandle, GWLP_WNDPROC, (LONG_PTR)mainWindowCallBack);

		globalScreenHeight = ws->currentRes.h;

		loadCurlFunctions(ad->curlDll);
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

	// Update input. (And other stuff.)
	{
		TIMER_BLOCK_NAMED("Input");

		int inputWaitMask = 0;
		int waitTimeout = 0;
		#ifdef RELEASE_BUILD
			inputWaitMask = QS_ALLINPUT;
			waitTimeout = INFINITE;
		#endif

		int message = -1;
		if(!ad->appIsBusy) message = MsgWaitForMultipleObjects(systemData->folderHandleCount, systemData->folderHandles, false, waitTimeout, inputWaitMask);

		if(message == WAIT_OBJECT_0 + WATCH_FOLDER_APP) {
			FindNextChangeNotification(systemData->folderHandles[WATCH_FOLDER_APP]);

			FILETIME newWriteTime = getLastWriteTime(App_Settings_File);

			if(CompareFileTime(&newWriteTime, &ad->settingsFileLastWriteTime) != 0) {
				ad->reloadSettings = true;
				ad->settingsFileLastWriteTime = newWriteTime;
			}
		} else if(message == WAIT_OBJECT_0 + WATCH_FOLDER_PLAYLISTS) {
			FindNextChangeNotification(systemData->folderHandles[WATCH_FOLDER_PLAYLISTS]);

			loadPlaylistFolder(ad->playlistFolder, &ad->playlistFolderCount);
		} else if(message == WAIT_OBJECT_0 + WATCH_FOLDER_SIZE) {
			// Input happened.
		}

		updateInput(ds->input, isRunning, windowHandle);
		ad->appIsBusy = false;

		ad->input = *ds->input;

		if(ds->console.isActive) {
			memSet(ad->input.keysPressed, 0, sizeof(ad->input.keysPressed));
			memSet(ad->input.keysDown, 0, sizeof(ad->input.keysDown));
		}

		if(input->mouseButtonPressed[0]) {
			ad->appIsBusy = true;
		}

		ad->dt = ds->dt;
		ad->time = ds->time;

		ad->frameCount++;

		ad->gui = &ad->newGui;
		newGuiBegin(ad->gui, input);
		ad->gui->windowSettings = ws;

		if(!ws->customCursor) {
			SetCursor(LoadCursor(0, IDC_ARROW));
		}
		ws->customCursor = false;

		if(init || reload)
		{
			NewAppSettings as;
			NewAppColors ac;

			as.font = "OpenSans-Regular.ttf";
			as.fontBold = "OpenSans-Bold.ttf";
			as.fontItalic = "OpenSans-Italic.ttf";
			as.fontHeight = 20;
			// as.fontHeight = 19;
			as.fontShadow = 2;
			// as.commentFont = "SourceSansPro-Regular.ttf";
			// as.commentFont = "OpenSans-Regular.ttf";
			// as.commentFont = "arial.ttf";
			// as.commentFontHeight = 20;
			// as.commentFontHeight = 19;
			// as.commentFontShadow = 2;

			as.graphTitleFontHeight = 30;
			as.graphFontShadow = 1;

			as.windowHeightMod = 1.0f;
			as.windowBorder = 4;
			as.border = 4;
			as.padding = 4;
			as.heightMod = 1.2f;

			as.textPaddingMod = 0.5f;

			as.rounding = 5;



			ac.font = vec4(0.8f, 1);
			// ac.font = vec4(0.7f, 1);
			ac.font2 = vec4(0.6f, 1);
			ac.fontShadow = vec4(0.1f,1);
			// ac.commentFont = ac.font;
			// ac.commentFontShadow = ac.fontShadow;

			Vec3 hslBackground = vec3(0.65f, 0.5f, 0.3f);
			Vec3 hslButton = hslBackground + vec3(0.25f,0,0);
			Vec3 hslGraph = hslBackground + vec3(0.1f,0,0);

			ac.background =            vec4(0.2f,1);
			ac.windowBorder =          ac.background + vec4(+0.08f,0);
			ac.button =                ac.background + vec4(0.05f,0);
			ac.uiBackground =		   ac.background + vec4(-0.03f,0);

			ac.graphFont =             ac.font;
			ac.graphBackgroundTop =    hslToRgbFloatAlpha(hslBackground + vec3(0,-0.4f,-0.12f));
			ac.graphBackgroundBottom = hslToRgbFloatAlpha(hslBackground + vec3(0,-0.54f,-0.2f));

			ac.graphData1 =            hslToRgbFloatAlpha(hslGraph + vec3(-0.25f, 0.2f,0.1f));
			ac.graphData2 =            hslToRgbFloatAlpha(hslGraph + vec3(0.18f,0.2f,0.1f));
			ac.graphData3 =            hslToRgbFloatAlpha(hslGraph + vec3(0.5,   0.2f,0.1f));

			ac.graphMark =             vec4(1,0.05f);
			ac.graphSubMark =          vec4(1,0.025f);

			ac.editCursor =            hslToRgbFloatAlpha(hslBackground + vec3(-0.3f,-0.1,0.3f));
			ac.editSelection =         hslToRgbFloatAlpha(hslGraph + vec3(0.25f,0.0f,0.1f));
			ac.edge =                  ac.background + vec4(0.2f,0);

			// 

			ad->newAppSettings = as;
			ad->newAppColors = ac;
		}


		// if(init || ad->reloadSettings) {
		if(init || ad->reloadSettings || reload) {
			ad->reloadSettings = false;

			if(!fileExists(App_Settings_File)) {
				AppSettings as;
				AppColors ac;
				appSettingsDefault(&as, &ac);

				appWriteSettings(App_Settings_File, &as, &ac);
			}

			appReadSettings(App_Settings_File, &ad->appSettings, &ad->appColors);


			{
				NewAppSettings* as = &ad->newAppSettings;

				ad->font = getFont(as->font, as->fontHeight, as->fontBold, as->fontItalic);
				ad->fontTitle = getFont(as->font, as->graphTitleFontHeight, as->fontBold, as->fontItalic);

			// 	NewAppSettings* as = &ad->newAppSettings;
			// 	NewAppColors* ac = &ad->newAppColors;

			// 	// Check folders for fonts.
			// 	int fontCount = 2;
			// 	for(int i = 0; i < fontCount; i++) {
			// 		char* fileName;
			// 		int fontSize;
			// 		if(i == 0) {
			// 			fileName = as->font;
			// 			fontSize = as->fontHeight;
			// 		} else if(i == 1) {
			// 			fileName = as->font;
			// 			fontSize = as->graphTitleFontHeight;
			// 		} 
			// 		char* filePath = App_Font_Folder;

			// 		if(!fileExists(fillString("%s%s", filePath, fileName))) {
			// 			if(fileExists(fillString("%s%s", Windows_Font_Folder, fileName))) filePath = Windows_Font_Folder;
			// 			else fileName = Default_Font;
			// 		}

			// 		Font* font = fontInit(&gs->fonts[FONT_SIZE + i][0], fileName, filePath, fontSize, -1);
			// 		if(i == 0) ad->font = font;
			// 		else if(i == 1) ad->fontBold = font;
			// 		else if(i == 2) ad->fontTitle = font;
			// 		else ad->fontComment = font;
			// 	}
			}
		}

		if(init) {
			if(!fileExists(App_Save_File)) {
				AppTempSettings at = {};
				appTempDefault(&at, ws->monitors[0].workRect);

				appWriteTempSettings(App_Save_File, &at);
			}

			AppTempSettings at = {};
			{
				appReadTempSettings(App_Save_File, &at);

				Rect r = at.windowRect;
		    	MoveWindow(windowHandle, r.left, r.top, r.right-r.left, r.bottom-r.top, true);

		    	ad->sidePanelWidth = at.sidePanelWidth;
				memCpy(ad->graphOffsets, at.graphOffsets, memberSize(AppTempSettings, graphOffsets));
				ad->playlistFolderIndex = at.playlistFolderIndex;

				ad->panelRect = at.panelRect;
			}
		}



		{
			NewAppSettings* newas = &ad->newAppSettings;
			NewAppColors* newac = &ad->newAppColors;

			Font* font = ad->font;
			// Font* font = ad->fontSlim;
			float textPadding = font->height * newas->textPaddingMod;

			TextBoxSettings textSettings = textBoxSettings(font, newac->uiBackground, newac->font, newas->fontShadow, newac->fontShadow, newas->rounding, newac->edge);

			ad->labelSettings = textBoxSettings(font, vec4(0,0), newac->font, newas->fontShadow, newac->fontShadow, 0, vec4(0,0));
			ad->buttonSettings = textBoxSettings(font, newac->button, newac->font, newas->fontShadow, newac->fontShadow, newas->rounding, newac->edge);
			ad->uiButtonSettings = ad->buttonSettings;
			ad->uiButtonSettings.roundedCorner = 0;

			ad->scrollButtonSettings = ad->buttonSettings;
			ad->scrollButtonSettings.roundedCorner = 0;
			ad->scrollButtonSettings.borderColor = vec4(0,0);

			ad->editSettings = textEditSettings(textSettings, ad->gui->editText, true, true, 2, newac->editSelection, newac->editCursor, font->height * newas->textPaddingMod);
			ad->sliderSettings = {textSettings, 20, 20, 0, 0, 0, newac->button, newac->font};
			ad->scrollSettings = {vec2(newas->padding,0), 20, vec2(4,4), 6, 0, 40, font->height+1, true, true, true, false, newac->button, vec4(0,0), newac->uiBackground, newac->edge};
			ad->commentScrollSettings = {vec2(textPadding,0), 20, vec2(4,4), 6, 0, 40, font->height+1, true, true, true, true, newac->button, vec4(0,0), newac->uiBackground, newac->edge};

		}

		{
			NewGui* gui = ad->gui;
			gui->buttonSettings = ad->buttonSettings;
			gui->textBoxSettings = ad->labelSettings;
			gui->editSettings = ad->editSettings;
			gui->sliderSettings = ad->sliderSettings;
			gui->scrollSettings = ad->scrollSettings;

			gui->zLevel = 2;

			gui->scissor = rectCenDim(0,0,10000000,10000000);
			gui->scissorStack[0] = gui->scissor;
			gui->scissorStackIndex = 0;

			gui->layoutStackIndex = 0;
		}
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
		if(ws->fullscreen) {
			if(input->keysPressed[KEYCODE_ESCAPE]) setWindowMode(windowHandle, ws, WINDOW_MODE_WINDOWED);
		} else {
			// *isRunning = false;
		}
	}

	if(input->keysPressed[KEYCODE_F11]) {
		if(ws->fullscreen) setWindowMode(windowHandle, ws, WINDOW_MODE_WINDOWED);
		else setWindowMode(windowHandle, ws, WINDOW_MODE_FULLBORDERLESS);
	}

	if(input->keysPressed[KEYCODE_F1]) {
		ad->panelActive = !ad->panelActive;
	}

	// Window drag.
	bool resize = false;
	if(!ws->fullscreen)
	{
		TIMER_BLOCK_NAMED("WindowDrag");

	    float z = 0;

	    RECT r; 
	    GetWindowRect(windowHandle, &r);
	    Rect wr = rect(r.left, -r.bottom, r.right, -r.top);

	    GuiWindowSettings windowSettings = {ad->appSettings.windowBorder, ad->appSettings.windowBorder*4, vec2(Window_Min_Size_X, Window_Min_Size_Y), vec2(ws->biggestMonitorSize), rect(0,0,0,0), true, true, true, true};
	    bool active = newGuiWindowUpdate(ad->gui, &wr, z, windowSettings);

	    if(active) {
	    	Rect r = rectTLDim(wr.left, -wr.top, wr.right-wr.left, -wr.bottom - -wr.top);

	    	ws->currentRes = vec2i(rectW(r) - 2, rectH(r) - 2); // 1 pixel border.
	    	ws->aspectRatio = ws->currentRes.w / (float)ws->currentRes.h;
	    	ad->updateFrameBuffers = true;	
	    	globalScreenHeight = ws->currentRes.h;

			BringWindowToTop(windowHandle);
			MoveWindow(windowHandle, r.left, r.top, rectW(r), rectH(r), true);
	    }
	}

	if(!resize) {
		if(windowSizeChanged(windowHandle, ws)) {
			if(!windowIsMinimized(windowHandle)) {
				TIMER_BLOCK_NAMED("UpdateWindowRes");

				updateResolution(windowHandle, ws);
				ad->updateFrameBuffers = true;
			}
		}
	}

	ad->screenShotMode = false;
	if(ad->startScreenShot) {
		ad->startScreenShot = false;
		ad->screenShotMode = true;

		ws->currentRes.w = ad->screenShotDim.w;
		ws->currentRes.h = ad->screenShotDim.h;

		ad->updateFrameBuffers = true;
	}

	if(ad->updateFrameBuffers) {
		TIMER_BLOCK_NAMED("Upd FBOs");

		ad->updateFrameBuffers = false;
		globalScreenHeight = ws->currentRes.h;

		if(ad->screenShotMode) {
			setDimForFrameBufferAttachmentsAndUpdate(FRAMEBUFFER_2dMsaa, ws->currentRes.w, ws->currentRes.h);
			setDimForFrameBufferAttachmentsAndUpdate(FRAMEBUFFER_2dNoMsaa, ws->currentRes.w, ws->currentRes.h);
			setDimForFrameBufferAttachmentsAndUpdate(FRAMEBUFFER_ScreenShot, ws->currentRes.w, ws->currentRes.h);
		}
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

		bindFrameBuffer(FRAMEBUFFER_2dMsaa);
		glClearColor(0,0,0,0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		bindFrameBuffer(FRAMEBUFFER_DebugMsaa);
		glClearColor(0,0,0,0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

	// Setup opengl.
	{
		glEnable(GL_DITHER);

		glDepthRange(-10.0,10.0);
		glFrontFace(GL_CW);
		// glEnable(GL_DEPTH_TEST);
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
		// glEnable(GL_MULTISAMPLE);
		// glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		// glBlendEquation(GL_FUNC_ADD);
		// glViewport(0,0, ad->cur3dBufferRes.x, ad->cur3dBufferRes.y);




		bindFrameBuffer(FRAMEBUFFER_2dMsaa);
		// glBindTexture(GL_TEXTURE_2D, getFrameBuffer(FRAMEBUFFER_2dMsaa)->colorSlot[0]->id);
		// glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glDepthFunc(GL_ALWAYS);
		glEnable(GL_DEPTH_TEST);
		// glDepthFunc(GL_NEVER);
		// glDisable(GL_DEPTH_TEST);

		glUseProgram(0);
		glBindProgramPipeline(0);
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);

		glLoadIdentity();
		Vec2i res = ws->currentRes;
		glViewport(0,0, res.w, res.h);
		glOrtho(0, res.w, -res.h, 0, -10,10);
	}

	TIMER_BLOCK_END(openglInit);




	// @AppLoop.

	TIMER_BLOCK_BEGIN_NAMED(appMain, "App Main");

	if(ad->modeData.switchMode) {
		DownloadModeData* md = &ad->modeData;
		md->switchMode = false;

		bool videoModeChange = false;

		// Abort running mode.
		if(md->newDownloadMode == 0) {
			if(md->downloadMode == Download_Mode_Videos) {
				videoModeChange = true;
			}
			md->downloadMode = 0;
		} else {
			// Can't abort important tasks.
			// if((md->downloadMode == Download_Mode_Videos || md->downloadMode == Download_Mode_ChannelPlaylists)) {
				// Nothing.
			// } else {
				if(md->downloadMode == Download_Mode_Videos) {
					videoModeChange = true;
				}
				md->downloadMode = md->newDownloadMode;
				md->downloadStep = 0;
			// }
		}

		if(videoModeChange) {
			ad->startLoadFile = true;
			memCpy(&ad->playlist, &ad->downloadPlaylist, sizeof(ad->downloadPlaylist));
			fclose(md->videosModeData.file);
			loadPlaylistFolder(ad->playlistFolder, &ad->playlistFolderCount);
		}
	}

	if(ad->modeData.downloadMode != 0) {
		ad->appIsBusy = true;
		CurlRequestData* requestData = &ad->requestData;
		DownloadModeData* modeData = &ad->modeData;
		int mode = modeData->downloadMode;
		int modeIndex = 0;


		if(mode == Download_Mode_Snippet) {
			int commentCount = 10;
			VideoSnippet* snippet = &ad->videoSnippet;
			YoutubeVideo* vid = ad->videos + ad->selectedVideo;

			if(downloadStepNext(&modeData->downloadStep, &modeIndex, requestData)) {
				snippet->selectedCommentCount = 0;
				ad->commentScrollValue = 0;
			}

			// Download thumbnail 
			if(downloadStepNext(&modeData->downloadStep, &modeIndex, requestData)) {
				curlRequestDataInitAdd(threadQueue, requestData, vid->thumbnail);
			}

			// Upload texture.
			if(downloadStepNext(&modeData->downloadStep, &modeIndex, requestData)) {
				if(snippet->thumbnailTexture.id != -1) deleteTexture(&snippet->thumbnailTexture);
				loadTextureFromMemory(&snippet->thumbnailTexture, requestData->buffer, requestData->size, -1, INTERNAL_TEXTURE_FORMAT, GL_RGB, GL_UNSIGNED_BYTE);

				curlRequestDataInitAdd(threadQueue, requestData, requestCommentThread(vid->id, commentCount));
			}

			// Get comments.
			if(downloadStepNext(&modeData->downloadStep, &modeIndex, requestData)) {
				TIMER_BLOCK_NAMED("ParseComments");

				downloadModeFinish(modeData);

				char* buffer = requestData->buffer;
				writeBufferToFile(buffer, "..\\lastSnippetDownload.json");

				JSonValue* object = jsonParseValue(&buffer);
				JSonValue* items = jsonGetMember(object, "items");
				if(items) {
					snippet->selectedCommentCount = items->size;
					int i = 0;
					For_JsonArray(items) {
						JSonValue* snip = jsonGetMember(it, "snippet");
						snippet->selectedCommentReplyCount[i] = jsonGetInt(snip, "totalReplyCount");

						JSonValue* sn = jsonGetMember(snip, "topLevelComment", "snippet");

						reallocString(&snippet->selectedTopComments[i], jsonGetString(sn, "textOriginal"));
						snippet->selectedCommentLikeCount[i] = jsonGetInt(sn, "likeCount");

						i++;
					}
				}
			}
		}

		if(mode == Download_Mode_Search) {

			SearchResult* searchResults = ad->searchResults;
			bool searchForChannels = !ad->lastSearchMode;

			if(downloadStepNext(&modeData->downloadStep, &modeIndex, requestData)) {
				ad->searchResultCount = 0;
				for(int i = 0; i < Search_Result_Count; i++) ad->searchResults[i].count = 0;
			}

			if(downloadStepNext(&modeData->downloadStep, &modeIndex, requestData)) {
				char* searchString = ad->searchString;

				char* search = getTString(strLen(searchString)+1); strClear(search);
				for(;;) {
					searchString = eatSpaces(searchString);
					int pos = strFind(searchString, ' ');
					if(pos != 0) {
						strAppend(search, searchString, pos-1);
						strAppend(search, ",");
						searchString += pos-1;
						continue;
					}

					pos = strLen(searchString);
					strAppend(search, searchString, pos);
					break;
				}

				curlRequestDataInitAdd(threadQueue, requestData, requestSearch(searchForChannels?"channel":"playlist", search, Search_Result_Count));
			}

			if(downloadStepNext(&modeData->downloadStep, &modeIndex, requestData)) {

				char* buffer = requestData->buffer;
				JSonValue* object = jsonParseValue(&buffer);

				JSonValue* items = jsonGetMember(object, "items");
				for(int i = 0; i < items->size; i++) {
					JSonValue* it = items->array + i;

					if(searchForChannels) strCpy(searchResults[i].id, jsonGetString(it, "id", "channelId"));
					else strCpy(searchResults[i].id, jsonGetString(it, "id", "playlistId"));

					strCpy(searchResults[i].title, jsonGetString(it, "snippet", "title"));
				}
				ad->searchResultCount = items->size;

				// Get count.
				int count = ad->searchResultCount;
				if(!searchForChannels) {
					char* idCollection = getTString(kiloBytes(20)); strClear(idCollection);
					for(int index = 0; index < count; index++) {
						strAppend(idCollection, fillString("%s,", searchResults[index].id));
					}

					curlRequestDataInitAdd(threadQueue, requestData, requestPlaylist("id", idCollection, "contentDetails", count));
				} else {
					// downloadModeFinish(&modeData->downloadStep);

					char* idCollection = getTString(kiloBytes(20)); strClear(idCollection);
					for(int index = 0; index < count; index++) {
						strAppend(idCollection, fillString("%s,", searchResults[index].id));
					}

					// for(int i = 0; i < count; i++) ad->searchResults[i].count = 0;

					// printf("%s", searchResults[0].id);

					curlRequestDataInitAdd(threadQueue, requestData, requestChannel("id", idCollection, "contentDetails", count));
				}

			} if(downloadStepNext(&modeData->downloadStep, &modeIndex, requestData)) {

				char* buffer = requestData->buffer;
				JSonValue* object = jsonParseValue(&buffer);
				JSonValue* items = jsonGetMember(object, "items");

				if(!searchForChannels) {
					downloadModeFinish(modeData);
					for(int i = 0; i < items->size; i++) {
						searchResults[i].count = jsonGetInt(&items->array[i], "contentDetails", "itemCount");
					}
				} else {
					// downloadModeFinish(&modeData->downloadStep);

					char* idCollection = getTString(kiloBytes(20)); strClear(idCollection);
					int idCount = 0;
					for(int si = 0; si < ad->searchResultCount; si++) {

						for(int i = 0; i < items->size; i++) {
							char* sId = jsonGetString(&items->array[i], "id");

							if(strCompare(sId, searchResults[si].id)) {
								char* s = jsonGetString(&items->array[i], "contentDetails", "relatedPlaylists", "uploads");
								if(s) {
									strAppend(idCollection, fillString("%s,", s));
									idCount++;
									strCpy(searchResults[si].allPlaylistId, s);
								}
								break;
							}
						}
					}

					curlRequestDataInitAdd(threadQueue, requestData, requestPlaylist("id", idCollection, "contentDetails", idCount));
				}
			} if(downloadStepNext(&modeData->downloadStep, &modeIndex, requestData)) {
				downloadModeFinish(modeData);

				for(int i = 0; i < ad->searchResultCount; i++) ad->searchResults[i].count = 0;

				// Results are not sorted, search for id and compare.
				char* buffer = requestData->buffer;
				JSonValue* items = jsonGetMember(jsonParseValue(&buffer), "items");
				for(int si = 0; si < ad->searchResultCount; si++) {
	
					for(int i = 0; i < items->size; i++) {
						char* sId = jsonGetString(&items->array[i], "id");

						if(strCompare(sId, searchResults[si].allPlaylistId)) {
							searchResults[si].count = jsonGetInt(&items->array[i], "contentDetails", "itemCount");
							break;
						}
					}
				}
			}
		}



		if(mode == Download_Mode_ChannelPlaylists) {
			SearchResult* playlists = ad->searchResults;
			int* playlistCount = &ad->searchResultCount;

			char* channelId = ad->channelId;
			int count = ad->playlistDownloadCount;

			char* buffer = requestData->buffer;

			static int i;
			static char pageToken[Page_Token_Size];

			downloadStepNext(&modeData->downloadStep, &modeIndex, requestData);

			if(downloadStepNext(&modeData->downloadStep, &modeIndex, requestData)) {

				// Get playlist count.
				curlRequestDataInitAdd(threadQueue, requestData, requestPlaylist("channelId", channelId, "id", 1));
			}

			if(downloadStepNext(&modeData->downloadStep, &modeIndex, requestData)) {
				JSonValue* object = jsonParseValue(&buffer);
				ad->playlistDownloadCount = jsonGetInt(object, "pageInfo", "totalResults");
				count = ad->playlistDownloadCount;



				i = 0;
				strClear(pageToken);
				
				if(ad->playlists != 0) free(ad->playlists);
				ad->playlists = mallocArray(YoutubePlaylist, count);

				downloadModeProgress(modeData, 0, count);

				*playlistCount = 0;
			} 

			if(downloadStepNext(&modeData->downloadStep, &modeIndex, requestData, false)) {
				downloadModeProgress(modeData, i, count);

				if(i >= count) {
					*playlistCount = count;
					downloadModeFinish(modeData);
				}

				int dCount = i + Max_Playlist_Download_Count > count ? count-i : Max_Playlist_Download_Count;
				curlRequestDataInitAdd(threadQueue, requestData, requestPlaylist("channelId", channelId, "snippet", dCount, pageToken));

					} if(downloadStepNext(&modeData->downloadStep, &modeIndex, requestData)) {

				int dCount = i + Max_Playlist_Download_Count > count ? count-i : Max_Playlist_Download_Count;

				buffer = requestData->buffer;
				JSonValue* object = jsonParseValue(&buffer);
				JSonValue* items = jsonGetMember(object, "items");

				char* pageTokenStr = jsonGetString(object, "nextPageToken");
				if(pageTokenStr) strCpy(pageToken, pageTokenStr);

				int index = i;
				For_JsonArray(items) {
					strCpy(playlists[index].id, jsonGetString(it, "id"));
					strCpy(playlists[index].title, jsonGetString(it, "snippet", "title"));
					index++;
				}

				// Get playlist video count.
				char* idCollection = getTString(kiloBytes(20)); strClear(idCollection);
				int maxCount = dCount;
				for(int index = 0; index < maxCount; index++) {
					strAppend(idCollection, fillString("%s,", playlists[i+index].id));
				}

				curlRequestDataInitAdd(threadQueue, requestData, requestPlaylist("id", idCollection, "contentDetails", maxCount));

					} if(downloadStepNext(&modeData->downloadStep, &modeIndex, requestData)) {

				JSonValue* items = jsonGetMember(jsonParseValue(&buffer), "items");
				int index = i;
				For_JsonArray(items) {
					playlists[index].count = jsonGetInt(it, "contentDetails", "itemCount");
					index++;
				}

				i += Max_Playlist_Download_Count;				
				modeData->downloadStep -= 3;

				*playlistCount = i;
			}
		}



		if(mode == Download_Mode_Videos) {
			DownloadVideosData* data = &modeData->videosModeData;

			char* buffer = requestData->buffer;

			downloadStepNext(&modeData->downloadStep, &modeIndex, requestData);

			// Get maxCount first.
			if(downloadStepNext(&modeData->downloadStep, &modeIndex, requestData)) {
				curlRequestDataInitAdd(threadQueue, requestData, requestPlaylistItems(ad->downloadPlaylist.id, 1));
			}

			if(downloadStepNext(&modeData->downloadStep, &modeIndex, requestData)) {
				JSonValue* object = jsonParseValue(&buffer);
				zeroStruct(data, DownloadVideosData);
				data->maxCount = jsonGetInt(object, "pageInfo", "totalResults");


				YoutubePlaylist* downloadPlaylist = &ad->downloadPlaylist;


				downloadPlaylist->count = data->maxCount;

				// Create file if non-existent.
				char* playlistFilePath = getPlaylistFilePath(downloadPlaylist->id);
				bool playlistFileExists = fileExists(playlistFilePath);
				if(!playlistFileExists) {
					FILE* file = fopen(playlistFilePath, "wb");

					PlaylistFileHeader fileHeader = {};
					strCpy(fileHeader.title, downloadPlaylist->title);
					strCpy(fileHeader.id, downloadPlaylist->id);
					fileHeader.maxCount = data->maxCount;

					fwrite(&fileHeader, sizeof(PlaylistFileHeader), 1, file);
					fclose(file);
				}

				PlaylistFileHeader fileHeader = loadPlaylistHeaderFromFileX(downloadPlaylist->id);

				strCpy(data->pageToken, fileHeader.pageToken);
				data->count = downloadPlaylist->count;
				data->lastCount = fileHeader.count;
				// data->maxCount = fileHeader.maxCount;

				bool skip = false;
				if(ad->update) {
					// data->count = clampIntMax(data->count, fileHeader.count);
					data->count = clampIntMax(data->count, data->maxCount);
				} else {
					if(data->maxCount == fileHeader.count) skip = true;
					if(data->count + data->lastCount > data->maxCount) {
						data->count = data->maxCount - data->lastCount;
						// ad->downloadPlaylist.count = data->count;
					}
				}
				if(data->count == 0) skip = true;

				if(skip) downloadModeFinish(modeData);
				else {
					zeroMemory(data->vids, sizeof(YoutubeVideo)*Max_Download_Count);

					downloadModeProgress(modeData, 0, data->count);
					data->i = 0;

					data->file = fopen(playlistFilePath, "rb+");
				}
			}

			if(downloadStepNext(&modeData->downloadStep, &modeIndex, requestData, false)) {
				if(data->i >= data->count) {
					modeData->downloadStep += 4;
				}

				downloadModeProgress(modeData, min(data->i,data->count), data->count);

				data->dCount = Max_Download_Count;
				if(data->i + data->dCount > data->count) data->dCount = data->count - data->i;

				{
					char* playlistId = ad->downloadPlaylist.id;

					// TIMER_BLOCK_NAMED("Request");
					curlRequestDataInitAdd(threadQueue, requestData, requestPlaylistItems(playlistId, data->dCount, data->pageToken));
				}

			} if(downloadStepNext(&modeData->downloadStep, &modeIndex, requestData)) {

				JSonValue* object = jsonParseValue(&buffer);
				JSonValue* items = jsonGetMember(object, "items");

				char* pageTokenStr = jsonGetString(object, "nextPageToken");
				if(pageTokenStr) strCpy(data->pageToken, pageTokenStr);

				int receivedCount;

				// Get Video ids.
				{
					int index = 0;
					int idCount = 0;
					For_JsonArray(items) {
						int reverseIndex = data->dCount-1 - index;

						char* s = jsonGetString(it, "contentDetails", "videoId");
						if(s) {
							data->vids[reverseIndex] = {};

							strCpy(data->vids[reverseIndex].id, s);
							idCount++;
						}

						index++;
					}

					receivedCount = idCount;
				}
				

				// Get Duration.
				{
					char* tempBuffer = getTString(kiloBytes(10)); strClear(tempBuffer);
					for(int videoIndex = 0; videoIndex < data->dCount; videoIndex++) {
						int reverseIndex = data->dCount-1 - videoIndex;
						strAppend(tempBuffer, fillString("%s,", data->vids[reverseIndex].id));
					}

					curlRequestDataInitAdd(threadQueue, requestData, requestVideos(tempBuffer, "contentDetails"));
				}

			} if(downloadStepNext(&modeData->downloadStep, &modeIndex, requestData)) {
				
				JSonValue* object = jsonParseValue(&buffer);
				JSonValue* items = jsonGetMember(object, "items");

				int index = 0;
				int advance = 0;
				For_JsonArray(items) {
					int reverseIndex = data->dCount-1 - index;

					char* durationString = jsonGetString(it, "contentDetails", "duration");
					data->vids[reverseIndex].duration = stringDurationToDate(durationString);;

					index++;
				}


				// Get Statistics.
				{
					char* tempBuffer = getTString(kiloBytes(10)); strClear(tempBuffer);
					for(int videoIndex = 0; videoIndex < data->dCount; videoIndex++) {
						int reverseIndex = data->dCount-1 - videoIndex;
						strAppend(tempBuffer, fillString("%s,", data->vids[reverseIndex].id));
					}

					curlRequestDataInitAdd(threadQueue, requestData, requestVideos(tempBuffer, "statistics"));
				}

			} if(downloadStepNext(&modeData->downloadStep, &modeIndex, requestData)) {
				
				JSonValue* object = jsonParseValue(&buffer);
				JSonValue* items = jsonGetMember(object, "items");

				int index = 0;
				int advance = 0;
				For_JsonArray(items) {
					int reverseIndex = data->dCount-1 - index;

					JSonValue* statistics = jsonGetMember(it, "statistics");
					if(statistics) {
						data->vids[reverseIndex].viewCount = strToIntSave(jsonGetString(statistics, "viewCount"));
						data->vids[reverseIndex].likeCount = strToIntSave(jsonGetString(statistics, "likeCount"));
						data->vids[reverseIndex].dislikeCount = strToIntSave(jsonGetString(statistics, "dislikeCount"));
						data->vids[reverseIndex].favoriteCount = strToIntSave(jsonGetString(statistics, "favoriteCount"));
						data->vids[reverseIndex].commentCount = strToIntSave(jsonGetString(statistics, "commentCount"));
					}

					index++;
				}

				// Get title and thumbnail.
				char* tempBuffer = getTString(kiloBytes(10)); strClear(tempBuffer);
				for(int videoIndex = 0; videoIndex < data->dCount; videoIndex++) {
					int reverseIndex = data->dCount-1 - videoIndex;
					strAppend(tempBuffer, fillString("%s,", data->vids[reverseIndex].id));
				}

				{
					// TIMER_BLOCK_NAMED("Title Request");
					curlRequestDataInitAdd(threadQueue, requestData, requestVideos(tempBuffer, "snippet"));
				}

			} if(downloadStepNext(&modeData->downloadStep, &modeIndex, requestData)) {
				char* tempBuffer = getTString(kiloBytes(10)); strClear(tempBuffer);

				JSonValue* object = jsonParseValue(&buffer);
				JSonValue* items = jsonGetMember(object, "items");

				int index = 0;
				int advance = 0;
				For_JsonArray(items) {
					int reverseIndex = data->dCount-1 - index;
		
					JSonValue* snippet = jsonGetMember(it, "snippet");

					char* s = jsonGetString(snippet, "publishedAt");
					strCpy(data->vids[reverseIndex].dateString, s);
					data->vids[reverseIndex].date = stringToDate(s);

					strCpy(data->vids[reverseIndex].title, jsonGetString(snippet, "title"));
					strCpy(data->vids[reverseIndex].thumbnail, jsonGetString(snippet, "thumbnails", "high", "url"));

					index++;
				}

				{
					{
						FILE* file = data->file;

						fseek(file, 0, SEEK_SET);
						fseek(file, memberSize(PlaylistFileHeader, title), SEEK_CUR);
						fseek(file, memberSize(PlaylistFileHeader, id), SEEK_CUR);
						if(!ad->update) {
							int currentCount = data->i + data->dCount + data->lastCount;
							fwrite(&currentCount, memberSize(PlaylistFileHeader, count), 1, file);
							fwrite(&data->maxCount, memberSize(PlaylistFileHeader, maxCount), 1, file);
							fwrite(data->pageToken, memberSize(PlaylistFileHeader, pageToken), 1, file);
						} else {
							fseek(file, memberSize(PlaylistFileHeader, count), SEEK_CUR);
							fseek(file, memberSize(PlaylistFileHeader, maxCount), SEEK_CUR);
							fseek(file, memberSize(PlaylistFileHeader, pageToken), SEEK_CUR);
						}

						int videoCount = data->count;
						YoutubeVideo* videos = data->vids;

						int fileVideoStart = (data->maxCount-data->i-data->dCount-data->lastCount);
						fseek(file, fileVideoStart*sizeof(YoutubeVideo), SEEK_CUR);
						fwrite(videos, data->dCount*sizeof(YoutubeVideo), 1, file);
					}

				}

				data->i += Max_Download_Count;
				modeData->downloadStep -= 5;
			}

			if(downloadStepNext(&modeData->downloadStep, &modeIndex, requestData)) {
				downloadModeFinish(modeData);
				ad->startLoadFile = true;
				memCpy(&ad->playlist, &ad->downloadPlaylist, sizeof(ad->downloadPlaylist));

				fclose(data->file);
			}
		}

	}





	// @Testing.
	// if(input->keysPressed[KEYCODE_T]) 
	if(false) 
	{
		char* channelId = "UCQvTDmHza8erxZqDkjQ4bQQ";
		char* videoId = "FvHiLLkPhQE";
		CurlRequestData* requestData = &ad->requestData;
		char* part;

		char* channelParts[] = {"auditDetails","brandingSettings","contentDetails","contentOwnerDetails","id","invideoPromotion","localizations","snippet","statistics","status","topicDetails"};
		char* videoParts[] = {"contentDetails", "fileDetails", "id", "liveStreamingDetails", "localizations", "player", "processingDetails", "recordingDetails", "snippet", "statistics", "status", "suggestions", "topicDetails"};

		// int count = arrayCount(channelParts);
		int count = arrayCount(videoParts);

		for(int i = 0; i < count; i++) {
			// part = channelParts[i];
			part = videoParts[i];

			curlRequestDataInitAdd(threadQueue, requestData, requestVideos(videoId, part));
			// curlRequestDataInitAdd(threadQueue, requestData, requestChannel("id", channelId, part, 1));

			threadQueueComplete(threadQueue);
			printf("======= %s ========= \n%s\n", part, requestData->buffer);
		}




	}





	if(init) {
		TIMER_BLOCK_NAMED("App Init");

		loadPlaylistFolder(ad->playlistFolder, &ad->playlistFolderCount);

		YoutubePlaylist* firstPlaylistFromFolder = ad->playlistFolder + ad->playlistFolderIndex;
		memCpy(&ad->playlist, firstPlaylistFromFolder, sizeof(YoutubePlaylist));

		ad->startLoadFile = true;
	}

	// @Load File.
	if(ad->startLoadFile && (ad->modeData.downloadMode != Download_Mode_Videos)) {
		TIMER_BLOCK_NAMED("LoadFile");

		TIMER_BLOCK_BEGIN_NAMED(filestart, "LoadFileStart");

		ad->startLoadFile = false;

		YoutubePlaylist tempPlaylist;		
		int maxCount = 1;
		loadPlaylistHeaderFromFile(&tempPlaylist, fillString("%s.playlist", ad->playlist.id), &maxCount);
		memCpy(&ad->playlist, &tempPlaylist, sizeof(YoutubePlaylist));

		pushTMemoryStack();

		int tempVidCount = tempPlaylist.count;
		YoutubeVideo* tempVids = getTArray(YoutubeVideo, tempVidCount);

		bool foundFile = loadPlaylistFromFile(&tempPlaylist, tempVids, &tempVidCount);

		TIMER_BLOCK_END(filestart);

		if(foundFile) {
			ad->downloadPlaylist.count = maxCount;

			// Filter videos.
			{
				TIMER_BLOCK_NAMED("Filtering");

				int* filteredIndexes = getTArray(int, tempVidCount);
				int filteredIndexesCount = 0;

				char* inclusiveFilterStrings[10];
				int inclusiveFilterStringCount = 0;
				{
					char* searchString = ad->inclusiveFilter;
					for(;;) {
						searchString = eatSpaces(searchString);
						int pos = strFind(searchString, ' ');
						if(pos != 0) {
							char* filter = getTString(pos-1 + 1);
							strCpy(filter, searchString, pos-1);
							inclusiveFilterStrings[inclusiveFilterStringCount++] = filter;

							searchString += pos-1;
							continue;
						}

						pos = strLen(searchString);
						if(pos == 0) break;

						char* filter = getTString(pos + 1);
						strCpy(filter, searchString, pos);
						inclusiveFilterStrings[inclusiveFilterStringCount++] = filter;

						break;
					}
				}
				char* exclusiveFilterStrings[10];
				int exclusiveFilterStringCount = 0;
				{
					char* searchString = ad->exclusiveFilter;
					for(;;) {
						searchString = eatSpaces(searchString);
						int pos = strFind(searchString, ' ');
						if(pos != 0) {
							char* filter = getTString(pos-1 + 1);
							strCpy(filter, searchString, pos-1);
							exclusiveFilterStrings[exclusiveFilterStringCount++] = filter;

							searchString += pos-1;
							continue;
						}

						pos = strLen(searchString);
						if(pos == 0) break;

						char* filter = getTString(pos + 1);
						strCpy(filter, searchString, pos);
						exclusiveFilterStrings[exclusiveFilterStringCount++] = filter;

						break;
					}
				}


				{
					TIMER_BLOCK_NAMED("ApplyFilter");

					bool inclusiveFilterActive = inclusiveFilterStringCount > 0;
					bool exclusiveFilterActive = exclusiveFilterStringCount > 0;

					bool notFiltered;
					for(int i = 0; i < tempVidCount; i++) {
						char* title = tempVids[i].title;

						bool inclusiveCorrect;
						if(inclusiveFilterActive) {
							inclusiveCorrect = true;
							for(int fi = 0; fi < inclusiveFilterStringCount; fi++) {
								char* filter = inclusiveFilterStrings[fi];
								bool correct = (strFind(title, filter) != -1);
								if(!correct) {
									inclusiveCorrect = false;
									break;
								}
							}
						}

						bool exclusiveCorrect;
						if(exclusiveFilterActive) {
							exclusiveCorrect = true;
							for(int fi = 0; fi < exclusiveFilterStringCount; fi++) {
								char* filter = exclusiveFilterStrings[fi];
								bool correct = (strFind(title, filter) == -1);
								if(!correct) {
									exclusiveCorrect = false;
									break;
								}
							}
						}
						
						if(!inclusiveFilterActive) inclusiveCorrect = true;
						if(!exclusiveFilterActive) exclusiveCorrect = true;

						notFiltered = inclusiveCorrect && exclusiveCorrect;

						// Filter corrupted videos for now.
						bool corrupted = false;
						if(notFiltered)
						{
							YoutubeVideo* vid = tempVids + i;
							if(vid->date.n < dateEncode(5,1,1,0,0,0) || vid->date.n > dateEncode(20,1,1,0,0,0) ||
							   vid->viewCount < 0 || vid->viewCount > 5000000000 ||
							   vid->likeCount < 0 || vid->likeCount > 100000000 ||
							   vid->dislikeCount < 0 || vid->dislikeCount > 100000000 ||
							   vid->favoriteCount < 0)
								corrupted = true;

							notFiltered = notFiltered && !corrupted;

							// if(!valueBetween(vid->viewCount, ad->clampFilter[0].min, ad->clampFilter[0].max)) notFiltered = false;
							// if(!valueBetween(vid->viewCount, ad->clampFilter[1].min, ad->clampFilter[1].max)) notFiltered = false;
							// if(!valueBetween(vid->viewCount, ad->clampFilter[2].min, ad->clampFilter[2].max)) notFiltered = false;
							// if(!valueBetween(vid->viewCount, ad->clampFilter[3].min, ad->clampFilter[3].max)) notFiltered = false;
						}

						if(notFiltered) filteredIndexes[filteredIndexesCount++] = i;
					}
				}

				ad->playlist.count = filteredIndexesCount;

				{
					TIMER_BLOCK_NAMED("Cpy Filtered");
	
					// Resize ad->videos if not big enough.
					if(ad->maxVideoCount < filteredIndexesCount) {
						free(ad->videos);
						int count = filteredIndexesCount;
						ad->videos = (YoutubeVideo*)malloc(sizeof(YoutubeVideo)*count);
						ad->maxVideoCount = count;
					}

					for(int i = 0; i < filteredIndexesCount; i++) {
						int index = filteredIndexes[i];
						ad->videos[i] = tempVids[index];
					}
				}
			}

			if(ad->playlist.count == 0) {
				for(int i = 0; i < ad->camCount; i++) {
					graphCamInit(&ad->cams[i], 0, 10000, 0, 1000);
				}
			} else {

				// Sort videos.
				{
					TIMER_BLOCK_NAMED("Sorting");

					YoutubeVideo* vids = ad->videos;
					int size = ad->playlist.count;

					struct SortNode {
						i64 val;
						int i;	
					};

					SortNode* list = (SortNode*)malloc(sizeof(SortNode)*size);
					SortNode* buffer = (SortNode*)malloc(sizeof(SortNode)*size);
					uchar* bytes = (uchar*)malloc(sizeof(uchar)*size);
					int stageCount = ad->sortByDate?6:4;

					TIMER_BLOCK_BEGIN_NAMED(pre, "pre");

					if(ad->sortByDate)         for(int i = 0; i < size; i++) list[i].val = vids[i].date.n;
					else if(ad->sortStat == 0) for(int i = 0; i < size; i++) list[i].val = vids[i].viewCount;
					else if(ad->sortStat == 1) for(int i = 0; i < size; i++) list[i].val = vids[i].likeCount + vids[i].dislikeCount;
					else if(ad->sortStat == 3) for(int i = 0; i < size; i++) list[i].val = vids[i].commentCount;
					else if(ad->sortStat == 2) for(int i = 0; i < size; i++) {
						float v = videoGetLikesDiff(&vids[i]);
						list[i].val = *((i64*)(&v));
					}

					for(int i = 0; i < size; i++) list[i].i = i;

					TIMER_BLOCK_END(pre);

					for(int stage = 0; stage < stageCount; stage++) {
						SortNode* src = stage%2 == 0 ? list : buffer;
						SortNode* dst = stage%2 == 0 ? buffer : list;
						int bucket[257] = {};
						int offset = 8*stage;

						for(int i = 0; i < size; i++) bytes[i] = src[i].val >> offset;
						for(int i = 0; i < size; i++) bucket[bytes[i]+1]++;
						for(int i = 0; i < 256-1; i++) bucket[i+1] += bucket[i];

						TIMER_BLOCK_BEGIN_NAMED(asdf, "SortCpy");

						for(int i = 0; i < size; i++) {
							uchar byte = bytes[i];
							dst[bucket[bytes[i]]] = src[i];
							bucket[byte]++;
						}

						TIMER_BLOCK_END(asdf);
					}

					TIMER_BLOCK_BEGIN_NAMED(post, "post");

					for(int i = 0; i < size; i++) {
						int index = list[i].i;
						tempVids[i] = vids[index];
					}

					if(ad->sortByDate) for(int i = 0; i < size; i++) vids[i] = tempVids[i];
					else if(!ad->sortByDate) for(int i = 0; i < size; i++) vids[i] = tempVids[size-i-1];

					TIMER_BLOCK_END(post);

					free(buffer);
					free(bytes);
					free(list);

				}

				Statistic statViews, statLikes, statLikesDiff, statDates, statComments;
				{
					TIMER_BLOCK_NAMED("Get Stats");

					beginStatistic(&statViews);
					beginStatistic(&statLikes);
					beginStatistic(&statDates);
					beginStatistic(&statLikesDiff);
					beginStatistic(&statComments);

					for(int i = 0; i < ad->playlist.count; i++) {
						YoutubeVideo* video = ad->videos + i;

						updateStatistic(&statViews, video->viewCount);
						updateStatistic(&statLikes, video->likeCount+video->dislikeCount);
						updateStatistic(&statLikesDiff, videoGetLikesDiff(video));
						updateStatistic(&statComments, video->commentCount);
						updateStatistic(&statDates, video->date.n);
					}

					endStatistic(&statViews);
					endStatistic(&statLikes);
					endStatistic(&statLikesDiff);
					endStatistic(&statDates);
					endStatistic(&statComments);

					ad->stats[0] = statViews;
					ad->stats[1] = statLikes;
					ad->stats[2] = statLikesDiff;
					ad->stats[3] = statComments;

					// ad->clampFilter[0].min = ad->stats[0].min;
					// ad->clampFilter[0].max = ad->stats[0].max;
					// ad->clampFilter[1].min = ad->stats[1].min;
					// ad->clampFilter[1].max = ad->stats[1].max;
					// ad->clampFilter[2].min = ad->stats[2].min;
					// ad->clampFilter[2].max = ad->stats[2].max;
					// ad->clampFilter[3].min = ad->stats[3].min;
					// ad->clampFilter[3].max = ad->stats[3].max;
				}

				// Init cam after loading.
				{
					double camLeft, camRight;

					if(ad->sortByDate) {
						if(statDates.max - statDates.min < Graph_Zoom_Min) {
							i64 timeMid = statDates.min != statDates.max ? statDates.min + (statDates.max - statDates.min)/2 : statDates.min;
							camLeft = timeMid - Graph_Zoom_Min/2;
							camRight = timeMid + Graph_Zoom_Min/2;
						} else {
							camLeft = statDates.min;
							camRight = statDates.max;
						}
					} else {
						camLeft = 0;
						camRight = ad->playlist.count-1;
					}

					graphCamInit(&ad->cams[0], camLeft, camRight, 0, statViews.max*1.1f);
					graphCamInit(&ad->cams[1], camLeft, camRight, 0, statLikes.max*1.1f);
					graphCamInit(&ad->cams[2], camLeft, camRight, 0, 1);
					graphCamInit(&ad->cams[3], camLeft, camRight, 0, statComments.max*1.1f);
				}

				calculateAverages(ad->videos, ad->playlist.count, &ad->averagesLineGraph, ad->statTimeSpan, ad->statWidth, ad->cams[0].xMax - ad->cams[0].xMin, &ad->releaseCountLineGraph);
			}

			ad->selectedVideo = -1;
		} 

		popTMemoryStack();
	}



	{
		TIMER_BLOCK_BEGIN_NAMED(appIntro, "appIntro");

		AppColors* ac = &ad->appColors;
		AppSettings* as = &ad->appSettings;

		NewAppColors* newac = &ad->newAppColors;
		NewAppSettings* newas = &ad->newAppSettings;

		Font* font = ad->font;
		int fontSize = font->height;

		// Options.
		float topBarHeightMod = newas->heightMod;
		float leftScaleOffset = 0;
		float bottomScaleOffset = 0;
		float windowBorderSize = newas->windowBorder;
		float windowTitleHeightMod = newas->windowHeightMod;

		// Colors.
		Vec4 colorWindowBorder = newac->windowBorder;
		Vec4 colorWindowClient = newac->background;

		//

		ad->leftTextWidth = clampMin(ad->leftTextWidth, 1.1f); // @Hack.

		// Screen layout.
		bool hasSidePanel = ad->selectedVideo!=-1;
		float sidePanelWidth = hasSidePanel ? ad->sidePanelWidth : 0;
		if(hasSidePanel) clamp(&ad->sidePanelWidth, Side_Panel_Min_Width, ws->currentRes.w*ad->sidePanelMax);
		float leftScaleWidth = ad->leftTextWidth + leftScaleOffset;
		float windowTitleHeight = font->height * windowTitleHeightMod;
		float topBarHeight = font->height * topBarHeightMod + newas->border*2;
		float bottomScaleHeight = font->height*2 + bottomScaleOffset;

		Layout* lay = layoutAlloc(layout(rectExpand(getScreenRect(ws), ws->fullscreen?0:-windowBorderSize*2), true));

		Layout* lTopBar = layoutAdd(lay, layout(vec2(0,ws->fullscreen?-1:windowTitleHeight)));
		if(!ws->fullscreen) layoutAdd(lay, layout(vec2(0,windowBorderSize)));
		Layout* lMain = layoutAdd(lay, layout(vec2(0,0)));

		Layout* lLeft = layoutAdd(lMain, layout(vec2(0,0), true));
		Layout* lSidePanel = ad->selectedVideo!=-1?layoutAdd(lMain, layout(vec2(sidePanelWidth,0))):0;

		Layout* lFilters = layoutAdd(lLeft, layout(vec2(0,topBarHeight)));
		Layout* lChart = layoutAdd(lLeft, layout(vec2(0,0), true, vec2i(-1,1)));
		Layout* bottom = layoutAdd(lLeft, layout(vec2(0,bottomScaleHeight)));
			layoutAdd(bottom, layout(vec2(leftScaleWidth,0)));
			Layout* lBottom = layoutAdd(bottom, layout(vec2(0,0)));


		Layout* lGraphs[Line_Graph_Count];
		for(int i = 0; i < ad->camCount; i++) {
			lGraphs[i] = layoutAdd(lChart, layout(vec2(0,ad->graphOffsets[i+1]-ad->graphOffsets[i])));
			layoutAdd(lGraphs[i], layout(vec2(leftScaleWidth, 0)));
			layoutAdd(lGraphs[i], layout(vec2(0, 0)));
		}

		layoutCalc(lay);

		Rect screenRect = getScreenRect(ws);
		Rect rChart = lChart->r;
		Rect rFilters = lFilters->r;
		Rect rSidePanel = lSidePanel!=0?lSidePanel->r:rect(0,0,0,0);
		Rect rBottomText = lBottom->r;

		Rect rLeftTexts = rectSetBR(rChart, vec2(rChart.left + leftScaleWidth, rChart.bottom));
		Rect rGraphs    = rectSetBL(rChart, vec2(rChart.left + leftScaleWidth, rChart.bottom));

		Rect graphRects[10];
		Rect leftTextRects[10];
		for(int i = 0; i < ad->camCount; i++) {
			leftTextRects[i] = lGraphs[i]->list->r;
			graphRects[i] = lGraphs[i]->list->next->r;
		}

		Rect rTopBar = lTopBar->r;
		ad->clientRect = lMain->r;


		// Title bar.
		if(!ws->fullscreen)
		{
			// Options.

			float titlePadding = newas->padding;
			float iconMargin = 0.4f;
			float iconWidth = 1.0f;
			char* titleText = fillString("%s - %i - %s", ad->playlist.title, ad->playlist.count, ad->playlist.id);

			Vec4 iconColor = newac->font;

			//

			drawRect(screenRect, colorWindowBorder);

			float z = 0;
			Layout* lay = layoutAlloc(layout(rTopBar, false, vec2i(1,0), vec2(titlePadding, titlePadding), vec2(0)));

			char* text = strLen(ad->playlist.id) != 0 ? titleText : "";
			Layout* lTitle = layoutAdd(lay, layout(vec2(0, 0)));
			Layout* lButtonMin = layoutAdd(lay, layout(vec2(layoutGetDim(lay).h, 0)));
			Layout* lButtonMax = layoutAdd(lay, layout(vec2(layoutGetDim(lay).h, 0)));
			Layout* lButtonClose = layoutAdd(lay, layout(vec2(layoutGetDim(lay).h, 0)));

			layoutCalc(lay);

			scissorState();
			Rect rTitle = layoutGetRect(lay);
			rTitle.right = lTitle->r.right;
			drawTextBox(rTitle, text, vec2i(-1,0), ad->gui->textBoxSettings);
			scissorState(false);

			float iconSize = font->height*iconMargin;

			ad->gui->buttonSettings = ad->uiButtonSettings;
			if(newGuiQuickButton(ad->gui, lButtonMin->r, "")) ShowWindow(windowHandle, SW_MINIMIZE);
			Rect a = rectExpand(lButtonMin->r, -iconSize);
			drawRect(rectSetT(a, a.bottom + iconWidth), iconColor);

			if(newGuiQuickButton(ad->gui, lButtonMax->r, "")) setWindowMode(windowHandle, ws, WINDOW_MODE_FULLBORDERLESS);
			drawRectHollow(rectExpand(lButtonMax->r, -iconSize), iconWidth, iconColor);

			if(newGuiQuickButton(ad->gui, lButtonClose->r, "")) *isRunning = false;
			drawCross(rectCen(lButtonClose->r), rectH(rectExpand(lButtonClose->r, -iconSize))/2, iconWidth, vec2(0,1), iconColor);
			ad->gui->buttonSettings = ad->buttonSettings;

		}
	
		drawRect(lMain->r, colorWindowClient);


		// Filters gui.
		{
			TIMER_BLOCK_NAMED("Filters Gui");

			char* labels[] = {"Panel (F1)", "Reset", "Settings", "Draw Mode:", "Stat config:", "0.000", "0.000", "<Filter_Inclusive>", "<Filter_Inclusive>"};
			int li = 0;
			float bp = font->height * newas->textPaddingMod * 2;
			float widths[] = {getTextDim(labels[li++], font).x+bp, getTextDim(labels[li++], font).x+bp, getTextDim(labels[li++], font).x+bp, 0, getTextDim(labels[li++], font).x+bp, 80, getTextDim(labels[li++], font).x+bp, getTextDim(labels[li++], font).x+bp/2, getTextDim(labels[li++], font).x+bp/2, getTextDim(labels[li++], font).x+bp, getTextDim(labels[li++], font).x+bp};
			li = 0;

			Layout* lay = layoutAlloc(layout(rFilters, false, vec2i(-1,0), vec2(newas->padding,0), vec2(newas->border)));
			for(int i = 0; i < arrayCount(widths); i++) layoutAdd(lay, layout(vec2(widths[i], 0)));
			layoutCalc(lay);
			Layout* l = lay->list;


			NewGui* gui = ad->gui;
			scissorState();

			if(newGuiQuickButton(gui, layoutInc(&l), labels[li++])) ad->panelActive = !ad->panelActive;
			if(newGuiQuickButton(gui, layoutInc(&l), labels[li++])) {
				ad->sortByDate = true;
				ad->sortStat = -1;
				ad->startLoadFile = true;
			}
			if(newGuiQuickButton(gui, layoutInc(&l), labels[li++])) {
				shellExecuteNoWindow(fillString("explorer.exe %s", App_Settings_File));
			}

			layoutInc(&l);

			newGuiQuickTextBox(gui, layoutInc(&l), labels[li++]);
			newGuiQuickSlider(gui, layoutInc(&l), &ad->graphDrawMode, 0, 2);

			newGuiQuickTextBox(gui, layoutInc(&l), labels[li++]);

			bool updateStats = false;
			if(newGuiQuickTextEdit(gui, layoutInc(&l), &ad->statTimeSpan)) updateStats = true;
			clamp(&ad->statTimeSpan, 0.01f, 12*10);
			if(newGuiQuickTextEdit(gui, layoutInc(&l), &ad->statWidth)) updateStats = true;
			clamp(&ad->statWidth, 0.01f, 12*10);

			gui->editSettings.defaultText = "<Filter_Inclusive>";
			if(newGuiQuickTextEdit(gui, layoutInc(&l), ad->inclusiveFilter, arrayCount(ad->inclusiveFilter))) ad->startLoadFile = true;
			gui->editSettings.defaultText = "<Filter_Exclusive>";
			if(newGuiQuickTextEdit(gui, layoutInc(&l), ad->exclusiveFilter, arrayCount(ad->exclusiveFilter))) ad->startLoadFile = true;
			gui->editSettings.defaultText = "";

			if(updateStats) calculateAverages(ad->videos, ad->playlist.count, &ad->averagesLineGraph, ad->statTimeSpan, ad->statWidth, ad->cams[0].xMax - ad->cams[0].xMin, &ad->releaseCountLineGraph);

			scissorState(false);
		}

		// Height sliders.
		{
			float z = 1;

			for(int i = 0; i < ad->camCount-1; i++) {
				Rect r = rectCenDim(rectB(leftTextRects[i]), vec2(ad->leftTextWidth, as->resizeRegionSize));

				int event = newGuiGoDragAction(ad->gui, r, z);
				if(event == 1) ad->gui->mouseAnchor = getMousePosS(false) - ad->graphOffsets[i+1]*rectH(rGraphs);
				if(event > 0) {
					ad->graphOffsets[i+1] = (getMousePosS(false) - ad->gui->mouseAnchor).y/rectH(rGraphs);
					clamp(&ad->graphOffsets[i+1], ad->graphOffsets[i] + Graph_Min_Height/2, ad->graphOffsets[i+2] - Graph_Min_Height/2);

					setCursor(ws, IDC_SIZENS);
				}

				if(newGuiIsWasHotOrActive(ad->gui)) setCursor(ws, IDC_SIZENS);
			}
		}

		// Update Cameras.
		for(int i = 0; i < ad->camCount; i++) {
			GraphCam* cam = ad->cams + i;

			Rect rGraph = graphRects[i];
			Rect rLeftText = leftTextRects[i];

			graphCamSetViewPort(cam, rectExpand(rGraph, vec2(-font->height,-font->height)));

			double camMinH;
			if(i == 0) camMinH = Cam_Min_Height_0;
			else if(i == 1) camMinH = Cam_Min_Height_1;
			else if(i == 2) camMinH = Cam_Min_Height_2;
			else if(i == 3) camMinH = Cam_Min_Height_3;

			float zoomMin = ad->sortByDate?Graph_Zoom_Min:Graph_Zoom_MinNoDate;

			{
				float z = 0;
				if(newGuiGoButtonAction(ad->gui, rLeftText, z, Gui_Focus_MWheel)) 
	    			graphCamScaleToPos(cam, 0, 0, 0, -input->mouseWheel, Graph_Zoom_Speed, camMinH, input->mousePosNegative);

	    		if(i == 0) {
					if(newGuiGoButtonAction(ad->gui, rGraphs, z, Gui_Focus_MWheel)) {
						graphCamScaleToPos(&ad->cams[i], -input->mouseWheel, Graph_Zoom_Speed, zoomMin, 0, 0, 0, input->mousePosNegative);
					}
	    		}
			}

			cam->x = ad->cams[0].x;
			cam->w = ad->cams[0].w;

			graphCamSizeClamp(cam, zoomMin, camMinH);

			{
				NewGui* gui = ad->gui;
				float z = 0;
				int event;

				if(i == 0) {
		    		event = newGuiGoDragAction(gui, rGraphs, z, Gui_Focus_MLeft);
		    		if(event == 1) gui->mouseAnchor.x = -getMousePosS().x - graphCamCamToScreenSpaceX(cam, cam->x);
		    		if(event > 0) {
		    			double camX = graphCamScreenToCamSpaceX(cam, -getMousePosS().x - gui->mouseAnchor.x);

						for(int i = 0; i < ad->camCount; i++) {
							ad->cams[i].x = camX;
							graphCamUpdateSides(&ad->cams[i]);
						}
					}
				}

				event = newGuiGoDragAction(gui, rLeftText, z, Gui_Focus_MLeft);
				if(event == 1) gui->mouseAnchor.y = -input->mousePosNegative.y - graphCamCamToScreenSpaceY(cam, cam->y);
				if(event > 0) {
					cam->y = graphCamScreenToCamSpaceY(cam, -input->mousePosNegative.y - gui->mouseAnchor.y);
					graphCamUpdateSides(cam);
				}

			}

			graphCamPosClamp(cam);
		}

		// Draw graph backgrounds.
		for(int i = 0; i < ad->camCount; i++) {
			drawRectNewColoredH(graphRects[i], newac->graphBackgroundBottom, newac->graphBackgroundTop);
		}

		TIMER_BLOCK_END(appIntro);

		// Draw left scale.
		{
			TIMER_BLOCK_NAMED("Left Text");

			float markLength = 10;
			float textMarginMod = 0.2f;
			float lineWidth = 1;
			float shadow = newas->fontShadow;

			Vec4 mainColor = newac->font;
			Vec4 semiColor = newac->font2;
			Vec4 horiLinesColor = newac->graphMark;
			Vec4 subHoriLinesColor = newac->graphSubMark;
			Vec4 shadowColor = newac->fontShadow;

			float div = 10;
			int subDiv = 4;
			float splitSizePixels = font->height*3;


			Vec4 leftScaleDividerColor = newac->edge;
			float leftScaleDividerSize = 1;


			glLineWidth(lineWidth);
			glEnable(GL_SCISSOR_TEST);

			float textMargin = font->height*textMarginMod;
			float markSizeDifference = 0.5f;

			bool calcMaxTextWidth;

			for(int camIndex = 0; camIndex < ad->camCount; camIndex++) {
				calcMaxTextWidth = camIndex == 0 ? true : false;

				GraphCam* cam = ad->cams + camIndex;
				Rect scaleRect = leftTextRects[camIndex];

				float maxTextWidth = 0;

				float splitSize = splitSizePixels * (cam->h / rectH(cam->viewPort));
				float stepSize = pow(div, roundUpFloat(logBase(splitSize, div)));
				// float subSplitSize = font->height * (cam->h / rectH(cam->viewPort));
				int i = 0;
				float p = roundModDown(cam->bottom, stepSize);
				while(p < cam->top + stepSize) {
					float y = graphCamMapY(cam, p);

					// Horizontal line.
					scissorTestScreen(cam->viewPort);
					glLineWidth(0.5f);
					drawLine(vec2(scaleRect.right, roundFloat(y)+0.5f), vec2(cam->viewPort.right, roundFloat(y)+0.5f), horiLinesColor); 
					glLineWidth(1);
					scissorTestScreen(scaleRect);

					// Base markers.
					drawLineNewOff(vec2(scaleRect.right, roundFloat(y)+0.5f), vec2(-markLength,0), mainColor); 

					char* text;
					if(stepSize/subDiv > 10) text = fillString("%i.",(int)p);
					else text = fillString("%f",p);

					drawText(text, font, vec2(scaleRect.right - markLength - textMargin, y), mainColor, vec2i(1,0), 0, shadow, shadowColor);
					
					float textWidth = getTextDim(text, font).w;

					if(calcMaxTextWidth) maxTextWidth = max(maxTextWidth, textWidth + markLength + textMargin*2);

					// Semi markers.
					for(int i = 1; i < subDiv; i++) {
						float y = graphCamMapY(cam, p+i*(stepSize/subDiv));
						drawLineNewOff(vec2(scaleRect.right, roundFloat(y)+0.5f), vec2(-markLength*markSizeDifference,0), semiColor); 

						char* subText;
						if(stepSize/subDiv > 10) subText = fillString("%i.",(int)(stepSize/subDiv));
						else subText = fillString("%f",(stepSize/subDiv));

						drawText(subText, font, vec2(scaleRect.right - markLength*markSizeDifference - textMargin, y), semiColor, vec2i(1,0), 0, shadow, shadowColor);

						// Draw horizontal line.
						scissorTestScreen(cam->viewPort);
						glLineWidth(0.5f);
						drawLine(vec2(scaleRect.right, roundFloat(y)+0.5f), vec2(cam->viewPort.right, roundFloat(y)+0.5f), subHoriLinesColor); 
						glLineWidth(1);
						scissorTestScreen(scaleRect);
					}

					p += stepSize;
				}

				if(calcMaxTextWidth) {
					if(ad->leftTextWidth != maxTextWidth) ad->appIsBusy = true;

					ad->leftTextWidth = maxTextWidth;
				}
			}

			glDisable(GL_SCISSOR_TEST);

			// Draw dividers.
			Vec4 color = leftScaleDividerColor;
			float offset = 0;
			glLineWidth(leftScaleDividerSize);
			for(int camIndex = 0; camIndex < ad->camCount-1; camIndex++) {
				Rect r = leftTextRects[camIndex];
				drawLine(rectBL(r), rectBR(r), color);
			}
			glLineWidth(1);
		}

		// Draw bottom scale.
		if(ad->sortByDate)
		{
			TIMER_BLOCK_NAMED("Bottom Text");


			int subStringMargin = font->height/2;
			float splitOffset = font->height;

			Vec4 mainColor = newac->font;
			Vec4 semiColor = newac->font2;
			Vec4 horiLinesColor = newac->graphMark;
			Vec4 subHoriLinesColor = newac->graphSubMark;

			float markLength = font->height - 3;
			float fontMargin = 0;
			float lineWidth = 1;

			float shadow = newas->fontShadow;
			Vec4 shadowColor = newac->fontShadow;

			float div = 10;
			float splitSizePixels = 1000;

			glLineWidth(lineWidth);
			glEnable(GL_SCISSOR_TEST);

			Rect scaleRect = rBottomText;

			i64 oneYearInSeconds = (i64)365*86400;
			i64 oneMonthInSeconds = (i64)30*86400;
			i64 oneDayInSeconds = (i64)1*86400;

			GraphCam* cam = &ad->cams[0];

			float yearInPixels = graphCamCamToScreenSpaceX(cam, oneYearInSeconds);
			float monthInPixels = graphCamCamToScreenSpaceX(cam, oneMonthInSeconds);
			float dayInPixels = graphCamCamToScreenSpaceX(cam, oneDayInSeconds);

			float yearStringSize = getTextDim("0000", font).w;
			float monthStringSize = getTextDim("00.0000", font).w;
			float dayStringSize = getTextDim("00.00.0000", font).w;
			float timeStringSize = getTextDim("00:00", font).w;


			int zoomStage = 0;
			if(dayInPixels > dayStringSize + splitOffset) zoomStage = 2;
			else if(monthInPixels > monthStringSize + splitOffset) zoomStage = 1;
			else zoomStage = 0;


			int subMarkerWidth = 0;
			int subMarkerCount = 0;

			Date startDate = dateDecode(cam->left);
			
			if(zoomStage == 0) {
				startDate = {startDate.y, 1,1,0,0,0};

				float markerDiff = (yearInPixels - yearStringSize)/(monthStringSize+splitOffset);
				if(markerDiff > 4-1) subMarkerWidth = 3;
				else if(markerDiff > 2-1) subMarkerWidth = 6;
				else subMarkerWidth = 12;

				subMarkerCount = subMarkerWidth>0?12/subMarkerWidth:0;

			} else if(zoomStage == 1) {
				startDate = {startDate.y, startDate.m, 1,0,0,0};

				float markerDiff = (monthInPixels - monthStringSize)/(dayStringSize+splitOffset);
				if(markerDiff > 6-1) subMarkerWidth = 5;
				else if(markerDiff > 3-1) subMarkerWidth = 10;
				else subMarkerWidth = 30;

				subMarkerCount = subMarkerWidth>0?30/subMarkerWidth:0;

			} else if(zoomStage == 2) {
				startDate = {startDate.y, startDate.m, startDate.d, 0,0,0};

				float markerDiff = (dayInPixels - dayStringSize)/(timeStringSize+splitOffset);
				if(markerDiff > 12-1) subMarkerWidth = 2;
				else if(markerDiff > 6-1) subMarkerWidth = 4;
				else if(markerDiff > 3-1) subMarkerWidth = 8;
				else subMarkerWidth = 24;

				subMarkerCount = subMarkerWidth>0?24/subMarkerWidth:0;
			}

			dateEncode(&startDate);

			for(;;) {

				Date nextDate = startDate;
				dateIncrement(&nextDate, zoomStage);
				dateEncode(&nextDate);

				double p = startDate.n;

				// Semi markers.
				{
					Date d = startDate;
					dateEncode(&d);
					double leftX = graphCamMapX(cam, d.n);
					for(int i = 0; i < subMarkerCount; i++) {
						double x = graphCamMapX(cam, d.n);

						char* dateString;
						if(d.m == 1 && d.d == 1) dateString = fillString("%s%i", d.y<10?"200":"20", d.y);
						else if(d.d == 1) dateString = fillString("%s%i..%s%i", d.m < 10?"0":"", d.m, d.y<10?"200":"20", d.y);
						else dateString = fillString("%s%i..%s%i..%s%i", d.d < 10?"0":"", d.d, d.m < 10?"0":"", d.m, d.y<10?"200":"20", d.y);
						
						if(zoomStage == 2 && i != 0) dateString = fillString("%s%i:00", d.h<10?"0":"", d.h);

						scissorTestScreen(rGraphs);
						glLineWidth(0.5f);
						drawLine(vec2(roundFloat(x)+0.5f, rGraphs.bottom), vec2(roundFloat(x)+0.5f, rGraphs.top), i==0?horiLinesColor:subHoriLinesColor);
						glLineWidth(1);
						scissorTestScreen(scaleRect);

						drawLineNewOff(vec2(roundFloat(x)+0.5f, scaleRect.top), vec2(0,-markLength), mainColor); 
						// drawText(dateString, font, vec2(x, scaleRect.top - markLength - fontMargin), mainColor, vec2i(0,1));
						drawText(dateString, font, vec2(x, scaleRect.top - markLength - fontMargin), mainColor, vec2i(0,1), 0, shadow, shadowColor);

						dateIncrement(&d, subMarkerWidth, zoomStage+1);
						dateEncode(&d);
						x = graphCamMapX(cam, d.n);

						int amount = subMarkerWidth;
						if(i == subMarkerCount-1) {
							if(zoomStage == 1) {
								amount = getMonthDayCount(startDate.m, startDate.y) - (i)*subMarkerWidth;
								x = graphCamMapX(cam, nextDate.n)-1;
							} 
						}

						char* subString = fillString("%i%s", amount, zoomStage==0?"m":zoomStage==1?"d":"h");
						// drawText(subString, font, vec2(leftX + (x-leftX)/2, scaleRect.top), semiColor, vec2i(0,1));
						drawText(subString, font, vec2(leftX + (x-leftX)/2, scaleRect.top), semiColor, vec2i(0,1), 0, shadow, shadowColor);

						leftX = x;
					}
				}

				if(startDate.n > cam->right) break;
				dateIncrement(&startDate, zoomStage);

				dateEncode(&startDate);
			}

			glDisable(GL_SCISSOR_TEST);
		}

		// Graphs.
		{
			TIMER_BLOCK_BEGIN_NAMED(drawGraphs, "DrawGraphs");

			float settingHoverSearchWidth = 100;
			float settingHoverDistance = 20;
			Vec4 c1 = newac->graphData1;
			Vec4 c2 = newac->graphData2;

			Vec4 statColor = newac->graphData3;

			int lineWidth1 = 1;
			int lineWidth2 = 3;
			int pointSize = 3;
			int statLineWidth = 1;



			int settingHoverPointSize = 10;
			int settingHoverPointOffset = 10;
			Font* graphTextFont = ad->font;
			Vec2 graphTextOffset = vec2(5);
			float graphTextOutline = newas->graphFontShadow;
			float hoverTextOutline = newas->graphFontShadow;

			Vec4 graphTextColor = newac->graphFont;




			Font* graphTitleFont = ad->fontTitle;
			Vec2 graphTitleOffset = vec2(as->titleOffset, -as->titleOffset);
			// Vec2 graphTitleOffset = vec2(graphTitleFont->height*0.25f,-1);
			Vec4 graphTitleColor = newac->font;
			float graphTitleOutline = newas->fontShadow;
			Vec4 graphTitleOutlineColor = newac->fontShadow;

			char* graphTitles[Line_Graph_Count] = {"Views", "Likes", "Likes/Dislikes", "Comments"};
			char* sortButtonText = "Sort";
			Vec2 sortButtonOffset = vec2(font->height*0.5f, 0);
			float textPadding = font->height * newas->textPaddingMod;


			float selectionWidth = 1;
			Vec4 selectionColor = newac->graphFont;
			selectionColor.a = 0.2f;



			YoutubeVideo* vids = ad->videos;
			int vidCount = ad->playlist.count;
			ad->hoveredVideo = -1;

			glEnable(GL_SCISSOR_TEST);

			// Draw Graphs.
			int graphCount = ad->camCount + 1;
			int mode = ad->graphDrawMode;
			for(int graphIndex = 0; graphIndex < graphCount; graphIndex++) {

				Vec4 colors[] = {c1,c1,c2,c2,c1};

				int camIndexes[] = {0,1,1,2,3};
				GraphCam* cam = ad->cams + camIndexes[graphIndex];
				Vec4 color = colors[graphIndex];
				Rect graphRect = graphRects[camIndexes[graphIndex]];

				scissorTestScreen(rectExpand(cam->viewPort, 1));

				if(ad->playlist.count == 1) mode = 2;

				if(mode == 0) {
					glLineWidth(lineWidth1);
					drawLineStripHeader(color);
				} else if(mode == 1) {
					glPointSize(pointSize);
					drawPointsHeader(color);
				} else if(mode == 2) {
					glLineWidth(lineWidth2);
					drawLinesHeader(color);
				}

				float statMouseDiff = FLT_MAX;
				double searchDistance = graphCamScreenToCamSpaceX(cam, settingHoverSearchWidth)/2;

				Vec2 mousePos = input->mousePosNegative;
				bool mouseInGraphRect = pointInRect(mousePos, graphRect);

				float camBottomY = graphCamMapY(cam, 0);

				bool lastOneWasOverRight = false;
				for(int i = 0; i < vidCount; i++) {

					double xValue;
					if(ad->sortByDate) {
						if(i+1 < vidCount && vids[i+1].date.n < cam->left) continue;
						if(lastOneWasOverRight) continue;
						if(vids[i].date.n > cam->right) lastOneWasOverRight = true;

						xValue = vids[i].date.n;
					} else {
						if(i < cam->left-1 || i > cam->right+1) continue;
						xValue = i;
					}

					double value;
					     if(graphIndex == 0) value = vids[i].viewCount;
					else if(graphIndex == 1) value = vids[i].likeCount + vids[i].dislikeCount;
					else if(graphIndex == 2) value = vids[i].dislikeCount;
					else if(graphIndex == 3) value = videoGetLikesDiff(&vids[i]);
					else if(graphIndex == 4) value = vids[i].commentCount;

					Vec2 point = graphCamMap(cam, xValue, value);

					// Get closest point to mouse.
					if(mouseInGraphRect) {
						if(valueBetweenDouble(xValue, xValue - searchDistance, xValue + searchDistance)) {
							float diff = lenLine(point, mousePos);

							if(diff < statMouseDiff && diff < settingHoverDistance) {
								statMouseDiff = diff;
								ad->hoveredPoint = point;
								ad->hoveredVideo = i;
								ad->hoveredVideoStat = value;
							}
						}
					}

					if(mode < 2) pushVec(point);
					else pushVecs(vec2(point.x, camBottomY), point);
				}

				glEnd();
			}

			// Average line.
			{
				glLineWidth(statLineWidth);
				glEnable(GL_SCISSOR_TEST);

				for(int graphIndex = 0; graphIndex < Line_Graph_Count; graphIndex++) {

					GraphCam* cam = ad->cams + graphIndex;

					double avgStartX = ad->videos[0].date.n;
					double avgWidth = monthsToInt(ad->statWidth);
					double avgTimeSpan = monthsToInt(ad->statTimeSpan);

					scissorTestScreen(rectExpand(cam->viewPort, 1));

					if(ad->sortByDate) {
						if(ad->playlist.count > 1) {
							drawLineStripHeader(statColor);

							double xPos = avgStartX;
							int endCount = ad->averagesLineGraph.count-1;
							for(int i = 0; i < endCount; i++) {
								if(xPos + avgWidth < cam->left) {
									xPos += avgWidth;
									continue;
								}
								if(xPos - avgWidth > cam->right) break;

								{
									Vec2 p0, p1, p2, p3;

									p1 = graphCamMap(cam, xPos, ad->averagesLineGraph.points[graphIndex][i]);
									p2 = graphCamMap(cam, xPos+avgWidth, ad->averagesLineGraph.points[graphIndex][i+1]);
									
									if(i != 0) p0 = graphCamMap(cam, xPos-avgWidth, ad->averagesLineGraph.points[graphIndex][i-1]);
									else p0 = p1 + (p1 - p2);
									if(i != endCount-1) p3 = graphCamMap(cam, xPos+avgWidth*2, ad->averagesLineGraph.points[graphIndex][i+2]);
									else p3 = p2 - (p2 - p1);

									int segmentCount = bezierCurveGuessLength(p0, p1, p2, p3)/Cubic_Curve_Segment_Mod;
									segmentCount = clampMin(segmentCount, Cubic_Curve_Segment_Min);

									for(int i = 0; i < segmentCount; i++) {
										pushVec(cubicInterpolationVec2(p0, p1, p2, p3, (float)i/(segmentCount-1)));
									}
								}

								xPos += avgWidth;
							}
							glEnd();
						}

					} else {
						Statistic* stat = ad->stats + graphIndex;
						float y = graphCamMapY(cam, stat->avg);
						drawLine(vec2(cam->viewPort.left, y), vec2(cam->viewPort.right, y), statColor);
					}

				}

				glDisable(GL_SCISSOR_TEST);
				glLineWidth(1);
			}

			TIMER_BLOCK_END(drawGraphs);

			glDisable(GL_SCISSOR_TEST);

			// Mouse hover.
			{
				TIMER_BLOCK_NAMED("Mouse Hover");

				Font* font = graphTextFont;
				Vec4 c = graphTextColor;
				float os = graphTextOutline;

				int id = newGuiIncrementId(ad->gui);
				bool mPosActive = newGuiGoMousePosAction(ad->gui, rGraphs, 0);
				if(mPosActive && ad->hoveredVideo != -1) {
					scissorTestScreen(rGraphs);
					glEnable(GL_SCISSOR_TEST);

					int vi = ad->hoveredVideo;

					// Draw text top right.

					Vec2 p = rChart.max - graphTextOffset;
					drawTextOutlined(fillString("%s", ad->videos[vi].title), font, p, os, c, vec4(0,1), vec2i(1,1)); p.y -= font->height;
					drawTextOutlined(fillString("%s", ad->videos[vi].dateString), font, p, os, c, vec4(0,1), vec2i(1,1)); p.y -= font->height;
					drawTextOutlined(fillString("%i", vi), font, p, os, c, vec4(0,1), vec2i(1,1)); p.y -= font->height;

					// Draw Point.
					glPointSize(settingHoverPointSize);
					// drawText(fillString("%i", ad->videos[vi].viewCount), font, ad->hoveredPoint + vec2(10,10), c, vec2i(-1,-1));
					char* text;
					if(ad->hoveredVideoStat > 10) text = fillString("%i.", (int)ad->hoveredVideoStat);
					else text = fillString("%f", ad->hoveredVideoStat);
					os = hoverTextOutline;

					// Get text pos and clamp to graph rect.
					Vec2 hoverPos = ad->hoveredPoint + vec2(settingHoverPointOffset,settingHoverPointOffset);
					Rect textRect = rectBLDim(hoverPos, getTextDim(text, font));
					Vec2 offset = rectInsideRectClamp(textRect, rectExpand(rGraphs, vec2(-4)));
					textRect = rectTrans(textRect, offset);
					hoverPos = rectBL(textRect);

					drawTextOutlined(text, font, hoverPos, os, c, vec4(0,1), vec2i(-1,-1));
					drawPoint(ad->hoveredPoint, c);
					glPointSize(1);

					glDisable(GL_SCISSOR_TEST);

					if(newGuiGoButtonAction(ad->gui, id, rGraphs, 0, Gui_Focus_MMiddle)) {
						if(vi != ad->selectedVideo) {
							ad->selectedPoint = ad->hoveredPoint;
							ad->selectedVideo = vi;

							downloadModeSet(&ad->modeData, Download_Mode_Snippet, &ad->appIsBusy);
						}
					}
				}
			}

			// Draw graph titles. 
			glEnable(GL_SCISSOR_TEST);
			for(int i = 0; i < ad->camCount; i++) {
				Rect rGraph = graphRects[i];
				Vec2 tp = rectTL(rGraph) + graphTitleOffset;

				scissorTestScreen(rGraph);

				char* text = graphTitles[i];
				float textWidth = getTextDim(text, graphTitleFont).w;

				drawTextOutlined(text, graphTitleFont, tp, graphTitleOutline, graphTitleColor, graphTitleOutlineColor, vec2i(-1,1));

				// Sort buttons.
				{
					char* text = sortButtonText;
					Rect r = rectTLDim(tp + vec2(textWidth, 0) + sortButtonOffset, vec2(getTextDim(text, font).w + textPadding*2, font->height));

					ad->gui->zLevel++;
					if(newGuiQuickButton(ad->gui, r, text)) {
						if(ad->sortStat != i) {
							ad->startLoadFile = true;
							ad->sortByDate = false;
							ad->sortStat = i;
						}
					}
					ad->gui->zLevel--;
				}
			}
			glDisable(GL_SCISSOR_TEST);

			// Graph selection line.
			if(ad->selectedVideo != -1)
			{
				float x;
				if(ad->sortByDate) x = graphCamMapX(&ad->cams[0], ad->videos[ad->selectedVideo].date.n) + 1;
				else x = graphCamMapX(&ad->cams[0], ad->selectedVideo);

				glLineWidth(selectionWidth);
				drawLine(vec2(x, rGraphs.bottom), vec2(x, rGraphs.top), selectionColor);
				glLineWidth(1);
			}
		}


		// Side panel.
		if(ad->selectedVideo != -1 && rectW(rSidePanel) != 0) {

			TIMER_BLOCK_NAMED("SidePanel");

			float padding = newas->padding;

			float resizeRegionSize = as->resizeRegionSize;
			Vec4 sidePanelColor = newac->background;
			float border = newas->border;
			Rect rPanel = rectExpand(rSidePanel, vec2(-border*2));
			Font* font = ad->font;
			float xMid = rectCen(rPanel).x;
			float width = rectW(rPanel);
			Vec4 fc = newac->font;
			Vec4 fc2 = newac->font2;
			Vec4 fcComment = newac->font;

			float heightMod = newas->heightMod;
			float rowHeight = font->height * heightMod;

			float yOffset = padding;

			float shadow = newas->fontShadow;
			Vec4 shadowColor = newac->fontShadow;

			float commentShadow = newas->fontShadow;
			Vec4 commentShadowColor = newac->fontShadow;

			NewGui* gui = ad->gui;


			// Panel width slider.
			{
				Rect r = rectCenDim(rectL(rSidePanel), vec2(resizeRegionSize, rectH(rSidePanel)));

				int event = newGuiGoDragAction(gui, r, 1);
				if(event == 1) gui->mouseAnchor.x = (ws->currentRes.w-input->mousePos.x) - ad->sidePanelWidth;
				if(event > 0) {
					ad->sidePanelWidth = (ws->currentRes.w-input->mousePos.x) - gui->mouseAnchor.x;
					clamp(&ad->sidePanelWidth, Side_Panel_Min_Width, ws->currentRes.w*ad->sidePanelMax);
				}

				if(newGuiIsWasHotOrActive(gui)) setCursor(ws, IDC_SIZEWE);
			}

			drawRect(rSidePanel, sidePanelColor);

			scissorState();
			newGuiScissorPush(gui, rSidePanel);
			newGuiLayoutPush(gui, layoutData(rPanel, rowHeight, yOffset, 0));
			LayoutData* ld = gui->ld;


			bool closePanel = false;
			// Top gui.
			{
				Layout* lay = layoutAlloc(layout(rect(0,0,0,0), false, vec2i(-1,0), vec2(padding, 0)));
				Layout* l = layoutQuickRow(lay, newGuiLRectAdv(gui), 40, 0, 40, 30);

				int modSelectedVideo = 0;
				Rect r;

				r = layoutInc(&l);
				if(ad->selectedVideo > 0) {
					if(newGuiQuickButton(gui, r, "")) modSelectedVideo = 1;
					drawTriangle(rectCen(r), font->height/3, vec2(-1,0), fc);
				}

				if(newGuiQuickButton(gui, layoutInc(&l), "Open in Browser")) {
					shellExecuteNoWindow(fillString("cmd.exe /c start \"link\" \"https://www.youtube.com/watch?v=%s\"", ad->videos[ad->selectedVideo].id));
				}

				r = layoutInc(&l);
				if(ad->selectedVideo != ad->playlist.count-1) {
					if(newGuiQuickButton(gui, r, "")) modSelectedVideo = 2;
					drawTriangle(rectCen(r), font->height/3, vec2(1,0), fc);
				}

				r = layoutInc(&l);
				if(newGuiQuickButton(gui, r, "")) closePanel = true;
				drawCross(rectCen(r), font->height/3, 2, vec2(0,1), fc);

				if(modSelectedVideo != 0) {
					if(ad->selectedVideo != -1) {
						int newSelection;
						if(modSelectedVideo == 1) newSelection = clampMin(ad->selectedVideo - 1, 0);
						else newSelection = clampMax(ad->selectedVideo + 1, ad->playlist.count - 1);

						if(newSelection != ad->selectedVideo) {
							ad->selectedVideo = newSelection;

							downloadModeSet(&ad->modeData, Download_Mode_Snippet, &ad->appIsBusy);
						}
					}
				}
			}


			YoutubeVideo* sv = ad->videos + ad->selectedVideo;
			VideoSnippet* sn = &ad->videoSnippet;

			newGuiQuickTextBox(gui, newGuiLRectAdv(gui), fillString("Index: %i, VideoId: %s", ad->selectedVideo, sv->id));

			// Draw thumbnail.
			{
				Vec2 imageDim = vec2(Thumbnail_Size_X, Thumbnail_Size_Y);
				if(imageDim.w > rectW(rPanel)) imageDim = vec2(rectW(rPanel), rectW(rPanel)*(divZero(imageDim.h, imageDim.w)));
				Rect imageRect = rectCenDim(rectCenX(rPanel), ld->pos.y - imageDim.h/2, imageDim.w, imageDim.h);
				newGuiLAdv(gui, rectH(imageRect));

				if(ad->modeData.downloadMode == Download_Mode_Snippet && ad->modeData.downloadStep < 3) 
					drawTriangle(rectCen(imageRect), font->height/2, rotateVec2(vec2(1,0), ad->time*2), fc);
				else drawRect(imageRect, rect(0.01,0,0.99,1), sn->thumbnailTexture.id);
			}

            drawText(sv->title, font, vec2(xMid, ld->pos.y), fc, vec2i(0,1), width, shadow, shadowColor);
            newGuiLAdv(gui, getTextHeight(sv->title, font, vec2(xMid, ld->pos.y), width));

			{
				Rect r = newGuiLRectAdv(gui);
				rectSetDim(&r, vec2(ld->dim.w, rectH(r)*0.7f));

				glLineWidth(1);
				float dislikePercent = videoGetLikesDiff(sv);
				drawRectProgressHollow(r, 1-dislikePercent, newac->windowBorder, newac->edge);
				drawText(fillString("%f%%", dislikePercent*100), font, rectCen(r), fc, vec2i(0,0), 0, shadow, shadowColor);
			}

			{
				int lineCount = 6;
				float height = font->height*lineCount;
				Vec2 top = rectT(newGuiLRectAdv(gui, height)); 

				float centerMargin = font->height;
				char* text[] = {"Date:", "Duration:", "Views:", "Likes/Dislikes:", "Comments:", "Favorites:"};

				Vec2 p;
				p = top - vec2(centerMargin/2, 0);
				for(int i = 0; i < lineCount; i++) {
					char* t = text[i];
				
					drawText(t, font, p, fc, vec2i(1,1), 0, shadow, shadowColor); 
					p.y -= font->height;
				}

				p = top + vec2(centerMargin/2, 0);
				// scissorTestScreen(rectTRDim(p, vec2(writeDim.w/2 - 1, height)));
				for(int i = 0; i < lineCount; i++) {
					char* t;
					if(i == 0) t = fillString("%s%i..%s%i..%s%i | %s%i:%s%i:%s%i.", sv->date.d<10?"0":"", sv->date.d, sv->date.m<10?"0":"", sv->date.m, sv->date.y<10?"200":"20", sv->date.y, sv->date.h<10?"0":"", sv->date.h, sv->date.mins<10?"0":"", sv->date.mins, sv->date.secs<10?"0":"", sv->date.secs);
					if(i == 1) t = fillString("%s%im %is", 
					                          sv->duration.h > 0 ? fillString("%ih ", sv->duration.h) : "", 
					                          sv->duration.mins, 
					                          sv->duration.secs);
					if(i == 2) t = fillString("%i.", sv->viewCount);
					if(i == 3) t = fillString("%i | %i", sv->likeCount, sv->dislikeCount);
					if(i == 4) t = fillString("%i", sv->commentCount);
					if(i == 5) t = fillString("%i", sv->favoriteCount);
				
					drawText(t, font, p, fc, vec2i(-1,1), 0, shadow, shadowColor); 
					p.y -= font->height;
				}
			}

			newGuiQuickTextBox(gui, newGuiLRectAdv(gui), "Comments");

			// Comments section.
			{
				static float scrollHeight = 200;
				Rect r = rectTLDim(ld->pos, vec2(ld->dim.w, ld->pos.y - ld->region.bottom));

				ScrollRegionValues scrollValues = {};
				newGuiQuickScroll(gui, r, scrollHeight, &ad->commentScrollValue, &scrollValues, &ad->commentScrollSettings);

				ld = newGuiLayoutPush(gui, scrollValues.region);
				ld->pos = scrollValues.pos;
				newGuiScissorPush(gui, scrollValues.scissor);

					float wrapWidth = ld->dim.w;
					if(!(ad->modeData.downloadMode == Download_Mode_Snippet && ad->modeData.downloadStep < 4)) {
						for(int i = 0; i < sn->selectedCommentCount; i++) {
							drawText(sn->selectedTopComments[i], font, ld->pos, fcComment, vec2i(-1,1), wrapWidth, commentShadow, commentShadowColor);
							ld->pos.y -= getTextHeight(sn->selectedTopComments[i], font, vec2(xMid, ld->pos.y), wrapWidth);
							
							drawText(fillString("Likes: %i, Replies: %i", sn->selectedCommentLikeCount[i], sn->selectedCommentReplyCount[i]), font, ld->pos + vec2(ld->dim.w,0), fc2, vec2i(1,1), wrapWidth, commentShadow, commentShadowColor);
							ld->pos.y -= font->height;
						}
						scrollHeight = scrollValues.pos.y - ld->pos.y + 4; // @Hack.
					} else {
						scrollHeight = rectH(r);
						drawTriangle(rectCen(r), font->height/2, rotateVec2(vec2(1,0), ad->time*2), fc);
					}

				ld = newGuiLayoutPop(gui);
				newGuiScissorPop(gui);
				ld->pos.y = rectB((ld+1)->region).y - ld->yPadding;
			}

			newGuiScissorPop(gui);
			newGuiLayoutPop(gui);
			scissorState(false);


			if(closePanel) {
				ad->selectedVideo = -1;
				if(ad->modeData.downloadMode == Download_Mode_Snippet) {
					downloadModeAbort(&ad->modeData, &ad->appIsBusy);
				}
			}
		}
		
		// Draw Borders.
		{
			glLineWidth(0.5f);
			drawRectOutline(rGraphs, newac->edge);
			drawRectOutline(ad->clientRect, newac->edge);
		}

	}

	TIMER_BLOCK_END(appMain);

	endOfMainLabel:




	// Main Panel.
	if(ad->panelActive) {
		NewAppSettings newas = ad->newAppSettings;
		NewAppColors newac = ad->newAppColors;

		bool clampToWindow = true;
		bool resizable = true;
		bool resizableX = true;
		bool resizableY = false;
		// bool resizableY = true;
		bool movable = true;

		Font* font = ad->font;

		float panelBorder = newas.windowBorder;
		float windowCornerGraphSize = panelBorder*4;
		float roundedCorners = newas.rounding;
		float titleHeightMod = newas.windowHeightMod;
		char* titleText = "App";
		float windowControlsPadding = 0.6f;
		float windowControlsSize = 1.5f;

		Vec2 clientBorder = vec2(newas.border);
		float heightMod = newas.heightMod;
		float rowHeight = font->height*heightMod;
		float padding = newas.padding;
		float rowOffset = padding;


		float sectionOffset = 20;
		float listOffset = 1;

		Vec4 fontColor = newac.font;
		Vec4 fontShadowColor = newac.fontShadow;
		Vec4 borderColor = newac.windowBorder;
		Vec4 backgroundColor = newac.background;
		Vec4 buttonColor = newac.button;

		Vec4 editBackgroundColor = newac.uiBackground;
		Vec4 editSelectionColor = newac.editSelection;
		Vec4 editCursorColor = newac.editCursor;

		Vec4 scrollRegionColor = newac.uiBackground;
		Vec4 scrollSliderColor = buttonColor;

		Vec4 windowControlsColor = buttonColor;

		Vec4 progressLeft = newac.windowBorder;

		Vec4 edgeColor = newac.edge;

		float textMargin = newas.textPaddingMod;
		float textPadding = font->height * newas.textPaddingMod;

		NewGui* gui = ad->gui;
		float zLevel = 2;
		gui->zLevel = zLevel;

		GuiWindowSettings windowSettings = {panelBorder, windowCornerGraphSize, vec2(Panel_Min_X, Panel_Min_Y), vec2(0,0), ad->clientRect, movable, resizableX, resizableY, false};

		static float lastPanelHeight = 0;

		static int panelMode = 0;

		ad->panelRect.bottom = ad->panelRect.top - lastPanelHeight;

		newGuiSetHotAllMouseOver(ad->gui, ad->panelRect, zLevel);
		newGuiWindowUpdate(gui, &ad->panelRect, zLevel, windowSettings);

		drawRectRounded(ad->panelRect, borderColor, roundedCorners);

		{
			Rect insideRect = rectExpand(ad->panelRect, -panelBorder*2);
			float titleHeight = font->height*titleHeightMod;

			Rect titleRect = rectSetB(insideRect, insideRect.top - titleHeight);
			drawTextBox(titleRect, titleText, ad->labelSettings);

			Rect rClose = rectSetL(titleRect, titleRect.right-rectH(titleRect));

			if(newGuiQuickButton(gui, rClose, "", vec2i(0,0), &ad->uiButtonSettings)) ad->panelActive = false;
			drawCross(rectCen(rClose), rectW(rClose)/2 * windowControlsPadding, windowControlsSize, vec2(1,0), fontColor);

			scissorTestScreen(titleRect);
			if(downloadModeActive(&ad->modeData)) {
				drawTriangle(rectL(titleRect) + vec2(rectH(titleRect)/2,0), rectH(titleRect)/3, rotateVec2(vec2(1,0), ad->time*2), fontColor);
			}

			insideRect.top -= titleHeight+panelBorder;


			Layout lay = layout(rect(0,0,0,0), false, vec2i(-1,0), vec2(padding,0));
			Layout layTemp = layout(rect(0,0,0,0), false, vec2i(-1,0), vec2(0,0));
			Layout* l;

			Rect modeRect = rectSetB(insideRect, insideRect.top - font->height);

			// Panel mode switches.
			{
				insideRect.top -= font->height;

				char* labels[] = {"Main", "Settings"};
				int li = 0;
				float widths[] = {getTextDim(labels[li++], font).w+textPadding*2, getTextDim(labels[li++], font).w+textPadding*2};
				li = 0;

				scissorState();
				newGuiScissorPush(gui, modeRect);

				Vec4 bgColor = ad->uiButtonSettings.backgroundColor;
				gui->buttonSettings = ad->uiButtonSettings;
				Vec2 p = rectTL(modeRect);
				
				for(int i = 0; i < arrayCount(labels); i++) {
					Rect r = rectTLDim(p, vec2(widths[li++], rectH(modeRect))); 

					gui->buttonSettings.backgroundColor = panelMode == i ? newac.windowBorder : bgColor;
					gui->buttonSettings.borderColor.a = panelMode == i ? 0 : 1;


					if(newGuiQuickButton(gui, r, labels[i])) panelMode = i;

					p += vec2(rectW(r), 0);
				}

				gui->buttonSettings = ad->buttonSettings;

				newGuiScissorPop(gui);
				scissorState(false);
			}


			drawRectOutlined(insideRect, backgroundColor, edgeColor);

			scissorState();
			newGuiScissorPush(gui, insideRect);
			newGuiLayoutPush(gui, layoutData(rectExpand(insideRect, -clientBorder*2), rowHeight, rowOffset, 0));
			LayoutData* ld = gui->ld;

			float yOffsetExtra = font->height * 0.5f;
			Rect r;


			// panelMode = 1;
			if(panelMode == 1) {

				float maxTextWidth = 0;
				char** colorStrings = newAppColorsStrings;
				float colorCount = arrayCount(newAppColorsStrings);
				for(int i = 0; i < colorCount; i++) {
					maxTextWidth = max(maxTextWidth, getTextDim(colorStrings[i], font).w);
				}
				maxTextWidth += 2;

				NewAppColors* cs = &ad->newAppColors;
				for(int i = 0; i < colorCount; i++) {
					l = layoutQuickRow(&lay, newGuiLRectAdv(gui), maxTextWidth,0,0,0);
					newGuiQuickTextBox(gui, layoutInc(&l), colorStrings[i], vec2i(1,0));

					Vec3 hslColor = rgbToHslFloat(cs->e[i].rgb);
					newGuiQuickSlider(gui, layoutInc(&l), &hslColor.x, 0, 1);
					newGuiQuickSlider(gui, layoutInc(&l), &hslColor.y, 0, 1);
					newGuiQuickSlider(gui, layoutInc(&l), &hslColor.z, 0, 1);

					cs->e[i] = vec4(hslToRgbFloat(hslColor), cs->e[i].a);
				}

			} else if(panelMode == 0) {

				l = layoutQuickRow(&lay, newGuiLRectAdv(gui), 0,0);
				if(newGuiQuickButton(gui, layoutInc(&l), "Open app folder")) shellExecuteNoWindow(fillString("explorer.exe %s", App_Folder));
				if(newGuiQuickButton(gui, layoutInc(&l), "Delete app save")) remove(App_Save_File);

				newGuiLAdv(gui, yOffsetExtra);

				l = layoutQuickRow(&lay, newGuiLRectAdv(gui), 0,0);
				if(newGuiQuickButton(gui, layoutInc(&l), "Make screenshot")) {
					ad->startScreenShot = true;
					ds->showMenu = false;
				}
				newGuiQuickTextEdit(gui, layoutInc(&l), ad->screenShotFilePath, 49);


				char* labelText = "Dimension:";

				l = layoutQuickRow(&lay, newGuiLRectAdv(gui), getTextDim(labelText, font).w + font->height*textMargin*2,0,0);
				newGuiQuickTextBox(gui, layoutInc(&l), labelText, vec2i(-1,0));
				int maxTextureSize;
				glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
				if(newGuiQuickTextEdit(gui, layoutInc(&l), &ad->screenShotDim.w)) 
					clampInt(&ad->screenShotDim.w, 2, maxTextureSize-1);
				ad->screenShotDim.w = clampMin(ad->screenShotDim.w, Screenshot_Min_X);
				if(newGuiQuickTextEdit(gui, layoutInc(&l), &ad->screenShotDim.h)) 
					clampInt(&ad->screenShotDim.h, 2, maxTextureSize-1);
				ad->screenShotDim.h = clampMin(ad->screenShotDim.h, Screenshot_Min_Y);

				newGuiLAdv(gui, yOffsetExtra);

				newGuiQuickTextBox(gui, newGuiLRectAdv(gui), fillString("Playlist folder (%i item%s)", ad->playlistFolderCount, ad->playlistFolderCount==1?"":"s")); 

				l = layoutQuickRow(&lay, newGuiLRectAdv(gui), 0);
				if(newGuiQuickButton(gui, layoutInc(&l), "Open folder")) shellExecuteNoWindow(fillString("explorer.exe %s", Playlist_Folder));			

				{
					static float scrollValue = 0;
					Rect r = rectTDim(ld->pos + vec2(ld->dim.w/2, 0), vec2(ld->dim.w - sectionOffset*2, 10 * (rowHeight+listOffset))); 

					ScrollRegionValues scrollValues = {};
					newGuiQuickScroll(gui, r, ad->playlistFolderCount * (rowHeight+listOffset), &scrollValue, &scrollValues);


					ld = newGuiLayoutPush(gui, scrollValues.region);
					ld->pos = scrollValues.pos;
					newGuiScissorPush(gui, scrollValues.scissor);
					ld->yPadding = listOffset;

						char* buttonText = "Del";
						float buttonTextWidth = getTextDim(buttonText, font).w + font->height*textMargin*2;

						for(int i = 0; i < ad->playlistFolderCount; i++) {
							YoutubePlaylist* playlist = ad->playlistFolder + i;

							l = layoutQuickRow(&lay, newGuiLRectAdv(gui), 0, buttonTextWidth); 
							char* maxCountText = playlist->count != playlist->maxCount ? fillString("|%i", playlist->maxCount) : "";
							char* mainButtonText = fillString("%s - (%i%s)", playlist->title, playlist->count, maxCountText);

							if(newGuiQuickButton(gui, layoutInc(&l), mainButtonText, vec2i(-1,0), &ad->scrollButtonSettings)) {
								if(ad->modeData.downloadMode != Download_Mode_Videos) {
									memCpy(&ad->playlist, ad->playlistFolder + i, sizeof(YoutubePlaylist));
									ad->playlist.count = playlist->count;
									ad->startLoadFile = true;

									strCpy(ad->downloadPlaylist.title, ad->playlist.title);
									strCpy(ad->downloadPlaylist.id, ad->playlist.id);

									ad->playlistFolderIndex = i;
								}
							}

							if(newGuiQuickButton(gui, layoutInc(&l), buttonText, vec2i(0,0), &ad->scrollButtonSettings)) {
								removePlaylistFile(playlist);
							}
						}

					ld = newGuiLayoutPop(gui);
					newGuiScissorPop(gui);
					ld->pos.y = rectB((ld+1)->region).y - ld->yPadding;
				}

				newGuiLAdv(gui, yOffsetExtra);

				{
					newGuiQuickTextBox(gui, newGuiLRectAdv(gui), "Quicksearch"); 

					l = layoutQuickRow(&lay, newGuiLRectAdv(gui), 0,0); 

					gui->editSettings.defaultText = "<Playlist_Name>";
					if(newGuiQuickTextEdit(gui, layoutInc(&l), ad->searchStringPlaylists, 50)) {
						if(strLen(ad->searchStringPlaylists) > 0) {
							ad->lastSearchMode = 1;
							strCpy(ad->searchString, ad->searchStringPlaylists);
							downloadModeSet(&ad->modeData, Download_Mode_Search, &ad->appIsBusy);
							ad->searchResultCount = 0;
						}
					}
					gui->editSettings.defaultText = "<Channel_Name>";
					if(newGuiQuickTextEdit(gui, layoutInc(&l), ad->searchStringChannels, 50)) {
						if(strLen(ad->searchStringChannels) > 0) {
							ad->lastSearchMode = 0;
							strCpy(ad->searchString, ad->searchStringChannels);
							downloadModeSet(&ad->modeData, Download_Mode_Search, &ad->appIsBusy);
							ad->searchResultCount = 0;
						}
					}
					gui->editSettings.defaultText = "";



					if(ad->modeData.downloadMode == Download_Mode_ChannelPlaylists) {
						// Show progress.
						Layout* lTemp = layoutQuickRow(&layTemp, newGuiLRectAdv(gui), 0, rowHeight); 

						Rect r = layoutInc(&lTemp);
						scissorTestIntersect(gui->scissor, r);
						// drawRectProgress(r, (float)divZero(ad->downloadProgress, ad->downloadMax), progressLeft, progressRight, true, edgeColor);
						drawRectProgressHollow(r, (float)divZero(ad->modeData.downloadProgress, ad->modeData.downloadMax), progressLeft, edgeColor);
						newGuiQuickTextBox(gui, r, fillString("%i/%i", ad->modeData.downloadProgress, ad->modeData.downloadMax));

						r = scissorTestIntersect(gui->scissor, layoutInc(&lTemp));
						if(newGuiQuickButton(gui, r, "")) downloadModeAbort(&ad->modeData, &ad->appIsBusy);
						drawCross(rectCen(r), rectH(r)/2 *0.5f, 2, vec2(1,0), fontColor);
					}

					if(ad->searchResultCount > 0) {
						// writePos.x += sectionOffset; rowDim.w -= sectionOffset*2;

						static float scrollValue = 0;
						Rect r = rectTDim(ld->pos + vec2(ld->dim.w/2, 0), vec2(ld->dim.w - sectionOffset*2, 10 * (rowHeight+listOffset))); 

						ScrollRegionValues scrollValues = {};
						newGuiQuickScroll(gui, r, ad->searchResultCount * (rowHeight+listOffset), &scrollValue, &scrollValues);

						ld = newGuiLayoutPush(gui, scrollValues.region);
						ld->pos = scrollValues.pos;
						newGuiScissorPush(gui, scrollValues.scissor);
						ld->yPadding = listOffset;

							gui->buttonSettings = ad->scrollButtonSettings;
							for(int i = 0; i < ad->searchResultCount; i++) {
								SearchResult* sResult = ad->searchResults + i;

								if(ad->lastSearchMode == 0) {
									char* buttonText = "Load playlists";
									Layout* l = layoutQuickRow(&lay, newGuiLRectAdv(gui), 0,getTextDim(buttonText, font).w + font->height*textMargin*2);

									if(newGuiQuickButton(gui, layoutInc(&l), fillString("%s - (%i)", sResult->title, sResult->count), vec2i(-1,0))) {

										strCpy(ad->downloadPlaylist.id, sResult->allPlaylistId);
										strCpy(ad->downloadPlaylist.title, sResult->title);
										ad->downloadPlaylist.count = sResult->count;

										downloadModeSet(&ad->modeData, Download_Mode_Videos, &ad->appIsBusy);
										ad->update = false;
									}
									if(newGuiQuickButton(gui, layoutInc(&l), buttonText)) {
										strCpy(ad->channelId, sResult->id);
										downloadModeSet(&ad->modeData, Download_Mode_ChannelPlaylists, &ad->appIsBusy);
										ad->lastSearchMode = 1;
										ad->searchResultCount = 0;
									}
								} else {
									if(newGuiQuickButton(gui, newGuiLRectAdv(gui), fillString("%s - (%i)", sResult->title, sResult->count), vec2i(-1,0))) {

										strCpy(ad->downloadPlaylist.id, sResult->id);
										strCpy(ad->downloadPlaylist.title, sResult->title);
										ad->downloadPlaylist.count = sResult->count;

										downloadModeSet(&ad->modeData, Download_Mode_Videos, &ad->appIsBusy);
										ad->update = false;
									}
								}
							}
							gui->buttonSettings = ad->buttonSettings;

						ld = newGuiLayoutPop(gui);
						newGuiScissorPop(gui);
						ld->pos.y = rectB((ld+1)->region).y - ld->yPadding;
					}
				}


				if(strLen(ad->downloadPlaylist.id) != 0) {

					newGuiLAdv(gui, yOffsetExtra);

					newGuiQuickTextBox(gui, newGuiLRectAdv(gui), fillString("%s - (%i)", ad->downloadPlaylist.title, ad->downloadPlaylist.count)); 

					Rect progressRect;
					bool activeDownload = ad->modeData.downloadMode == Download_Mode_Videos;
					{
						l = layoutQuickRow(&lay, newGuiLRectAdv(gui), 0,0);
						Rect rLeft = layoutInc(&l);
						Rect rRight = layoutInc(&l);

						if(activeDownload && !ad->update) progressRect = rLeft;
						if(activeDownload && ad->update) progressRect = rRight;

						if(!activeDownload) {
							if(newGuiQuickButton(gui, rLeft, "Download")) {
								downloadModeSet(&ad->modeData, Download_Mode_Videos, &ad->appIsBusy);
								ad->update = false;
							}
							if(newGuiQuickButton(gui, rRight, "Update")) {
								downloadModeSet(&ad->modeData, Download_Mode_Videos, &ad->appIsBusy);
								ad->update = true;
							}
						}
					}

					// Show progress.
					if(activeDownload) {
						Layout* lTemp = layoutQuickRow(&layTemp, progressRect, 0, rowHeight);

						Rect r = layoutInc(&lTemp);
						scissorTestIntersect(gui->scissor, r);
						// drawRectProgress(r, (float)divZero(ad->downloadProgress, ad->downloadMax), progressLeft, progressRight, true, edgeColor);
						drawRectProgressHollow(r, (float)divZero(ad->modeData.downloadProgress, ad->modeData.downloadMax), progressLeft, edgeColor);
						newGuiQuickTextBox(gui, r, fillString("%i/%i", ad->modeData.downloadProgress, ad->modeData.downloadMax));

						r = scissorTestIntersect(gui->scissor, layoutInc(&lTemp));
						if(newGuiQuickButton(gui, r, "")) downloadModeAbort(&ad->modeData, &ad->appIsBusy);
						drawCross(rectCen(r), rectH(r)/2 *0.5f, 2, vec2(1,0), fontColor);
					}

				}
			}

			// lastPanelHeight = (insideRect.top - writePos.y) + clientBorder.y*2 + panelBorder*2 + titleHeight - rowOffset;
			// lastPanelHeight = (insideRect.top - ld->pos.y) + clientBorder.y*2 + panelBorder*2 + titleHeight - rowOffset*2 + clientBorder.y;
			lastPanelHeight = (insideRect.top - ld->pos.y) + clientBorder.y*2 + panelBorder*2 + titleHeight - rowOffset*2 + clientBorder.y + rectH(modeRect);

			newGuiScissorPop(gui);
			newGuiLayoutPop(gui);
			scissorState(false);
		}
	}

	// Block everything for debug panels.
	{
		int id = newGuiIncrementId(ad->gui);
		if(ds->showMenu) {
			newGuiSetHotAllMouseOver(ad->gui, id, ds->gui->getPanelBackgroundRect(), 5);
		}
		if(ds->showStats) {
			newGuiSetHotAllMouseOver(ad->gui, id, ds->gui2->getPanelBackgroundRect(), 5);
		}
	}

	if(true)
	{
		drawRect(getScreenRect(ws), vec4(0.2f,1));
		drawRectRoundedOutlined(rectCenDim(400,-400,200,200), vec4(0.4f,1), vec4(1.0f,1), 5, 10);

		// struct TextSettings {
		// 	Font* font;
		// 	Vec4 color;
		// 	int wrapWidth;

		// 	int shadowMode; // 0 = noShadow, 1 = shadow, 2 = outline;
		// 	Vec2 shadowDir;
		// 	float shadowSize;
		// 	Vec4 shadowColor;
		// };

		TextSettings settings = {ad->font, vec4(1,1), 0, 0, vec2(23423,0), 0.2f, vec4(0,0,0,1)};
		drawText("We <c>#FF000000 can now draw <b>Bold Text<b> and <i>Italic Text<i>!", vec2(800,-400), settings);
	}

	if(false)
	{
		char* buffer = getTString(kiloBytes(20));
		readFileToBuffer(buffer, "..\\test.json");

		char* b = buffer;
		JSonValue* value = jsonParseValue(&b);
		

		// for(int i = 

		jsonPrint(value);


		*isRunning = false;
	}

	newGuiEnd(ad->gui);

	{ TIMER_BLOCK_NAMED("DebugBegin"); }

	debugMain(ds, appMemory, ad, reload, isRunning, init, threadQueue);

	{ TIMER_BLOCK_NAMED("DebugEnd"); }

	// Render.
	{
		TIMER_BLOCK_NAMED("Render");

		Vec2i frameBufferRes = getFrameBuffer(FRAMEBUFFER_2dNoMsaa)->colorSlot[0]->dim;
		Vec2i res = ws->currentRes;
		Rect frameBufferUV = rect(0,(float)res.h/frameBufferRes.h,(float)res.w/frameBufferRes.w,0);

		{
			TIMER_BLOCK_NAMED("BlitBuffers");

			blitFrameBuffers(FRAMEBUFFER_2dMsaa, FRAMEBUFFER_2dNoMsaa, res, GL_COLOR_BUFFER_BIT, GL_LINEAR);

			if(!ad->screenShotMode) {
				blitFrameBuffers(FRAMEBUFFER_DebugMsaa, FRAMEBUFFER_DebugNoMsaa, res, GL_COLOR_BUFFER_BIT, GL_LINEAR);

				glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				glBlendEquation(GL_FUNC_ADD);
				bindFrameBuffer(FRAMEBUFFER_2dNoMsaa);

				glLoadIdentity();
				glViewport(0,0, res.w, res.h);
				glOrtho(0,1,1,0, -1, 1);

				drawRect(rect(0, 1, 1, 0), vec4(1,1,1,ds->guiAlpha), frameBufferUV, getFrameBuffer(FRAMEBUFFER_DebugNoMsaa)->colorSlot[0]->id);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glBlendEquation(GL_FUNC_ADD);
			}
		}


		#if USE_SRGB 
			glEnable(GL_FRAMEBUFFER_SRGB);
		#endif 

		if(ad->screenShotMode) {
			ad->screenShotMode = false;
			ds->showMenu = true;
			ad->updateFrameBuffers = true;

			bindFrameBuffer(FRAMEBUFFER_ScreenShot);
			// glClearColor(0,1,0,1);
			// glClear(GL_COLOR_BUFFER_BIT);
			glLoadIdentity();
			glOrtho(0, res.w, -res.h, 0, -10,10);

			glDisable(GL_BLEND);
			drawRect(rect(0, -ws->currentRes.h, ws->currentRes.w, 0), rect(0,0,1,1), getFrameBuffer(FRAMEBUFFER_2dNoMsaa)->colorSlot[0]->id);
			glEnable(GL_BLEND);

			int w = ws->currentRes.w;
			int h = ws->currentRes.h;

			char* buffer = (char*)malloc(w*h*4);
			int texId = getFrameBuffer(FRAMEBUFFER_ScreenShot)->colorSlot[0]->id;
			glGetTextureSubImage(texId, 0, 0, 0, 0, w, h, 1, GL_RGBA, GL_UNSIGNED_BYTE, w*h*4, buffer);

			int result = stbi_write_png(ad->screenShotFilePath, w, h, 4, buffer, w*4);

			free(buffer);

			setDimForFrameBufferAttachmentsAndUpdate(FRAMEBUFFER_2dMsaa, ad->frameBufferSize.w, ad->frameBufferSize.h);
			setDimForFrameBufferAttachmentsAndUpdate(FRAMEBUFFER_2dNoMsaa, ad->frameBufferSize.w, ad->frameBufferSize.h);
		} else {
			TIMER_BLOCK_NAMED("RenderMainBuffer");

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glLoadIdentity();
			glViewport(0,0, res.w, res.h);
			glOrtho(0,1,1,0, -1, 1);
			drawRect(rect(0, 1, 1, 0), frameBufferUV, getFrameBuffer(FRAMEBUFFER_2dNoMsaa)->colorSlot[0]->id);
		}

		#if USE_SRGB
			glDisable(GL_FRAMEBUFFER_SRGB);
		#endif
	}

	// Swap window background buffer.
	{
		TIMER_BLOCK_NAMED("Swap");

		// Sleep until monitor refresh.
		if(!init) {
			double frameTime = timerUpdate(ad->swapTime);
			double sleepTime = ((double)1/60) - frameTime;
			if(sleepTime < 0) sleepTime = ((double)1/30) - frameTime;

			int sleepTimeMS = sleepTime*1000;
			if(sleepTimeMS > 1) Sleep(sleepTimeMS);
		}

		swapBuffers(&ad->systemData);
		ad->swapTime = timerInit();

		if(init) {
			showWindow(windowHandle);
			GLenum glError = glGetError(); printf("GLError: %i\n", glError);
		}
	}

	// @App End.

	// Save app state.
	if(*isRunning == false && fileExists(App_Save_File)) {
		AppTempSettings at = {};

		RECT r; 
		GetWindowRect(windowHandle, &r);

		at.windowRect = rect(r.left, r.bottom, r.right, r.top);
		at.sidePanelWidth = ad->sidePanelWidth;
		memCpy(at.graphOffsets, ad->graphOffsets, memberSize(AppTempSettings, graphOffsets));
		at.playlistFolderIndex = ad->playlistFolderIndex;

		at.panelRect = ad->panelRect;

		appWriteTempSettings(App_Save_File, &at);
	}

	// if(init) printf("Startup Time: %fs\n", timerUpdate(startupTimer));
}



void debugMain(DebugState* ds, AppMemory* appMemory, AppData* ad, bool reload, bool* isRunning, bool init, ThreadQueue* threadQueue) {
	// @DebugStart.

	#define PVEC3(v) v.x, v.y, v.z
	#define PVEC2(v) v.x, v.y

	globalMemory->debugMode = true;
	clearTMemoryDebug();
	i64 timeStamp = timerInit();



	Input* input = ds->input;
	WindowSettings* ws = &ad->wSettings;

	ExtendibleMemoryArray* debugMemory = &appMemory->extendibleMemoryArrays[1];
	ExtendibleMemoryArray* pMemory = globalMemory->pMemory;

	bindFrameBuffer(FRAMEBUFFER_DebugMsaa);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
	glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);

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
		// gui->start(ds->gInput, getFont(FONT_CALIBRI, fontHeight), ws->currentRes);
		gui->start(ds->gInput, ad->font, ws->currentRes);

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
				// 	drawRect(line, highlightColor);
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
				glLineWidth(barWidth);

				gui->empty();
				Rect r = gui->getCurrentRegion();
				float rheight = gui->getDefaultHeight();
				float fontBaseOffset = 4;

				drawLinesHeader(vec4(1,1));
				float xOffset = 0;
				for(int statIndex = 0; statIndex < barCount; statIndex++) {
					Statistic* stat = statistics + i;
					u64 coh = ds->timings[statIndex][i].cyclesOverHits;

					if(coh == 0) continue;
					float height = mapRangeClamp(coh, stat->min, stat->max, 1, rheight);

					Vec2 rmin = r.min + vec2(xOffset, fontBaseOffset);
					float colorOffset = mapRange(coh, stat->min, stat->max, 0, 1);

					pushColor(vec4(colorOffset,1-colorOffset,0,1));
					pushVecs(rmin, rmin+vec2(0,height));

					xOffset += barWidth;
				}
				glEnd();
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

			float graphWidth = rectDim(bgRect).w;

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
				drawRect(cyclesRect, gui->colors.sectionColor);
				Vec2 cyclesDim = rectDim(cyclesRect);

				drawRect(headerRect, vec4(1,1,1,0.1f));
				Vec2 headerDim = rectDim(headerRect);

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

					drawRect(rect(viewAreaLeft, cyclesRect.min.y, viewAreaRight, cyclesRect.max.y), vec4(1,1,1,0.03f));
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

				glLineWidth(3);
				double startPos = roundModDouble(orthoLeft, timelineSection) - timelineSection;
				double pos = startPos;
				while(pos < orthoRight + timelineSection) {
					double p = mapRangeDouble(pos, orthoLeft, orthoRight, bgRect.min.x, bgRect.max.x);

					// Big line.
					{
						float h = headerDim.h*heightMod;
						drawLine(vec2(p,headerRect.min.y), vec2(p,headerRect.min.y + h), vec4(g,g,g,1));
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

						drawText(s, gui->font, textPos, gui->colors.textColor, vec2i(0,0), 0, 1, gui->colors.shadowColor);
					}

					pos += timelineSection;
				}
				glLineWidth(1);

				pos = startPos;
				timelineSection /= div;
				heightMod *= 0.6f;
				int index = 0;
				while(pos < orthoRight + timelineSection) {

					// Small line.
					if((index%(int)div) != 0) {
						double p = mapRangeDouble(pos, orthoLeft, orthoRight, bgRect.min.x, bgRect.max.x);
						float h = headerDim.h*heightMod;
						drawLine(vec2(p,headerRect.min.y), vec2(p,headerRect.min.y + h), vec4(g,g,g,1));
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

						drawText(s, gui->font, textPos, gui->colors.textColor, vec2i(0,0), 0, gui->settings.textShadow, gui->colors.shadowColor);
					}

					pos += timelineSection;
					index++;

				}
			}

			glLineWidth(1);

			bool mouseHighlight = false;
			Rect hRect;
			Vec4 hc;
			char* hText;
			GraphSlot* hSlot;

			Vec2 startPos = rectTL(bgRect);
			startPos -= vec2(0, lineHeight);

			int firstBufferIndex = oldIndex;
			int bufferCount = ds->savedBufferCount;
			for(int threadIndex = 0; threadIndex < threadQueue->threadCount; threadIndex++) {

				// Horizontal lines to distinguish thread bars.
				if(threadIndex > 0) {
					Vec2 p = startPos + vec2(0,lineHeight);
					float g = 0.8f;
					drawLine(p, vec2(bgRect.max.x, p.y), vec4(g,g,g,1));
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
						drawLine(vec2(barRight, bgRect.min.y), vec2(barRight, bgRect.max.y), vec4(g,g,g,1));
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

							// gui->drawTextBox(textRect, fillString("%s %s (%i.c)", tInfo->function, tInfo->name, slot->size), c, 0, rectDim(textRect).w);

							gui->drawRect(textRect, c, false);
							gui->settings.textShadow = 2;
							gui->drawText(fillString("%s %s (%i.c)", tInfo->function, tInfo->name, slot->size), 0, textRect, rectDim(textRect).w);
							gui->settings.textShadow = 0;
						}
					}

				}

				if(threadIndex == 0) startPos.y -= lineHeight*3;
				else startPos.y -= lineHeight*2;

			}

			if(mouseHighlight) {
				if(hRect.min.x < bgRect.min.x) hRect.min.x = bgRect.min.x;

				float tw = getTextDim(hText, gui->font).w + 2;
				if(tw > rectDim(hRect).w) hRect.max.x = hRect.min.x + tw;

				float g = 0.8f;
				gui->drawRect(hRect, vec4(g,g,g,1));

				Rect textRect = rect(hRect.min+vec2(1,1), hRect.max-vec2(1,1));
				// gui->drawTextBox(textRect, hText, hc);

				gui->drawRect(textRect, hc, false);
				gui->settings.textShadow = 2;
				gui->drawText(hText, 0, textRect, rectDim(textRect).w);
				gui->settings.textShadow = 0;
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
			glLineWidth(1);

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

					drawLine(vec2(rectNumbers.min.x, y), vec2(rectNumbers.min.x + length, y), vec4(1,1,1,1)); 
					drawText(fillString("%i64.c",(i64)p), gui->font, vec2(rectNumbers.min.x + length + 4, y), vec4(1,1,1,1), vec2i(-1,0));
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
				drawLine(vec2(rectLines.min.x - textWidth - 2, yAvg), vec2(rectLines.max.x, yAvg), color);
				gui->scissorPop();

				gui->scissorPush(rectLines);

				if(timerIndex == ds->lineGraphHighlight) glLineWidth(3);
				else glLineWidth(1);

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
					float xOff = rectDim(rectLines).w/(cycleCount-1);
					Vec2 np = vec2(rectLines.min.x + xOff*i, y);

					if(lastElementEmpty) np.y = yAvg;

					drawLine(p, np, color);
					p = np;
				}

				glLineWidth(1);

				gui->scissorPop();
			}

			gui->empty();
			Rect r = gui->getCurrentRegion();
			Vec2 rc = rectCen(r);
			float rw = rectDim(r).w;

			// Draw color rectangles.
			float width = (rw/timerCount)*0.75f;
			float height = fontHeight*0.8f;
			float sw = (rw-(timerCount*width))/(timerCount+1);

			for(int i = 0; i < timerCount; i++) {
				TimerInfo* info = &timer->timerInfos[i];

				Vec4 color = vec4(info->color[0], info->color[1], info->color[2], 1);
				Vec2 pos = vec2(r.min.x + sw+width/2 + i*(width+sw), rc.y);
				drawRect(rectCenDim(pos, vec2(width, height)), color);
			}

		}

		gui->end();

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
				strCpy(ds->notificationStack[i], ds->notificationStack[i+deletionCount]);
				ds->notificationTimes[i] = ds->notificationTimes[i+deletionCount];
			}
			ds->notificationCount -= deletionCount;
		}

		// Draw notes.
		Font* font = ad->font;
		float fontSize = font->height;
		Vec4 color = vec4(1,0.5f,0,1);

		float y = -fontSize/2;
		for(int i = 0; i < ds->notificationCount; i++) {
			char* note = ds->notificationStack[i];
			drawText(note, font, vec2(ws->currentRes.w/2, y), color, vec2i(0,0), 0, 2);
			y -= fontSize;
		}
	}

	if(ds->showHud) {
		int pi = 0;
		// Vec4 c = vec4(1.0f,0.2f,0.0f,1);
		Vec4 c = vec4(1.0f,0.4f,0.0f,1);
		Vec4 c2 = vec4(0,0,0,1);
		Font* font = ad->font;
		int fontSize = font->height;
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

		// drawText(fillString("Fps  : %i", fps), font, tp, c, ali, 0, sh, c2); tp.y -= fontSize;
		// drawText(fillString("BufferIndex: %i",    ds->timer->bufferIndex), font, tp, c, ali, 0, sh, c2); tp.y -= fontSize;
		// drawText(fillString("LastBufferIndex: %i",ds->lastBufferIndex), font, tp, c, ali, 0, sh, c2); tp.y -= fontSize;

		for(int i = 0; i < ds->infoStackCount; i++) {
			drawText(fillString("%s", ds->infoStack[i]), font, tp, c, ali, 0, sh, c2); tp.y -= fontSize;
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

	globalMemory->debugMode = false;
}

#pragma optimize( "", on ) 
