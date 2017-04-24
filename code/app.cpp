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
	- Gui textbox returns true only on new value when hitting enter.
	- Fixed bug in debugNotifications.
	- Massive changes in rect functions.


    - Clamp window to monitor.
	- Scale font size to windows. (For high resolution monitors.)
	- Filter -> percentage of views.
	- Make it so downloading spawns a thread that shows progress and a cancel button.
	  Shuld be more reasonable than what we have now.
	- Make divs a better system in generell. Add things like min max size in addition to float elements.
	  - You could also make that system stacking.
	- Use stb_truetype advance font system for smaller fonts.
	- Cursor image still flickering somtimes. 
	  Has something to do with the window changing screens, or a specific location on the screen?
	- Standard deviation graph.
	- Avg. line of number videos of released in a month or so.

	- Memory leak at startup that goes away when you do something.
	- Screenshot function not working.

	- Done Today:
	  Deleted unused rendering code. Fixed Timer Block bug. Reduced startup time. Sleep before swapBuffers. 
	  Fixed framebuffer size -> Window resize smoother. Release build mode.

	Bug:
	- Memory leak at text snippet panel above jpg. 
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

char* stringGetBetween(char* buffer, char* leftToken, char* rightToken, char** bufferIncrement = 0) {
	char* string;

	int leftPos = strFindRight(buffer, leftToken);
	if(leftPos != -1) {
		int rightPos = strFind(buffer + leftPos, rightToken);
		int size = rightPos;
		string = getTString(size+1); 
		strClear(string);
		strCpy(string, buffer + leftPos, rightPos);
		if(bufferIncrement) {
			(*bufferIncrement) += leftPos + rightPos + 1; 
		}
	} else {
		string = getTString(1); strClear(string);
	}

	return string;
}

char* getJSONString(char* buffer, char* name, char** bufferIncrement = 0) {
	return stringGetBetween(buffer, fillString("\"%s\": \"", name), "\"", bufferIncrement);
}

char* getJSONInt(char* buffer, char* name, char** bufferIncrement = 0) {
	return stringGetBetween(buffer, fillString("\"%s\": ", name), ",", bufferIncrement);
}

char* getJSONIntNewline(char* buffer, char* name, char** bufferIncrement = 0) {
	return stringGetBetween(buffer, fillString("\"%s\": ", name), "\n", bufferIncrement);
}


char* getString(char** string, int size) {
	char* stringBase = *string;
	*string = (*string) + size;
	return stringBase;
}

bool isLeapYear(int year, bool addFrontDigits = true) {
	if(addFrontDigits) year += 2000;

	if(year % 4 != 0) return false;
	else if(year % 100 != 0) return true;
	else if(year % 400 != 0) return false;
	else return true;
}

const int monthDayCount[] = {31,28,31,30,31,30,31,31,30,31,30,31};

int getYearDayCount(int year) {
	return isLeapYear(year)?366:365;
}

int getMonthDayCount(int month, int year) {
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
	result += (i64)(day-1)*86400;
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
		int daysOfMonth = monthDayCount[date->m-1];
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



// Date String Format: 2017-02-01T12:03:23.000Z

// #pragma pack(push,1)
struct YoutubeVideo {
	char id[12];
	char title[151];
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
	char title[151];
	int count;
};

struct SearchResult {
	char title[151];
	char id[40];
};

char* curlPath = "C:\\Standalone\\curl\\curl.exe";
char* apiKey = "AIzaSyD-qRen5fSH7M3ePBey1RY0vRTyW0PKyLw";
char* youtubeApiPlaylistItems = "https://www.googleapis.com/youtube/v3/playlistItems";
char* youtubeApiPlaylist = "https://www.googleapis.com/youtube/v3/playlists";
char* youtubeApiVideos = "https://www.googleapis.com/youtube/v3/videos";
char* youtubeApiCommentThread = "https://www.googleapis.com/youtube/v3/commentThreads";
char* youtubeApiChannel = "https://www.googleapis.com/youtube/v3/channels";
char* youtubeApiSearch = "https://www.googleapis.com/youtube/v3/search";

int maxDownloadCount = 40;
// int maxDownloadCount = 20;
#define page_token_size 10

char* rocketBeansId = "UCQvTDmHza8erxZqDkjQ4bQQ";



int requestPlaylistItems(CURL* curlHandle, char* buffer, char* playlistId, int maxResults, char* pageToken = 0) {
	char* request = fillString("%s?key=%s&maxResults=%i&playlistId=%s&part=contentDetails%s", 
								youtubeApiPlaylistItems, apiKey, maxResults, playlistId, pageToken?fillString("&pageToken=%s",pageToken):"");
	return curlRequest(curlHandle, request, buffer);
}

int requestCommentThread(CURL* curlHandle, char* buffer, char* videoId, int maxResults) {
	char* request = fillString("%s?key=%s&maxResults=%i&videoId=%s&part=snippet&order=relevance&=textFormat=plainText", 
								youtubeApiCommentThread, apiKey, maxResults, videoId);
	return curlRequest(curlHandle, request, buffer);
}

int requestPlaylist(CURL* curlHandle, char* buffer, char* idType, char* id, char* part, int maxResults, char* pageToken = 0) {
	char* request = fillString("%s?key=%s&maxResults=%i&%s=%s&part=%s%s", 
								youtubeApiPlaylist, apiKey, maxResults, idType, id, part, pageToken?fillString("&pageToken=%s",pageToken):"");
	return curlRequest(curlHandle, request, buffer);
}

int requestChannel(CURL* curlHandle, char* buffer, char* idType, char* id, char* part, int maxResults) {
	char* request = fillString("%s?key=%s&maxResults=%i&%s=%s&part=%s", 
								youtubeApiChannel, apiKey, maxResults, idType, id, part);
	return curlRequest(curlHandle, request, buffer);
}

int requestVideos(CURL* curlHandle, char* buffer, char* id, char* part) {
	char* request = fillString("%s?key=%s&id=%s&part=%s", 
								youtubeApiVideos, apiKey, id, part);
	return curlRequest(curlHandle, request, buffer);
}

int requestSearch(CURL* curlHandle, char* buffer, char* type, char* searchString, int maxResults) {
	char* request = fillString("%s?key=%s&maxResults=%i&part=snippet&type=%s&q=%s", 
								youtubeApiSearch, apiKey, maxResults, type, searchString);
	return curlRequest(curlHandle, request, buffer);
}

struct VideoSnippet {
	bool selectedLoaded;
	char* selectedTopComments[10];
	int selectedCommentLikeCount[10];
	int selectedCommentReplyCount[10];
	int selectedCommentCount;
	Texture thumbnailTexture;
};

void copyOverTextAndFixTokens(char* buffer, char* text) {
	int index = 0;
	for(;;) {
		char c = text[0];
		if(c == '\"') break;

		if(c == '\\') {
			if(text[1] == 'n') buffer[index++] = '\n';
			else if(text[1] == '\\') buffer[index++] = '\\';
			else if(text[1] == '\"') buffer[index++] = '\"';
			else if(text[1] == '\'') buffer[index++] = '\'';

			text += 2;
		} else {
			buffer[index++] = text[0];
			text++;
		}
	}
	buffer[index] = '\0';
}

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
		requestCommentThread(curlHandle, buffer, vid->id, commentCount);

		int totalResults = strToInt(getJSONInt(buffer, "totalResults"));

		char* commentBuffer = getTString(kiloBytes(1)); strClear(commentBuffer);

		char* content;
		int advance;
		for(int i = 0; i < totalResults; i++) {
			int commentStart = strFindRight(buffer, "\"textOriginal\": \"");
			if(commentStart == -1) break;

			buffer += commentStart;
			copyOverTextAndFixTokens(commentBuffer, buffer);

			char* s = getPString(strLen(commentBuffer)+1);
			strCpy(s, commentBuffer);

			buffer += strLen(commentBuffer);

			snippet->selectedTopComments[snippet->selectedCommentCount] = s;

			content = getJSONInt(buffer, "likeCount", &buffer);
			snippet->selectedCommentLikeCount[snippet->selectedCommentCount] = strToInt(content);

			content = getJSONInt(buffer, "totalReplyCount", &buffer);
			snippet->selectedCommentReplyCount[snippet->selectedCommentCount] = strToInt(content);

			snippet->selectedCommentCount++;
		}
	}
}

int downloadChannelPlaylistCount(CURL* curlHandle, char* channelId) {
	char* buffer = (char*)getTMemory(kiloBytes(2));
	requestPlaylist(curlHandle, buffer, "channelId", channelId, "id", 1);
	int count = strToInt(getJSONInt(buffer, "totalResults"));

	requestChannel(curlHandle, buffer, "id", channelId, "contentDetails", 1);

	bool hasAllUploadsPlaylist = strLen(getJSONString(buffer, "uploads")) != 0 ? true : false;
	if(hasAllUploadsPlaylist) count++;

	return count;
}

void downloadChannelPlaylists(CURL* curlHandle, YoutubePlaylist* playlists, int* playlistCount, char* channelId, int count) {
	char* buffer = (char*)getTMemory(megaBytes(3));
	char* idCollection = getTString(kiloBytes(20));
	char* tempBuffer = getTString(kiloBytes(3));

	bool hasAllUploadsPLaylist = false;

	// Get all uploads playlist.
	{
		requestPlaylist(curlHandle, buffer, "channelId", channelId, "snippet", 1, 0);

		// Get Channel name.
		requestChannel(curlHandle, tempBuffer, "id", channelId, "snippet", 1);
		char* s = getJSONString(tempBuffer, "title");
		if(strLen(s) != 0) strCpy(playlists[0].title, s);

		requestChannel(curlHandle, tempBuffer, "id", channelId, "contentDetails", 1);
		s = getJSONString(tempBuffer, "uploads");
		if(strLen(s) != 0) {
			strCpy(playlists[0].id, s);
			hasAllUploadsPLaylist = true;
		}

		// Get playlist video count.
		if(hasAllUploadsPLaylist) {
			requestPlaylist(curlHandle, buffer, "id", playlists[0].id, "contentDetails", 1);
			char* s = getJSONIntNewline(buffer, "itemCount", &buffer);
			if(strLen(s) != 0) playlists[0].count = strToInt(s);
		}
	}

	int startIndex = hasAllUploadsPLaylist?1:0;

	char* pageToken = 0;
	for(int i = startIndex; i < count; i += maxDownloadCount) {
		int dCount = i + maxDownloadCount > count ? count-i : maxDownloadCount;

		requestPlaylist(curlHandle, buffer, "channelId", channelId, "snippet", dCount, pageToken);
		int resultCount = strToInt(getJSONInt(buffer, "totalResults"));
		pageToken = getJSONString(buffer, "nextPageToken");

		if(i == 0) count = min(resultCount, count);

		int index = i;
		int advance = 0;
		for(;;) {
			if(index > count) break;

			char* s = getJSONString(buffer, "id", &buffer);
			if(strLen(s) == 0) break;

			strCpy(playlists[index].id, s);

			s = getJSONString(buffer, "title", &buffer);
			strCpy(playlists[index].title, s);
			index++;
		}

		// Get playlist video count.
		{
			strClear(idCollection);
			int maxCount = dCount;
			for(int index = 0; index < maxCount; index++) {
				strAppend(idCollection, fillString("%s,", playlists[i+index].id));
			}

			requestPlaylist(curlHandle, buffer, "id", idCollection, "contentDetails", maxCount);

			int index = i;
			int advance = 0;
			for(;;) {
				char* s = getJSONIntNewline(buffer, "itemCount", &buffer);
				if(strLen(s) == 0) break;

				playlists[index].count = strToInt(s);
				index++;
			}
		}
	}

	*playlistCount = count;
}

char* downloadChannelIdFromUserName(CURL* curlHandle, char* userName) {
	char* buffer = (char*)getTMemory(kiloBytes(1));
	requestChannel(curlHandle, buffer, "forUsername", userName, "id", 1);
	char* channelId = getJSONString(buffer, "id");

	return channelId;
}

char* eatWhiteSpace(char* str) {
	int index = 0;
	while(str[index] == ' ') index++;
	return str + index;
}

void quickSearch(CURL* curlHandle, SearchResult* searchResults, int* channelCount, char* searchString, bool searchForChannels = true) {
	char* buffer = (char*)getTMemory(kiloBytes(50));
	char* search = getTString(strLen(searchString)+1); strClear(search);
	for(;;) {
		searchString = eatWhiteSpace(searchString);
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

	requestSearch(curlHandle, buffer, searchForChannels?"channel":"playlist", search, 10);

	int resultCount = strToInt(getJSONInt(buffer, "totalResults"));
	int count = min(resultCount, 10);

	int advance = 0;
	for(int i = 0; i < count; i++) {
		if(searchForChannels) {
			char* s = getJSONString(buffer, "channelId", &buffer);
			strCpy(searchResults[i].id, s);
		} else {
			char* s = getJSONString(buffer, "playlistId", &buffer);
			strCpy(searchResults[i].id, s);
		}

		char* s = getJSONString(buffer, "title", &buffer);
		strCpy(searchResults[i].title, s);
	}

	*channelCount = count;
}

char* playlistFolder = "..\\playlists\\";

void savePlaylistToFile(YoutubePlaylist* playlist, YoutubeVideo* videos, int videoCount, int maxVideoCount, int oldVideoCount, char* pageToken) {
	char* filePath = fillString("%s%s.playlist", playlistFolder, playlist->id);

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
	fwrite(pageToken, page_token_size, 1, file);

	fseek(file, (maxVideoCount-videoCount-oldVideoCount)*sizeof(YoutubeVideo), SEEK_CUR);
	fwrite(videos, videoCount*sizeof(YoutubeVideo), 1, file);

	fclose(file);
}

bool loadPlaylistFromFile(YoutubePlaylist* playlist, YoutubeVideo* videos, int* videoCount, int* maxVideoCount = 0, char* pageToken = 0) {
	char* filePath = fillString("%s%s.playlist", playlistFolder, playlist->id);
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
		if(pageToken == 0) fseek(file, page_token_size, SEEK_CUR);
		else fread(&pageToken, page_token_size, 1, file);
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
	char* filePath = fillString("%s%s", playlistFolder, fileName);
	FILE* file = fopen(filePath, "rb");
	if(file) {
		fread(&playlist->title, memberSize(YoutubePlaylist, title), 1, file);
		fread(&playlist->id, memberSize(YoutubePlaylist, id), 1, file);
		fread(&playlist->count, sizeof(int), 1, file);

		if(maxVideoCount == 0) fseek(file, sizeof(int), SEEK_CUR);
		else fread(maxVideoCount, sizeof(int), 1, file);
		if(pageToken == 0) fseek(file, page_token_size, SEEK_CUR);
		else fread(pageToken, page_token_size, 1, file);

		fclose(file);

		return true;
	}

	return false;
}

void loadPlaylistFolder(YoutubePlaylist* playlists, int* playlistCount) {
	char* folderPath = fillString("%s*",playlistFolder);
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
	char* filePath = fillString("%s%s.playlist", playlistFolder, playlist->id);
	remove(filePath);
}

inline double divZero(double a, double b) {
	if(a == 0 || b == 0) return 0;
	else return a/b;
}

inline double videoGetLikesDiff(YoutubeVideo* vid) {
	return divZero(vid->dislikeCount, (vid->likeCount + vid->dislikeCount));
}

#define Line_Graph_Count 4

struct LineGraph {
	double* points[Line_Graph_Count];
	int count;
};

void calculateAverages(YoutubeVideo* videos, int videoCount, LineGraph* lineGraph, float timeSpanInMonths, float widthInMonths, double minMaxWidth) {
	TIMER_BLOCK();

	double avgStartX = videos[0].date.n;
	double avgWidth = monthsToInt(widthInMonths);
	double avgTimeSpan = monthsToInt(timeSpanInMonths);

	int statCount = (minMaxWidth/avgWidth) + 2;
	lineGraph->count = statCount;
	for(int i = 0; i < Line_Graph_Count; i++) {
		if(lineGraph->points[i] != 0) free(lineGraph->points[i]);
		lineGraph->points[i] = (double*)malloc(statCount*sizeof(double));
	}

	Statistic* stats[4];
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
				updateStatistic(&stats[2][si], divZero(vid->dislikeCount, (vid->likeCount + vid->dislikeCount)));
				updateStatistic(&stats[3][si], vid->commentCount);
			}
		}
	}

	double lastValidData = 0;
	for(int i = 0; i < statCount; i++) {
		for(int gi = 0; gi < Line_Graph_Count; gi++) {
			endStatistic(stats[gi] + i);
			lineGraph->points[gi][i] = stats[gi][i].count > 0 ? stats[gi][i].avg : 0;
		}
	}
}

