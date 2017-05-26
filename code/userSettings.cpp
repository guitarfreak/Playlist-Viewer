
#define USE_SRGB 1

#if USE_SRGB == 1
#define INTERNAL_TEXTURE_FORMAT GL_SRGB8_ALPHA8
#define COLOR_SRGB(color) colorSRGB(color)
#else 
#define INTERNAL_TEXTURE_FORMAT GL_RGBA8
#define COLOR_SRGB(color) color
#endif

#define editor_executable_path "C:\\Program Files\\Sublime Text 3\\sublime_text.exe"

#define APP_FONT_COUNT 4

//

// const char* watchFolders[] = {
	// "..\\data\\Textures\\Misc\\", 
	// "..\\data\\Textures\\Skyboxes\\", 
	// "..\\data\\Textures\\Minecraft\\"
// };

//

enum TextureId {
	TEXTURE_WHITE = 0,
	// TEXTURE_RECT,
	// TEXTURE_CIRCLE,
	// TEXTURE_TEST,
	TEXTURE_SIZE,
};

char* texturePaths[] = {
	"..\\data\\Textures\\Misc\\white.png",
};

//

enum FontId {
	FONT_LIBERATION_MONO = 0,
	FONT_SOURCESANS_PRO,
	FONT_CONSOLAS,
	FONT_ARIAL,
	FONT_CALIBRI,
	FONT_SIZE,
};

char* fontPaths[] = {
	// "..\\data\\Fonts\\LiberationMono-Bold.ttf",
	"..\\data\\Fonts\\LiberationMono-Regular.ttf",
	"..\\data\\Fonts\\SourceSansPro-Regular.ttf",
	"..\\data\\Fonts\\consola.ttf",
	"..\\data\\Fonts\\arial.ttf",
	"..\\data\\Fonts\\calibri.ttf",
};

//

enum SamplerType {
	SAMPLER_NORMAL = 0,
	SAMPLER_SIZE,
};

//

enum FrameBufferType {
	FRAMEBUFFER_2dMsaa = 0,
	FRAMEBUFFER_2dNoMsaa,

	FRAMEBUFFER_DebugMsaa,
	FRAMEBUFFER_DebugNoMsaa,

	FRAMEBUFFER_ScreenShot,

	FRAMEBUFFER_SIZE,
};

enum WatchFolder {
	WATCH_FOLDER_APP = 0,
	WATCH_FOLDER_PLAYLISTS,
	
	WATCH_FOLDER_SIZE,
};