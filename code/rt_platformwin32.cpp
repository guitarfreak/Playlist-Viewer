#pragma once

enum Keycode {
	KEYCODE_CTRL = 0,
	KEYCODE_CTRL_RIGHT,
	KEYCODE_SHIFT,
	KEYCODE_SHIFT_RIGHT,
	KEYCODE_ALT,
	KEYCODE_CAPS,
	KEYCODE_TAB,
	KEYCODE_SPACE,
	KEYCODE_RETURN,
	KEYCODE_ESCAPE,
	KEYCODE_BACKSPACE,
	KEYCODE_DEL,
	KEYCODE_HOME,
	KEYCODE_END,
	KEYCODE_PAGEUP,
	KEYCODE_PAGEDOWN,
	KEYCODE_UP,
	KEYCODE_DOWN,
	KEYCODE_LEFT,
	KEYCODE_RIGHT,

	KEYCODE_A,
	KEYCODE_B,
	KEYCODE_C,
	KEYCODE_D,
	KEYCODE_E,
	KEYCODE_F,
	KEYCODE_G,
	KEYCODE_H,
	KEYCODE_I,
	KEYCODE_J,
	KEYCODE_K,
	KEYCODE_L,
	KEYCODE_M,
	KEYCODE_N,
	KEYCODE_O,
	KEYCODE_P,
	KEYCODE_Q,
	KEYCODE_R,
	KEYCODE_S,
	KEYCODE_T,
	KEYCODE_U,
	KEYCODE_V,
	KEYCODE_W,
	KEYCODE_X,
	KEYCODE_Y,
	KEYCODE_Z,

	KEYCODE_0,
	KEYCODE_1,
	KEYCODE_2,
	KEYCODE_3,
	KEYCODE_4,
	KEYCODE_5,
	KEYCODE_6,
	KEYCODE_7,
	KEYCODE_8,
	KEYCODE_9,

	KEYCODE_F1,
	KEYCODE_F2,
	KEYCODE_F3,
	KEYCODE_F4,
	KEYCODE_F5,
	KEYCODE_F6,
	KEYCODE_F7,
	KEYCODE_F8,
	KEYCODE_F9,
	KEYCODE_F10,
	KEYCODE_F11,
	KEYCODE_F12,

	KEYCODE_COUNT,
};

struct Input {
	bool firstFrame;
	Vec2 mousePos;
	Vec2 mousePosNegative;
	
	int mouseDeltaX, mouseDeltaY; // These are useless.

	Vec2 lastMousePos;
	Vec2 mouseDelta;
	int mouseWheel;
	bool mouseButtonPressed[8];
	bool mouseButtonDown[8];
	bool mouseButtonReleased[8];

	bool keysDown[KEYCODE_COUNT];
	bool keysPressed[KEYCODE_COUNT];
	char inputCharacters[32];
	int inputCharacterCount;

	bool mShift, mCtrl, mAlt;

	bool anyKey;
};



#define WIN_KEY_NUMERIC_START 0x30
#define WIN_KEY_NUMERIC_END 0x39
#define WIN_KEY_LETTERS_START 0x41
#define WIN_KEY_LETTERS_END 0x5a
#define WIN_KEY_F_START 0x70
#define WIN_KEY_F_END 0x7B