// void downloadVideos(CURL* curlHandle, char* buffer, char* tempBuffer, YoutubeVideo* vids, char* playlistId, int* totalCount, int count, char** pageToken = 0) {

// 	pushTMemoryStack();
// 	for(int i = 0; i < count; i += maxDownloadCount) {

// 		int dCount = maxDownloadCount;
// 		if(i + dCount > count) {
// 			dCount = count - i;
// 		}

// 		{
// 			TIMER_BLOCK_NAMED("Request");
// 			requestPlaylistItems(curlHandle, buffer, playlistId, dCount, *pageToken);
// 		}

// 		*pageToken = getJSONString(buffer, "nextPageToken");

// 		*totalCount = strToInt(getJSONInt(buffer, "totalResults"));

// 		int receivedCount;

// 		// Get Video ids.
// 		{
// 			TIMER_BLOCK_NAMED("Video ids");

// 			int index = i;
// 			int advance = 0;
// 			int idCount = 0;
// 			for(;;) {
// 				int reverseIndex = count-1 - index;

// 				char* s = getJSONString(buffer, "videoId", &buffer);
// 				if(strLen(s) == 0) break;

// 				strCpy(vids[reverseIndex].id, s);

// 				index++;
// 				idCount++;
// 			}

// 			receivedCount = idCount;
// 		}
		
// 		// Get Statistics.
// 		{

// 			strClear(tempBuffer);
// 			for(int videoIndex = i; videoIndex < i+dCount; videoIndex++) {
// 				int reverseIndex = count-1 - videoIndex;
// 				strAppend(tempBuffer, fillString("%s,", vids[reverseIndex].id));
// 			}
// 			// Get rid of last comma.
// 			// if(dCount > 0) tempBuffer[strLen(tempBuffer)-1] = '\0';

// 			{
// 				TIMER_BLOCK_NAMED("Statistics Request");
// 				requestVideos(curlHandle, buffer, tempBuffer, "statistics");
// 			}

// 			TIMER_BLOCK_NAMED("Statistics");

// 			int index = i;
// 			int advance = 0;
// 			for(;;) {
// 				int reverseIndex = count-1 - index;
// 				char* s;

// 				int pos = strFind(buffer, "\"statistics\":");
// 				if(pos == -1) break;

// 				buffer += pos;
// 				int endBracketPos = strFind(buffer, '}');
// 				if(endBracketPos == 0) break;

// 				vids[reverseIndex].viewCount = 0;
// 				vids[reverseIndex].likeCount = 0;
// 				vids[reverseIndex].dislikeCount = 0;
// 				vids[reverseIndex].favoriteCount = 0;
// 				vids[reverseIndex].commentCount = 0;

// 				pos = strFind(buffer, "viewCount");
// 				if(pos != -1 && pos < endBracketPos) vids[reverseIndex].viewCount = strToInt(getJSONString(buffer, "viewCount"));
// 				pos = strFind(buffer, "likeCount");
// 				if(pos != -1 && pos < endBracketPos) vids[reverseIndex].likeCount = strToInt(getJSONString(buffer, "likeCount"));
// 				pos = strFind(buffer, "dislikeCount");
// 				if(pos != -1 && pos < endBracketPos) vids[reverseIndex].dislikeCount = strToInt(getJSONString(buffer, "dislikeCount"));
// 				pos = strFind(buffer, "favoriteCount");
// 				if(pos != -1 && pos < endBracketPos) vids[reverseIndex].favoriteCount = strToInt(getJSONString(buffer, "favoriteCount"));
// 				pos = strFind(buffer, "commentCount");
// 				if(pos != -1 && pos < endBracketPos) vids[reverseIndex].commentCount = strToInt(getJSONString(buffer, "commentCount"));

// 				YoutubeVideo* vid = vids + reverseIndex;
// 				if(vid->viewCount < 0 || vid->likeCount < 0 || vid->dislikeCount < 0 || vid->favoriteCount < 0) {
// 					myAssert(false);
// 				}

// 				buffer += endBracketPos;
// 				index++;
// 			}
// 		}

// 		// Get title and thumbnail.
// 		{
// 			strClear(tempBuffer);
// 			for(int videoIndex = i; videoIndex < i+dCount; videoIndex++) {
// 				int reverseIndex = count-1 - videoIndex;
// 				strAppend(tempBuffer, fillString("%s,", vids[reverseIndex].id));
// 			}

// 			{
// 				TIMER_BLOCK_NAMED("Title Request");
// 				requestVideos(curlHandle, buffer, tempBuffer, "snippet");
// 			}

// 			TIMER_BLOCK_NAMED("Title");

// 			int index = i;
// 			int advance = 0;
// 			for(;;) {
// 				int reverseIndex = count-1 - index;

// 				int start = strFind(buffer, "publishedAt");
// 				if(start == -1) break;

// 				char* s = getJSONString(buffer, "publishedAt", &buffer);
// 				strCpy(vids[reverseIndex].dateString, s);
// 				vids[reverseIndex].date = stringToDate(vids[reverseIndex].dateString);

// 				int titleStart = strFindRight(buffer, "\"title\": \"");
// 				buffer += titleStart;
// 				copyOverTextAndFixTokens(tempBuffer, buffer);
// 				buffer += strLen(tempBuffer);
// 				if(strLen(tempBuffer) > memberSize(YoutubeVideo, title)) {
// 					int maxSize = memberSize(YoutubeVideo, title);
// 					int i = 0;
// 					while(tempBuffer[maxSize-i-1] > 127) i--;
// 					tempBuffer[maxSize-i-1] = '\0';
// 				}

// 				strCpy(vids[reverseIndex].title, tempBuffer);

// 				// Thumbnail.
// 				// default, medium, high, standard, maxres.
// 				int pos = strFind(buffer, "\"medium\":"); buffer += pos;
// 				s = getJSONString(buffer, "url", &buffer);
// 				strCpy(vids[reverseIndex].thumbnail, s);

// 				// Skip the second "title" from localization data.
// 				buffer += strFindRight(buffer, "title");

// 				index++;
// 			}

// 		}
		
// 		clearTMemoryToStackIndex();
// 	}

// 	popTMemoryStack();
// }




char* fillString(char** stringBuffer, char* text, ...) {
	va_list vl;
	va_start(vl, text);

	int length = strLen(text);
	char* buffer = getString(stringBuffer, length+1);

	char valueBuffer[20] = {};

	int ti = 0;
	int bi = 0;
	while(true) {
		char t = text[ti];

		if(text[ti] == '%' && text[ti+1] == '.') {
			float v = va_arg(vl, double);
			floatToStr(valueBuffer, v, charToInt(text[ti+2]));
			int sLen = strLen(valueBuffer);
			memCpy(buffer + bi, valueBuffer, sLen);

			ti += 4;
			bi += sLen;
			getString(stringBuffer, sLen);
		} else if(text[ti] == '%' && text[ti+1] == 'f') {
			float v = va_arg(vl, double);
			floatToStr(valueBuffer, v, 2);
			int sLen = strLen(valueBuffer);
			memCpy(buffer + bi, valueBuffer, sLen);

			ti += 2;
			bi += sLen;
			getString(stringBuffer, sLen);
		} else if(text[ti] == '%' && text[ti+1] == 'i') {
			if(text[ti+2] == '6') {
				// 64 bit signed integer.

				myAssert(text[ti+3] == '4');

				i64 v = va_arg(vl, i64);
				intToStr(valueBuffer, v);
				int sLen = strLen(valueBuffer);

				if(text[ti+4] == '.') {
					ti += 1;

					int digitCount = intDigits(v);
					int commaCount = digitCount/3;
					if(digitCount%3 == 0) commaCount--;
					for(int i = 0; i < commaCount; i++) {
						strInsert(valueBuffer, sLen - (i+1)*3 - i, ',');
						sLen++;
					}
				}

				memCpy(buffer + bi, valueBuffer, sLen);
				ti += 4;
				bi += sLen;
				getString(stringBuffer, sLen);
			} else {
				// 32 bit signed integer.
				int v = va_arg(vl, int);
				intToStr(valueBuffer, v);
				int sLen = strLen(valueBuffer);

				if(text[ti+2] == '.') {
					ti += 1;

					int digitCount = intDigits(v);
					int commaCount = digitCount/3;
					if(digitCount%3 == 0) commaCount--;
					for(int i = 0; i < commaCount; i++) {
						strInsert(valueBuffer, sLen - (i+1)*3 - i, ',');
						sLen++;
					}
				}

				memCpy(buffer + bi, valueBuffer, sLen);

				ti += 2;
				bi += sLen;
				getString(stringBuffer, sLen);
			}
		} else if(text[ti] == '%' && text[ti+1] == 's') {
			char* str = va_arg(vl, char*);
			int sLen = strLen(str);
			memCpy(buffer + bi, str, sLen);

			ti += 2;
			bi += sLen;
			getString(stringBuffer, sLen);
		} else if(text[ti] == '%' && text[ti+1] == 'b') {
			bool str = va_arg(vl, bool);
			char* s = str == 1 ? "true" : "false";
			int sLen = strLen(s);
			memCpy(buffer + bi, s, sLen);

			ti += 2;
			bi += sLen;
			getString(stringBuffer, sLen);
		} else if(text[ti] == '%' && text[ti+1] == '%') {
			buffer[bi++] = '%';
			ti += 2;
			getString(stringBuffer, 1);
		} else {
			buffer[bi++] = text[ti++];
			getString(stringBuffer, 1);

			if(buffer[bi-1] == '\0') break;
		}
	}

	return buffer;
}

char* stringGetBetween(char** stringBuffer, char* buffer, char* leftToken, char* rightToken, char** bufferIncrement = 0) {
	char* string;

	int leftPos = strFindRight(buffer, leftToken);
	if(leftPos != -1) {
		int rightPos = strFind(buffer + leftPos, rightToken);
		int size = rightPos;
		string = getString(stringBuffer, size+1); 
		strClear(string);
		strCpy(string, buffer + leftPos, rightPos);
		if(bufferIncrement) {
			(*bufferIncrement) += leftPos + rightPos + 1; 
		}
	} else {
		string = getString(stringBuffer, 1); strClear(string);
	}

	return string;
}

char* getJSONString(char** stringBuffer, char* buffer, char* name, char** bufferIncrement = 0) {
	return stringGetBetween(buffer, fillString(stringBuffer, "\"%s\": \"", name), "\"", bufferIncrement);
}

char* getJSONInt(char** stringBuffer, char* buffer, char* name, char** bufferIncrement = 0) {
	return stringGetBetween(buffer, fillString(stringBuffer, "\"%s\": ", name), ",", bufferIncrement);
}

char* getJSONIntNewline(char** stringBuffer, char* buffer, char* name, char** bufferIncrement = 0) {
	return stringGetBetween(buffer, fillString(stringBuffer, "\"%s\": ", name), "\n", bufferIncrement);
}


int requestPlaylistItems(CURL* curlHandle, char** stringBuffer, char* buffer, char* playlistId, int maxResults, char* pageToken = 0) {
	char* request = fillString(stringBuffer, "%s?key=%s&maxResults=%i&playlistId=%s&part=contentDetails%s", 
								youtubeApiPlaylistItems, apiKey, maxResults, playlistId, pageToken?fillString("&pageToken=%s",pageToken):"");
	return curlRequest(curlHandle, request, buffer);
}

int requestVideos(CURL* curlHandle, char** stringBuffer, char* buffer, char* id, char* part) {
	char* request = fillString(stringBuffer, "%s?key=%s&id=%s&part=%s", 
								youtubeApiVideos, apiKey, id, part);
	return curlRequest(curlHandle, request, buffer);
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
	char pageToken[page_token_size];

	int lastVideoCount;
	int maxVideoCount;
	bool continuedDownload;

	int progressIndex;
	bool finishedDownload;
};

