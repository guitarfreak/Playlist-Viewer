
struct DrawCommandList;
extern DrawCommandList* globalCommandList;
struct GraphicsState;
extern GraphicsState* globalGraphicsState;


// 
// Misc.
// 

char* fillString(char* text, ...) {
	va_list vl;
	va_start(vl, text);

	int length = strLen(text);
	char* buffer = getTStringX(length+1);

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
			getTStringX(sLen);
		} else if(text[ti] == '%' && text[ti+1] == 'f') {
			float v = va_arg(vl, double);
			floatToStr(valueBuffer, v, 2);
			int sLen = strLen(valueBuffer);
			memCpy(buffer + bi, valueBuffer, sLen);

			ti += 2;
			bi += sLen;
			getTStringX(sLen);
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
				getTStringX(sLen);
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
				getTStringX(sLen);
			}
		} else if(text[ti] == '%' && text[ti+1] == 's') {
			char* str = va_arg(vl, char*);
			int sLen = strLen(str);
			memCpy(buffer + bi, str, sLen);

			ti += 2;
			bi += sLen;
			getTStringX(sLen);
		} else if(text[ti] == '%' && text[ti+1] == 'b') {
			bool str = va_arg(vl, bool);
			char* s = str == 1 ? "true" : "false";
			int sLen = strLen(s);
			memCpy(buffer + bi, s, sLen);

			ti += 2;
			bi += sLen;
			getTStringX(sLen);
		} else if(text[ti] == '%' && text[ti+1] == '%') {
			buffer[bi++] = '%';
			ti += 2;
			getTStringX(1);
		} else {
			buffer[bi++] = text[ti++];
			getTStringX(1);

			if(buffer[bi-1] == '\0') break;
		}
	}

	return buffer;
}

//
// Textures.
// 

struct Texture {
	// char* name;
	uint id;
	Vec2i dim;
	int channels;
	int levels;
	int internalFormat;
	int channelType;
	int channelFormat;

	bool isRenderBuffer;
	int msaa;
};

int getMaximumMipmapsFromSize(int size) {
	int mipLevels = 1;
	while(size >= 2) {
		size /= 2;
		mipLevels++;
	}

	return mipLevels;
}