int vkToKeycode(int vk) {

	switch(vk) {
		case VK_CONTROL: return KEYCODE_CTRL;
		case VK_RCONTROL: return KEYCODE_CTRL_RIGHT;
		case VK_SHIFT: return KEYCODE_SHIFT;
		case VK_RSHIFT: return KEYCODE_SHIFT_RIGHT;
		case VK_MENU: return KEYCODE_ALT;
		case VK_CAPITAL: return KEYCODE_CAPS;
		case VK_TAB: return KEYCODE_TAB;
		case VK_SPACE: return KEYCODE_SPACE;
		case VK_RETURN: return KEYCODE_RETURN;
		case VK_ESCAPE: return KEYCODE_ESCAPE;
		case VK_BACK: return KEYCODE_BACKSPACE;
		case VK_DELETE: return KEYCODE_DEL;
		case VK_HOME: return KEYCODE_HOME;
		case VK_END: return KEYCODE_END;
		case VK_PRIOR: return KEYCODE_PAGEUP;
		case VK_NEXT: return KEYCODE_PAGEDOWN;
		case VK_UP: return KEYCODE_UP;
		case VK_DOWN: return KEYCODE_DOWN;
		case VK_LEFT: return KEYCODE_LEFT;
		case VK_RIGHT: return KEYCODE_RIGHT;

		default: {
				 if(vk >= WIN_KEY_NUMERIC_START && vk <= WIN_KEY_NUMERIC_END) return KEYCODE_0 + vk - WIN_KEY_NUMERIC_START;
			else if(vk >= WIN_KEY_LETTERS_START && vk <= WIN_KEY_LETTERS_END) return KEYCODE_A + vk - WIN_KEY_LETTERS_START;
			else if(vk >= WIN_KEY_F_START 		&& vk <= WIN_KEY_F_END) 	  return KEYCODE_F1 + vk - WIN_KEY_F_START;
		}
	}

	return -1;
}

void initInput(Input* input) {
    *input = {};

    input->firstFrame = true;
}

enum RequestType {
	REQUEST_TYPE_GET = 0,
	REQUEST_TYPE_POST,
	REQUEST_TYPE_COUNT,
};

struct HttpRequest {
	char* link;
	RequestType type;
	char* additionalBodyContent;

	char* contentBuffer;
	char* contentFile;
	char* contentFilePath;
	char* headerResponseFile;

	bool finished;
	int size;
	float progress;

	bool stopProcess;
};

enum ShellCommand {
	SHELLCOMMAND_FILE,
	SHELLCOMMAND_PATH,
	SHELLCOMMAND_URL,
	SHELLCOMMAND_COUNT,
};


// void atomicAdd(unsigned int* n) {
void atomicAdd(volatile unsigned int* n) {
	InterlockedIncrement(n);
}

void atomicSub(volatile unsigned int* n) {
	InterlockedDecrement(n);
}

struct ThreadQueue;

int getThreadId() {
	int tId = GetCurrentThreadId();
	return tId;
}

struct SystemData {
	WindowsData windowsData;
	HINSTANCE instance;
	HDC deviceContext;
	HWND windowHandle;
	
	// 1. Misc, 2. CubeMaps, 3. Minecraft
	HANDLE folderHandles[3]; 
};

void systemDataInit(SystemData* sd, HINSTANCE instance) {
	sd->instance = instance;
}

LRESULT CALLBACK mainWindowCallBack(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch(message) {
        case WM_DESTROY: {
            PostMessage(window, message, wParam, lParam);
        } break;

        case WM_CLOSE: {
            PostMessage(window, message, wParam, lParam);
        } break;

        case WM_QUIT: {
            PostMessage(window, message, wParam, lParam);
        } break;

        case WM_KILLFOCUS: {
        	DWORD id = GetThreadId(GetCurrentThread());
            PostThreadMessage(id, message, wParam, lParam);

            return DefWindowProc(window, message, wParam, lParam);
        } break;

        case WM_SETCURSOR: {
            SetCursor(GetCursor());
            return true;
        } break;

        default: {
            return DefWindowProc(window, message, wParam, lParam);
        } break;
    }

    return 1;
}

struct MonitorData {
	Rect fullRect;
	Rect workRect;
	HMONITOR handle;
};

struct WindowSettings {
	Vec2i res;
	Vec2i fullRes;
	bool fullscreen;
	uint style;
	WINDOWPLACEMENT g_wpPrev;

	MonitorData monitors[3];
	int monitorCount;

	Vec2i currentRes;
	float aspectRatio;

	bool customCursor;
};

void setCursor(WindowSettings* ws, LPCSTR type) {
	SetCursor(LoadCursor(0, type));
	ws->customCursor = true;
}