void downloadVideos(void* data) {

	DownloadInfo* dInfo = (DownloadInfo*)data;

	CURL* curlHandle = dInfo->curlHandle;
	char* buffer = dInfo->buffer;
	char* tempBuffer = dInfo->tempBuffer;
	int count = dInfo->count;
	YoutubeVideo* vids = dInfo->vids;

	char* stringBuffer = dInfo->stringBuffer;

	for(int i = 0; i < count; i += maxDownloadCount) {

		dInfo->progressIndex = i;

		int dCount = maxDownloadCount;
		if(i + dCount > count) {
			dCount = count - i;
		}

		{
			TIMER_BLOCK_NAMED("Request");
			requestPlaylistItems(curlHandle, &stringBuffer, buffer, dInfo->playlistId, dCount, dInfo->pageToken);
		}

		// dInfo->pageToken = getJSONString(&stringBuffer, buffer, "nextPageToken");
		char* newPageToken = getJSONString(&stringBuffer, buffer, "nextPageToken");
		strCpy(dInfo->pageToken, newPageToken, page_token_size);

		dInfo->totalCount = strToInt(getJSONInt(&stringBuffer, buffer, "totalResults"));

		int receivedCount;

		// Get Video ids.
		{
			TIMER_BLOCK_NAMED("Video ids");

			int index = i;
			int advance = 0;
			int idCount = 0;
			for(;;) {
				int reverseIndex = count-1 - index;

				char* s = getJSONString(&stringBuffer, buffer, "videoId", &buffer);
				if(strLen(s) == 0) break;

				strCpy(vids[reverseIndex].id, s);

				index++;
				idCount++;
			}

			receivedCount = idCount;
		}
		
		// Get Statistics.
		{

			strClear(tempBuffer);
			for(int videoIndex = i; videoIndex < i+dCount; videoIndex++) {
				int reverseIndex = count-1 - videoIndex;
				char* ss = fillString(&stringBuffer, "%s,", vids[reverseIndex].id);
				strAppend(tempBuffer, ss);
			}
			// Get rid of last comma.
			// if(dCount > 0) tempBuffer[strLen(tempBuffer)-1] = '\0';

			{
				TIMER_BLOCK_NAMED("Statistics Request");
				requestVideos(curlHandle, &stringBuffer, buffer, tempBuffer, "statistics");
			}

			TIMER_BLOCK_NAMED("Statistics");

			int index = i;
			int advance = 0;
			for(;;) {
				int reverseIndex = count-1 - index;
				char* s;

				int pos = strFind(buffer, "\"statistics\":");
				if(pos == -1) break;

				buffer += pos;
				int endBracketPos = strFind(buffer, '}');
				if(endBracketPos == 0) break;

				vids[reverseIndex].viewCount = 0;
				vids[reverseIndex].likeCount = 0;
				vids[reverseIndex].dislikeCount = 0;
				vids[reverseIndex].favoriteCount = 0;
				vids[reverseIndex].commentCount = 0;

				pos = strFind(buffer, "viewCount");
				if(pos != -1 && pos < endBracketPos) vids[reverseIndex].viewCount = strToInt(getJSONString(&stringBuffer, buffer, "viewCount"));
				pos = strFind(buffer, "likeCount");
				if(pos != -1 && pos < endBracketPos) vids[reverseIndex].likeCount = strToInt(getJSONString(&stringBuffer, buffer, "likeCount"));
				pos = strFind(buffer, "dislikeCount");
				if(pos != -1 && pos < endBracketPos) vids[reverseIndex].dislikeCount = strToInt(getJSONString(&stringBuffer, buffer, "dislikeCount"));
				pos = strFind(buffer, "favoriteCount");
				if(pos != -1 && pos < endBracketPos) vids[reverseIndex].favoriteCount = strToInt(getJSONString(&stringBuffer, buffer, "favoriteCount"));
				pos = strFind(buffer, "commentCount");
				if(pos != -1 && pos < endBracketPos) vids[reverseIndex].commentCount = strToInt(getJSONString(&stringBuffer, buffer, "commentCount"));

				YoutubeVideo* vid = vids + reverseIndex;
				if(vid->viewCount < 0 || vid->likeCount < 0 || vid->dislikeCount < 0 || vid->favoriteCount < 0) {
					myAssert(false);
				}

				buffer += endBracketPos;
				index++;
			}
		}

		// Get title and thumbnail.
		{
			strClear(tempBuffer);
			for(int videoIndex = i; videoIndex < i+dCount; videoIndex++) {
				int reverseIndex = count-1 - videoIndex;
				strAppend(tempBuffer, fillString(&stringBuffer, "%s,", vids[reverseIndex].id));
			}

			{
				TIMER_BLOCK_NAMED("Title Request");
				requestVideos(curlHandle, &stringBuffer, buffer, tempBuffer, "snippet");
			}

			TIMER_BLOCK_NAMED("Title");

			int index = i;
			int advance = 0;
			for(;;) {
				int reverseIndex = count-1 - index;

				int start = strFind(buffer, "publishedAt");
				if(start == -1) break;

				char* s = getJSONString(&stringBuffer, buffer, "publishedAt", &buffer);
				strCpy(vids[reverseIndex].dateString, s);
				vids[reverseIndex].date = stringToDate(vids[reverseIndex].dateString);

				int titleStart = strFindRight(buffer, "\"title\": \"");
				buffer += titleStart;
				copyOverTextAndFixTokens(tempBuffer, buffer);
				buffer += strLen(tempBuffer);
				if(strLen(tempBuffer) > memberSize(YoutubeVideo, title)) {
					int maxSize = memberSize(YoutubeVideo, title);
					int i = 0;
					while(tempBuffer[maxSize-i-1] > 127) i--;
					tempBuffer[maxSize-i-1] = '\0';
				}

				strCpy(vids[reverseIndex].title, tempBuffer);

				// Thumbnail.
				// default, medium, high, standard, maxres.
				int pos = strFind(buffer, "\"medium\":"); buffer += pos;
				s = getJSONString(&stringBuffer, buffer, "url", &buffer);
				strCpy(vids[reverseIndex].thumbnail, s);

				// Skip the second "title" from localization data.
				buffer += strFindRight(buffer, "title");

				index++;
			}

		}

	}

	dInfo->finishedDownload = true;
}




enum GuiFocus {
	Gui_Focus_MLeft = 0,
	Gui_Focus_MRight,
	Gui_Focus_MMiddle,
	Gui_Focus_MWheel,
	Gui_Focus_MPos,

	Gui_Focus_Size,
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

	// Temp vars for convenience.

	Vec2 mouseAnchor;
	int mode;

	char editText[100];
	int editInt;
	float editFloat;
	int editMaxSize;

	TextEditVars editVars;
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

int newGuiGoDragAction(NewGui* gui, Rect r, float z, bool input, bool inputRelease, int focus = 0) {
	int id = newGuiDragAction(gui, r, z, gui->input->mousePosNegative, input, inputRelease, focus);
	if(newGuiGotActive(gui, id)) return 1;
	if(newGuiIsActive(gui, id)) return 2;
	if(newGuiWasActive(gui, id)) return 3;
	
	return 0;
}
int newGuiGoDragAction(NewGui* gui, Rect r, float z, int focus = 0) {
	bool input = newGuiInputFromFocus(gui->input, focus, true);
	bool inputRelease = newGuiInputFromFocus(gui->input, focus, false);
	return newGuiGoDragAction(gui, r, z, input, inputRelease, focus);
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

	if(event == 3 && (leftMouse || enter)) {
		if(mode == 0)      strCpy((char*)var, gui->editText);
		else if(mode == 1) *((int*)var) = strToInt(gui->editText);
		else if(mode == 2) *((float*)var) = strToFloat(gui->editText);
	}

	if(event == 1 || event == 2) {
		textEditBox(gui->editText, maxTextSize, font, textRect, input, vec2i(-1,1), editSettings, editVars);
	}

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

void drawTextBox(Rect r, Vec4 bc, char* text, Font* font, Vec4 tc, Vec2i align, Rect scissor) {
	scissorTestScreen(scissor);
	// drawRect(r, bc);
	drawRectRounded(r, bc, 4);

	scissorTestScreen(rectExpand(scissor, vec2(-1,-1)));
	float xPos = rectCen(r).x + (rectW(r)/2)*align.x;
	float yPos = rectCen(r).y + (rectH(r)/2)*align.y;
	drawText(text, font, vec2(xPos, yPos), tc, align);
}
void drawTextBox(Rect r, Vec4 bc, char* text, Font* font, Vec4 tc) {
	drawTextBox(r, bc, text, font, tc, vec2i(0,0), r);
}
void drawTextBox(Rect r, Vec4 bc, char* text, Font* font, Vec4 tc, Vec2i align) {
	drawTextBox(r, bc, text, font, tc, align, r);
}
void drawTextBox(Rect r, Vec4 bc, char* text, Font* font, Vec4 tc, Rect scissor) {
	drawTextBox(r, bc, text, font, tc, vec2i(0,0), scissor);
}


void drawTextEditBox(char* text, Font* font, Rect textRect, TextEditVars editVars, TextEditSettings editSettings, bool active, Rect scissor) {
	Vec2 startPos = rectL(textRect);
	if(active) startPos += editVars.scrollOffset;

	scissorTestScreen(scissor);
	drawRect(textRect, editSettings.colorBackground);

	scissorTestScreen(rectExpand(scissor, vec2(-1,-1)));

	Vec2i align = vec2i(-1,0);
	if(active) {
		// Selection.
		drawTextSelection(text, font, startPos, editVars.cursorIndex, editVars.markerIndex, editSettings.colorSelection, align);
	}

	// Text.
	drawText(text, font, startPos, editSettings.colorText, align);

	if(active) {
		// Cursor.
		Vec2 cPos = textIndexToPos(text, font, startPos, editVars.cursorIndex, align);
		Rect cRect = rectCenDim(cPos, vec2(editSettings.cursorWidth, font->height));
		if(editVars.cursorIndex == 0) cRect = rectTrans(cRect, vec2(editSettings.cursorWidth/2));
		drawRect(cRect, editSettings.colorCursor);
	}
}

void drawTextEditBox(char* textBuffer, char* text, Font* font, Rect textRect, TextEditVars editVars, TextEditSettings editSettings, bool active, Rect scissor) {
	char* t = active ? textBuffer : text;
	drawTextEditBox(t, font, textRect, editVars, editSettings, active, scissor);
}
void drawTextEditBox(char* textBuffer, char* text, Font* font, Rect textRect, TextEditVars editVars, TextEditSettings editSettings, bool active) {
	return drawTextEditBox(textBuffer, text, font, textRect, editVars, editSettings, active, textRect);
}

void drawTextEditBox(char* textBuffer, int number, Font* font, Rect textRect, TextEditVars editVars, TextEditSettings editSettings, bool active, Rect scissor) {
	char* t = active ? textBuffer : fillString("%i", number);
	drawTextEditBox(t, font, textRect, editVars, editSettings, active, scissor);
}
void drawTextEditBox(char* textBuffer, int number, Font* font, Rect textRect, TextEditVars editVars, TextEditSettings editSettings, bool active) {
	return drawTextEditBox(textBuffer, number, font, textRect, editVars, editSettings, active, textRect);
}

void drawTextEditBox(char* textBuffer, float number, Font* font, Rect textRect, TextEditVars editVars, TextEditSettings editSettings, bool active, Rect scissor) {
	char* t = active ? textBuffer : fillString("%f", number);
	drawTextEditBox(t, font, textRect, editVars, editSettings, active, scissor);
}
void drawTextEditBox(char* textBuffer, float number, Font* font, Rect textRect, TextEditVars editVars, TextEditSettings editSettings, bool active) {
	return drawTextEditBox(textBuffer, number, font, textRect, editVars, editSettings, active, textRect);
}


void drawSlider(void* val, bool type, Rect br, Rect sr, Font* font, Vec4 bc, Vec4 tc, Vec4 sc, Rect scissor) {

	glLineWidth(1);
	scissorTestScreen(scissor);

	drawLine(rectL(br), rectR(br), bc);
	// drawRect(sr, sc);
	drawRectRounded(sr, sc, 4);

	scissorTestScreen(rectExpand(scissor, vec2(-1,-1)));

	char* text = type == 0 ? fillString("%f", *((float*)val)) : fillString("%i", *((int*)val)) ;
	drawText(text, font, rectCen(br), tc, vec2i(0,0));
}

void drawSlider(float val, Rect br, Rect sr, Font* font, Vec4 bc, Vec4 tc, Vec4 sc, Rect scissor) {
	return drawSlider(&val, 0, br, sr, font, bc, tc, sc, scissor);
}
void drawSlider(float val, Rect br, Rect sr, Font* font, Vec4 bc, Vec4 tc, Vec4 sc) {
	return drawSlider(val, br, sr, font, bc, tc, sc, br);
}

void drawSlider(int val, Rect br, Rect sr, Font* font, Vec4 bc, Vec4 tc, Vec4 sc, Rect scissor) {
	return drawSlider(&val, 1, br, sr, font, bc, tc, sc, scissor);
}
void drawSlider(int val, Rect br, Rect sr, Font* font, Vec4 bc, Vec4 tc, Vec4 sc) {
	return drawSlider(val, br, sr, font, bc, tc, sc, br);
}

//

bool getRectScissor(Rect* scissor, Rect r) {
	*scissor = rectIntersect(*scissor, r);
	if(rectEmpty(*scissor)) return false;
	return true;
}

bool newGuiQuickButton(NewGui* gui, Rect r, float z, char* text, Font* font, Vec4 bc, Vec4 tc, Rect scissor) {
	if(!getRectScissor(&scissor, r)) return false;

	bool active = newGuiGoButtonAction(gui, scissor, z);
	drawTextBox(r, bc + newGuiColorModB(gui), text, font, tc, scissor);

	return active;
}

void drawQuickTextBox(Rect r, char* t, Font* font, Vec4 bc, Vec4 tc, Vec2i align, Rect scissor) {
	if(!getRectScissor(&scissor, r)) return;

	drawTextBox(r, bc, t, font, tc, align, scissor);
}

bool newGuiQuickSlider(NewGui* gui, Rect r, float z, float* val, float min, float max, float sliderSize, Font* font, Vec4 bc, Vec4 tc, Vec4 cc, Rect scissor) {
	if(!getRectScissor(&scissor, r)) return false;

	Rect slider = newGuiCalcSlider(*val, r, sliderSize, min, max, true);

	int event = newGuiGoDragAction(gui, slider, z);
	if(event == 1) gui->mouseAnchor = gui->input->mousePosNegative - rectCen(slider);
	if(event > 0) {
		*val = newGuiSliderGetValue(gui->input->mousePosNegative - gui->mouseAnchor, r, sliderSize, min, max, true);
		slider = newGuiCalcSlider(*val, r, sliderSize, min, max, true);
	}

	drawSlider(*val, r, slider, font, bc, tc, cc + newGuiColorMod(gui), r);

	if(event > 3) return true;
	return false;
}
bool newGuiQuickSlider(NewGui* gui, Rect r, float z, int* val, int min, int max, float sliderSize, Font* font, Vec4 bc, Vec4 tc, Vec4 cc, Rect scissor) {
	if(!getRectScissor(&scissor, r)) return false;

	float floatVal = *val;

	Rect slider = newGuiCalcSlider(floatVal, r, sliderSize, min, max, true);

	int event = newGuiGoDragAction(gui, slider, z);
	if(event == 1) gui->mouseAnchor = gui->input->mousePosNegative - rectCen(slider);
	if(event > 0) {
		floatVal = newGuiSliderGetValue(gui->input->mousePosNegative - gui->mouseAnchor, r, sliderSize, min, max, true);
		floatVal = roundInt(floatVal);
		slider = newGuiCalcSlider(floatVal, r, sliderSize, min, max, true);
	}

	*val = floatVal;

	drawSlider(*val, r, slider, font, bc, tc, cc + newGuiColorMod(gui), r);

	if(event > 3) return true;
	return false;
}

bool newGuiQuickTextEdit(NewGui* gui, Rect r, char* data, float z, Font* font, TextEditSettings editSettings, Rect scissor, int maxSize) {
	if(!getRectScissor(&scissor, r)) return false;

	int event = newGuiGoTextEdit(gui, scissor, z, data, font, editSettings, maxSize);
	drawTextEditBox(gui->editText, data, font, r, gui->editVars, editSettings, event > 0, scissor);
	if(event == 3) return true;

	return false;
}
bool newGuiQuickTextEdit(NewGui* gui, Rect r, int* data, float z, Font* font, TextEditSettings editSettings, Rect scissor) {
	if(!getRectScissor(&scissor, r)) return false;

	int event = newGuiGoTextEdit(gui, scissor, z, data, font, editSettings);
	drawTextEditBox(gui->editText, *data, font, r, gui->editVars, editSettings, event > 0, scissor);
	if(event == 3) return true;

	return false;
}
bool newGuiQuickTextEdit(NewGui* gui, Rect r, float* data, float z, Font* font, TextEditSettings editSettings, Rect scissor) {
	if(!getRectScissor(&scissor, r)) return false;

	int event = newGuiGoTextEdit(gui, scissor, z, data, font, editSettings);
	drawTextEditBox(gui->editText, *data, font, r, gui->editVars, editSettings, event > 0, scissor);
	if(event == 3) return true;

	return false;
}



struct AppColors {
	Vec4 graphBackgroundBottom;
	Vec4 graphBackgroundTop;
	Vec4 graphLine1;
	Vec4 graphLine2;
	Vec4 graphLineAvg;
	Vec4 font1;
	Vec4 font2;
	Vec4 font3;
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
};

struct AppSettings {
	int windowX;
	int windowY;
	int windowW;
	int windowH;

	int windowMinWidth;
	int windowMinHeight;

	float sidePanelWidth;
	float graphHeights[4];
	float graphHeightMin;

	int font;
	int fontSize;

	int fontComment;
	int fontCommentSize;

	int fontTitle;
	int fontTitleSize;

	int marginBottomText;
	int marginLeftText;
	int marginSidePanel;
	int marginComments;
	int marginButtons;
	int marginButtonsHeight;
	int marginFilters;
	int marginSlider;

	int commentScrollbarWidth;

	int sidePanelMinWidth;

	int resizeRegionSize;

	int titleOffset;
};

#define Graph_Zoom_Min 24*60*60
#define Graph_Zoom_MinNoDate 10

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

	bool updateWindow;
	int updateWindowX, updateWindowY;
	int updateWidth, updateHeight;

	bool screenShotMode;

	// App.

	AppColors appColors;
	AppSettings appSettings;

	Font* font;

	CURL* curlHandle;
	HMODULE curlDll;

	GraphCam cams[10];
	int camCount;

	int heightMoveMode;
	bool widthMove;

	float leftTextWidth;

	float graphOffsets[5];
	float sidePanelWidth;
	float sidePanelMax;

	//

	bool activeDownload;
	DownloadInfo dInfo;

	//

	NewGui newGui;
	NewGui* gui;

	//

	YoutubePlaylist playlist;
	YoutubeVideo* videos;
	int maxVideoCount;

	VideoSnippet videoSnippet;
	Vec2 hoveredPoint;
	int hoveredVideo;
	float hoveredVideoStat;

	Vec2 selectedPoint;
	int selectedVideo;

	YoutubePlaylist downloadPlaylist;

	bool startScreenShot;
	char screenShotFilePath[50];
	Vec2i screenShotDim;

	char* searchString;

	bool startDownload;
	bool update;
	bool startLoadFile;

	char* userName;
	char* channelId;
	int playlistDownloadCount;

	YoutubePlaylist playlists[1000];
	int playlistCount;

	SearchResult searchResults[10];
	int searchResultCount;

	int lastSearchMode;

	YoutubePlaylist playlistFolder[50];
	int playlistFolderCount;

	float statTimeSpan;
	float statWidth;

	LineGraph averagesLineGraph;

	char exclusiveFilter[50];
	char inclusiveFilter[50];

	int graphDrawMode;

	bool sortByDate;
	int sortStat;
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
		
		initSystem(systemData, ws, windowsData, vec2i(1920*0.85f, 1080*0.85f), true, true, true, 1);
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

			attachToFrameBuffer(FRAMEBUFFER_2dMsaa, FRAMEBUFFER_SLOT_COLOR, GL_RGBA16F, 0, 0, ad->msaaSamples);
			attachToFrameBuffer(FRAMEBUFFER_2dMsaa, FRAMEBUFFER_SLOT_DEPTH, GL_DEPTH_COMPONENT32F, 0, 0, ad->msaaSamples);
			attachToFrameBuffer(FRAMEBUFFER_2dNoMsaa, FRAMEBUFFER_SLOT_COLOR, GL_RGBA16F, 0, 0);

			attachToFrameBuffer(FRAMEBUFFER_DebugMsaa, FRAMEBUFFER_SLOT_COLOR, GL_RGBA8, 0, 0, ad->msaaSamples);
			attachToFrameBuffer(FRAMEBUFFER_DebugNoMsaa, FRAMEBUFFER_SLOT_COLOR, GL_RGBA8, 0, 0);

			// attachToFrameBuffer(FRAMEBUFFER_ScreenShot, FRAMEBUFFER_SLOT_COLOR, GL_SRGB8_ALPHA8, 0, 0);
			attachToFrameBuffer(FRAMEBUFFER_ScreenShot, FRAMEBUFFER_SLOT_COLOR, GL_SRGB8, 0, 0);

			ad->updateFrameBuffers = true;

			Vec2 fRes = vec2(2560, 1440);
			setDimForFrameBufferAttachmentsAndUpdate(FRAMEBUFFER_2dMsaa, fRes.w, fRes.h);
			setDimForFrameBufferAttachmentsAndUpdate(FRAMEBUFFER_2dNoMsaa, fRes.w, fRes.h);
			setDimForFrameBufferAttachmentsAndUpdate(FRAMEBUFFER_DebugMsaa, fRes.w, fRes.h);
			setDimForFrameBufferAttachmentsAndUpdate(FRAMEBUFFER_DebugNoMsaa, fRes.w, fRes.h);
			// setDimForFrameBufferAttachmentsAndUpdate(FRAMEBUFFER_ScreenShot, ws->currentRes.w, ws->currentRes.h);
		}