void loadTexture(Texture* texture, unsigned char* buffer, int w, int h, int mipLevels, int internalFormat, int channelType, int channelFormat, bool reload = false) {

	if(!reload) {
		glCreateTextures(GL_TEXTURE_2D, 1, &texture->id);
		glTextureStorage2D(texture->id, mipLevels, internalFormat, w, h);

		texture->dim = vec2i(w,h);
		texture->channels = 4;
		texture->levels = mipLevels;
	}	

	glTextureSubImage2D(texture->id, 0, 0, 0, w, h, channelType, channelFormat, buffer);

	glTextureParameteri(texture->id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(texture->id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(texture->id, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTextureParameteri(texture->id, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glGenerateTextureMipmap(texture->id);
}

void loadTextureFromFile(Texture* texture, char* path, int mipLevels, int internalFormat, int channelType, int channelFormat, bool reload = false) {
	int x,y,n;
	unsigned char* stbData = stbi_load(path, &x, &y, &n, 0);

	if(mipLevels == -1) mipLevels = getMaximumMipmapsFromSize(min(x,y));
	
	loadTexture(texture, stbData, x, y, mipLevels, internalFormat, channelType, channelFormat, reload);

	stbi_image_free(stbData);
}

void loadTextureFromMemory(Texture* texture, char* buffer, int length, int mipLevels, int internalFormat, int channelType, int channelFormat, bool reload = false) {
	int x,y,n;
	unsigned char* stbData = stbi_load_from_memory((uchar*)buffer, length, &x, &y, &n, 0);

	if(mipLevels == -1) mipLevels = getMaximumMipmapsFromSize(min(x,y));
	
	loadTexture(texture, stbData, x, y, mipLevels, internalFormat, channelType, channelFormat, reload);

	stbi_image_free(stbData);
}

void createTexture(Texture* texture, bool isRenderBuffer = false) {	
	if(!isRenderBuffer) glCreateTextures(GL_TEXTURE_2D, 1, &texture->id);
	else glCreateRenderbuffers(1, &texture->id);
}

void recreateTexture(Texture* t) {
	glDeleteTextures(1, &t->id);
	glCreateTextures(GL_TEXTURE_2D, 1, &t->id);

	glTextureStorage2D(t->id, 1, t->internalFormat, t->dim.w, t->dim.h);
}

void deleteTexture(Texture* t) {
	glDeleteTextures(1, &t->id);
}

//
// Fonts.
//

struct Font {
	char* name;
	int id;

	stbtt_fontinfo info;
	float pixelScale;

	char* fileBuffer;
	Texture tex;
	Vec2i glyphRanges[4];
	int glyphRangeCount;
	stbtt_packedchar* cData;
	int height;
	float baseOffset;
};

//
// Framebuffers.
//

enum FrameBufferSlot {
	FRAMEBUFFER_SLOT_COLOR,
	FRAMEBUFFER_SLOT_DEPTH,
	FRAMEBUFFER_SLOT_STENCIL,
	FRAMEBUFFER_SLOT_DEPTH_STENCIL,
};

struct FrameBuffer {
	uint id;

	union {
		struct {
			Texture* colorSlot[4];
			Texture* depthSlot[4];
			Texture* stencilSlot[4];
			Texture* depthStencilSlot[4];
		};

		struct {
			Texture* slots[16];
		};
	};
};

FrameBuffer* getFrameBuffer(int id);
Texture* addTexture(Texture tex);

void initFrameBuffer(FrameBuffer* fb) {
	glCreateFramebuffers(1, &fb->id);

	for(int i = 0; i < arrayCount(fb->slots); i++) {
		fb->slots[i] = 0;
	}
}

void attachToFrameBuffer(int id, int slot, int internalFormat, int w, int h, int msaa = 0) {
	FrameBuffer* fb = getFrameBuffer(id);

	bool isRenderBuffer = msaa > 0;

	Texture t;
	createTexture(&t, isRenderBuffer);
	t.internalFormat = internalFormat;
	t.dim.w = w;
	t.dim.h = h;
	t.isRenderBuffer = isRenderBuffer;
	t.msaa = msaa;

	Texture* tex = addTexture(t);

	Vec2i indexRange;
	if(slot == FRAMEBUFFER_SLOT_COLOR) indexRange = vec2i(0,4);
	else if(slot == FRAMEBUFFER_SLOT_DEPTH) indexRange = vec2i(4,8);
	else if(slot == FRAMEBUFFER_SLOT_STENCIL) indexRange = vec2i(8,12);
	else if(slot == FRAMEBUFFER_SLOT_DEPTH_STENCIL) indexRange = vec2i(12,16);

	for(int i = indexRange.x; i <= indexRange.y; i++) {
		if(fb->slots[i] == 0) {
			fb->slots[i] = tex;
			break;
		}
	}

}

void reloadFrameBuffer(int id) {
	FrameBuffer* fb = getFrameBuffer(id);

	for(int i = 0; i < arrayCount(fb->slots); i++) {
		if(!fb->slots[i]) continue;
		Texture* t = fb->slots[i];

		int slot;
		if(valueBetween(i, 0, 3)) slot = GL_COLOR_ATTACHMENT0 + i;
		else if(valueBetween(i, 4, 7)) slot = GL_DEPTH_ATTACHMENT;
		else if(valueBetween(i, 8, 11)) slot = GL_STENCIL_ATTACHMENT;
		else if(valueBetween(i, 12, 15)) slot = GL_DEPTH_STENCIL_ATTACHMENT;

		if(t->isRenderBuffer) {
			glNamedRenderbufferStorageMultisample(t->id, t->msaa, t->internalFormat, t->dim.w, t->dim.h);
			glNamedFramebufferRenderbuffer(fb->id, slot, GL_RENDERBUFFER, t->id);
		} else {
			glDeleteTextures(1, &t->id);
			glCreateTextures(GL_TEXTURE_2D, 1, &t->id);
			glTextureStorage2D(t->id, 1, t->internalFormat, t->dim.w, t->dim.h);
			glNamedFramebufferTexture(fb->id, slot, t->id, 0);
		}
	}

}

void blitFrameBuffers(int id1, int id2, Vec2i dim, int bufferBit, int filter) {
	FrameBuffer* fb1 = getFrameBuffer(id1);
	FrameBuffer* fb2 = getFrameBuffer(id2);

	glBlitNamedFramebuffer (fb1->id, fb2->id, 0,0, dim.x, dim.y, 0,0, dim.x, dim.y, bufferBit, filter);
}

void bindFrameBuffer(int id, int slot = GL_FRAMEBUFFER) {
	FrameBuffer* fb = getFrameBuffer(id);
	glBindFramebuffer(slot, fb->id);
}

void setDimForFrameBufferAttachmentsAndUpdate(int id, int w, int h) {
	FrameBuffer* fb = getFrameBuffer(id);

	for(int i = 0; i < arrayCount(fb->slots); i++) {
		if(!fb->slots[i]) continue;
		Texture* t = fb->slots[i];

		t->dim = vec2i(w, h);
	}

	reloadFrameBuffer(id);
}

uint checkStatusFrameBuffer(int id) {
	FrameBuffer* fb = getFrameBuffer(id);
	GLenum result = glCheckNamedFramebufferStatus(fb->id, GL_FRAMEBUFFER);
	return result;
}

//
// Data.
//

struct GraphicsState {
	Texture textures[32]; //TEXTURE_SIZE
	int textureCount;
	Texture textures3d[2];
	GLuint samplers[SAMPLER_SIZE];
	Font fonts[FONT_SIZE + APP_FONT_COUNT][20];

	GLuint textureUnits[16];
	GLuint samplerUnits[16];

	FrameBuffer frameBuffers[FRAMEBUFFER_SIZE];
};

Texture* getTexture(int textureId) {
	Texture* t = globalGraphicsState->textures + textureId;
	return t;
}

Texture* getTextureX(int textureId) {
	GraphicsState* gs = globalGraphicsState;
	for(int i = 0; i < arrayCount(gs->textures); i++) {
		if(gs->textures[i].id == textureId) {
			return gs->textures + i;
		}
	}

	return 0;
}

Texture* addTexture(Texture tex) {
	GraphicsState* gs = globalGraphicsState;
	gs->textures[gs->textureCount++] = tex;
	return gs->textures + (gs->textureCount - 1);
}

FrameBuffer* getFrameBuffer(int id) {
	FrameBuffer* fb = globalGraphicsState->frameBuffers + id;
	return fb;
}

#define Font_Error_Glyph (int)0x20-1

Font* fontInit(Font* fontSlot, char* file, char* filePath, int height, int id) {
	TIMER_BLOCK();

	Font font;
	char* path = fillString("%s%s", filePath, file);

	char* fileBuffer = (char*)getPMemory(fileSize(path) + 1);
	// char* fileBuffer = getTArray(char, fileSize(path) + 1);

	if(!fileExists(path)) return 0;

	readFileToBuffer(fileBuffer, path);
	// Vec2i size = vec2i(512,512);
	Vec2i size = vec2i(800,800);
	unsigned char* fontBitmapBuffer = (unsigned char*)getTMemory(size.x*size.y);
	unsigned char* fontBitmap = (unsigned char*)getTMemory(size.x*size.y*4);
	
	font.name = getPString(strLen(file)+1);
	strCpy(font.name, file);

	font.id = id;
	font.height = height;
	font.baseOffset = 0.8f;

	font.glyphRangeCount = 0;
	font.glyphRanges[0].x = (int)0x20-1;
	font.glyphRanges[0].y = 0x7F - font.glyphRanges[0].x;
	font.glyphRangeCount++;
	font.glyphRanges[1].x = 0xA0;
	font.glyphRanges[1].y = 0xFF - font.glyphRanges[1].x;
	font.glyphRangeCount++;
	
	int totalGlyphCount = 0;
	for(int i = 0; i < font.glyphRangeCount; i++) totalGlyphCount += font.glyphRanges[i].y;

	font.cData = getPArray(stbtt_packedchar, totalGlyphCount);


	int fontFileOffset = stbtt_GetFontOffsetForIndex((uchar*)fileBuffer, 0);
	stbtt_InitFont(&font.info, (uchar*)fileBuffer, fontFileOffset);


	stbtt_pack_context context;
	int result = stbtt_PackBegin(&context, fontBitmapBuffer, size.w, size.h, 0, 1, 0);

	int sampling = 2;
	if(font.height < 25) sampling = 4;
	stbtt_PackSetOversampling(&context, sampling, sampling);
	
	stbtt_pack_range range[4];
	int cDataOffset = 0;
	for(int i = 0; i < font.glyphRangeCount; i++) {
		range[i].first_unicode_codepoint_in_range = font.glyphRanges[i].x;
		range[i].array_of_unicode_codepoints = NULL;
		range[i].num_chars                   = font.glyphRanges[i].y;
		range[i].chardata_for_range          = font.cData + cDataOffset;
		range[i].font_size                   = font.height;

		cDataOffset += font.glyphRanges[i].y;
	}

	// We assume glyphRanges in the front have more importance.
	for(int i = 0; i < font.glyphRangeCount; i++) {
		stbtt_PackFontRanges(&context, (uchar*)fileBuffer, 0, range + i, 1);
	}
	// stbtt_PackFontRanges(&context, (uchar*)fileBuffer, 0, range, font.glyphRangeCount);

	stbtt_PackEnd(&context);

	font.pixelScale = stbtt_ScaleForPixelHeight(&font.info, font.height);

	for(int i = 0; i < size.w*size.h; i++) {
		fontBitmap[i*4] = fontBitmapBuffer[i];
		fontBitmap[i*4+1] = fontBitmapBuffer[i];
		fontBitmap[i*4+2] = fontBitmapBuffer[i];
		fontBitmap[i*4+3] = fontBitmapBuffer[i];
	}

	Texture tex;
	loadTexture(&tex, fontBitmap, size.w, size.h, 1, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
	font.tex = tex;

	addTexture(tex);

	*fontSlot = font;
	return fontSlot;
}

Font* getFont(int fontId, int height) {

	// Search if Font in this size exists, if not create it.

	int fontSlots = arrayCount(globalGraphicsState->fonts[0]);
	Font* fontArray = globalGraphicsState->fonts[fontId];
	Font* fontSlot = 0;
	for(int i = 0; i < fontSlots; i++) {
		int fontHeight = fontArray[i].height;
		if(fontHeight == height || fontHeight == 0) {
			fontSlot = fontArray + i;
			break;
		}
	}

	// We are going to assume for now that a font size of 0 means it is uninitialized.

	if(fontSlot->height == 0) {
		TIMER_BLOCK();

		Font font;
		char* path = fontPaths[fontId];

		char* fileBuffer = (char*)getPMemory(fileSize(path) + 1);
		// char* fileBuffer = getTArray(char, fileSize(path) + 1);

		readFileToBuffer(fileBuffer, path);
		// Vec2i size = vec2i(512,512);
		Vec2i size = vec2i(800,800);
		unsigned char* fontBitmapBuffer = (unsigned char*)getTMemory(size.x*size.y);
		unsigned char* fontBitmap = (unsigned char*)getTMemory(size.x*size.y*4);
		
		font.id = fontId;
		font.height = height;
		font.baseOffset = 0.8f;

		font.glyphRangeCount = 0;
		font.glyphRanges[0].x = (int)0x20-1;
		font.glyphRanges[0].y = 0x7F - font.glyphRanges[0].x;
		font.glyphRangeCount++;
		font.glyphRanges[1].x = 0xA0;
		font.glyphRanges[1].y = 0xFF - font.glyphRanges[1].x;
		font.glyphRangeCount++;
		
		int totalGlyphCount = 0;
		for(int i = 0; i < font.glyphRangeCount; i++) totalGlyphCount += font.glyphRanges[i].y;

		font.cData = getPArray(stbtt_packedchar, totalGlyphCount);


		int fontFileOffset = stbtt_GetFontOffsetForIndex((uchar*)fileBuffer, 0);
		stbtt_InitFont(&font.info, (uchar*)fileBuffer, fontFileOffset);


		stbtt_pack_context context;
		int result = stbtt_PackBegin(&context, fontBitmapBuffer, size.w, size.h, 0, 1, 0);

		int sampling = 2;
		if(font.height < 25) sampling = 4;
		stbtt_PackSetOversampling(&context, sampling, sampling);
		
		stbtt_pack_range range[4];
		int cDataOffset = 0;
		for(int i = 0; i < font.glyphRangeCount; i++) {
			range[i].first_unicode_codepoint_in_range = font.glyphRanges[i].x;
			range[i].array_of_unicode_codepoints = NULL;
			range[i].num_chars                   = font.glyphRanges[i].y;
			range[i].chardata_for_range          = font.cData + cDataOffset;
			range[i].font_size                   = font.height;

			cDataOffset += font.glyphRanges[i].y;
		}

		// We assume glyphRanges in the front have more importance.
		for(int i = 0; i < font.glyphRangeCount; i++) {
			stbtt_PackFontRanges(&context, (uchar*)fileBuffer, 0, range + i, 1);
		}
		// stbtt_PackFontRanges(&context, (uchar*)fileBuffer, 0, range, font.glyphRangeCount);

		stbtt_PackEnd(&context);

		font.pixelScale = stbtt_ScaleForPixelHeight(&font.info, font.height);

		for(int i = 0; i < size.w*size.h; i++) {
			fontBitmap[i*4] = fontBitmapBuffer[i];
			fontBitmap[i*4+1] = fontBitmapBuffer[i];
			fontBitmap[i*4+2] = fontBitmapBuffer[i];
			fontBitmap[i*4+3] = fontBitmapBuffer[i];
		}

		Texture tex;
		loadTexture(&tex, fontBitmap, size.w, size.h, 1, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
		font.tex = tex;

		addTexture(tex);

		*fontSlot = font;
	}



	return fontSlot;
}

Rect scissorRectScreenSpace(Rect r, float screenHeight) {
	Rect scissorRect = {r.min.x, r.min.y+screenHeight, r.max.x, r.max.y+screenHeight};
	return scissorRect;
}

void scissorTest(Rect r) {
	Rect sr = r;
	Vec2 dim = rectDim(sr);
	glScissor(sr.min.x, sr.min.y, dim.x, dim.y);
}

void scissorTest(Rect r, float screenHeight) {
	Rect sr = scissorRectScreenSpace(r, screenHeight);
	if(rectW(sr) < 0 || rectH(sr) < 0) sr = rect(0,0,0,0);

	glScissor(sr.min.x, sr.min.y, rectW(sr), rectH(sr));
}

extern float globalScreenHeight;

void scissorTestScreen(Rect r) {
	Rect sr = scissorRectScreenSpace(r, globalScreenHeight);
	if(rectW(sr) < 0 || rectH(sr) < 0) sr = rect(0,0,0,0);

	glScissor(sr.min.x, sr.min.y, rectW(sr), rectH(sr));
}

void scissorState(bool state = true) {
	if(state) glEnable(GL_SCISSOR_TEST);
	else glDisable(GL_SCISSOR_TEST);
}

void drawLinesHeader(Vec4 color) {
	glBindTexture(GL_TEXTURE_2D, 0);
	Vec4 c = colorSRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);
	glBegin(GL_LINES);
}

void drawLineStripHeader(Vec4 color) {
	glBindTexture(GL_TEXTURE_2D, 0);
	Vec4 c = colorSRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);
	glBegin(GL_LINE_STRIP);
}

void drawPointsHeader(Vec4 color) {
	glBindTexture(GL_TEXTURE_2D, 0);
	Vec4 c = colorSRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);
	glBegin(GL_POINTS);
}

inline void pushVecs(Vec2 p0, Vec2 p1) {
	glVertex3f(p0.x, p0.y, 0.0f);
	glVertex3f(p1.x, p1.y, 0.0f);
}

inline void pushVec(Vec2 p0, float z = 0) {
	glVertex3f(p0.x, p0.y, z);
}

inline void pushColor(Vec4 c) {
	glColor4f(c.r, c.g, c.b, c.a);
}

void drawPoint(Vec2 p, Vec4 color) {
	glBindTexture(GL_TEXTURE_2D, 0);
	Vec4 c = colorSRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);
	glBegin(GL_POINTS);
		glVertex3f(p.x, p.y, 0.0f);
	glEnd();
}

void drawLine(Vec2 p0, Vec2 p1, Vec4 color) {
	glBindTexture(GL_TEXTURE_2D, 0);

	Vec4 c = colorSRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);
	glBegin(GL_LINES);
		glVertex3f(p0.x, p0.y, 0.0f);
		glVertex3f(p1.x, p1.y, 0.0f);
	glEnd();
}

void drawLineNewOff(Vec2 p0, Vec2 p1, Vec4 color) {
	glBindTexture(GL_TEXTURE_2D, 0);

	Vec4 c = colorSRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);
	glBegin(GL_LINES);
		glVertex3f(p0.x, p0.y, 0.0f);
		glVertex3f(p0.x + p1.x, p0.y + p1.y, 0.0f);
	glEnd();
}