BOOL CALLBACK monitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {

	MONITORINFO mi = { sizeof(MONITORINFO) };
	GetMonitorInfo(hMonitor, &mi);

	WindowSettings* ws = (WindowSettings*)(dwData);
	MonitorData* md = ws->monitors + ws->monitorCount;
	md->fullRect = rect(mi.rcMonitor.left, mi.rcMonitor.bottom, mi.rcMonitor.right, mi.rcMonitor.top);
	md->workRect = rect(mi.rcWork.left, mi.rcWork.bottom, mi.rcWork.right, mi.rcWork.top);
	md->handle = hMonitor;
	ws->monitorCount++;

	return true;
}

void initSystem(SystemData* systemData, WindowSettings* ws, WindowsData wData, Vec2i res, bool resizable, bool maximizable, bool visible, int monitor = 0) {
	TIMER_BLOCK();

	systemData->windowsData = wData;

	EnumDisplayMonitors(0, 0, monitorEnumProc, ((LPARAM)ws));

	ws->currentRes = res;
	ws->fullscreen = false;
	ws->fullRes.x = GetSystemMetrics(SM_CXSCREEN);
	ws->fullRes.y = GetSystemMetrics(SM_CYSCREEN);
	ws->aspectRatio = (float)res.w / (float)res.h;

	// ws->style = WS_OVERLAPPED;
	// ws->style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	// if(resizable) ws->style |= WS_THICKFRAME;
	// if(maximizable) ws->style |= WS_MAXIMIZEBOX;
	// if(visible) ws->style |= WS_VISIBLE;

	// ws->style = WS_POPUP | WS_BORDER | WS_SYSMENU;
	ws->style = WS_POPUP | WS_BORDER;
	// ws->style = WS_POPUP | WS_VISIBLE | WS_CAPTION;
	// ws->style = WS_POPUP;

	RECT cr = {0, 0, res.w, res.h};
	AdjustWindowRectEx(&cr, ws->style, 0, 0);

	int ww = cr.right - cr.left;
	int wh = cr.bottom - cr.top;
	// int wx = ws->fullRes.x/2 - ww/2;
	// int wy = ws->fullRes.y/2 - wh/2;
	int wx, wy;
	{
		MonitorData* md = ws->monitors + monitor;
		wx = rectCen(md->workRect).x - res.w/2;
		wy = rectCen(md->workRect).y - res.h/2;
	}
	ws->res = vec2i(ww, wh);

    WNDCLASS windowClass = {};
    windowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    // windowClass.style = CS_HREDRAW|CS_VREDRAW;
    // windowClass.style = CS_OWNDC;
    windowClass.lpfnWndProc = mainWindowCallBack;
    windowClass.hInstance = systemData->instance;
    windowClass.lpszClassName = "App";
    // windowClass.hCursor = LoadCursor(0, IDC_ARROW);
    windowClass.hCursor = 0;
    // windowClass.hbrBackground = CreateSolidBrush(RGB(30,30,30));
    // windowClass.hbrBackground = (HBRUSH)CreateSolidBrush(0x00000000);

    if(!RegisterClass(&windowClass)) {
        DWORD errorCode = GetLastError();
        int dummy = 2;   
    }

	TIMER_BLOCK_BEGIN_NAMED(createWindow, "createWindow");

    systemData->windowHandle = CreateWindowEx(0, windowClass.lpszClassName, "", ws->style, wx,wy,ww,wh, 0, 0, systemData->instance, 0);

	TIMER_BLOCK_END(createWindow);

    if(!systemData->windowHandle) {
        DWORD errorCode = GetLastError();
    }

	SetFocus(systemData->windowHandle);

	// DWM_BLURBEHIND bb = {0};
	// HRGN hRgn = CreateRectRgn(0, 0, -1, -1);
	// bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
	// bb.hRgnBlur = hRgn;
	// bb.fEnable = TRUE;
	// DwmEnableBlurBehindWindow(systemData->windowHandle, &bb);

	TIMER_BLOCK_BEGIN_NAMED(setPixelFormat, "setPixelFormat");

    PIXELFORMATDESCRIPTOR pixelFormatDescriptor =
    {
        +    sizeof(PIXELFORMATDESCRIPTOR),
        1,
        /*PFD_SUPPORT_COMPOSITION |*/ PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
        PFD_TYPE_RGBA,            //The kind of framebuffer. RGBA or palette.
        24,                        //Colordepth of the framebuffer.
        0, 0, 0, 0, 0, 0,
        0, //8
        0,
        0,
        0, 0, 0, 0,
        24,                        //Number of bits for the depthbuffer
        // 32,                        //Number of bits for the depthbuffer
        // 0,                        //Number of bits for the depthbuffer
        // 8,                        //Number of bits for the stencilbuffer
        0,                        //Number of bits for the stencilbuffer
        0,                        //Number of Aux buffers in the framebuffer.
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };    
    
    HDC deviceContext = GetDC(systemData->windowHandle);
    systemData->deviceContext = deviceContext;
    int pixelFormat;
    pixelFormat = ChoosePixelFormat(deviceContext, &pixelFormatDescriptor);
	SetPixelFormat(deviceContext, pixelFormat, &pixelFormatDescriptor);
	
	TIMER_BLOCK_END(setPixelFormat);

	TIMER_BLOCK_BEGIN_NAMED(createContext, "CreateContext");

    HGLRC openglContext = wglCreateContext(systemData->deviceContext);
    bool result = wglMakeCurrent(systemData->deviceContext, openglContext);
    if(!result) { printf("Could not set Opengl Context.\n"); }

	TIMER_BLOCK_END(createContext);


    // #ifndef HID_USAGE_PAGE_GENERIC
    // #define HID_USAGE_PAGE_GENERIC         ((USHORT) 0x01)
    // #endif
    // #ifndef HID_USAGE_GENERIC_MOUSE
    // #define HID_USAGE_GENERIC_MOUSE        ((USHORT) 0x02)
    // #endif

    // RAWINPUTDEVICE Rid[1];
    // Rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC; 
    // Rid[0].usUsage = HID_USAGE_GENERIC_MOUSE; 
    // Rid[0].dwFlags = RIDEV_INPUTSINK;   
    // Rid[0].hwndTarget = systemData->windowHandle;
    // bool r = RegisterRawInputDevices(Rid, 1, sizeof(Rid[0]));

    // printf("%Opengl Version: %s\n", (char*)glGetString(GL_VERSION));

	SetCursor(LoadCursor(0, IDC_ARROW));
}