		TIMER_BLOCK_END(initGraphics);

		//

		// TIMER_BLOCK_BEGIN_NAMED(initRest, "Init Rest");

		int maxVideoCount = 5000;
		ad->maxVideoCount = maxVideoCount;

		ad->videos = (YoutubeVideo*)malloc(sizeof(YoutubeVideo)*maxVideoCount);
		// zeroMemory(ad->videos, maxVideoCount * sizeof(YoutubeVideo));

		ad->curlHandle = curl_easy_initX();
		curl_easy_setoptX(ad->curlHandle, CURLOPT_WRITEFUNCTION, curlWrite);

		ad->videoSnippet.thumbnailTexture.id = -1;

		ad->selectedVideo = -1;

		ad->channelId = (char*)getPMemory(100); strClear(ad->channelId);
		strCpy(ad->channelId, rocketBeansId);

		ad->userName = (char*)getPMemory(100); strClear(ad->userName);
		ad->searchString = (char*)getPMemory(100); strClear(ad->searchString);

		ad->statTimeSpan = 3.0f;
		ad->statWidth = 1.0f;

		strCpy(ad->downloadPlaylist.id, "UUaTznQhurW5AaiYPbhEA-KA");

		strClear(ad->screenShotFilePath);
		strCpy(ad->screenShotFilePath, "..\\screenshot.png");
		ad->screenShotDim = vec2i(5000, 1000);

		ad->leftTextWidth = 100;
		ad->camCount = 4;

		ad->font = getFont(FONT_CALIBRI, 20);

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