void drawRect(Rect r, Vec4 color, Rect uv, int texture, float z = 0) {	
	// if(texture == -1) texture = getTexture(TEXTURE_WHITE)->id;

	glBindTexture(GL_TEXTURE_2D, texture);

	Vec4 c = colorSRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);
	glBegin(GL_QUADS);
		glTexCoord2f(uv.left,  uv.top); glVertex3f(r.left, r.bottom, z);
		glTexCoord2f(uv.left,  uv.bottom); glVertex3f(r.left, r.top, z);
		glTexCoord2f(uv.right, uv.bottom); glVertex3f(r.right, r.top, z);
		glTexCoord2f(uv.right, uv.top); glVertex3f(r.right, r.bottom, z);
	glEnd();
}

void drawRect(Rect r, Rect uv, int texture, float z = 0) {
	drawRect(r, vec4(1,1), uv, texture, z);
}

void drawRect(Rect r, Vec4 color, float z = 0) {	
	glBindTexture(GL_TEXTURE_2D, 0);

	Vec4 c = colorSRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);
	glBegin(GL_QUADS);
		glVertex3f(r.left, r.bottom, z);
		glVertex3f(r.left, r.top, z);
		glVertex3f(r.right, r.top, z);
		glVertex3f(r.right, r.bottom, z);
	glEnd();
}