void showWindow(HWND windowHandle) {
    ShowWindow(windowHandle, SW_SHOW);
}

float getScalingFactor(HWND windowHandle) {
    HDC deviceContext = GetDC(windowHandle);
    int LogicalScreenHeight = GetDeviceCaps(deviceContext, VERTRES);
    int PhysicalScreenHeight = GetDeviceCaps(deviceContext, DESKTOPVERTRES); 

    float ScreenScalingFactor = (float)PhysicalScreenHeight / (float)LogicalScreenHeight;

    return ScreenScalingFactor;
}

Vec2 getMousePos(HWND windowHandle, bool yInverted = true) {
	POINT point;    
	GetCursorPos(&point);
	ScreenToClient(windowHandle, &point);
	Vec2 mousePos = vec2(0,0);
	mousePos.x = point.x;
	mousePos.y = point.y;
	if(yInverted) mousePos.y = -mousePos.y;

	return mousePos;
}

void updateInput(Input* input, bool* isRunning, HWND windowHandle, bool waitForMessages) {
	#ifdef RELEASE_BUILD
		if(waitForMessages) WaitMessage();
	#endif

	input->anyKey = false;

    input->mouseWheel = 0;
    for(int i = 0; i < arrayCount(input->mouseButtonPressed); i++) input->mouseButtonPressed[i] = 0;
    for(int i = 0; i < arrayCount(input->mouseButtonReleased); i++) input->mouseButtonReleased[i] = 0;
    for(int i = 0; i < arrayCount(input->keysPressed); i++) input->keysPressed[i] = 0;
    input->mShift = 0;
    input->mCtrl = 0;
    input->mAlt = 0;
    input->inputCharacterCount = 0;

    input->mouseDeltaX = 0;
    input->mouseDeltaY = 0;

    bool killedFocus = false;

    MSG message;
    while(PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
        switch(message.message) {
            case WM_MOUSEWHEEL: {
                short wheelDelta = HIWORD(message.wParam);
                input->mouseWheel = wheelDelta / WHEEL_DELTA;
            } break;

            case WM_LBUTTONDOWN: { 
            	SetCapture(windowHandle);
            	input->mouseButtonPressed[0] = true; 
				input->mouseButtonDown[0] = true; 
			} break;
            case WM_RBUTTONDOWN: { 
            	SetCapture(windowHandle);
            	input->mouseButtonPressed[1] = true; 
				input->mouseButtonDown[1] = true; 
			} break;
            case WM_MBUTTONDOWN: { 
            	SetCapture(windowHandle);
            	input->mouseButtonPressed[2] = true; 
				input->mouseButtonDown[2] = true; 
			} break;

	        case WM_LBUTTONUP: { 
            	ReleaseCapture();
				input->mouseButtonDown[0] = false; 
				input->mouseButtonReleased[0] = true;
			} break;
	        case WM_RBUTTONUP: { 
            	ReleaseCapture();
				input->mouseButtonDown[1] = false; 
				input->mouseButtonReleased[1] = true;
			} break;
	        case WM_MBUTTONUP: { 
            	ReleaseCapture();
				input->mouseButtonDown[2] = false; 
				input->mouseButtonReleased[2] = true;
			} break;

            case WM_KEYDOWN:
            case WM_KEYUP: {
                uint vk = uint(message.wParam);

                bool keyDown = (message.message == WM_KEYDOWN);
                int keycode = vkToKeycode(vk);
                input->keysDown[keycode] = keyDown;
                input->keysPressed[keycode] = keyDown;
                input->mShift = ((GetKeyState(VK_SHIFT) & 0x80) != 0);
                input->mCtrl = ((GetKeyState(VK_CONTROL) & 0x80) != 0);
                input->mAlt = ((GetKeyState(VK_MENU) & 0x80) != 0);

                if(keyDown) {
                	input->anyKey = true;

                    TranslateMessage(&message); 
                }
            } break;

            case WM_CHAR: {
                // input->inputCharacters[input->inputCharacterCount] = (char)uint(message.wParam);
            	uint charIndex = uint(message.wParam);
            	if(charIndex < ' ' || charIndex > '~') break;
            	char c = (char)charIndex;
                input->inputCharacters[input->inputCharacterCount] = c;
                input->inputCharacterCount++;
            } break;

            // case WM_INPUT: {
            // 	RAWINPUT inputBuffer;
            // 	UINT rawInputSize = sizeof(inputBuffer);
            // 	GetRawInputData((HRAWINPUT)(message.lParam), RID_INPUT, &inputBuffer, &rawInputSize, sizeof(RAWINPUTHEADER));
            // 	RAWINPUT* raw = (RAWINPUT*)(&inputBuffer);
            	
            // 	if (raw->header.dwType == RIM_TYPEMOUSE) {
            // 	    int xPosRelative = raw->data.mouse.lLastX;
            // 	    int yPosRelative = raw->data.mouse.lLastY;

            // 	    input->mouseDeltaX = -xPosRelative;
            // 	    input->mouseDeltaY = -yPosRelative;
            // 	} break;
            // } break;

            case WM_DESTROY: {
                *isRunning = false;
            } break;

            case WM_CLOSE: {
                *isRunning = false;
            } break;

            case WM_QUIT: {
                *isRunning = false;
            } break;

            case WM_KILLFOCUS: {
            	killedFocus = true;
            }

            default: {
                TranslateMessage(&message); 
                DispatchMessage(&message); 
            } break;
        }
    }

    if(killedFocus) {
    	for(int i = 0; i < KEYCODE_COUNT; i++) {
    		input->keysDown[i] = false;
    	}
    }

    POINT point;    
    GetCursorPos(&point);
    ScreenToClient(windowHandle, &point);
    input->mousePos.x = point.x;
    input->mousePos.y = point.y;
    input->mousePosNegative = vec2(input->mousePos.x, -input->mousePos.y);

    input->mouseDelta = input->mousePos - input->lastMousePos;
    input->lastMousePos = input->mousePos;

    // input->mouseDelta = vec2(point.x-input->lastMousePos.x, point.y-input->lastMousePos.y);
    // input->lastMousePos = vec2(point.x, point.y);

    input->firstFrame = false;
}