		// TIMER_BLOCK_END(initRest);
	}



	// @AppStart.

	TIMER_BLOCK_BEGIN_NAMED(reload, "Reload");

	if(reload) {
		loadFunctions();
		SetWindowLongPtr(systemData->windowHandle, GWLP_WNDPROC, (LONG_PTR)mainWindowCallBack);

		globalScreenHeight = ws->currentRes.h;

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

		ad->gui = &ad->newGui;
		newGuiBegin(ad->gui, input);

		SetCursor(LoadCursor(0, IDC_ARROW));

		{
			ad->appColors.graphBackgroundBottom = vec4(0.2f, 0.1f, 0.4f, 1);
			ad->appColors.graphBackgroundTop    = vec4(0.1f, 0.1f, 0.2f, 1);
			ad->appColors.graphLine1            = vec4(0,0.7f,1,1);
			ad->appColors.graphLine2            = vec4(0.9f,0.5f,1,1);
			ad->appColors.graphLineAvg          = vec4(0,0.9f,0.3f,0.8f);
			ad->appColors.font1                 = vec4(0.9f,1);
			ad->appColors.font2                 = vec4(0.6f,1);
			ad->appColors.font3                 = vec4(1.0f,0.5f,0,1);
			ad->appColors.background            = vec4(0.45f, 0.15f, 0.15f, 1);
			ad->appColors.buttons               = vec4(0.52,0.25,0.20,1);
			ad->appColors.sidePanel             = vec4(0,0.9f);
			ad->appColors.likes                 = vec4(0.2f,0.2f,0.5f,1);
			ad->appColors.dislikes              = vec4(0.5f,0.2f,0.2f,1);
			ad->appColors.comments              = vec4(0.2f,0.1f,0,1);
			ad->appColors.commentScroll         = vec4(0.3f,0.15f,0,1);
			ad->appColors.sidePanelButtons      = vec4(0.3f,0.1f,0,1);
			ad->appColors.editBackground        = vec4(0.33,0.11,0.11,1);
			ad->appColors.editSelection         = vec4(0.24,0.41,0.59,1); 
			ad->appColors.editCursor            = vec4(0.2f,0.7f,0,1);
			ad->appColors.mouseHover            = vec4(0.8f,0,0.2f,0.2f);
			ad->appColors.outlines              = vec4(0,1);
			ad->appColors.graphMarkers          = vec4(1,0.05f);
			ad->appColors.graphSubMarkers       = vec4(1,0.01f);
		}

		{
			ad->appSettings.windowX;
			ad->appSettings.windowY;
			ad->appSettings.windowW;
			ad->appSettings.windowH;
			ad->appSettings.sidePanelWidth;
			ad->appSettings.graphHeights[4];

			ad->appSettings.windowMinWidth = 300;
			ad->appSettings.windowMinHeight = 300;
			ad->appSettings.graphHeightMin = 0.1f;
			ad->appSettings.font = FONT_CALIBRI;
			ad->appSettings.fontSize = 20;
			ad->appSettings.fontComment = FONT_SOURCESANS_PRO;
			ad->appSettings.fontCommentSize = 23;
			ad->appSettings.fontTitle = FONT_CALIBRI;
			ad->appSettings.fontTitleSize = 30;
			ad->appSettings.marginBottomText = -3;
			ad->appSettings.marginLeftText = 2;
			ad->appSettings.marginSidePanel = 20;
			ad->appSettings.marginComments = 10;
			ad->appSettings.marginButtons = 4;
			ad->appSettings.marginButtonsHeight = 4;
			ad->appSettings.marginFilters = 8;
			ad->appSettings.marginSlider = 7;
			ad->appSettings.commentScrollbarWidth = 20;
			ad->appSettings.sidePanelMinWidth = 200;
			ad->appSettings.resizeRegionSize = 10;
			ad->appSettings.titleOffset = 10;
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
    	if(!ad->gui->activeId)
    		*isRunning = false;
    }

	bool resize = false;
	// Window drag.
	{
	    NewGui* gui = ad->gui;

	    float z = 2;

		bool setupResize = false;

	    Vec2i res = ws->currentRes;
	    Rect screenRect = getScreenRect(ws);
	    int id = newGuiDragAction(gui, screenRect, z, Gui_Focus_MRight);

	    if(newGuiGotActive(gui, id)) {
	    	if(!input->keysDown[KEYCODE_CTRL]) {
	    		gui->mode = 0;
		    	POINT p;
		    	GetCursorPos(&p);
		    	ScreenToClient(windowHandle, &p);

	    		gui->mouseAnchor = vec2(p.x+1, p.y+1);
	    	} else {
	    		gui->mode = 1;
	    		setupResize = true;
	    	}
	    }

	    if(newGuiIsActive(gui, id)) {
	    	POINT p; 
	    	GetCursorPos(&p);
	    	RECT r; 
	    	GetWindowRect(windowHandle, &r);

	    	if(!gui->mode) {
		    	MoveWindow(windowHandle, p.x - gui->mouseAnchor.x, p.y - gui->mouseAnchor.y, r.right-r.left, r.bottom-r.top, true);
	    	} else {
	    		resize = true;
	    	}
	    }

	    float w = ad->appSettings.resizeRegionSize;
    	Rect resizeRight = rectSetL(screenRect, screenRect.right - w);
    	Rect resizeBottom = rectSetT(screenRect, screenRect.bottom + w);
    	Rect resizeDR = rectSetTL(screenRect, rectBR(screenRect) + vec2(-w,w));

	    int idResizeRight = newGuiDragAction(gui, resizeRight, z, Gui_Focus_MLeft);
	    int idResizeBottom = newGuiDragAction(gui, resizeBottom, z, Gui_Focus_MLeft);
	    int idResizeDR = newGuiDragAction(gui, resizeDR, z, Gui_Focus_MLeft);

	    if(newGuiGotActive(gui, idResizeRight) || newGuiGotActive(gui, idResizeBottom) || newGuiGotActive(gui, idResizeDR)) setupResize = true;
	    if(newGuiIsActive(gui, idResizeRight) || newGuiIsActive(gui, idResizeBottom) || newGuiIsActive(gui, idResizeDR)) resize = true;



		int mode = -1;
        if(newGuiIsActive(gui, idResizeRight)) mode = 0;
		if(newGuiIsActive(gui, idResizeBottom)) mode = 1;
		if(newGuiIsActive(gui, idResizeDR)) mode = 2;

		if(newGuiIsWasHotOrActive(gui, idResizeRight)) SetCursor(LoadCursor(0, IDC_SIZEWE));
		if(newGuiIsWasHotOrActive(gui, idResizeBottom)) SetCursor(LoadCursor(0, IDC_SIZENS));
		if(newGuiIsWasHotOrActive(gui, idResizeDR)) SetCursor(LoadCursor(0, IDC_SIZENWSE));

		if(setupResize) {
			POINT p;
			GetCursorPos(&p);
			ScreenToClient(windowHandle, &p);
			RECT r;
			GetWindowRect(windowHandle, &r);

			gui->mouseAnchor = vec2((r.right - r.left) - p.x, (r.bottom - r.top) - p.y);
		}

        if(resize) {
        	POINT p; 
        	GetCursorPos(&p);
        	RECT r; 
        	GetWindowRect(windowHandle, &r);

	    	ScreenToClient(windowHandle, &p);

	    	int newWidth = mode == 1 ? r.right - r.left : clampMin(p.x + gui->mouseAnchor.x, ad->appSettings.windowMinWidth);
	    	int newHeight = mode == 0 ? r.bottom - r.top : clampMin(p.y + gui->mouseAnchor.y, ad->appSettings.windowMinHeight);

	    	ad->updateWindow = true;

	    	ad->updateWindowX = r.left;
	    	ad->updateWindowY = r.top;
	    	ad->updateWidth = newWidth;
	    	ad->updateHeight = newHeight;

	    	ws->currentRes.w = newWidth-2;
	    	ws->currentRes.h = newHeight-2;
	    	ws->aspectRatio = ws->currentRes.x / (float)ws->currentRes.y;

	    	ad->updateFrameBuffers = true;	

	    	globalScreenHeight = ws->currentRes.h;
        }
	}	

	if(!resize) {
		if(windowSizeChanged(windowHandle, ws)) {
			if(!windowIsMinimized(windowHandle)) {
				updateResolution(windowHandle, ws);
				globalScreenHeight = ws->currentRes.h;
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
		ad->aspectRatio = ws->currentRes.w / ws->currentRes.h;

		ad->updateFrameBuffers = true;
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

		// glDepthRange(-1.0,1.0);
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
		glBlendEquation(GL_FUNC_ADD);

		// glViewport(0,0, ad->cur3dBufferRes.x, ad->cur3dBufferRes.y);


		bindFrameBuffer(FRAMEBUFFER_2dMsaa);
		// glBindTexture(GL_TEXTURE_2D, getFrameBuffer(FRAMEBUFFER_2dMsaa)->colorSlot[0]->id);
		// glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glDisable(GL_DEPTH_TEST);

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

	if(ad->startDownload) { 
		ad->dInfo = {};
		DownloadInfo* dInfo = &ad->dInfo;

		YoutubePlaylist headerPlaylist;
		char* fileName = fillString("%s.playlist", ad->downloadPlaylist.id);
		dInfo->maxVideoCount = 0;
		char* pt = getTString(page_token_size);
		dInfo->continuedDownload = loadPlaylistHeaderFromFile(&headerPlaylist, fileName, &dInfo->maxVideoCount, pt);

		bool skip = false;

		dInfo->lastVideoCount = 0;
		strClear(dInfo->pageToken);
		if(!ad->update && dInfo->continuedDownload) {
			strCpy(dInfo->pageToken, pt, page_token_size);
			dInfo->lastVideoCount = headerPlaylist.count;

			if(dInfo->maxVideoCount == headerPlaylist.count) skip = true;
			else if(ad->downloadPlaylist.count + dInfo->lastVideoCount > dInfo->maxVideoCount) {
				ad->downloadPlaylist.count = dInfo->maxVideoCount - dInfo->lastVideoCount;
			}
		}

		if(skip) {
			ad->startDownload = false;
		} else {
			dInfo->totalCount = 0;

			// dInfo->buffer = (char*)malloc(megaBytes(3)); 
			dInfo->buffer = (char*)malloc(megaBytes(5)); 
			dInfo->tempBuffer = (char*)malloc(kiloBytes(20));
			dInfo->vids = (YoutubeVideo*)malloc(sizeof(YoutubeVideo)*ad->downloadPlaylist.count);
			dInfo->stringBuffer = (char*)malloc(megaBytes(1));

			zeroMemory(dInfo->vids, sizeof(YoutubeVideo)*ad->downloadPlaylist.count);

			dInfo->playlistId = ad->downloadPlaylist.id;
			dInfo->count = ad->downloadPlaylist.count;
			dInfo->curlHandle = ad->curlHandle;

			threadQueueAdd(threadQueue, downloadVideos, &ad->dInfo);

			ad->startDownload = false;
			ad->activeDownload = true;

			dInfo->finishedDownload = false;
		}
	}

	if(ad->activeDownload && ad->dInfo.finishedDownload) {

		DownloadInfo* dInfo = &ad->dInfo;

		memCpy(&ad->playlist, &ad->downloadPlaylist, sizeof(ad->downloadPlaylist));

		if(!ad->update && dInfo->continuedDownload) {
			if(ad->downloadPlaylist.count + dInfo->lastVideoCount > dInfo->maxVideoCount) {
				ad->playlist.count = (ad->downloadPlaylist.count + dInfo->lastVideoCount) - dInfo->maxVideoCount;
			}

			savePlaylistToFile(&ad->playlist, dInfo->vids, ad->playlist.count, dInfo->totalCount, dInfo->lastVideoCount, dInfo->pageToken);
		} else if(!ad->update && !dInfo->continuedDownload) {
			savePlaylistToFile(&ad->playlist, dInfo->vids, ad->playlist.count, dInfo->totalCount, 0, dInfo->pageToken);
		} else if(ad->update) {
			savePlaylistToFile(&ad->playlist, dInfo->vids, ad->playlist.count, dInfo->totalCount, -1, dInfo->pageToken);
		}

		loadPlaylistFolder(ad->playlistFolder, &ad->playlistFolderCount);

		free(dInfo->buffer);
		free(dInfo->tempBuffer);
		free(dInfo->vids);
		free(dInfo->stringBuffer);

		zeroMemory(&ad->dInfo, sizeof(DownloadInfo));

		ad->update = false;
		ad->activeDownload = false;
	}



	// if(ad->startDownload) { 
	// 	ad->startDownload = false;

	// 	TIMER_BLOCK_NAMED("Total download");

	// 	bool update = ad->update;
	// 	ad->update = false;

	// 	char* playlist = ad->downloadPlaylist.id;
	// 	int count = ad->downloadPlaylist.count;

	// 	YoutubeVideo* vids = getTArray(YoutubeVideo, count);
	// 	zeroMemory(vids, sizeof(YoutubeVideo)*count);

	// 	char* pageToken = 0;

	// 	YoutubePlaylist tempPlaylist;
	// 	char* fileName = fillString("%s.playlist", ad->downloadPlaylist.id);
	// 	int maxVideoCount = 0;
	// 	char* pt = getTString(page_token_size);
	// 	bool continuedDownload = loadPlaylistHeaderFromFile(&tempPlaylist, fileName, &maxVideoCount, pt);
	// 	int lastVideoCount = 0;

	// 	if(!update && continuedDownload) {
	// 		pageToken = pt;			
	// 		lastVideoCount = tempPlaylist.count;

	// 		if(maxVideoCount == tempPlaylist.count) return;
	// 		if(count + lastVideoCount > maxVideoCount) {
	// 			count = maxVideoCount - lastVideoCount;
	// 			ad->downloadPlaylist.count = count;
	// 		}
	// 	}

	// 	int totalCount = 0;

	// 	char* buffer = (char*)getTMemory(megaBytes(3)); 
	// 	char* tempBuffer = getTString(kiloBytes(20));
	// 	downloadVideos(ad->curlHandle, buffer, tempBuffer, vids, playlist, &totalCount, count, &pageToken);



	// 	memCpy(&ad->playlist, &ad->downloadPlaylist, sizeof(ad->downloadPlaylist));

	// 	if(!update && continuedDownload) {
	// 		if(ad->downloadPlaylist.count + lastVideoCount > maxVideoCount) {
	// 			ad->playlist.count = (ad->downloadPlaylist.count + lastVideoCount) - maxVideoCount;
	// 		}

	// 		savePlaylistToFile(&ad->playlist, vids, ad->playlist.count, totalCount, lastVideoCount, pageToken);
	// 	} else if(!update && !continuedDownload) {
	// 		savePlaylistToFile(&ad->playlist, vids, ad->playlist.count, totalCount, 0, pageToken);
	// 	} else if(update) {
	// 		savePlaylistToFile(&ad->playlist, vids, ad->playlist.count, totalCount, -1, pageToken);
	// 	}

	// 	loadPlaylistFolder(ad->playlistFolder, &ad->playlistFolderCount);
	// }

	// Load playlist folder.
	if(init) {
		loadPlaylistFolder(ad->playlistFolder, &ad->playlistFolderCount);

		int startVideo = 0;
		for(int i = 0; i < ad->playlistFolderCount; i++) {
			if(strFind(ad->playlistFolder[i].title, "Rocket Beans TV") != -1) {
				startVideo = i;
				break;
			}
		}
		YoutubePlaylist* firstPlaylistFromFolder = ad->playlistFolder + startVideo;
		memCpy(&ad->playlist, firstPlaylistFromFolder, sizeof(YoutubePlaylist));

		ad->startLoadFile = true;
	}

	// @Load File.
	if(ad->startLoadFile && !ad->activeDownload) {
		TIMER_BLOCK_NAMED("LoadFile");

		TIMER_BLOCK_BEGIN_NAMED(filestart, "LoadFileStart");

		ad->startLoadFile = false;

		YoutubePlaylist tempPlaylist;		
		loadPlaylistHeaderFromFile(&tempPlaylist, fillString("%s.playlist", ad->playlist.id));
		memCpy(&ad->playlist, &tempPlaylist, sizeof(YoutubePlaylist));

		pushTMemoryStack();

		int tempVidCount = tempPlaylist.count;
		YoutubeVideo* tempVids = getTArray(YoutubeVideo, tempVidCount);

		bool foundFile = loadPlaylistFromFile(&tempPlaylist, tempVids, &tempVidCount);

		TIMER_BLOCK_END(filestart);

		if(foundFile) {
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
						searchString = eatWhiteSpace(searchString);
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
						searchString = eatWhiteSpace(searchString);
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

						// Filter corrupted videos for now.
						bool corrupted = false;
						{
							YoutubeVideo* vid = tempVids + i;
							if(vid->date.n < dateEncode(5,1,1,0,0,0) || vid->date.n > dateEncode(20,1,1,0,0,0) ||
							   vid->viewCount < 0 || vid->viewCount > 5000000000 ||
							   vid->likeCount < 0 || vid->likeCount > 100000000 ||
							   vid->dislikeCount < 0 || vid->dislikeCount > 100000000 ||
							   vid->favoriteCount < 0)
								corrupted = true;

							int stopForVS = 123;
						}

						if(inclusiveCorrect && exclusiveCorrect && !corrupted) filteredIndexes[filteredIndexesCount++] = i;
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

				Statistic statViews, statLikes, statDislikes, statDates, statComments;
				{
					TIMER_BLOCK_NAMED("Get Stats");

					beginStatistic(&statViews);
					beginStatistic(&statLikes);
					beginStatistic(&statDates);
					beginStatistic(&statComments);

					for(int i = 0; i < ad->playlist.count; i++) {
						YoutubeVideo* video = ad->videos + i;

						updateStatistic(&statViews, video->viewCount);
						updateStatistic(&statLikes, video->likeCount+video->dislikeCount);
						updateStatistic(&statComments, video->commentCount);

						if(video->date.n > 0) updateStatistic(&statDates, video->date.n);
					}
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
						camRight = ad->playlist.count;
					}

					graphCamInit(&ad->cams[0], camLeft, camRight, 0, statViews.max*1.1f);
					graphCamInit(&ad->cams[1], camLeft, camRight, 0, statLikes.max*1.1f);
					graphCamInit(&ad->cams[2], camLeft, camRight, 0, 1);
					graphCamInit(&ad->cams[3], camLeft, camRight, 0, statComments.max*1.1f);
				}

				calculateAverages(ad->videos, ad->playlist.count, &ad->averagesLineGraph, ad->statTimeSpan, ad->statWidth, ad->cams[0].xMax - ad->cams[0].xMin);
			}

			ad->selectedVideo = -1;
		} 

		popTMemoryStack();
	}



	{
		AppColors* ac = &ad->appColors;
		AppSettings* as = &ad->appSettings;

		int fontSize = as->fontSize;
		Font* font = getFont(as->font, as->fontSize);

		float zoomSpeed = 1.2;

		// Screen layout.
		Rect screenRect, rChart, rFilters, rSidePanel, rBottomText, rLeftTexts, rGraphs;

		screenRect = getScreenRect(ws);

		// float sidePanelWidth = ad->selectedVideo!=-1 ? rectW(screenRect)*0.25f : 0;
		float sidePanelWidth = ad->selectedVideo!=-1 ? ad->sidePanelWidth : 0;
		float filtersHeight = font->height + as->marginFilters;
		float bottomTextHeight = font->height*2 + as->marginBottomText;
		float leftTextMargin = as->marginLeftText;

		clamp(&ad->sidePanelWidth, as->sidePanelMinWidth, ws->currentRes.w*ad->sidePanelMax);

		rSidePanel = rectSetL(screenRect, screenRect.right-sidePanelWidth);
		rFilters = rectSetBR(screenRect, vec2(screenRect.right-sidePanelWidth, screenRect.top-filtersHeight));
		rChart = rectSetTR(screenRect, vec2(screenRect.right-sidePanelWidth, screenRect.top-filtersHeight));

		rBottomText = rectSetTL(rChart, vec2(rChart.left+ad->leftTextWidth+leftTextMargin, rChart.bottom+bottomTextHeight));
		rLeftTexts = rectSetBR(rChart, vec2(rChart.left+ad->leftTextWidth+leftTextMargin, rChart.bottom+bottomTextHeight));

		rGraphs = rectSetBL(rChart, vec2(rChart.left+ad->leftTextWidth+leftTextMargin, rChart.bottom+bottomTextHeight));

		Rect graphRects[10];
		Rect leftTextRects[10];

		float tempHeight = rGraphs.top;
		float gh = rectH(rGraphs);
		for(int i = 0; i < ad->camCount; i++) {
			graphRects[i] = rect(rGraphs.left, rGraphs.top-ad->graphOffsets[i+1]*gh, rGraphs.right, rGraphs.top-ad->graphOffsets[i]*gh);
			leftTextRects[i] = rect(rLeftTexts.left, rGraphs.top-ad->graphOffsets[i+1]*gh, rLeftTexts.right, rGraphs.top-ad->graphOffsets[i]*gh);
		}

		// Draw background.
		drawRect(screenRect, ac->background);

		for(int i = 0; i < ad->camCount; i++) {
			drawRectNewColored(graphRects[i], ac->graphBackgroundBottom, ac->graphBackgroundTop, ac->graphBackgroundTop, ac->graphBackgroundBottom);
		}

		// Height sliders.
		{
			float z = 1;

			for(int i = 0; i < ad->camCount-1; i++) {
				Rect r = rectCenDim(rectB(leftTextRects[i]), vec2(ad->leftTextWidth, as->resizeRegionSize));

				int event = newGuiGoDragAction(ad->gui, r, z);
				if(event == 1) ad->gui->mouseAnchor = input->mousePos - ad->graphOffsets[i+1]*rectH(rGraphs);
				if(event > 0) {
					ad->graphOffsets[i+1] = (input->mousePos - ad->gui->mouseAnchor).y/rectH(rGraphs);
					clamp(&ad->graphOffsets[i+1], ad->graphOffsets[i] + as->graphHeightMin/2, ad->graphOffsets[i+2] - as->graphHeightMin/2);

					SetCursor(LoadCursor(0, IDC_SIZENS));
				}

				if(newGuiIsWasHotOrActive(ad->gui)) SetCursor(LoadCursor(0, IDC_SIZENS));
			}
		}

		// Update Cameras.
		for(int i = 0; i < ad->camCount; i++) {
			GraphCam* cam = ad->cams + i;

			Rect rGraph = graphRects[i];
			Rect rLeftText = leftTextRects[i];

			graphCamSetViewPort(cam, rectExpand(rGraph, vec2(-font->height,-font->height)));

			{
				float z = 0;
				if(newGuiGoButtonAction(ad->gui, rLeftText, z, Gui_Focus_MWheel)) 
	    			graphCamScaleToPos(cam, 0, zoomSpeed, -input->mouseWheel, zoomSpeed, input->mousePosNegative);

	    		if(i == 0) {
					if(newGuiGoButtonAction(ad->gui, rGraphs, z, Gui_Focus_MWheel)) {
						for(int i = 0; i < ad->camCount; i++) {
							graphCamScaleToPos(&ad->cams[i], -input->mouseWheel, zoomSpeed, 0, zoomSpeed, input->mousePosNegative);
						}
					}
	    		}
			}

			double camMinH = 1000;
			if(i == 0) camMinH = 1000;
			if(i == 1) camMinH = 100;
			if(i == 2) camMinH = 0.1;
			if(i == 3) camMinH = 10;
			if(ad->sortByDate) graphCamSizeClamp(cam, Graph_Zoom_Min, camMinH);
			else graphCamSizeClamp(cam, Graph_Zoom_MinNoDate, camMinH);

			{
				NewGui* gui = ad->gui;
				float z = 0;
				int event;

				if(i == 0) {
		    		event = newGuiGoDragAction(gui, rGraphs, z, Gui_Focus_MLeft);
		    		if(event == 1) gui->mouseAnchor.x = -input->mousePosNegative.x - graphCamCamToScreenSpaceX(cam, cam->x);
		    		if(event > 0) {
		    			double camX = graphCamScreenToCamSpaceX(cam, -input->mousePosNegative.x - gui->mouseAnchor.x);

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

		// Draw left text.
		{
			Vec4 mainColor = ac->font1;
			Vec4 semiColor = ac->font2;
			Vec4 horiLinesColor = ac->graphMarkers;
			Vec4 subHoriLinesColor = ac->graphSubMarkers;
			float markLength = 10;
			float fontMargin = 4;
			float div = 10;
			int subDiv = 4;
			float splitSizePixels = font->height*3;

			glLineWidth(1);
			glEnable(GL_SCISSOR_TEST);

			bool calcMaxTextWidth;

			for(int camIndex = 0; camIndex < ad->camCount; camIndex++)
			{
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
					drawLine(vec2(scaleRect.right, y), vec2(cam->viewPort.right, y), horiLinesColor); 
					scissorTestScreen(scaleRect);

					// Base markers.
					drawLineNewOff(vec2(scaleRect.right, y), vec2(-markLength,0), mainColor); 

					char* text;
					if(stepSize/subDiv > 10) text = fillString("%i.",(int)p);
					else text = fillString("%f",p);

					drawText(text, font, vec2(scaleRect.right - markLength - fontMargin, y), mainColor, vec2i(1,0));
					
					float textWidth = getTextDim(text, font).w;

					if(calcMaxTextWidth) maxTextWidth = max(maxTextWidth, textWidth + markLength+fontMargin);

					// Semi markers.
					for(int i = 1; i < subDiv; i++) {
						float y = graphCamMapY(cam, p+i*(stepSize/subDiv));
						drawLineNewOff(vec2(scaleRect.right, y), vec2(-markLength*0.5f,0), semiColor); 

						char* subText;
						if(stepSize/subDiv > 10) subText = fillString("%i.",(int)(stepSize/subDiv));
						else subText = fillString("%f",(stepSize/subDiv));

						drawText(subText, font, vec2(scaleRect.right - markLength*0.5f - fontMargin, y), semiColor, vec2i(1,0));

						// Draw horizontal line.
						scissorTestScreen(cam->viewPort);
						drawLine(vec2(scaleRect.right, y), vec2(cam->viewPort.right, y), subHoriLinesColor); 
						scissorTestScreen(scaleRect);

					}

					p += stepSize;
				}

				if(calcMaxTextWidth) ad->leftTextWidth = maxTextWidth;
			}

			glDisable(GL_SCISSOR_TEST);
		}

		// Draw bottom text.
		if(ad->sortByDate)
		{
			GraphCam* cam = &ad->cams[0];

			int subStringMargin = font->height/2;
			float splitOffset = font->height;

			Vec4 mainColor = ac->font1;
			Vec4 semiColor = ac->font2;
			Vec4 horiLinesColor = ac->graphMarkers;
			float markLength = font->height - 3;
			float fontMargin = 0;
			float div = 10;
			float splitSizePixels = 1000;

			glLineWidth(1);
			glEnable(GL_SCISSOR_TEST);

			Rect scaleRect = rBottomText;

			i64 oneYearInSeconds = (i64)365*86400;
			i64 oneMonthInSeconds = (i64)30*86400;
			i64 oneDayInSeconds = (i64)1*86400;

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
						drawLine(vec2(x, rGraphs.bottom), vec2(x, rGraphs.top), i==0?ac->graphMarkers:ac->graphSubMarkers);
						scissorTestScreen(scaleRect);

						drawLineNewOff(vec2(x, scaleRect.top), vec2(0,-markLength), mainColor); 
						drawText(dateString, font, vec2(x, scaleRect.top - markLength - fontMargin), mainColor, vec2i(0,1));

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
						drawText(subString, font, vec2(leftX + (x-leftX)/2, scaleRect.top), semiColor, vec2i(0,1));

						leftX = x;
					}
				}

				if(startDate.n > cam->right) break;
				dateIncrement(&startDate, zoomStage);

				dateEncode(&startDate);
			}

			glDisable(GL_SCISSOR_TEST);
		}



		YoutubeVideo* vids = ad->videos;
		int vidCount = ad->playlist.count;

		glEnable(GL_SCISSOR_TEST);

		TIMER_BLOCK_BEGIN_NAMED(drawGraphs, "DrawGraphs");

		float settingHoverSearchWidth = 100;
		float settingHoverDistance = 20;
		ad->hoveredVideo = -1;

		// Draw Graphs.
		int graphCount = ad->camCount + 1;
		int mode = ad->graphDrawMode;
		for(int graphIndex = 0; graphIndex < graphCount; graphIndex++) {

			Vec4 c1 = ac->graphLine1;
			Vec4 c2 = ac->graphLine2;
			Vec4 colors[] = {c1,c1,c2,c2,c1};

			int camIndexes[] = {0,1,1,2,3};
			GraphCam* cam = ad->cams + camIndexes[graphIndex];
			Vec4 color = colors[graphIndex];
			Rect graphRect = graphRects[camIndexes[graphIndex]];

			scissorTestScreen(rectExpand(cam->viewPort, 1));

			if(mode == 0) {
				glLineWidth(1);
				drawLineStripHeader(color);
			} else if(mode == 1) {
				glPointSize(3);
				drawPointsHeader(color);
			} else if(mode == 2) {
				glLineWidth(3);
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
					// x = graphCamMapX(cam, vids[i].date.n) + 1;
				} else {
					if(i < cam->left-1 || i > cam->right+1) continue;
					xValue = i;
				}

				double value;
				     if(graphIndex == 0) value = vids[i].viewCount;
				else if(graphIndex == 1) value = vids[i].likeCount + vids[i].dislikeCount;
				else if(graphIndex == 2) value = vids[i].dislikeCount;
				else if(graphIndex == 3) value = divZero(vids[i].dislikeCount, (vids[i].likeCount + vids[i].dislikeCount));
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
		if(ad->sortByDate)
		{
			glLineWidth(1);
			glEnable(GL_SCISSOR_TEST);
			Vec4 c = ac->graphLineAvg;

			for(int graphIndex = 0; graphIndex < Line_Graph_Count; graphIndex++) {

				GraphCam* cam = ad->cams + graphIndex;

				double avgStartX = ad->videos[0].date.n;
				double avgWidth = monthsToInt(ad->statWidth);
				double avgTimeSpan = monthsToInt(ad->statTimeSpan);

				scissorTestScreen(cam->viewPort);

				drawLineStripHeader(c);
				double xPos = avgStartX;
				for(int i = 0; i < ad->averagesLineGraph.count; i++) {
					if(xPos + avgWidth < cam->left) {
						xPos += avgWidth;
						continue;
					}
					if(xPos - avgWidth > cam->right) break;

					pushVec(graphCamMap(cam, xPos, ad->averagesLineGraph.points[graphIndex][i]));

					xPos += avgWidth;
				}

				glEnd();
			}

			glDisable(GL_SCISSOR_TEST);
			glLineWidth(1);
		}

		TIMER_BLOCK_END(drawGraphs);

		glDisable(GL_SCISSOR_TEST);

		// Mouse hover.
		{
			int settingHoverPointSize = 10;
			int settingHoverPointOffset = 10;

			int id = newGuiIncrementId(ad->gui);
			bool mPosActive = newGuiGoMousePosAction(ad->gui, rGraphs, 0);
			if(mPosActive && ad->hoveredVideo != -1) {
				scissorTestScreen(rGraphs);
				glEnable(GL_SCISSOR_TEST);

				int vi = ad->hoveredVideo;

				// Draw text top right.
				Font* font = getFont(as->font, as->fontSize);
				Vec4 c = ac->font3;
				Vec2 p = rChart.max - vec2(5);
				float os = 2;
				drawTextOutlined(fillString("%s", ad->videos[vi].title), font, p, os, c, vec4(0,1), vec2i(1,1)); p.y -= font->height;
				drawTextOutlined(fillString("%s", ad->videos[vi].dateString), font, p, os, c, vec4(0,1), vec2i(1,1)); p.y -= font->height;
				drawTextOutlined(fillString("%i", vi), font, p, os, c, vec4(0,1), vec2i(1,1)); p.y -= font->height;

				// Draw Point.
				glPointSize(settingHoverPointSize);
				// drawText(fillString("%i", ad->videos[vi].viewCount), font, ad->hoveredPoint + vec2(10,10), c, vec2i(-1,-1));
				char* text;
				if(ad->hoveredVideoStat > 10) text = fillString("%i.", (int)ad->hoveredVideoStat);
				else text = fillString("%f", ad->hoveredVideoStat);
				os = 1;

				// Get text pos and clamp to graph rect.
				Vec2 hoverPos = ad->hoveredPoint + vec2(settingHoverPointOffset,settingHoverPointOffset);
				Rect textRect = rectBLDim(hoverPos, getTextDim(text, font));
				Vec2 offset = rectInsideRectClamp(textRect, rGraphs);
				textRect = rectTrans(textRect, offset);
				hoverPos = rectBL(textRect);

				drawTextOutlined(text, font, hoverPos, os, c, vec4(0,1), vec2i(-1,-1));
				drawPoint(ad->hoveredPoint, ac->font3);
				glPointSize(1);

				glDisable(GL_SCISSOR_TEST);

				if(newGuiGoButtonAction(ad->gui, id, rGraphs, 0, Gui_Focus_MMiddle)) {
					if(vi != ad->selectedVideo) {
						ad->selectedPoint = ad->hoveredPoint;
						ad->selectedVideo = vi;
						downloadVideoSnippet(ad->curlHandle, &ad->videoSnippet, ad->videos + ad->selectedVideo);
					}
				}
			}
		}



		Font* titleFont = getFont(as->fontTitle, as->fontTitleSize);

		for(int i = 0; i < ad->camCount; i++) {
			Rect rGraph = graphRects[i];
			Vec2 tp = rectTL(rGraph) + vec2(as->titleOffset,-as->titleOffset);

			char* text;
			if(i == 0) text = "Views";
			else if(i == 1) text = "Likes";
			else if(i == 2) text = "Likes/Dislikes";
			else if(i == 3) text = "Comments";
			else text = "";

			float textWidth = getTextDim(text, titleFont).w;

			drawTextOutlined(text, titleFont, tp, 2, ac->font1, vec4(0,1), vec2i(-1,1));

			// Sort buttons.
			{
				char* text = "Sort";
				Rect r = rectTLDim(tp + vec2(textWidth + font->height/2, 0), vec2(getTextDim(text, font).w + 4, font->height));
				glEnable(GL_SCISSOR_TEST);

				if(newGuiQuickButton(ad->gui, r, 1, text, font, ac->buttons, ac->font1, r)) {
					if(ad->sortStat != i) {
						ad->startLoadFile = true;
						ad->sortByDate = false;
						ad->sortStat = i;
					}
				}

				glDisable(GL_SCISSOR_TEST);
			}
		}



		TIMER_BLOCK_BEGIN_NAMED(sidePanel, "SidePanel");

		// Side panel.
		if(ad->selectedVideo != -1) {

			// Graph selection line.
			{
				float x;
				if(ad->sortByDate) x = graphCamMapX(&ad->cams[0], ad->videos[ad->selectedVideo].date.n) + 1;
				else x = graphCamMapX(&ad->cams[0], ad->selectedVideo);

				glLineWidth(1);
				drawLine(vec2(x, rGraphs.bottom), vec2(x, rGraphs.top), ac->mouseHover);
			}

			// Panel width slider.
			{
				// Rect r = rectCenDim(rectL(rSidePanel), vec2(font->height, rectH(rSidePanel)-4));
				Rect r = rectCenDim(rectL(rSidePanel), vec2(as->resizeRegionSize, rectH(rSidePanel)));

				int event = newGuiGoDragAction(ad->gui, r, 1);
				if(event == 1) ad->gui->mouseAnchor.x = (ws->currentRes.w-input->mousePos.x) - ad->sidePanelWidth;
				if(event > 0) {
					ad->sidePanelWidth = (ws->currentRes.w-input->mousePos.x) - ad->gui->mouseAnchor.x;
					clamp(&ad->sidePanelWidth, as->sidePanelMinWidth, ws->currentRes.w*ad->sidePanelMax);
				}

				if(newGuiIsWasHotOrActive(ad->gui)) SetCursor(LoadCursor(0, IDC_SIZEWE));
			}

			Vec4 sidePanelColor = ac->sidePanel;
			drawRect(rSidePanel, sidePanelColor);

			float border = as->marginSidePanel;
			Rect rPanel = rectExpand(rSidePanel, -vec2(border,border));
			Font* font = getFont(as->font, as->fontSize);
			Font* font2 = getFont(as->fontComment, as->fontCommentSize);
			float xMid = rectCen(rPanel).x;
			float width = rectW(rPanel);
			float yPos = rPanel.top;
			Vec4 fc = ac->font1;

			Rect infoPanelTop = rPanel;
			infoPanelTop.min.y = infoPanelTop.top - font->height - as->marginButtonsHeight;

			bool closePanel = false;

			// Top gui.
			{
				glEnable(GL_SCISSOR_TEST);

				float offset = as->marginButtons;
				float divs[] = {40,0,0,40, 30};
				Rect buttons[arrayCount(divs)];
				newGuiDiv(rectW(infoPanelTop), divs, arrayCount(divs), offset);
				newGuiRectsFromWidths(infoPanelTop, divs, arrayCount(divs), buttons, offset);

				float z = 1;
				Vec4 buttonColor = ac->sidePanelButtons;

				if(ad->selectedVideo > 0) {
					Rect r = buttons[0];

					if(newGuiGoButtonAction(ad->gui, r, z)) {
						if(ad->selectedVideo != -1) {
							int newSelection = clampMin(ad->selectedVideo - 1, 0);
							if(newSelection != ad->selectedVideo) {
								ad->selectedVideo = newSelection;
								downloadVideoSnippet(ad->curlHandle, &ad->videoSnippet, ad->videos + ad->selectedVideo);
							}
						}
					}

					// drawTextBox(r, buttonColor + newGuiColorModB(ad->gui), "<-", font, fc, vec2i(0,0));

					drawTextBox(r, buttonColor + newGuiColorModB(ad->gui), "", font, fc, vec2i(0,0));
					drawTriangle(rectCen(r), font->height/3, vec2(-1,0), fc);
				}

				{
					Rect r = buttons[1];
					if(newGuiGoButtonAction(ad->gui, r, z)) {
						shellExecuteNoWindow(fillString("chrome https://www.youtube.com/watch?v=%s", ad->videos[ad->selectedVideo].id), false);
					}

					drawTextBox(r, buttonColor + newGuiColorModB(ad->gui), "Open in Chrome.", font, fc, vec2i(0,0));
				}

				{
					Rect r = buttons[2];
					if(newGuiGoButtonAction(ad->gui, r, z)) {
						shellExecuteNoWindow(fillString("C:\\Program Files (x86)\\Mozilla Firefox\\firefox.exe https://www.youtube.com/watch?v=%s", ad->videos[ad->selectedVideo].id), false);
					}

					drawTextBox(r, buttonColor + newGuiColorModB(ad->gui), "Open in Firefox.", font, fc, vec2i(0,0));
				}

				if(ad->selectedVideo != ad->playlist.count-1) {
					Rect r = buttons[3];
					if(newGuiGoButtonAction(ad->gui, r, z)) {
						if(ad->selectedVideo != -1) {
							int newSelection = clampMax(ad->selectedVideo + 1, ad->playlist.count - 1);
							if(newSelection != ad->selectedVideo) {
								ad->selectedVideo = newSelection;
								downloadVideoSnippet(ad->curlHandle, &ad->videoSnippet, ad->videos + ad->selectedVideo);
							}
						}
					}

					// drawTextBox(r, buttonColor + newGuiColorModB(ad->gui), "->", font, fc, vec2i(0,0));

					drawTextBox(r, buttonColor + newGuiColorModB(ad->gui), "", font, fc, vec2i(0,0));
					drawTriangle(rectCen(r), font->height/3, vec2(1,0), fc);
				}

				{
					Rect r = buttons[4];
					if(newGuiGoButtonAction(ad->gui, r, z)) {
						// ad->selectedVideo = -1;
						closePanel = true;
					}

					// drawTextBox(r, buttonColor + newGuiColorModB(ad->gui), "x", font, fc, vec2i(0,0));

					drawTextBox(r, buttonColor + newGuiColorModB(ad->gui), "", font, fc, vec2i(0,0));
					drawCross(rectCen(r), font->height/3, 2, vec2(0,1), fc);
				}

				glDisable(GL_SCISSOR_TEST);
			}

			yPos -= rectH(infoPanelTop) + 8;

			glEnable(GL_SCISSOR_TEST);
			scissorTestScreen(rPanel);

			YoutubeVideo* sv = ad->videos + ad->selectedVideo;
			VideoSnippet* sn = &ad->videoSnippet;

			drawText(fillString("Index: %i, VideoId: %s", ad->selectedVideo, sv->id), font, vec2(xMid, yPos), fc, vec2i(0,1));

			yPos -= font->height;


			Vec2 imageDim = vec2(sn->thumbnailTexture.dim);
			if(imageDim.w > rectW(rPanel)) imageDim = vec2(rectW(rPanel), rectW(rPanel)*(imageDim.h/imageDim.w));
			Rect imageRect = rectCenDim(rectCenX(rPanel), yPos - imageDim.h/2, imageDim.w, imageDim.h);
			drawRect(imageRect, rect(0,0,1,1), sn->thumbnailTexture.id);
			yPos -= rectH(imageRect);
			yPos -= font->height/2;

			drawText(sv->title, font, vec2(xMid, yPos), fc, vec2i(0,1), width);
			yPos -= getTextHeight(sv->title, font, vec2(xMid, yPos), width);
			yPos -= font->height;

			float dislikePercent = divZero(sv->dislikeCount,(float)(sv->likeCount+sv->dislikeCount));
			glLineWidth(1);
			drawRectOutlined(rectLDim(rPanel.left, yPos, rectW(rPanel)*(1-dislikePercent), font->height*0.6f), ac->likes, vec4(0,1), 0);
			drawRectOutlined(rectRDim(rPanel.right, yPos, rectW(rPanel)*dislikePercent, font->height*0.6f), ac->dislikes, vec4(0,1), 0);
			yPos -= font->height;

			{
				glEnable(GL_SCISSOR_TEST);

				int lineCount = 6;
				scissorTestScreen(rectTLDim(rPanel.left, yPos, rectW(rPanel)/2 - 1, font->height*lineCount));
				drawText("Date:\nTime:\nViews:\nLikes/Dislikes:\nComments:\nFavorites:", font, vec2(rPanel.left, yPos), fc, vec2i(-1,1));

				scissorTestScreen(rectTRDim(rPanel.right, yPos, rectW(rPanel)/2 - 1, font->height*lineCount));
				char* t;
				for(int i = 0; i < lineCount; i++) {
					if(i == 0) t = fillString("%s%i..%s%i..%s%i", sv->date.d<10?"0":"", sv->date.d, sv->date.m<10?"0":"", sv->date.m, sv->date.y<10?"200":"20", sv->date.y);
					if(i == 1) t = fillString("%s%i:%s%i:%s%i.", sv->date.h<10?"0":"", sv->date.h, sv->date.mins<10?"0":"", sv->date.mins, sv->date.secs<10?"0":"", sv->date.secs);
					if(i == 2) t = fillString("%i.", sv->viewCount);
					if(i == 3) t = fillString("%i | %i - %f", sv->likeCount, sv->dislikeCount, divZero(sv->dislikeCount,(float)(sv->likeCount+sv->dislikeCount)));
					if(i == 4) t = fillString("%i", sv->commentCount);
					if(i == 5) t = fillString("%i", sv->favoriteCount);
				
					drawText(t, font, vec2(rPanel.right, yPos), fc, vec2i(1,1)); 
					yPos -= font->height;
				}

				glDisable(GL_SCISSOR_TEST);
			}

			yPos -= font->height/2;

			drawText("Comments", font, vec2(xMid, yPos), fc, vec2i(0,1));
			yPos -= font->height;

			{
				static float commentHeight = 300;
				bool hasScrollbar = commentHeight > 0;

				float scrollBarWidth = hasScrollbar?as->commentScrollbarWidth:0;
				Rect commentScrollRect = rect(rPanel.left, rPanel.bottom, rPanel.right, yPos);
				Rect commentRect = rect(rPanel.left, rPanel.bottom, rPanel.right-scrollBarWidth, yPos);
				Rect scrollBarRect = rect(rPanel.right - scrollBarWidth, rPanel.bottom, rPanel.right, yPos);

				drawRect(commentRect, ac->comments);
				drawRect(scrollBarRect, ac->comments);
				Vec4 scrollBarColor = ac->commentScroll;

				rectExpand(&commentRect, vec2(-as->marginComments,0));

				static float scrollValue = 0.0f;
				static Vec2 mouseAnchor;
				float z = 1;
		
				float sliderSize = (rectH(scrollBarRect) / (rectH(commentRect)+commentHeight)) * rectH(scrollBarRect);
				clampMin(&sliderSize, 20);

				// Scroll with mousewheel.
				{
					if(newGuiGoButtonAction(ad->gui, commentScrollRect, z, Gui_Focus_MWheel)) {
						float scrollAmount = font2->height;
						if(input->keysDown[KEYCODE_SHIFT]) scrollAmount *= 2;
						scrollValue = clamp(scrollValue + -input->mouseWheel*(scrollAmount/commentHeight), 0, 1);
					}
				}

				Rect slider;
				int sliderId;
				// Scroll with handle.
				{
					slider = newGuiCalcSlider(1-scrollValue, scrollBarRect, sliderSize, 0, 1, false);
					int event = newGuiGoDragAction(ad->gui, slider, z);
					sliderId = newGuiCurrentId(ad->gui);
					if(event == 1) mouseAnchor = input->mousePosNegative - rectCen(slider);
					if(event > 0) {
						scrollValue = 1-newGuiSliderGetValue(input->mousePosNegative - mouseAnchor, scrollBarRect, sliderSize, 0, 1, false);
						slider = newGuiCalcSlider(1-scrollValue, scrollBarRect, sliderSize, 0, 1, false);
					}
				}

				// Scroll with background drag.
				{
					int event = newGuiGoDragAction(ad->gui, commentRect, z);
					if(event == 1) mouseAnchor.y = input->mousePosNegative.y - (scrollValue*commentHeight);
					if(event > 0) {
						scrollValue = (input->mousePosNegative.y - mouseAnchor.y)/commentHeight;
						clamp(&scrollValue, 0, 1);

						slider = newGuiCalcSlider(1-scrollValue, scrollBarRect, sliderSize, 0, 1, false);
					}
				}

				rectExpand(&slider, -as->marginSlider);
				if(hasScrollbar) drawRectRounded(slider, scrollBarColor + newGuiColorModId(ad->gui, sliderId), rectW(slider)/2);

				float startYPos = yPos + scrollValue*commentHeight;
				if(hasScrollbar) yPos = startYPos;

				glEnable(GL_SCISSOR_TEST);
				scissorTestScreen(commentRect);
				float wrapWidth = rectW(commentRect);
				for(int i = 0; i < sn->selectedCommentCount; i++) {
					drawText(sn->selectedTopComments[i], font2, vec2(commentRect.left, yPos), fc, vec2i(-1,1), wrapWidth);
					yPos -= getTextHeight(sn->selectedTopComments[i], font2, vec2(xMid, yPos), wrapWidth);
					
					drawText(fillString("Likes: %i, Replies: %i", sn->selectedCommentLikeCount[i], sn->selectedCommentReplyCount[i]), font2, vec2(commentRect.right, yPos), ac->font2, vec2i(1,1), wrapWidth);
					yPos -= font2->height;
				}

				commentHeight = startYPos - yPos - (rectH(commentRect));
			}

			glDisable(GL_SCISSOR_TEST);

			if(closePanel) ad->selectedVideo = -1;
		}

		TIMER_BLOCK_END(sidePanel);

		// Filters gui.
		{
			char* labels[] = {"Panel (F1)", "Reset", "Draw Mode:", "Avg. (width/res months):", "Filter (Incl./Excl.):"};
			int li = 0;
			float bp = font->height;
			float widths[] = {getTextDim(labels[li++], font).x+bp, getTextDim(labels[li++], font).x+bp, 0, getTextDim(labels[li++], font).x, 80, getTextDim(labels[li++], font).x, 40, 40, getTextDim(labels[li++], font).x, 200, 200};
			Rect regions[arrayCount(widths)];

			glEnable(GL_SCISSOR_TEST);
			Rect scissor = rFilters;

			float offset = 2;
			Rect r = rectExpand(rFilters, vec2(-offset, -offset)*2);

			float padding = font->height/3;
			newGuiDiv(rectW(r), widths, arrayCount(widths), padding);
			newGuiRectsFromWidths(r, widths, arrayCount(widths), regions, padding);

			Vec4 bc = ac->buttons;
			Vec4 ec = ac->editBackground;
			Vec4 tc = ac->font1;

			TextEditSettings editSettings = {true, true, ws->currentRes.h, 2, ec, tc, ac->editSelection, ac->editCursor};

			float z = 0;
			int ri = 0;
			li = 0;

			if(newGuiQuickButton(ad->gui, regions[ri++], z, labels[li++], font, bc, tc, scissor)) ds->showMenu = !ds->showMenu;
			if(newGuiQuickButton(ad->gui, regions[ri++], z, labels[li++], font, bc, tc, scissor)) {
				ad->sortByDate = true;
				ad->sortStat = -1;
				ad->startLoadFile = true;
			}

			ri++;

			drawQuickTextBox(regions[ri++], labels[li++], font, vec4(0,0), tc, vec2i(0,0), scissor);
			newGuiQuickSlider(ad->gui, regions[ri++], z, &ad->graphDrawMode, 0,2, 20, font, ac->font2, tc, ac->buttons, scissor);

			drawQuickTextBox(regions[ri++], labels[li++], font, vec4(0,0), tc, vec2i(0,0), scissor);

			bool updateStats = false;

			if(newGuiQuickTextEdit(ad->gui, regions[ri++], &ad->statTimeSpan, z, font, editSettings, scissor)) updateStats = true;
			if(newGuiIsWasHotOrActive(ad->gui)) SetCursor(LoadCursor(0, IDC_IBEAM));
			if(newGuiQuickTextEdit(ad->gui, regions[ri++], &ad->statWidth, z, font, editSettings, scissor)) updateStats = true;
			if(newGuiIsWasHotOrActive(ad->gui)) SetCursor(LoadCursor(0, IDC_IBEAM));

			drawQuickTextBox(regions[ri++], labels[li++], font, vec4(0,0), tc, vec2i(0,0), scissor);

			if(newGuiQuickTextEdit(ad->gui, regions[ri++], ad->inclusiveFilter, z, font, editSettings, scissor, arrayCount(ad->inclusiveFilter))) 
				ad->startLoadFile = true;
			if(newGuiIsWasHotOrActive(ad->gui)) SetCursor(LoadCursor(0, IDC_IBEAM));
			if(newGuiQuickTextEdit(ad->gui, regions[ri++], ad->exclusiveFilter, z, font, editSettings, scissor, arrayCount(ad->exclusiveFilter))) 
				ad->startLoadFile = true;
			if(newGuiIsWasHotOrActive(ad->gui)) SetCursor(LoadCursor(0, IDC_IBEAM));


			if(updateStats) calculateAverages(ad->videos, ad->playlist.count, &ad->averagesLineGraph, ad->statTimeSpan, ad->statWidth, ad->cams[0].xMax - ad->cams[0].xMin);

			glDisable(GL_SCISSOR_TEST);	
		}


		// Draw outlines.
		{
			glLineWidth(1.0f);

			Vec4 color = ac->outlines;
			float offset = 0;
			for(int camIndex = 0; camIndex < ad->camCount; camIndex++) {
				drawRectOutline(leftTextRects[camIndex], color, offset);
			}

			// Rect r = leftTextRects[1];
			// Rect r2 = leftTextRects[2];

			// float g = 0.15f;
			// Vec4 c1 = vec4(0.5f,g,g,1);
			// Vec4 c2 = vec4(0.40f,g,g,1);
			// float h = 5;
			// drawRectNewColored(rectBDim(rectB(r), vec2(rectW(r), h)), c1,c2,c2,c1);
			// drawRectNewColored(rectTDim(rectT(r2), vec2(rectW(r2), h)), c2,c1,c1,c2);

			drawRectOutline(rBottomText, color, offset);
			drawRectOutline(rSidePanel, color, offset);
			drawRectOutline(rFilters, color, offset);

			drawRectOutline(rectSetTR(rChart, rectTL(rBottomText)), color, offset);

		}
	}

	TIMER_BLOCK_END(appMain);

	endOfMainLabel:



	if(false)
	{
		NewGui* gui = ad->gui;

		Font* font = getFont(ad->appSettings.font, ad->appSettings.fontSize);

		static Vec2 panelPos = vec2(800,-100);
		static Vec2 panelDim = vec2(400,600);

		Rect panelRect = rectTLDim(panelPos, panelDim);
		float z = 2;

		newGuiSetHotAllMouseOver(ad->gui, panelRect, z);

		static bool resizeMode = false;
		int event = newGuiGoDragAction(gui, panelRect, z);
		if(event == 1) {
			resizeMode = input->keysDown[KEYCODE_CTRL];

			if(!resizeMode) gui->mouseAnchor = input->mousePosNegative - panelPos;
			else gui->mouseAnchor = input->mousePos - panelDim;
		}
		if(event > 0) {
			if(!resizeMode) panelPos = input->mousePosNegative - gui->mouseAnchor;
			else panelDim = input->mousePos - gui->mouseAnchor;

			panelRect = rectTLDim(panelPos, panelDim);
		}

		Rect sr = getScreenRect(ws);
		if(resizeMode) {
			clamp(&panelDim.w, 100, rectW(sr) - panelPos.x);
			clamp(&panelDim.h, 100, rectH(sr) + panelPos.y);
			panelRect = rectTLDim(panelPos, panelDim);
		} else {
			clamp(&panelPos.x, sr.left, sr.right-rectW(panelRect));
			clamp(&panelPos.y, sr.bottom+rectH(panelRect), sr.top);
			panelRect = rectTLDim(panelPos, panelDim);
		}

		// float offset = 4;
		// float divs[] = {40,0,0,40, 30};
		// Rect buttons[arrayCount(divs)];
		// newGuiDiv(rectW(infoPanelTop), divs, arrayCount(divs), offset);
		// newGuiRectsF

		// drawRectRounded(panelRect, vec4(0.4,0.2,0.6,1) + newGuiColorMod(gui), 5);
		drawRectRounded(panelRect, vec4(0.4,0.2,0.6,1), 5);
	
		{
			glEnable(GL_SCISSOR_TEST);

			Vec2 margin = vec2(10,10);
			Rect insideRect = rectExpand(panelRect, vec2(-margin.x, -margin.y));
			float titleHeight = font->height*1.2f;

			Rect titleRect = rectSetB(insideRect, insideRect.top - titleHeight);
			drawTextBox(titleRect, vec4(0,0), "App", font, vec4(0.9f,1.0f), vec2i(0,0));

			Rect rClose = rectSetL(titleRect, titleRect.right-rectH(titleRect));
			drawRect(rClose, vec4(0.4f,0,0,1));
			rectExpand(&rClose, -rectH(rClose)*0.4f);
			glLineWidth(2);
			drawLine(rectBL(rClose), rectTR(rClose),  vec4(0.7,1));
			drawLine(rectTL(rClose), rectBR(rClose),  vec4(0.7,1));

			insideRect.top -= titleHeight;

			Vec4 bc = vec4(0.2f,0,0.5,1);
			Vec4 tc = vec4(0.9f, 1);

			Rect scissor = insideRect;
			scissorTestScreen(scissor);

			drawRect(insideRect, bc);

			Vec2 writePos = rectTL(insideRect);

			float rowHeight = font->height;
			float rowOffset = 2;
			Rect region;

			region = rectTLDim(writePos, vec2(rectW(insideRect), rowHeight));
			writePos.y -= rowHeight;
			newGuiQuickButton(gui, region, z, "Button", font, bc + vec4(0.1f,0,0.2f,0), tc, scissor);

			region = rectTLDim(writePos, vec2(rectW(insideRect), rowHeight));
			writePos.y -= rowHeight + rowOffset;
			static float myFloat = 40.2f;
			newGuiQuickSlider(gui, region, z, &myFloat, 20, 99999999, 20, font, vec4(0.5f,1), vec4(0.9,1), vec4(0.3f,0.5f,0.0f,1), scissor);

			region = rectTLDim(writePos, vec2(rectW(insideRect), rowHeight));
			writePos.y -= rowHeight + rowOffset;
			static int myInt = 4;
			newGuiQuickSlider(gui, region, z, &myInt, 2, 10, 20, font, vec4(0.5f,1), vec4(0.9,1), vec4(0.3f,0.5f,0.0f,1), scissor);

			region = rectTLDim(writePos, vec2(rectW(insideRect), rowHeight));
			writePos.y -= rowHeight + rowOffset;
			static char myText[50] = "TextBoxChar";
			TextEditSettings editSettings = {true, true, ws->currentRes.h, 2, bc - vec4(0.1f,0,0.1f,0), tc, vec4(0.24,0.41,0.59,1), vec4(0.2f,0.7f,0,1)};
			newGuiQuickTextEdit(ad->gui, region, myText, z, font, editSettings, scissor, arrayCount(myText));

			region = rectTLDim(writePos, vec2(rectW(insideRect), rowHeight));
			writePos.y -= rowHeight + rowOffset;
			static float myFloat2;
			newGuiQuickTextEdit(ad->gui, region, &myFloat2, z, font, editSettings, scissor);

			region = rectTLDim(writePos, vec2(rectW(insideRect), rowHeight));
			writePos.y -= rowHeight + rowOffset;
			static int myInt2;
			newGuiQuickTextEdit(ad->gui, region, &myInt2, z, font, editSettings, scissor);

			glDisable(GL_SCISSOR_TEST);
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

	newGuiEnd(ad->gui);

	{
		// Font* font = getFont(FONT_CALIBRI, 20);
		// drawRect(rectBLDim(getScreenRect(ws).min, vec2(font->tex.dim)), vec4(1,1), rect(0,0,1,1), font->tex.id);

		// Rect r, uv;
		// getTextQuad((int)'X', font, vec2(0,0), &r, &uv);
		// int stop = 234;
	}

	if(ds->savedBuffer == 0) {
		int stop = 234;
	}

	debugMain(ds, appMemory, ad, reload, isRunning, init, threadQueue);

	// Render.
	{
		TIMER_BLOCK_NAMED("Render");

		Vec2i frameBufferRes = getFrameBuffer(FRAMEBUFFER_2dNoMsaa)->colorSlot[0]->dim;
		Vec2i res = ws->currentRes;
		Rect frameBufferUV = rect(0,(float)res.h/frameBufferRes.h,(float)res.w/frameBufferRes.w,0);


		blitFrameBuffers(FRAMEBUFFER_DebugMsaa, FRAMEBUFFER_DebugNoMsaa, res, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		blitFrameBuffers(FRAMEBUFFER_2dMsaa, FRAMEBUFFER_2dNoMsaa, res, GL_COLOR_BUFFER_BIT, GL_LINEAR);

		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquation(GL_FUNC_ADD);
		bindFrameBuffer(FRAMEBUFFER_2dNoMsaa);

		glLoadIdentity();
		glViewport(0,0, res.w, res.h);
		glOrtho(0,1,1,0, -1, 1);

		drawRect(rect(0, 1, 1, 0), vec4(1,1,1,ds->guiAlpha), frameBufferUV, getFrameBuffer(FRAMEBUFFER_DebugNoMsaa)->colorSlot[0]->id);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquation(GL_FUNC_ADD);


		#if USE_SRGB 
			glEnable(GL_FRAMEBUFFER_SRGB);
		#endif 

		if(ad->screenShotMode) {
			ds->showMenu = true;
			ad->updateFrameBuffers = true;

			bindFrameBuffer(FRAMEBUFFER_ScreenShot);
			glClearColor(0,0,0,1);
			glClear(GL_COLOR_BUFFER_BIT);

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
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glLoadIdentity();
		glViewport(0,0, res.w, res.h);
		glOrtho(0,1,1,0, -1, 1);
		drawRect(rect(0, 1, 1, 0), frameBufferUV, getFrameBuffer(FRAMEBUFFER_2dNoMsaa)->colorSlot[0]->id);

		#if USE_SRGB
			glDisable(GL_FRAMEBUFFER_SRGB);
		#endif

	}

	// Swap window background buffer.
	{
		TIMER_BLOCK_NAMED("Swap");
		if(ad->updateWindow) {
			MoveWindow(windowHandle, ad->updateWindowX, ad->updateWindowY, ad->updateWidth, ad->updateHeight, true);
			ad->updateWindow = false;
		}

		// Sleep untill monitor refresh.
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

	// debugMain(ds, appMemory, ad, reload, isRunning, init, threadQueue);

	// debugUpdatePlayback(ds, appMemory);

	// @App End.

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

	if(input->keysPressed[KEYCODE_F1]) ds->showMenu = !ds->showMenu;
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

		gui->div(vec2(0,fontSize));
		gui->label("App", 1, gui->colors.sectionColor, vec4(0,0,0,1));
		if(gui->button("x",0,1, gui->colors.sectionColor)) ds->showMenu = false;

		gui->div(0,0,0);
		if(gui->button("Options", (int)(sectionMode == 0) + 1)) sectionMode = 0;
		if(gui->button("Videos", (int)(sectionMode == 1) + 1)) sectionMode = 1;
		if(gui->button("Playlists", (int)(sectionMode == 2) + 1)) sectionMode = 2;

		if(sectionMode == 0) {
			gui->empty();

			gui->div(vec4(0,0,0,0));
			if(gui->button("Screenshot.")) {
				ad->startScreenShot = true;
				ds->showMenu = false;
			}
			gui->textBoxChar(ad->screenShotFilePath, 0, 49);

			int maxTextureSize;
			glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

			if(gui->textBoxInt(&ad->screenShotDim.w)) clampInt(&ad->screenShotDim.w, 2, maxTextureSize-1);
			if(gui->textBoxInt(&ad->screenShotDim.h)) clampInt(&ad->screenShotDim.h, 2, maxTextureSize-1);

			gui->empty();

			gui->div(vec3(0,0,0));
			gui->label("Playlist folder contents: ", 0);
			if(gui->button("Update folder.")) {
				ad->playlistFolderCount = 0;
				loadPlaylistFolder(ad->playlistFolder, &ad->playlistFolderCount);
			}
			if(gui->button("Open folder.")) {
				shellExecute("start ..\\playlists");
			}
			for(int i = 0; i < ad->playlistFolderCount; i++) {
				YoutubePlaylist* playlist = ad->playlistFolder + i;
				gui->div(0,0.1f,0,0.1f);
				if(gui->button(fillString("%s", playlist->title))) {
					memCpy(&ad->playlist, ad->playlistFolder + i, sizeof(YoutubePlaylist));
					ad->playlist.count = playlist->count;
					ad->startLoadFile = true;

					strCpy(ad->downloadPlaylist.title, ad->playlist.title);
					strCpy(ad->downloadPlaylist.id, ad->playlist.id);
				}
				gui->label(fillString("%i", playlist->count), 0);
				gui->label(playlist->id);
				if(gui->button("Del")) {
					removePlaylistFile(playlist);
					ad->playlistFolderCount = 0;
					loadPlaylistFolder(ad->playlistFolder, &ad->playlistFolderCount);
				}
			}

			gui->empty();

			gui->div(vec2(0,0));
			gui->label("Loaded playlist.", 0);
			gui->label(ad->playlist.title);
			gui->div(vec2(0,0));
			gui->label(ad->playlist.id);
			gui->label(fillString("VideoCount: %i", ad->playlist.count));

			gui->empty();

			gui->div(vec2(0.2f,0));
			gui->label("Playlist Title:", 0);
			gui->label(ad->downloadPlaylist.title, 0);
			gui->div(vec2(0.2f,0));
			gui->label("Url: ", 0);
			gui->textBoxChar(ad->downloadPlaylist.id, strLen(ad->downloadPlaylist.id), 50);

			gui->div(0.2f,0.2f,0);
			gui->label("Count: ", 0);
			gui->textBoxInt(&ad->downloadPlaylist.count);
			if(gui->button("Get video count from url.")) {
				char* buffer = (char*)getTMemory(1000);
				requestPlaylistItems(ad->curlHandle, buffer, ad->downloadPlaylist.id, 1);
				ad->downloadPlaylist.count = strToInt(getJSONInt(buffer, "totalResults"));
			}

			gui->div(vec3(0,0,0));
			if(gui->button("Download and Save.")) {
				ad->startDownload = true;
				ad->startLoadFile = true;
			}
			// if(gui->button("Save to file.")) {
			// 	savePlaylistToFile(&ad->playlist, ad->videos, ad->playlist.count);

			// 	ad->playlistFolderCount = 0;
			// 	loadPlaylistFolder(ad->playlistFolder, &ad->playlistFolderCount);
			// }
			if(gui->button("Load from file.")) ad->startLoadFile = true;
			if(gui->button("Update.")) {
				ad->startDownload = true;
				ad->update = true;
			}

			if(ad->activeDownload) {
				gui->label(fillString("Active Download: %i/%i", ad->dInfo.progressIndex, ad->dInfo.count));
			}

			gui->empty();

			gui->label("QuickSearch:", 0);
			gui->div(0,0,0,50);
			gui->textBoxChar(ad->searchString, strLen(ad->searchString), 50);
			if(gui->button("Playlists.")) {
				quickSearch(ad->curlHandle, ad->searchResults, &ad->searchResultCount, ad->searchString, false);
				ad->lastSearchMode = 1;
			}
			if(gui->button("Channels.")) {
				quickSearch(ad->curlHandle, ad->searchResults, &ad->searchResultCount, ad->searchString);
				ad->lastSearchMode = 0;
			}
			if(gui->button("X")) {
				ad->searchResultCount = 0;
			}

			if(ad->searchResultCount > 0) {
				gui->label("Channels");

				float leftPad = 20;
				for(int i = 0; i < ad->searchResultCount; i++) {
					SearchResult* sResult = ad->searchResults + i;

					if(gui->button(fillString("%i: %s", i, sResult->title), 0, 0)) {
						if(ad->lastSearchMode == 0) {
							strCpy(ad->channelId, sResult->id);
						} else {
							strCpy(ad->downloadPlaylist.id, sResult->id);
							strCpy(ad->downloadPlaylist.title, sResult->title);
						}
					}
				}
			}

			{
				gui->empty();
	
				// gui->div(vec2(0,0));
				// if(gui->button("Get id from username.")) {
				// 	char* channelId = downloadChannelIdFromUserName(ad->curlHandle, ad->userName);
				// 	strCpy(ad->channelId, channelId);
				// }
				// gui->textBoxChar(ad->userName, strLen(ad->userName), 50);

				gui->label("Channel Playlists:", 0);
				gui->div(vec2(0.2f,0));
				gui->label("Channel Id:", 0);
				gui->textBoxChar(ad->channelId, strLen(ad->channelId), 50);

				gui->div(0.2f,0.2f,0);
				gui->label("Count: ", 0);
				gui->textBoxInt(&ad->playlistDownloadCount);
				if(gui->button("Get playlist count.")) {
					ad->playlistDownloadCount = downloadChannelPlaylistCount(ad->curlHandle, ad->channelId);
				}
				
				gui->div(vec2(0,50));
				if(gui->button("Load Playlist from Channel")) {
					downloadChannelPlaylists(ad->curlHandle, ad->playlists, &ad->playlistCount, ad->channelId, ad->playlistDownloadCount);
				}
				if(gui->button("X")) {
					ad->playlistCount = 0;
				}

				if(ad->playlistCount > 0) {
					gui->label("Playlists");

					float leftPad = 20;
					for(int i = 0; i < ad->playlistCount; i++) {
						YoutubePlaylist* playlist = ad->playlists + i;

						if(gui->button(fillString("%i: %s. (%i.)", i, playlist->title, playlist->count), 0, 0)) {
							strCpy(ad->downloadPlaylist.id, playlist->id);
							strCpy(ad->downloadPlaylist.title, playlist->title);
							ad->downloadPlaylist.count = playlist->count;
						}
					}
				}
			}
		}

		if(sectionMode == 1) {
			gui->label(fillString("VideoCount: %i.", ad->playlist.count), 0);

			float leftPad = 20;
			for(int i = 0; i < ad->playlist.count; i++) {
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
		float fontSize = 24;
		Font* font = getFont(FONT_CALIBRI, fontSize);
		Vec4 color = vec4(1,0.5f,0,1);

		float y = -fontSize/2;
		for(int i = 0; i < ds->notificationCount; i++) {
			char* note = ds->notificationStack[i];
			drawText(note, font, vec2(ws->currentRes.w/2, y), color, vec2i(0,0), 0, 2);
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