void drawRectOutline(Rect r, Vec4 color, int offset = -1, float z = 0) {	
	// We assume glLineWidth(1.0f) for now.
	// Offset -1 means draw the lines inside, offset 0 is directly on edge.

	drawLineStripHeader(color);
	rectExpand(&r, offset);
	pushVec(rectBL(r));
	pushVec(rectTL(r));
	pushVec(rectTR(r));
	pushVec(rectBR(r));
	pushVec(rectBL(r));
	glEnd();
}

void drawRectOutlined(Rect r, Vec4 color, Vec4 colorOutline, int offset = -1, float z = 0) {	
	drawRect(r, color, z);
	drawRectOutline(r, colorOutline, offset, z);
}

void drawRectNewColored(Rect r, Vec4 c0, Vec4 c1, Vec4 c2, Vec4 c3) {	
	glBindTexture(GL_TEXTURE_2D, 0);

	Vec4 cs0 = colorSRGB(c0);
	Vec4 cs1 = colorSRGB(c1);
	Vec4 cs2 = colorSRGB(c2);
	Vec4 cs3 = colorSRGB(c3);

	glColor4f(1,1,1,1);
	glBegin(GL_QUADS);
		glColor4f(cs0.r, cs0.g, cs0.b, cs0.a); glVertex3f(r.left, r.bottom, 0.0);
		glColor4f(cs1.r, cs1.g, cs1.b, cs1.a); glVertex3f(r.left, r.top, 0.0);
		glColor4f(cs2.r, cs2.g, cs2.b, cs2.a); glVertex3f(r.right, r.top, 0.0);
		glColor4f(cs3.r, cs3.g, cs3.b, cs3.a); glVertex3f(r.right, r.bottom, 0.0);
	glEnd();
}