// MetaPlatformFunction();
// const char* getClipboard(MemoryBlock* memory) {
//     BOOL result = OpenClipboard(0);
//     HANDLE clipBoardData = GetClipboardData(CF_TEXT);

//     return (char*)clipBoardData;
// }

char* getClipboard() {
    BOOL result = OpenClipboard(0);
    HANDLE clipboardHandle = GetClipboardData(CF_TEXT);
    // char* data = GlobalLock(clipboardHandle);
    char* data = (char*)clipboardHandle;

    return data;
}

void closeClipboard() {
    CloseClipboard();
}

// MetaPlatformFunction();
void setClipboard(char* text) {
    int textSize = strLen(text) + 1;
    HANDLE clipHandle = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, textSize);
    char* pointer = (char*)GlobalLock(clipHandle);
    memCpy(pointer, (char*)text, textSize);
    GlobalUnlock(clipHandle);

    OpenClipboard(0);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, clipHandle);
    CloseClipboard();
}

void getWindowProperties(HWND windowHandle, int* viewWidth, int* viewHeight, int* width, int* height, int* x, int* y) {
    RECT cr; 
    GetClientRect(windowHandle, &cr);
    *viewWidth = cr.right - cr.left;
    *viewHeight = cr.bottom - cr.top;

    if(width && height) {
    	RECT wr; 
    	GetWindowRect(windowHandle, &wr);
    	*width = wr.right - wr.left;
    	*height = wr.bottom - wr.top;
    }

    if(x && y) {
    	WINDOWPLACEMENT placement;
    	GetWindowPlacement(windowHandle, &placement);
    	RECT r; 
    	r = placement.rcNormalPosition; 
    	*x = r.left;
    	*y = r.top;    	
    }
}

// MetaPlatformFunction();
void setWindowProperties(HWND windowHandle, int width, int height, int x, int y) {
    WINDOWPLACEMENT placement;
    GetWindowPlacement(windowHandle, &placement);
    RECT r = placement.rcNormalPosition;

    if(width != -1) r.right = r.left + width;
    if(height != -1) r.bottom = r.top + height;
    if(x != -1) {
        int width = r.right - r.left;
        r.left = x;
        r.right = x + width;
    }
    if(y != -1) {
        int height = r.bottom - r.top;
        r.top = y;
        r.bottom = y + height;
    }

    placement.rcNormalPosition = r;
    SetWindowPlacement(windowHandle, &placement);
}

enum WindowMode {
	WINDOW_MODE_WINDOWED = 0,
	WINDOW_MODE_FULLBORDERLESS,

	WINDOW_MODE_COUNT,
};

void setWindowStyle(HWND hwnd, DWORD dwStyle) {
	SetWindowLong(hwnd, GWL_STYLE, dwStyle);
}

DWORD getWindowStyle(HWND hwnd) {
	return GetWindowLong(hwnd, GWL_STYLE);
}

void updateResolution(HWND windowHandle, WindowSettings* ws) {
	getWindowProperties(windowHandle, &ws->currentRes.x, &ws->currentRes.y,0,0,0,0);
	ws->aspectRatio = ws->currentRes.x / (float)ws->currentRes.y;
}