void drawRectNewColoredH(Rect r, Vec4 c0, Vec4 c1) {	
	glBindTexture(GL_TEXTURE_2D, 0);

	Vec4 cs0 = colorSRGB(c0);
	Vec4 cs1 = colorSRGB(c1);

	glColor4f(1,1,1,1);
	glBegin(GL_QUADS);
		glColor4f(cs0.r, cs0.g, cs0.b, cs0.a); glVertex3f(r.left, r.bottom, 0.0);
		glColor4f(cs1.r, cs1.g, cs1.b, cs1.a); glVertex3f(r.left, r.top, 0.0);
		glColor4f(cs1.r, cs1.g, cs1.b, cs1.a); glVertex3f(r.right, r.top, 0.0);
		glColor4f(cs0.r, cs0.g, cs0.b, cs0.a); glVertex3f(r.right, r.bottom, 0.0);
	glEnd();
}

void drawRectRounded(Rect r, Vec4 color, float size, float steps = 0) {
	if(steps == 0) steps = 6;
	float s = size;

	drawRect(rect(r.min.x+s, r.min.y, r.max.x-s, r.max.y), color);
	drawRect(rect(r.min.x, r.min.y+s, r.max.x, r.max.y-s), color);

	glBindTexture(GL_TEXTURE_2D, 0);
	Vec4 c = colorSRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);
	glBegin(GL_TRIANGLE_FAN);

	Rect rc = rectExpand(r, -vec2(s,s)*2);
	Vec2 corners[] = {rc.max, vec2(rc.max.x, rc.min.y), rc.min, vec2(rc.min.x, rc.max.y)};
	for(int cornerIndex = 0; cornerIndex < 4; cornerIndex++) {

		Vec2 corner = corners[cornerIndex];
		float round = s;
		float start = M_PI_2*cornerIndex;

		glVertex3f(corner.x, corner.y, 0.0);

		for(int i = 0; i < steps; i++) {
			float angle = start + i*(M_PI_2/(steps-1));
			Vec2 v = vec2(sin(angle), cos(angle));
			Vec2 vv = corner + v*round;

			glVertex3f(vv.x, vv.y, 0.0);
		}
	}
	glEnd();
};

void drawRectHollow(Rect r, float size, Vec4 c, float z = 0) {
	drawRect(rectSetB(r, r.top-size), c);
	drawRect(rectSetL(r, r.right-size), c);
	drawRect(rectSetT(r, r.bottom+size), c);
	drawRect(rectSetR(r, r.left+size), c);
}

void drawRectProgress(Rect r, float p, Vec4 c0, Vec4 c1, bool outlined, Vec4 oc) {	
	if(outlined) {
		drawRectOutlined(rectSetR(r, r.left + rectW(r)*p), c0, oc, 0);
		drawRectOutlined(rectSetL(r, r.right - rectW(r)*(1-p)), c1, oc, 0);
	} else {
		drawRect(rectSetR(r, r.left + rectW(r)*p), c0);
		drawRect(rectSetL(r, r.right - rectW(r)*(1-p)), c1);
	}
}

void drawTriangle(Vec2 p, float size, Vec2 dir, Vec4 color, float z = 0) {
	glBindTexture(GL_TEXTURE_2D, 0);

	Vec4 c = colorSRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);
	dir = normVec2(dir) * size;
	Vec2 tp;
	glBegin(GL_TRIANGLES);
		tp = p + dir;
		glVertex3f(tp.x, tp.y, z);
		tp = p + rotateVec2(dir, degreeToRadian(360/3));
		glVertex3f(tp.x, tp.y, z);
		tp = p + rotateVec2(dir, degreeToRadian(360/3*2));
		glVertex3f(tp.x, tp.y, z);
	glEnd();
}

void drawCross(Vec2 p, float size, float size2, Vec2 dir, Vec4 color, float z = 0) {
	glBindTexture(GL_TEXTURE_2D, 0);

	size2 = size2 / 2 * 1/0.707f;

	dir = normVec2(dir);

	Vec4 c = colorSRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);

	Vec2 tr = p + vec2(1,1)*size;
	Vec2 br = p + vec2(1,-1)*size;
	Vec2 bl = p + vec2(-1,-1)*size;
	Vec2 tl = p + vec2(-1,1)*size;

	glBegin(GL_TRIANGLE_FAN);
		pushVec(p);

		pushVec(p + vec2(0,1) * size2);
			pushVec(tr + vec2(-1,0) * size2);
			pushVec(tr);
			pushVec(tr + vec2(0,-1) * size2);
		pushVec(p + vec2(1,0) * size2);
			pushVec(br + vec2(0,1) * size2);
			pushVec(br);
			pushVec(br + vec2(-1,0) * size2);
		pushVec(p + vec2(0,-1) * size2);
			pushVec(bl + vec2(1,0) * size2);
			pushVec(bl);
			pushVec(bl + vec2(0,1) * size2);
		pushVec(p + vec2(-1,0) * size2);
			pushVec(tl + vec2(0,-1) * size2);
			pushVec(tl);
			pushVec(tl + vec2(1,0) * size2);
		pushVec(p + vec2(0,1) * size2);
	glEnd();
}

enum TextStatus {
	TEXTSTATUS_END = 0, 
	TEXTSTATUS_NEWLINE, 
	TEXTSTATUS_WRAPPED, 
	TEXTSTATUS_DEFAULT, 
	TEXTSTATUS_SIZE, 
};