void setWindowMode(HWND hwnd, WindowSettings* wSettings, int mode) {
	if(mode == WINDOW_MODE_FULLBORDERLESS && !wSettings->fullscreen) {
		wSettings->g_wpPrev = {};

		DWORD dwStyle = getWindowStyle(hwnd);
		if (dwStyle & WS_OVERLAPPEDWINDOW) {
		  MONITORINFO mi = { sizeof(mi) };
		  if (GetWindowPlacement(hwnd, &wSettings->g_wpPrev) &&
		      GetMonitorInfo(MonitorFromWindow(hwnd,
		                     MONITOR_DEFAULTTOPRIMARY), &mi)) {
		    SetWindowLong(hwnd, GWL_STYLE,
		                  dwStyle & ~WS_OVERLAPPEDWINDOW);
			setWindowStyle(hwnd, dwStyle & ~WS_OVERLAPPEDWINDOW);

		    SetWindowPos(hwnd, HWND_TOP,
		                 mi.rcMonitor.left, mi.rcMonitor.top,
		                 mi.rcMonitor.right - mi.rcMonitor.left,
		                 mi.rcMonitor.bottom - mi.rcMonitor.top,
		                 SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		  }
		}

		wSettings->fullscreen = true;
	} else if(mode == WINDOW_MODE_WINDOWED && wSettings->fullscreen) {
		setWindowStyle(hwnd, wSettings->style);
		SetWindowPlacement(hwnd, &wSettings->g_wpPrev);
		SetWindowPos(hwnd, NULL, 0,0, wSettings->res.w, wSettings->res.h, SWP_NOZORDER | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

		wSettings->fullscreen = false;
	}
}

void swapBuffers(SystemData* systemData) {
    SwapBuffers(systemData->deviceContext);
}

Rect getScreenRect(WindowSettings* ws) {
	return rect(0, -ws->currentRes.h, ws->currentRes.w, 0);
}

void captureMouse(HWND windowHandle, bool t) {
	if(t) {
		int w,h;
		Vec2i wPos;
		getWindowProperties(windowHandle, &w, &h, 0, 0, &wPos.x, &wPos.y);
		SetCursorPos(wPos.x + w/2, wPos.y + h/2);

		while(ShowCursor(false) >= 0);
	} else {
		while(ShowCursor(true) < 0);
	}
}

bool windowHasFocus(HWND windowHandle) {
	bool result = GetFocus() == windowHandle;
	return result;
}

bool windowSizeChanged(HWND windowHandle, WindowSettings* ws) {
	Vec2i cr;
	getWindowProperties(windowHandle, &cr.x, &cr.y, 0, 0, 0, 0);

	bool result = cr != ws->currentRes;
	return result;
}

// MetaPlatformFunction();
uint getTicks() {
    uint result = GetTickCount();

    return result;
}

__int64 getCycleStamp() {
	return __rdtsc();
}

i64 timerInit() {
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);

	return counter.QuadPart;
}

// Returns time in milliseconds;
f64 timerUpdate(i64 lastTimeStamp, i64* setTimeStamp = 0) {
	LARGE_INTEGER counter;
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency); 
	QueryPerformanceCounter(&counter);

	i64 timeStamp = counter.QuadPart;
	f64 dt = (timeStamp - lastTimeStamp);
	dt *= 1000000;
	dt /= frequency.QuadPart;
	dt /= 1000000;
	
	if(setTimeStamp) *setTimeStamp = timeStamp;

	return dt;
}

void shellExecute(char* command) {
	system(command);
}

bool windowIsMinimized(HWND windowHandle) {	
	return IsIconic(windowHandle);
}

void shellExecuteNoWindow(char* command, bool wait = true) {
	STARTUPINFO si = {};
	PROCESS_INFORMATION pi = {};
	si.cb = sizeof(si);

	if (CreateProcess(NULL, command, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
		if(wait) {
		    WaitForSingleObject(pi.hProcess, INFINITE);
		}
	    CloseHandle(pi.hProcess);
	    CloseHandle(pi.hThread);
	}
}

// MetaPlatformFunction();
void sleep(int milliseconds) {
    Sleep(milliseconds);
}