inline char getRightBits(char n, int count) {
	int bitMask = 0;
	for(int i = 0; i < count; i++) bitMask += (1 << i);
	return n&bitMask;
}

int unicodeDecode(uchar* s, int* byteCount) {
	if(s[0] <= 127) {
		*byteCount = 1;
		return s[0];
	}

	int bitCount = 1;
	for(;;) {
		char bit = (1 << 8-bitCount-1);
		if(s[0]&bit) bitCount++;
		else break;
	}

	(*byteCount) = bitCount;

	int unicodeChar = 0;
	for(int i = 0; i < bitCount; i++) {
		char byte = i==0 ? getRightBits(s[i], 8-(bitCount+1)) : getRightBits(s[i], 6);

		unicodeChar += ((int)byte) << (6*((bitCount-1)-i));
	}

	return unicodeChar;
}

int getUnicodeRangeOffset(int c, Font* font) {
	int unicodeOffset = -1;

	bool found = false;
	for(int i = 0; i < font->glyphRangeCount; i++) {
		if(valueBetweenInt(c, font->glyphRanges[i].x, font->glyphRanges[i].x+font->glyphRanges[i].y)) {
			unicodeOffset += c - font->glyphRanges[i].x + 1;
			found = true;
			break;
		}
		unicodeOffset += font->glyphRanges[i].y;
	}

	if(!found) {
		unicodeOffset = getUnicodeRangeOffset(Font_Error_Glyph, font);
	}

	return unicodeOffset;
}

void getTextQuad(int c, Font* font, Vec2 pos, Rect* r, Rect* uv) {
	stbtt_aligned_quad q;
	int unicodeOffset = getUnicodeRangeOffset(c, font);
	stbtt_GetPackedQuad(font->cData, font->tex.dim.w, font->tex.dim.h, unicodeOffset, pos.x, pos.y, &q, false);

	float baseLine = 0.8f;
	float off = baseLine * font->height;
	*r = rect(q.x0, q.y0 - off, q.x1, q.y1 - off);
	*uv = rect(q.s0, q.t0, q.s1, q.t1);
}

float getCharAdvance(int c, Font* font) {
	int unicodeOffset = getUnicodeRangeOffset(c, font);
	float result = font->cData[unicodeOffset].xadvance;
	return result;
}

float getCharAdvance(int c, int c2, Font* font) {
	int unicodeOffset = getUnicodeRangeOffset(c, font);
	float result = font->cData[unicodeOffset].xadvance;
	float kernAdvance = stbtt_GetCodepointKernAdvance(&font->info, c, c2) * font->pixelScale;
	result += kernAdvance;

	return result;
}



struct TextInfo {
	Vec2 pos;
	int index;
	Vec2 posAdvance;
	Rect r;
	Rect uv;

	bool lineBreak;
	Vec2 breakPos;
};

struct TextSimInfo {
	Vec2 pos;
	int index;
	int wrapIndex;

	bool lineBreak;
	Vec2 breakPos;
};

TextSimInfo initTextSimInfo(Vec2 startPos) {
	TextSimInfo tsi;
	tsi.pos = startPos;
	tsi.index = 0;
	tsi.wrapIndex = 0;
	tsi.lineBreak = false;
	tsi.breakPos = vec2(0,0);
	return tsi;
}

int textSim(char* text, Font* font, TextSimInfo* tsi, TextInfo* ti, Vec2 startPos = vec2(0,0), int wrapWidth = 0) {
	ti->lineBreak = false;

	if(tsi->lineBreak) {
		ti->lineBreak = true;
		ti->breakPos = tsi->breakPos;
		tsi->lineBreak = false;
	}

	if(text[tsi->index] == '\0') {
		ti->pos = tsi->pos;
		ti->index = tsi->index;
		return 0;
	}

	Vec2 oldPos = tsi->pos;

	int i = tsi->index;
	int tSize;
	int t = unicodeDecode((uchar*)(&text[i]), &tSize);

	bool wrapped = false;

	if(wrapWidth != 0 && i == tsi->wrapIndex) {
		int size;
		int c = unicodeDecode((uchar*)(&text[i]), &size);
		float wordWidth = 0;
		if(c == '\n') wordWidth = getCharAdvance(c, font);

		int it = i;
		while(c != '\n' && c != '\0' && c != ' ') {
			wordWidth += getCharAdvance(c, font);
			it += size;
			c = unicodeDecode((uchar*)(&text[it]), &size);
		}

		if(tsi->pos.x + wordWidth > startPos.x + wrapWidth) {
			wrapped = true;
		}

		if(it != i) tsi->wrapIndex = it;
		else tsi->wrapIndex++;
	}

	if(t == '\n' || wrapped) {
		tsi->lineBreak = true;
		if(t == '\n') tsi->breakPos = tsi->pos + vec2(getCharAdvance(t, font),0);
		if(wrapped) tsi->breakPos = tsi->pos;

		tsi->pos.x = startPos.x;
		tsi->pos.y -= font->height;

		if(wrapped) {
			return textSim(text, font, tsi, ti, startPos, wrapWidth);
		}
	} else {
		getTextQuad(t, font, tsi->pos, &ti->r, &ti->uv);
		// tsi->pos.x += getCharAdvance(t, font);

		if(text[i+1] != '\0') {
			int tSize2;
			int t2 = unicodeDecode((uchar*)(&text[i+tSize]), &tSize2);
			tsi->pos.x += getCharAdvance(t, t2, font);
		} else tsi->pos.x += getCharAdvance(t, font);
	}

	if(ti) {
		ti->pos = oldPos;
		ti->index = tsi->index;
		ti->posAdvance = tsi->pos - oldPos;
	}

	tsi->index += tSize;

	return 1;
}

float getTextHeight(char* text, Font* font, Vec2 startPos = vec2(0,0), int wrapWidth = 0) {

	TextSimInfo tsi = initTextSimInfo(startPos);
	while(true) {
		TextInfo ti;
		if(!textSim(text, font, &tsi, &ti, startPos, wrapWidth)) break;
	}

	float height = startPos.y - (tsi.pos.y - font->height);
	return height;
}

Vec2 getTextDim(char* text, Font* font, Vec2 startPos = vec2(0,0), int wrapWidth = 0) {
	float maxX = startPos.x;

	TextSimInfo tsi = initTextSimInfo(startPos);
	while(true) {
		TextInfo ti;
		if(!textSim(text, font, &tsi, &ti, startPos, wrapWidth)) break;

		maxX = max(maxX, ti.pos.x + ti.posAdvance.x);
	}

	Vec2 dim = vec2(maxX - startPos.x, startPos.y - (tsi.pos.y - font->height));

	return dim;
}

Vec2 testgetTextStartPos(char* text, Font* font, Vec2 startPos, Vec2i align = vec2i(-1,1), int wrapWidth = 0) {
	Vec2 dim = getTextDim(text, font, startPos, wrapWidth);
	startPos.x -= (align.x+1)*0.5f*dim.w;
	startPos.y -= (align.y-1)*0.5f*dim.h;

	return startPos;
}

Rect getTextLineRect(char* text, Font* font, Vec2 startPos, Vec2i align = vec2i(-1,1)) {
	startPos = testgetTextStartPos(text, font, startPos, align, 0);

	Vec2 textDim = getTextDim(text, font);
	Rect r = rectTLDim(startPos, textDim);

	return r;
}

void drawText(char* text, Font* font, Vec2 startPos, Vec4 color, Vec2i align = vec2i(-1,1), int wrapWidth = 0) {

	startPos = testgetTextStartPos(text, font, startPos, align, wrapWidth);
	// startPos = vec2(roundInt((int)startPos.x), roundInt((int)startPos.y));

	glBindTexture(GL_TEXTURE_2D, font->tex.id);
	Vec4 c = colorSRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);
	glBegin(GL_QUADS);

	TextSimInfo tsi = initTextSimInfo(startPos);
	while(true) {
		TextInfo ti;
		if(!textSim(text, font, &tsi, &ti, startPos, wrapWidth)) break;
		if(text[ti.index] == '\n') continue;

		glTexCoord2f(ti.uv.left,  ti.uv.top);    glVertex3f(ti.r.left, ti.r.bottom, 0.0);
		glTexCoord2f(ti.uv.left,  ti.uv.bottom); glVertex3f(ti.r.left, ti.r.top, 0.0);
		glTexCoord2f(ti.uv.right, ti.uv.bottom); glVertex3f(ti.r.right, ti.r.top, 0.0);
		glTexCoord2f(ti.uv.right, ti.uv.top);    glVertex3f(ti.r.right, ti.r.bottom, 0.0);
	}
	
	glEnd();
}

void drawTextOutlined(char* text, Font* font, Vec2 startPos, float outlineSize, Vec4 color, Vec4 colorOutline, Vec2i align = vec2i(-1,1), int wrapWidth = 0) {

	startPos = testgetTextStartPos(text, font, startPos, align, wrapWidth);
	// startPos = vec2(roundInt((int)startPos.x), roundInt((int)startPos.y));

	glBindTexture(GL_TEXTURE_2D, font->tex.id);
	Vec4 c = colorSRGB(color);
	Vec4 c2 = colorSRGB(colorOutline);
	glBegin(GL_QUADS);

	TextSimInfo tsi = initTextSimInfo(startPos);
	while(true) {
		TextInfo ti;
		if(!textSim(text, font, &tsi, &ti, startPos, wrapWidth)) break;
		if(text[ti.index] == '\n') continue;

		glColor4f(c2.r,c2.g,c2.b,c2.a);

		for(int i = 0; i < 8; i++) {
			Vec2 dir = rotateVec2(vec2(1,0), (M_2PI/8)*i);

			Rect nr = rectTrans(ti.r, dir*outlineSize);
			glTexCoord2f(ti.uv.left,  ti.uv.top);    glVertex3f(nr.left, nr.bottom, 0.0);
			glTexCoord2f(ti.uv.left,  ti.uv.bottom); glVertex3f(nr.left, nr.top, 0.0);
			glTexCoord2f(ti.uv.right, ti.uv.bottom); glVertex3f(nr.right, nr.top, 0.0);
			glTexCoord2f(ti.uv.right, ti.uv.top);    glVertex3f(nr.right, nr.bottom, 0.0);
		}

		glColor4f(c.r, c.g, c.b, c.a);

		glTexCoord2f(ti.uv.left,  ti.uv.top);    glVertex3f(ti.r.left, ti.r.bottom, 0.0);
		glTexCoord2f(ti.uv.left,  ti.uv.bottom); glVertex3f(ti.r.left, ti.r.top, 0.0);
		glTexCoord2f(ti.uv.right, ti.uv.bottom); glVertex3f(ti.r.right, ti.r.top, 0.0);
		glTexCoord2f(ti.uv.right, ti.uv.top);    glVertex3f(ti.r.right, ti.r.bottom, 0.0);

	}
	
	glEnd();
}

void drawText(char* text, Font* font, Vec2 startPos, Vec4 color, Vec2i align, int wrapWidth, float shadow, Vec4 shadowColor = vec4(0,1)) {

	Vec2 dr = normVec2(vec2(1,-1));

	startPos = testgetTextStartPos(text, font, startPos, align, wrapWidth);
	// startPos = vec2(roundInt((int)startPos.x), roundInt((int)startPos.y));

	Vec4 c, sc;
	glBindTexture(GL_TEXTURE_2D, font->tex.id);
	c = colorSRGB(color);
	sc = colorSRGB(shadowColor);
	glColor4f(c.r, c.g, c.b, c.a);
	glBegin(GL_QUADS);

	TextSimInfo tsi = initTextSimInfo(startPos);
	while(true) {
		TextInfo ti;
		if(!textSim(text, font, &tsi, &ti, startPos, wrapWidth)) break;
		if(text[ti.index] == '\n') continue;

		if(shadow > 0) {
			glColor4f(sc.r, sc.g, sc.b, sc.a);

			Rect r = rectTrans(ti.r, dr * shadow);
			glTexCoord2f(ti.uv.left,  ti.uv.top);    glVertex3f(r.left, r.bottom, 0.0);
			glTexCoord2f(ti.uv.left,  ti.uv.bottom); glVertex3f(r.left, r.top, 0.0);
			glTexCoord2f(ti.uv.right, ti.uv.bottom); glVertex3f(r.right, r.top, 0.0);
			glTexCoord2f(ti.uv.right, ti.uv.top);    glVertex3f(r.right, r.bottom, 0.0);

			glColor4f(c.r, c.g, c.b, c.a);
		}

		glTexCoord2f(ti.uv.left,  ti.uv.top);    glVertex3f(ti.r.left, ti.r.bottom, 0.0);
		glTexCoord2f(ti.uv.left,  ti.uv.bottom); glVertex3f(ti.r.left, ti.r.top, 0.0);
		glTexCoord2f(ti.uv.right, ti.uv.bottom); glVertex3f(ti.r.right, ti.r.top, 0.0);
		glTexCoord2f(ti.uv.right, ti.uv.top);    glVertex3f(ti.r.right, ti.r.bottom, 0.0);
		
	}

	glEnd();
}

void drawTextLineCulled(char* text, Font* font, Vec2 startPos, float width, Vec4 color, Vec2i align = vec2i(-1,1)) {
	startPos = testgetTextStartPos(text, font, startPos, align, 0);
	startPos = vec2(roundInt((int)startPos.x), roundInt((int)startPos.y));

	TextSimInfo tsi = initTextSimInfo(startPos);
	while(true) {
		TextInfo ti;
		if(!textSim(text, font, &tsi, &ti, startPos, 0)) break;
		if(text[ti.index] == '\n') continue;

		if(ti.pos.x > startPos.x + width) break;

		drawRect(ti.r, color, ti.uv, font->tex.id);
	}
}

Vec2 textIndexToPos(char* text, Font* font, Vec2 startPos, int index, Vec2i align = vec2i(-1,1), int wrapWidth = 0) {
	startPos = testgetTextStartPos(text, font, startPos, align, wrapWidth);

	TextSimInfo tsi = initTextSimInfo(startPos);
	while(true) {
		TextInfo ti;
		int result = textSim(text, font, &tsi, &ti, startPos, wrapWidth);

		if(ti.index == index) {
			Vec2 pos = ti.pos - vec2(0, font->height/2);
			return pos;
		}

		if(!result) break;
	}

	return vec2(0,0);
}

void drawTextSelection(char* text, Font* font, Vec2 startPos, int index1, int index2, Vec4 color, Vec2i align = vec2i(-1,1), int wrapWidth = 0) {
	if(index1 == index2) return;
	if(index1 > index2) swap(&index1, &index2);

	startPos = testgetTextStartPos(text, font, startPos, align, wrapWidth);

	Vec2 lineStart;
	bool drawSelection = false;

	TextSimInfo tsi = initTextSimInfo(startPos);
	while(true) {
		TextInfo ti;
		int result = textSim(text, font, &tsi, &ti, startPos, wrapWidth);

		bool endReached = ti.index == index2;

		if(drawSelection) {
			if(ti.lineBreak || endReached) {

				Vec2 lineEnd;
				if(ti.lineBreak) lineEnd = ti.breakPos;
				else if(!result) lineEnd = tsi.pos;
				else lineEnd = ti.pos;

				Rect r = rect(lineStart - vec2(0,font->height), lineEnd);
				drawRect(r, color);

				lineStart = ti.pos;

				if(endReached) break;
			}
		}

		if(!drawSelection && (ti.index >= index1)) {
			drawSelection = true;
			lineStart = ti.pos;
		}

		if(!result) break;
	}
}

int textMouseToIndex(char* text, Font* font, Vec2 startPos, Vec2 mousePos, Vec2i align = vec2i(-1,1), int wrapWidth = 0) {
	startPos = testgetTextStartPos(text, font, startPos, align, wrapWidth);

	if(mousePos.y > startPos.y) return 0;
	
	bool foundLine = false;
	TextSimInfo tsi = initTextSimInfo(startPos);
	while(true) {
		TextInfo ti;
		int result = textSim(text, font, &tsi, &ti, startPos, wrapWidth);
		
		bool fLine = valueBetween(mousePos.y, ti.pos.y - font->height, ti.pos.y);
		if(fLine) foundLine = true;
		else if(foundLine) return ti.index-1;

	    if(foundLine) {
	    	float charMid = ti.pos.x + ti.posAdvance.x*0.5f;
			if(mousePos.x < charMid) return ti.index;
		}

		if(!result) break;
	}

	return tsi.index;
}

char* textSelectionToString(char* text, int index1, int index2) {
	myAssert(index1 >= 0 && index2 >= 0);

	int range = abs(index1 - index2);
	char* str = getTStringDebug(range + 1); // We assume text selection will only be used for debug things.
	strCpy(str, text + minInt(index1, index2), range);
	return str;
}

uint createSampler(float ani, int wrapS, int wrapT, int magF, int minF, int wrapR = GL_CLAMP_TO_EDGE) {
	uint result;
	glCreateSamplers(1, &result);

	glSamplerParameteri(result, GL_TEXTURE_MAX_ANISOTROPY_EXT, ani);
	glSamplerParameteri(result, GL_TEXTURE_WRAP_S, wrapS);
	glSamplerParameteri(result, GL_TEXTURE_WRAP_T, wrapT);
	glSamplerParameteri(result, GL_TEXTURE_MAG_FILTER, magF);
	glSamplerParameteri(result, GL_TEXTURE_MIN_FILTER, minF);

	glSamplerParameteri(result, GL_TEXTURE_WRAP_R, wrapR);

	return result;
}
