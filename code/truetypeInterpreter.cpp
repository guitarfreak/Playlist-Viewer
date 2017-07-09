//MIT, 2015, Michael Popoloski's SharpFont

#define assertPrint(condition, string) \
	if((condition)) printf(string); \
	STBTT_assert(!(condition));

struct TrueTypeVertex {
	Vec2 p;
	bool onCurve;
};

TrueTypeVertex trueTypeVertex(Vec2 p, bool onCurve) {
	TrueTypeVertex v;
	v.onCurve = onCurve;
	v.p = p;
	return v;
}

void scaleGlyphShape(TrueTypeVertex* vertices, int vertexCount, float scale) {
	for(int i = 0; i < vertexCount; i++) vertices[i].p *= scale;
}

int glyphAddPhantomPoints(stbtt_fontinfo* info, int glyphIndex, TrueTypeVertex* vertices, int vertexCount) {
	int asc, desc, lineGap;
	stbtt_GetFontVMetrics(info, &asc, &desc, &lineGap);
	int advanceWidth, leftSideBearing;
	stbtt_GetGlyphHMetrics(info, glyphIndex, &advanceWidth, &leftSideBearing);

	vertices[vertexCount] = trueTypeVertex(vec2(0,0), false); vertexCount += 1;
	vertices[vertexCount] = trueTypeVertex(vec2(advanceWidth,0), false); vertexCount += 1;
	vertices[vertexCount] = trueTypeVertex(vec2(0,asc), false); vertexCount += 1;
	vertices[vertexCount] = trueTypeVertex(vec2(0,desc), false); vertexCount += 1;
	return vertexCount;
}



// Should be used to put glyph shapes into interpreter.points.Original
STBTT_DEF int getGlyphShapeStraight(const stbtt_fontinfo *info, int glyph_index, TrueTypeVertex** pvertices)
{
   stbtt_int16 numberOfContours;
   stbtt_uint8 *endPtsOfContours;
   stbtt_uint8 *data = info->data;
   stbtt_vertex *vertices=0;
   TrueTypeVertex *ttvertices=0;
   int num_vertices=0;
   int g = stbtt__GetGlyfOffset(info, glyph_index);

   if (g < 0) return 0;

   numberOfContours = ttSHORT(data + g);

   if (numberOfContours > 0) {
      stbtt_uint8 flags=0,flagcount;
      stbtt_int32 ins, i,j=0,m,n, next_move, was_off=0, off, start_off=0;
      stbtt_int32 x,y,cx,cy,sx,sy, scx,scy;
      stbtt_uint8 *points;
      endPtsOfContours = (data + g + 10);
      ins = ttUSHORT(data + g + 10 + numberOfContours * 2);
      points = data + g + 10 + numberOfContours * 2 + 2 + ins;

      n = 1+ttUSHORT(endPtsOfContours + numberOfContours*2-2);

      m = n + 2*numberOfContours;  // a loose bound on how many vertices we might need
      vertices = (stbtt_vertex *) STBTT_malloc(m * sizeof(vertices[0]), info->userdata);
      ttvertices = (TrueTypeVertex *) STBTT_malloc((m+4/*phantompoints*/) * sizeof(ttvertices[0]), info->userdata);

      if (vertices == 0)
         return 0;

      next_move = 0;
      flagcount=0;

      // in first pass, we load uninterpreted data into the allocated array
      // above, shifted to the end of the array so we won't overwrite it when
      // we create our final data starting from the front

      off = m - n; // starting offset for uninterpreted data, regardless of how m ends up being calculated

      // first load flags

      for (i=0; i < n; ++i) {
         if (flagcount == 0) {
            flags = *points++;
            if (flags & 8)
               flagcount = *points++;
         } else
            --flagcount;
         vertices[off+i].type = flags;
      }

      // now load x coordinates
      x=0;
      for (i=0; i < n; ++i) {
         flags = vertices[off+i].type;
         if (flags & 2) {
            stbtt_int16 dx = *points++;
            x += (flags & 16) ? dx : -dx; // ???
         } else {
            if (!(flags & 16)) {
               x = x + (stbtt_int16) (points[0]*256 + points[1]);
               points += 2;
            }
         }
         vertices[off+i].x = (stbtt_int16) x;
      }

      // now load y coordinates
      y=0;
      for (i=0; i < n; ++i) {
         flags = vertices[off+i].type;
         if (flags & 4) {
            stbtt_int16 dy = *points++;
            y += (flags & 32) ? dy : -dy; // ???
         } else {
            if (!(flags & 32)) {
               y = y + (stbtt_int16) (points[0]*256 + points[1]);
               points += 2;
            }
         }
         vertices[off+i].y = (stbtt_int16) y;
      }

      // now convert them to our format
      num_vertices=0;
      sx = sy = cx = cy = scx = scy = 0;
      for (i=0; i < n; ++i) {
         flags = vertices[off+i].type;
         x     = (stbtt_int16) vertices[off+i].x;
         y     = (stbtt_int16) vertices[off+i].y;

         if (next_move == i) {

            // now start the new one               
            start_off = !(flags & 1);

           sx = x;
           sy = y;

           ttvertices[num_vertices++] = trueTypeVertex(vec2(sx, sy), !start_off);

            was_off = 0;
            next_move = 1 + ttUSHORT(endPtsOfContours+j*2);
            ++j;
         } else {
           	ttvertices[num_vertices++] = trueTypeVertex(vec2(x, y), (flags & 1));
         }
      }

   	stbtt_FreeShape(info, vertices);	

   } else if (numberOfContours == -1) {
      STBTT_assert(0); // We dont want to handle these right now.

   } else if (numberOfContours < 0) {
      // @TODO other compound variations?
      STBTT_assert(0);
   } else {
      // numberOfCounters == 0, do nothing
   }

   *pvertices = ttvertices;
   return num_vertices;
}

int trueTypeGlyphToStb(const stbtt_fontinfo *info, TrueTypeVertex* ttvertices, int ttcount, int* contours, int contourCount, float scale, stbtt_vertex **pvertices) {

	int vertexCount = 0;
    stbtt_vertex* vertices = (stbtt_vertex *) STBTT_malloc((ttcount + contourCount) * sizeof(vertices[0]), info->userdata);


    int num_vertices;

    stbtt_int32 ins, i,j=0,m,n, next_move, was_off=0, off, start_off=0;
    stbtt_int32 x,y,cx,cy,sx,sy, scx,scy;
    stbtt_uint8 *points;

    n = ttcount;
	next_move = 0;

	// now convert them to our format
	num_vertices=0;
	sx = sy = cx = cy = scx = scy = 0;
	for (int i=0; i < n; ++i) {
	   x     = (stbtt_int16) (ttvertices[i].p.x / scale);
	   y     = (stbtt_int16) (ttvertices[i].p.y / scale);

	   if (next_move == i) {
	      if (i != 0)
	         num_vertices = stbtt__close_shape(vertices, num_vertices, was_off, start_off, sx,sy,scx,scy,cx,cy);

	      // now start the new one               
	      start_off = !ttvertices[i].onCurve;
	      if (start_off) {
	         // if we start off with an off-curve point, then when we need to find a point on the curve
	         // where we can start, and we need to save some state for when we wraparound.
	         scx = x;
	         scy = y;
	         if (!(ttvertices[i+1].onCurve)) {
	            // next point is also a curve point, so interpolate an on-point curve
	            sx = (x + (stbtt_int32) (ttvertices[i+1].p.x / scale)) >> 1;
	            sy = (y + (stbtt_int32) (ttvertices[i+1].p.y / scale)) >> 1;
	         } else {
	            // otherwise just use the next point as our start point
	            sx = (stbtt_int32) (ttvertices[i+1].p.x / scale);
	            sy = (stbtt_int32) (ttvertices[i+1].p.y / scale);
	            ++i; // we're using point i+1 as the starting point, so skip it
	         }
	      } else {
	         sx = x;
	         sy = y;
	      }
	      stbtt_setvertex(&vertices[num_vertices++], STBTT_vmove,sx,sy,0,0);
	      was_off = 0;
	      next_move = 1 + contours[j];
	      ++j;
	   } else {
	      if (!ttvertices[i].onCurve) { // if it's a curve
	         if (was_off) // two off-curve control points in a row means interpolate an on-curve midpoint
	            stbtt_setvertex(&vertices[num_vertices++], STBTT_vcurve, (cx+x)>>1, (cy+y)>>1, cx, cy);
	         cx = x;
	         cy = y;
	         was_off = 1;
	      } else {
	         if (was_off)
	            stbtt_setvertex(&vertices[num_vertices++], STBTT_vcurve, x,y, cx, cy);
	         else
	            stbtt_setvertex(&vertices[num_vertices++], STBTT_vline, x,y,0,0);
	         was_off = 0;
	      }
	   }
	}
	num_vertices = stbtt__close_shape(vertices, num_vertices, was_off, start_off, sx,sy,scx,scy,cx,cy);

	*pvertices = vertices;

	return num_vertices;
}

int getNextCompoundGlyph(stbtt_uint16* flags, stbtt_uint16* gidx, float mtx[6], float* m, float* n, stbtt_uint8** compPtr) {
	stbtt_uint8* comp = *compPtr;

	(*flags) = ttSHORT(comp); comp+=2;
	*gidx = ttSHORT(comp); comp+=2;
	TrueTypeVertex *comp_verts = 0, *tmp = 0;

	float mtxInit[6] = {1,0,0,1,0,0};
	for(int i = 0; i < 6; i++) mtx[i] = mtxInit[i];

	bool more = (*flags) & (1<<5);

	if ((*flags) & 2) { // XY values
	   if ((*flags) & 1) { // shorts
	      mtx[4] = ttSHORT(comp); comp+=2;
	      mtx[5] = ttSHORT(comp); comp+=2;
	   } else {
	      mtx[4] = ttCHAR(comp); comp+=1;
	      mtx[5] = ttCHAR(comp); comp+=1;
	   }
	}
	else {
	   // @TODO handle matching point
	   STBTT_assert(0);
	}
	if ((*flags) & (1<<3)) { // WE_HAVE_A_SCALE
	   mtx[0] = mtx[3] = ttSHORT(comp)/16384.0f; comp+=2;
	   mtx[1] = mtx[2] = 0;
	} else if ((*flags) & (1<<6)) { // WE_HAVE_AN_X_AND_YSCALE
	   mtx[0] = ttSHORT(comp)/16384.0f; comp+=2;
	   mtx[1] = mtx[2] = 0;
	   mtx[3] = ttSHORT(comp)/16384.0f; comp+=2;
	} else if ((*flags) & (1<<7)) { // WE_HAVE_A_TWO_BY_TWO
	   mtx[0] = ttSHORT(comp)/16384.0f; comp+=2;
	   mtx[1] = ttSHORT(comp)/16384.0f; comp+=2;
	   mtx[2] = ttSHORT(comp)/16384.0f; comp+=2;
	   mtx[3] = ttSHORT(comp)/16384.0f; comp+=2;
	}

	// Find transformation scales.
	*m = (float) STBTT_sqrt(mtx[0]*mtx[0] + mtx[1]*mtx[1]);
	*n = (float) STBTT_sqrt(mtx[2]*mtx[2] + mtx[3]*mtx[3]);

	*compPtr = comp;
	return more;
}



#define Sqrt2Over2 ((float)(sqrt(2) / 2))
#define MaxCallStack 128
#define Epsilon 0.000001f

struct TrueTypeInterpreter {
	enum OpCode {
	    OPCODE_SVTCA0,
	    OPCODE_SVTCA1,
	    OPCODE_SPVTCA0,
	    OPCODE_SPVTCA1,
	    OPCODE_SFVTCA0,
	    OPCODE_SFVTCA1,
	    OPCODE_SPVTL0,
	    OPCODE_SPVTL1,
	    OPCODE_SFVTL0,
	    OPCODE_SFVTL1,
	    OPCODE_SPVFS,
	    OPCODE_SFVFS,
	    OPCODE_GPV,
	    OPCODE_GFV,
	    OPCODE_SFVTPV,
	    OPCODE_ISECT,
	    OPCODE_SRP0,
	    OPCODE_SRP1,
	    OPCODE_SRP2,
	    OPCODE_SZP0,
	    OPCODE_SZP1,
	    OPCODE_SZP2,
	    OPCODE_SZPS,
	    OPCODE_SLOOP,
	    OPCODE_RTG,
	    OPCODE_RTHG,
	    OPCODE_SMD,
	    OPCODE_ELSE,
	    OPCODE_JMPR,
	    OPCODE_SCVTCI,
	    OPCODE_SSWCI,
	    OPCODE_SSW,
	    OPCODE_DUP,
	    OPCODE_POP,
	    OPCODE_CLEAR,
	    OPCODE_SWAP,
	    OPCODE_DEPTH,
	    OPCODE_CINDEX,
	    OPCODE_MINDEX,
	    OPCODE_ALIGNPTS,
	    /* unused: 0x28 */
	    OPCODE_UTP = 0x29,
	    OPCODE_LOOPCALL,
	    OPCODE_CALL,
	    OPCODE_FDEF,
	    OPCODE_ENDF,
	    OPCODE_MDAP0,
	    OPCODE_MDAP1,
	    OPCODE_IUP0,
	    OPCODE_IUP1,
	    OPCODE_SHP0,
	    OPCODE_SHP1,
	    OPCODE_SHC0,
	    OPCODE_SHC1,
	    OPCODE_SHZ0,
	    OPCODE_SHZ1,
	    OPCODE_SHPIX,
	    OPCODE_IP,
	    OPCODE_MSIRP0,
	    OPCODE_MSIRP1,
	    OPCODE_ALIGNRP,
	    OPCODE_RTDG,
	    OPCODE_MIAP0,
	    OPCODE_MIAP1,
	    OPCODE_NPUSHB,
	    OPCODE_NPUSHW,
	    OPCODE_WS,
	    OPCODE_RS,
	    OPCODE_WCVTP,
	    OPCODE_RCVT,
	    OPCODE_GC0,
	    OPCODE_GC1,
	    OPCODE_SCFS,
	    OPCODE_MD0,
	    OPCODE_MD1,
	    OPCODE_MPPEM,
	    OPCODE_MPS,
	    OPCODE_FLIPON,
	    OPCODE_FLIPOFF,
	    OPCODE_DEBUG,
	    OPCODE_LT,
	    OPCODE_LTEQ,
	    OPCODE_GT,
	    OPCODE_GTEQ,
	    OPCODE_EQ,
	    OPCODE_NEQ,
	    OPCODE_ODD,
	    OPCODE_EVEN,
	    OPCODE_IF,
	    OPCODE_EIF,
	    OPCODE_AND,
	    OPCODE_OR,
	    OPCODE_NOT,
	    OPCODE_DELTAP1,
	    OPCODE_SDB,
	    OPCODE_SDS,
	    OPCODE_ADD,
	    OPCODE_SUB,
	    OPCODE_DIV,
	    OPCODE_MUL,
	    OPCODE_ABS,
	    OPCODE_NEG,
	    OPCODE_FLOOR,
	    OPCODE_CEILING,
	    OPCODE_ROUND0,
	    OPCODE_ROUND1,
	    OPCODE_ROUND2,
	    OPCODE_ROUND3,
	    OPCODE_NROUND0,
	    OPCODE_NROUND1,
	    OPCODE_NROUND2,
	    OPCODE_NROUND3,
	    OPCODE_WCVTF,
	    OPCODE_DELTAP2,
	    OPCODE_DELTAP3, 
	    OPCODE_DELTAC1,
	    OPCODE_DELTAC2,
	    OPCODE_DELTAC3,
	    OPCODE_SROUND,
	    OPCODE_S45ROUND,
	    OPCODE_JROT,
	    OPCODE_JROF,
	    OPCODE_ROFF,
	    /* unused: 0x7B */
	    OPCODE_RUTG = 0x7C,
	    OPCODE_RDTG,
	    OPCODE_SANGW,
	    OPCODE_AA,
	    OPCODE_FLIPPT,
	    OPCODE_FLIPRGON,
	    OPCODE_FLIPRGOFF,
	    /* unused: 0x83 - 0x84 */
	    OPCODE_SCANCTRL = 0x85,
	    OPCODE_SDPVTL0,
	    OPCODE_SDPVTL1,
	    OPCODE_GETINFO,
	    OPCODE_IDEF,
	    OPCODE_ROLL,
	    OPCODE_MAX,
	    OPCODE_MIN,
	    OPCODE_SCANTYPE,
	    OPCODE_INSTCTRL,
	    /* unused: 0x8F - 0xAF */
	    OPCODE_PUSHB1 = 0xB0,
	    OPCODE_PUSHB2,
	    OPCODE_PUSHB3,
	    OPCODE_PUSHB4,
	    OPCODE_PUSHB5,
	    OPCODE_PUSHB6,
	    OPCODE_PUSHB7,
	    OPCODE_PUSHB8,
	    OPCODE_PUSHW1,
	    OPCODE_PUSHW2,
	    OPCODE_PUSHW3,
	    OPCODE_PUSHW4,
	    OPCODE_PUSHW5,
	    OPCODE_PUSHW6,
	    OPCODE_PUSHW7,
	    OPCODE_PUSHW8,
	    OPCODE_MDRP,           // range of 32 values, 0xC0 - 0xDF,
	    OPCODE_MIRP = 0xE0     // range of 32 values, 0xE0 - 0xFF
	};

	enum TouchStateEnum {
	    TOUCH_STATE_None = 0,
	    TOUCH_STATE_X = 0x1,
	    TOUCH_STATE_Y = 0x2,
	    TOUCH_STATE_Both = 0x1 | 0x2,
	};

	enum InstructionControlFlags {
	    CONTROL_FLAG_None,
	    CONTROL_FLAG_InhibitGridFitting = 0x1,
	    CONTROL_FLAG_UseDefaultGraphicsState = 0x2
	};

	enum RoundMode {
	    ROUND_MODE_ToHalfGrid,
	    ROUND_MODE_ToGrid,
	    ROUND_MODE_ToDoubleGrid,
	    ROUND_MODE_DownToGrid,
	    ROUND_MODE_UpToGrid,
	    ROUND_MODE_Off,
	    ROUND_MODE_Super,
	    ROUND_MODE_Super45,
	};

	struct Zone {
	    bool IsTwilight;
	    TrueTypeVertex* Current;
	    TrueTypeVertex* Original;
	    int* TouchState;
	    int Count;
	    int MaxCount;

	    void init(int maxCount, bool isTwilight) {
			this->IsTwilight = isTwilight;
			this->Count = 0;
			this->MaxCount = maxCount;

			Current = mallocArray(TrueTypeVertex, maxCount);
			for(int i = 0; i < maxCount; i++) Current[i] = {};

			Original = mallocArray(TrueTypeVertex, maxCount);
			for(int i = 0; i < maxCount; i++) Original[i] = {};

			TouchState = mallocArray(int, maxCount);
			for(int i = 0; i < maxCount; i++) TouchState[i] = TOUCH_STATE_None;
	    }

	    void free() {
	    	::free(Current);
	    	::free(Original);
	    	::free(TouchState);
	    }

		// Assumes you already setup Original.
	    void setupPointsFromOriginal() {
	    	assert(this->Count <= this->MaxCount);

			for(int i = 0; i < Count; i++) {
				Current[i] = Original[i];
			}

			// Round last 4 phantom points for current.
			for(int i = 0; i < 4; i++) {
				Current[Count-1-i].p.x = roundFloat(Current[Count-1-i].p.x);
				Current[Count-1-i].p.y = roundFloat(Current[Count-1-i].p.y);
			}

	    	for(int i = 0; i < Count; i++) TouchState[i] = TOUCH_STATE_None;
	    }

	    Vec2 GetCurrent(int index) { return Current[index].p; }
	    Vec2 GetOriginal(int index) { return Original[index].p; }
	};

    struct GraphicsState {
        Vec2 Freedom;
        Vec2 DualProjection;
        Vec2 Projection;
        InstructionControlFlags InstructionControl;
        RoundMode RoundState;
        float MinDistance;
        float ControlValueCutIn;
        float SingleWidthCutIn;
        float SingleWidthValue;
        int DeltaBase;
        int DeltaShift;
        int Loop;
        int Rp0;
        int Rp1;
        int Rp2;
        bool AutoFlip;

        void Init() {
            Freedom = vec2(1,0);
            Projection = vec2(1,0);
            DualProjection = vec2(1,0);
            InstructionControl = CONTROL_FLAG_None;
            RoundState = ROUND_MODE_ToGrid;
            MinDistance = 1.0f;
            ControlValueCutIn = 17.0f / 16.0f;
            SingleWidthCutIn = 0.0f;
            SingleWidthValue = 0.0f;
            DeltaBase = 9;
            DeltaShift = 3;
            Loop = 1;
            Rp0 = Rp1 = Rp2 = 0;
            AutoFlip = true;
        }
    };

	struct InstructionStream {
	    stbtt_uint8* instructions;
	    int instructionsCount;
	    int ip;

	    bool IsValid() { return instructions != 0; }
	    bool Done() { return ip >= instructionsCount; }

	    InstructionStream() {}
	    InstructionStream(uchar* instructions, int count) {
	        this->instructions = instructions;
	        this->instructionsCount = count;
	        ip = 0;
	    }

	    int NextByte() {
	        assertPrint(Done(), "");
	        return instructions[ip++];
	    }

	    int NextOpCode() { return NextByte(); }
	    int NextWord() { 
	    	int byte1 = NextByte();
	    	int byte2 = NextByte();
	    	byte1 = byte1 << 8;
	    	short word = (short)(byte1 | byte2);
		    return word;
	    }
	    void Jump(int offset) { ip += offset; }
	};

	struct ExecutionStack {
	    int* s;
	    int sMaxCount;
	    int count;

	    ExecutionStack() {};
	    ExecutionStack(int maxStack) {
	        s = mallocArray(int, maxStack);
	        sMaxCount = maxStack;
	        count = 0;
	    }

	    int Peek() { return Peek(0); }
	    bool PopBool() { return Pop() != 0; }
	    float PopFloat() { return F26Dot6ToFloat(Pop()); }
	    void Push(bool value) { Push(value ? 1 : 0); }
	    void Push(float value) { Push(FloatToF26Dot6(value)); }

	    void Clear() { count = 0; }
	    void Depth() { Push(count); }
	    void Duplicate() { Push(Peek()); }
	    void Copy() { Copy(Pop() - 1); }
	    void Copy(int index) { Push(Peek(index)); }
	    void Move() { Move(Pop() - 1); }
	    void Roll() { Move(2); }

	    void Move(int index) {
	        int val = Peek(index);
	        for (int i = count - index - 1; i < count - 1; i++)
	            s[i] = s[i + 1];
	        s[count - 1] = val;
	    }

	    void Swap() {
	        STBTT_assert(!(count < 2));

	        int tmp = s[count - 1];
	        s[count - 1] = s[count - 2];
	        s[count - 2] = tmp;
	    }

	    void Push(int value) {
	        assertPrint(count == sMaxCount, "");
	        s[count++] = value;
	    }

	    int Pop() {
	        assertPrint(count == 0, "");
	        return s[--count];
	    }

	    int Peek(int index) {
	        assertPrint(index < 0 || index >= count, "");
	        return s[count - index - 1];
	    }
	};



	// Vars.

    GraphicsState state;
    GraphicsState cvtState;
    ExecutionStack stack;
    InstructionStream* functions;
    int functionsCount;
    InstructionStream* instructionDefs;
    int instructionDefsCount;
    float* controlValueTable;
    int controlValueTableCount;
    Zone points, twilight;

    InstructionStream callStack[20]; // Enough?

    int* storage;
    int storageCount;
    ushort contours[20]; // Should be enough?.
    int contoursCount;
    float scale;
    int ppem;
    int callStackSize;
    float fdotp;
    float roundThreshold;
    float roundPhase;
    float roundPeriod;
    Zone *zp0, *zp1, *zp2;	

    TrueTypeVertex* finalPoints;
    int finalPointCount;
    TrueTypeVertex finalPhantomPoints[4];
    int finalContours[20];
    int finalContourCount;



    void init() {
    	*this = {};

    	int maxStack = 10000;
    	int maxStorage = 300;
    	int maxFunctions = 300;
    	int maxInstructionDefs = 100;
    	int maxTwilightPoints = 200;
    	int maxPoints = 400;

        stack = ExecutionStack(maxStack);
        storage = mallocArray(int, maxStorage);
        storageCount = maxStorage;
        functions = mallocArray(InstructionStream, maxFunctions);
        functionsCount = 0;
        instructionDefs = mallocArray(InstructionStream, (maxInstructionDefs > 0 ? 256 : 0));
        state = GraphicsState();
        cvtState = GraphicsState();
        controlValueTable = 0;

        points.init(maxPoints, false);
        twilight.init(maxTwilightPoints, true);

        finalPoints = mallocArray(TrueTypeVertex, maxPoints);
        finalPointCount = 0;
        finalContourCount = 0;
    }

    void freeInterpreter() {
    	::free(stack.s);
        ::free(storage);
        ::free(functions);
        ::free(instructionDefs);
        points.free();
        twilight.free();
    }

    void InitializeFunctionDefs(uchar* instructions, int count) {
    	callStackSize = 0;
    	callStack[callStackSize] = InstructionStream(instructions, count);
		Execute(false, true);
    }

    void SetControlValueTable(short* cvt, int cvtCount, float scale, float ppem, uchar* cvProgram, int cvProgramCount) {

        if (this->scale == scale || cvt == 0) return;

        if (controlValueTable == 0) controlValueTable = mallocArray(float, cvtCount);
        
        controlValueTableCount = cvtCount;
        //copy cvt and apply scale
        for (int i = cvtCount - 1; i >= 0; --i) {
        	short v = swapI16(cvt[i]);
            controlValueTable[i] = v * scale;
        }

        this->scale = scale;
        this->ppem = roundInt(ppem);



        // Execute cvprogram.

        state.Init();
        setupRest();

        if(setupCVInstructions(cvProgram, cvProgramCount)) {

			Execute(false, false);

			// save off the CVT graphics state so that we can restore it for each glyph we hint
			if ((state.InstructionControl & CONTROL_FLAG_UseDefaultGraphicsState) != 0) cvtState.Init();
			else {
			    // always reset a few fields; copy the reset
			    cvtState = state;
			    cvtState.Freedom = vec2(1,0);
			    cvtState.Projection = vec2(1,0);
			    cvtState.DualProjection = vec2(1,0);
			    cvtState.RoundState = ROUND_MODE_ToGrid;
			    cvtState.Loop = 1;
			}
        }
    }

	void setupFunctionsAndCvt(stbtt_fontinfo* info, float height) {
		float scale = stbtt_ScaleForPixelHeight(info, height);

		if (info->fpgm != 0) {
		    InitializeFunctionDefs(info->data + info->fpgm, info->fpgmSize);
		}

		if (info->cvt != 0) {
		    SetControlValueTable((short*)(info->data + info->cvt), info->cvtSize, scale, height, info->data + info->prep, info->prepSize);
		}
	}

    void setupContours(uchar* data, int goff) {
    	stbtt_uint8* contourData = (data + goff + 10);
   		int numberOfContours = ttSHORT(data + goff);

    	this->contoursCount = numberOfContours;
    	for(int i = 0; i < contoursCount; i++) 
    		contours[i] = ttUSHORT(contourData + i*2);
    }

    bool setupInstructions(uchar* data) {
    	int instructionsCount = ttSHORT(data); data += sizeof(short);
    	stbtt_uint8* instructions = data;

    	callStackSize = 0;
    	callStack[0] = InstructionStream(instructions, instructionsCount);

    	bool hasInstructions = instructions || instructionsCount;
    	return hasInstructions;
    }

    bool setupInstructions(uchar* data, int goff) {
   		int numberOfContours = ttSHORT(data + goff);
    	int instructionOffset = 5*(sizeof(short)) + numberOfContours*(sizeof(short));

    	return setupInstructions(data + goff + instructionOffset);
    }

    bool setupCVInstructions(uchar* instructions, int instructionsCount) {
    	callStackSize = 0;
    	callStack[0] = InstructionStream(instructions, instructionsCount);

    	bool hasInstructions = instructions || instructionsCount;
    	return hasInstructions;
    }

    int setupRest() {
    	// check if the CVT program disabled hinting
    	if ((state.InstructionControl & CONTROL_FLAG_InhibitGridFitting) != 0) return 0;

    	zp0 = zp1 = zp2 = &points;
    	state = cvtState;
    	stack.Clear();
    	OnVectorsUpdated();

    	// normalize the round state settings
    	switch (state.RoundState) {
    	    case ROUND_MODE_Super: SetSuperRound(1.0f); break;
    	    case ROUND_MODE_Super45: SetSuperRound(Sqrt2Over2); break;
    	}

    	return 1;
    }

    int HintGlyph(stbtt_fontinfo* info, int glyph, TrueTypeVertex** glyphPoints) {

		uchar* data = info->data;

		finalPointCount = 0;
		finalContourCount = 0;

		int goff = stbtt__GetGlyfOffset(info, glyph);
		if(goff == -1) return 0;

   		int numberOfContours = ttSHORT(data + goff);
   		if(numberOfContours > 0) {

	   		// Simple glyph.

   			TrueTypeVertex* vertices;
   			int vertexCount = getGlyphShapeStraight(info, glyph, &vertices);
   			if(vertexCount == 0) return 0;

   			for(int i = 0; i < vertexCount; i++) points.Original[i] = vertices[i];
   			points.Count = vertexCount;
   			points.Count = glyphAddPhantomPoints(info, glyph, points.Original, points.Count);
   			scaleGlyphShape(points.Original, points.Count, scale);
   			points.setupPointsFromOriginal();

			STBTT_free(vertices, info->userdata);


   			setupContours(data, goff);
   			if(!setupInstructions(data, goff)) return 0;
   			if(!setupRest()) return 0;

   			Execute(false, false);


   			for(int i = 0; i < contoursCount; i++) finalContours[finalContourCount++] = contours[i];
   			for(int i = 0; i < points.Count-4; i++) finalPoints[finalPointCount++] = points.Current[i];
   			for(int i = 0; i < 4; i++) finalPhantomPoints[i] = points.Current[points.Count-4+i];

		} else if (numberOfContours == -1) {

			// Compound glyph.

  			stbtt_uint8 *comp = data + goff + 10;
   			bool haveInstructions = false;
   			bool more = true;
   			while(more) {
   	     		stbtt_uint16 flags, gidx;
   	     		TrueTypeVertex *comp_verts = 0;
   	     		float mtx[6], m, n;
   	     		more = getNextCompoundGlyph(&flags, &gidx, mtx, &m, &n, &comp);
		
				int comp_num_verts = getGlyphShapeStraight(info, gidx, &comp_verts);
				if (comp_num_verts > 0) {

					for(int i = 0; i < comp_num_verts; i++) points.Original[i] = comp_verts[i];
 		  			points.Count = comp_num_verts;
					points.Count = glyphAddPhantomPoints(info, gidx, points.Original, points.Count);

					// Transform vertices.
					for (int i = 0; i < points.Count; ++i) {
					   TrueTypeVertex* v = &points.Original[i];
					   float x,y;
					   x=v->p.x; y=v->p.y;
					   v->p.x = (m * (mtx[0]*x + mtx[2]*y + mtx[4]));
					   v->p.y = (n * (mtx[1]*x + mtx[3]*y + mtx[5]));
					}

					scaleGlyphShape(points.Original, points.Count, scale);
					points.setupPointsFromOriginal();


					int goff = stbtt__GetGlyfOffset(info, gidx);
   					setupContours(data, goff);
   					if(!setupInstructions(data, goff)) return 0;
		   			if(!setupRest()) return 0;

					Execute(false, false);


					for(int i = 0; i < contoursCount; i++) finalContours[finalContourCount++] = finalPointCount + contours[i];
					for(int i = 0; i < points.Count-4; i++) finalPoints[finalPointCount++] = points.Current[i];

				}
   	        	STBTT_free(comp_verts, info->userdata);

   	     		haveInstructions = (flags & (1<<8)); // WE_HAVE_INSTRUCTIONS
   			}

   			if(finalPointCount == 0) return 0;

   			if(haveInstructions) {

   				// Hint combined points again with additional global instructions.

   				for(int i = 0; i < finalPointCount; i++) points.Original[i] = finalPoints[i];
   				points.Count = finalPointCount;
   				points.Count = glyphAddPhantomPoints(info, glyph, points.Original, points.Count);

   				for(int i = 0; i < 4; i++) points.Original[points.Count-4+i].p *= scale;
   				for(int i = 0; i < finalContourCount; i++) contours[i] = finalContours[i];
   				contoursCount = finalContourCount;

   				points.setupPointsFromOriginal();


   				if(!setupInstructions(comp)) return 0;
   				setupRest();

   				Execute(false, false);


   				finalContourCount = 0;
   				finalPointCount = 0;
   				for(int i = 0; i < contoursCount; i++) finalContours[finalContourCount++] = finalPointCount + contours[i];
   				for(int i = 0; i < points.Count-4; i++) finalPoints[finalPointCount++] = points.Current[i];
   				for(int i = 0; i < 4; i++) finalPhantomPoints[i] = points.Current[points.Count-4+i];

   			} else {
	   			// No extra instructions so just add untouched phantom points and round.
	   			glyphAddPhantomPoints(info, glyph, finalPhantomPoints, 0);
	   			for(int i = 0; i < 4; i++) {
	   				finalPhantomPoints[i].p.x = roundFloat(finalPhantomPoints[i].p.x*scale);
	   				finalPhantomPoints[i].p.y = roundFloat(finalPhantomPoints[i].p.y*scale);
	   			}
   			}
   		}


        if(glyphPoints) *glyphPoints = finalPoints;
        return finalPointCount;
    }

    int calcBoundingBox(int* x0, int* y0, int* x1, int* y1) {
    	TrueTypeVertex* vertices = finalPoints;

    	if(finalPointCount == 0) {
    		*x0 = 0;
    		*y0 = 0;

    		if(x1) *x1 = 0;
    		if(y1) *y1 = 0;

	    	return 0;
    	}

    	int vertexCount = finalPointCount;

    	float xMin = FLT_MAX, yMin = FLT_MAX;
    	float xMax = FLT_MIN, yMax = FLT_MIN;

    	for(int i = 0; i < vertexCount; i++) {
    		TrueTypeVertex v = vertices[i];

    		if(v.onCurve) {
    			Vec2 p = v.p;

				if(p.x < xMin) xMin = p.x; 
				if(p.y < yMin) yMin = p.y; 
				if(p.x > xMax) xMax = p.x; 
				if(p.y > yMax) yMax = p.y; 
    		}
    	}

    	*x0 = STBTT_ifloor(xMin);
    	*y0 = STBTT_ifloor(yMin);

    	if(x1) *x1 = STBTT_iceil(xMax);
    	if(y1) *y1 = STBTT_iceil(yMax);

    	return 1;
    }

	void getMetrics(int* xAdvance) {
		float advance = finalPhantomPoints[1].p.x;
		*xAdvance = advance;
	}

    void Execute(bool inFunction, bool allowFunctionDefs)
    {
    	InstructionStream* stream = &callStack[callStackSize];

        // dispatch each instruction in the stream
        while (!stream->Done())
        {
            int opcode = stream->NextOpCode();
            switch (opcode) {
	            // ==== PUSH INSTRUCTIONS ====
	            case OPCODE_NPUSHB:
	            case OPCODE_PUSHB1:
	            case OPCODE_PUSHB2:
	            case OPCODE_PUSHB3:
	            case OPCODE_PUSHB4:
	            case OPCODE_PUSHB5:
	            case OPCODE_PUSHB6:
	            case OPCODE_PUSHB7:
	            case OPCODE_PUSHB8: {
                    uchar count = opcode == OPCODE_NPUSHB ? stream->NextByte() : opcode - OPCODE_PUSHB1 + 1;
                    for (int i = count - 1; i >= 0; --i) stack.Push(stream->NextByte());
                } break;
	            case OPCODE_NPUSHW:
	            case OPCODE_PUSHW1:
	            case OPCODE_PUSHW2:
	            case OPCODE_PUSHW3:
	            case OPCODE_PUSHW4:
	            case OPCODE_PUSHW5:
	            case OPCODE_PUSHW6:
	            case OPCODE_PUSHW7:
	            case OPCODE_PUSHW8: {
	                uchar count = opcode == OPCODE_NPUSHW ? stream->NextByte() : opcode - OPCODE_PUSHW1 + 1;
	                for (int i = count - 1; i >= 0; --i) {
	                	int word = stream->NextWord();
		                stack.Push(word);
	                }
	            } break;

	            // ==== STORAGE MANAGEMENT ====
	            case OPCODE_RS:
	                {
	                    int loc = CheckIndex(stack.Pop(), storageCount);
	                    stack.Push(storage[loc]);
	                }
	                break;
	            case OPCODE_WS:
	                {
	                    int value = stack.Pop();
	                    int loc = CheckIndex(stack.Pop(), storageCount);
	                    storage[loc] = value;
	                }
	                break;

	            // ==== CONTROL VALUE TABLE ====
	            case OPCODE_WCVTP:
	                {
	                    float value = stack.PopFloat();
	                    int loc = CheckIndex(stack.Pop(), controlValueTableCount);
	                    controlValueTable[loc] = value;
	                }
	                break;
	            case OPCODE_WCVTF:
	                {
	                    int value = stack.Pop();
	                    int loc = CheckIndex(stack.Pop(), controlValueTableCount);
	                    controlValueTable[loc] = value * scale;
	                }
	                break;
	            case OPCODE_RCVT: stack.Push(ReadCvt()); break;

	            // ==== STATE VECTORS ====
	            case OPCODE_SVTCA0:
	            case OPCODE_SVTCA1:
	                {
	                    int axis = opcode - OPCODE_SVTCA0;
	                    SetFreedomVectorToAxis(axis);
	                    SetProjectionVectorToAxis(axis);
	                }
	                break;
	            case OPCODE_SFVTPV: state.Freedom = state.Projection; OnVectorsUpdated(); break;
	            case OPCODE_SPVTCA0:
	            case OPCODE_SPVTCA1: SetProjectionVectorToAxis(opcode - OPCODE_SPVTCA0); break;
	            case OPCODE_SFVTCA0:
	            case OPCODE_SFVTCA1: SetFreedomVectorToAxis(opcode - OPCODE_SFVTCA0); break;
	            case OPCODE_SPVTL0:
	            case OPCODE_SPVTL1:
	            case OPCODE_SFVTL0:
	            case OPCODE_SFVTL1: SetVectorToLine(opcode - OPCODE_SPVTL0, false); break;
	            case OPCODE_SDPVTL0:
	            case OPCODE_SDPVTL1: SetVectorToLine(opcode - OPCODE_SDPVTL0, true); break;
	            case OPCODE_SPVFS:
	            case OPCODE_SFVFS:
	                {
	                    int y = stack.Pop();
	                    int x = stack.Pop();
	                    Vec2 vec = normVec2(vec2(F2Dot14ToFloat(x), F2Dot14ToFloat(y)));
	                    if (opcode == OPCODE_SFVFS)
	                        state.Freedom = vec;
	                    else
	                    {
	                        state.Projection = vec;
	                        state.DualProjection = vec;
	                    }
	                    OnVectorsUpdated();
	                }
	                break;
	            case OPCODE_GPV:
	            case OPCODE_GFV:
	                {
	                    Vec2 vec = opcode == OPCODE_GPV ? state.Projection : state.Freedom;
	                    stack.Push(FloatToF2Dot14(vec.x));
	                    stack.Push(FloatToF2Dot14(vec.y));
	                }
	                break;

	            // ==== GRAPHICS STATE ====
	            case OPCODE_SRP0: state.Rp0 = stack.Pop(); break;
	            case OPCODE_SRP1: state.Rp1 = stack.Pop(); break;
	            case OPCODE_SRP2: state.Rp2 = stack.Pop(); break;
	            case OPCODE_SZP0: zp0 = GetZoneFromStack(); break;
	            case OPCODE_SZP1: zp1 = GetZoneFromStack(); break;
	            case OPCODE_SZP2: zp2 = GetZoneFromStack(); break;
	            case OPCODE_SZPS: zp0 = zp1 = zp2 = GetZoneFromStack(); break;
	            case OPCODE_RTHG: state.RoundState = ROUND_MODE_ToHalfGrid; break;
	            case OPCODE_RTG: state.RoundState = ROUND_MODE_ToGrid; break;
	            case OPCODE_RTDG: state.RoundState = ROUND_MODE_ToDoubleGrid; break;
	            case OPCODE_RDTG: state.RoundState = ROUND_MODE_DownToGrid; break;
	            case OPCODE_RUTG: state.RoundState = ROUND_MODE_UpToGrid; break;
	            case OPCODE_ROFF: state.RoundState = ROUND_MODE_Off; break;
	            case OPCODE_SROUND: state.RoundState = ROUND_MODE_Super; SetSuperRound(1.0f); break;
	            case OPCODE_S45ROUND: state.RoundState = ROUND_MODE_Super45; SetSuperRound(Sqrt2Over2); break;
	            case OPCODE_INSTCTRL:
	                {
	                    int selector = stack.Pop();
	                    if (selector >= 1 && selector <= 2)
	                    {
	                        // value is false if zero, otherwise shift the right bit into the flags
	                        int bit = 1 << (selector - 1);
	                        if (stack.Pop() == 0)
	                            state.InstructionControl = (InstructionControlFlags)((int)state.InstructionControl & ~bit);
	                        else
	                            state.InstructionControl = (InstructionControlFlags)((int)state.InstructionControl | bit);
	                    }
	                }
	                break;
	            case OPCODE_SCANCTRL: /* instruction unspported */ stack.Pop(); break;
	            case OPCODE_SCANTYPE: /* instruction unspported */ stack.Pop(); break;
	            case OPCODE_SANGW: /* instruction unspported */ stack.Pop(); break;
	            case OPCODE_SLOOP: state.Loop = stack.Pop(); break;
	            case OPCODE_SMD: state.MinDistance = stack.PopFloat(); break;
	            case OPCODE_SCVTCI: state.ControlValueCutIn = stack.PopFloat(); break;
	            case OPCODE_SSWCI: state.SingleWidthCutIn = stack.PopFloat(); break;
	            case OPCODE_SSW: state.SingleWidthValue = stack.Pop() * scale; break;
	            case OPCODE_FLIPON: state.AutoFlip = true; break;
	            case OPCODE_FLIPOFF: state.AutoFlip = false; break;
	            case OPCODE_SDB: state.DeltaBase = stack.Pop(); break;
	            case OPCODE_SDS: state.DeltaShift = stack.Pop(); break;

	            // ==== POINT MEASUREMENT ====
	            case OPCODE_GC0: stack.Push(Project(zp2->GetCurrent(stack.Pop()))); break;
	            case OPCODE_GC1: stack.Push(DualProject(zp2->GetOriginal(stack.Pop()))); break;
	            case OPCODE_SCFS:
	                {
	                    auto value = stack.PopFloat();
	                    auto index = stack.Pop();
	                    auto point = zp2->GetCurrent(index);
	                    MovePoint(zp2, index, value - Project(point));

	                    // moving twilight points moves their "original" value also
	                    if (zp2->IsTwilight)
	                        zp2->Original[index].p = zp2->Current[index].p;
	                }
	                break;
	            case OPCODE_MD0:
	                {
	                    auto p1 = zp1->GetOriginal(stack.Pop());
	                    auto p2 = zp0->GetOriginal(stack.Pop());
	                    stack.Push(DualProject(p2 - p1));
	                }
	                break;
	            case OPCODE_MD1:
	                {
	                    auto p1 = zp1->GetCurrent(stack.Pop());
	                    auto p2 = zp0->GetCurrent(stack.Pop());
	                    stack.Push(Project(p2 - p1));
	                }
	                break;
	            case OPCODE_MPS: // MPS should return point size, but we assume DPI so it's the same as pixel size
	            case OPCODE_MPPEM: stack.Push(ppem); break;
	            case OPCODE_AA: /* deprecated instruction */ stack.Pop(); break;

	            // ==== POINT MODIFICATION ====
	            case OPCODE_FLIPPT:
	                {
	                    for (int i = 0; i < state.Loop; i++)
	                    {
	                        auto index = stack.Pop();
	                        //review here again!
	                        points.Current[index].onCurve = !points.Current[index].onCurve;
	                    }
	                    state.Loop = 1;
	                }
	                break;
	            case OPCODE_FLIPRGON:
	                {
	                    auto end = stack.Pop();
	                    for (int i = stack.Pop(); i <= end; i++)
	                        points.Current[i].onCurve = true;
	                }
	                break;
	            case OPCODE_FLIPRGOFF:
	                {
	                    auto end = stack.Pop();
	                    for (int i = stack.Pop(); i <= end; i++)
	                        points.Current[i].onCurve = false;
	                }
	                break;
	            case OPCODE_SHP0:
	            case OPCODE_SHP1:
	                {
	                    Zone zone;
	                    int point;
	                    auto displacement = ComputeDisplacement((int)opcode, &zone, &point);
	                    ShiftPoints(displacement);
	                }
	                break;
	            case OPCODE_SHPIX: ShiftPoints(stack.PopFloat() * state.Freedom); break;
	            case OPCODE_SHC0:
	            case OPCODE_SHC1:
	                {
	                    Zone zone;
	                    int point;
	                    auto displacement = ComputeDisplacement((int)opcode, &zone, &point);
	                    auto touch = GetTouchState();
	                    auto contour = stack.Pop();
	                    auto start = contour == 0 ? 0 : contours[contour - 1] + 1;
	                    auto count = zp2->IsTwilight ? zp2->Count : contours[contour] + 1;

	                    for (int i = start; i < count; i++)
	                    {
	                        // don't move the reference point
	                        if (zone.Current != zp2->Current || point != i)
	                        {
	                            zp2->Current[i].p += displacement;
	                            zp2->TouchState[i] |= touch;
	                        }
	                    }
	                }
	                break;
	            case OPCODE_SHZ0:
	            case OPCODE_SHZ1:
	                {
	                    Zone zone;
	                    int point;
	                    auto displacement = ComputeDisplacement((int)opcode, &zone, &point);
	                    auto count = 0;
	                    if (zp2->IsTwilight)
	                        count = zp2->Count;
	                    else if (contoursCount > 0)
	                        count = contours[contoursCount - 1] + 1;

	                    for (int i = 0; i < count; i++)
	                    {
	                        // don't move the reference point
	                        if (zone.Current != zp2->Current || point != i)
	                            zp2->Current[i].p += displacement;
	                    }
	                }
	                break;
	            case OPCODE_MIAP0:
	            case OPCODE_MIAP1:
	                {
	                    float distance = ReadCvt();
	                    int pointIndex = stack.Pop();

	                    // this instruction is used in the CVT to set up twilight points with original values
	                    if (zp0->IsTwilight)
	                    {
	                        auto original = state.Freedom * distance;
	                        zp0->Original[pointIndex].p = original;
	                        zp0->Current[pointIndex].p = original;
	                    }

	                    // current position of the point along the projection vector
	                    auto point = zp0->GetCurrent(pointIndex);
	                    auto currentPos = Project(point);
	                    if (opcode == OPCODE_MIAP1)
	                    {
	                        // only use the CVT if we are above the cut-in point
	                        if (fabs(distance - currentPos) > state.ControlValueCutIn)
	                            distance = currentPos;
	                        distance = Round(distance);
	                    }

	                    MovePoint(zp0, pointIndex, distance - currentPos);
	                    state.Rp0 = pointIndex;
	                    state.Rp1 = pointIndex;
	                }
	                break;
	            case OPCODE_MDAP0:
	            case OPCODE_MDAP1:
	                {
	                    auto pointIndex = stack.Pop();
	                    auto point = zp0->GetCurrent(pointIndex);
	                    auto distance = 0.0f;
	                    if (opcode == OPCODE_MDAP1)
	                    {
	                        distance = Project(point);
	                        distance = Round(distance) - distance;
	                    }

	                    MovePoint(zp0, pointIndex, distance);
	                    state.Rp0 = pointIndex;
	                    state.Rp1 = pointIndex;
	                }
	                break;
	            case OPCODE_MSIRP0:
	            case OPCODE_MSIRP1:
	                {
	                    auto targetDistance = stack.PopFloat();
	                    auto pointIndex = stack.Pop();

	                    // if we're operating on the twilight zone, initialize the points
	                    if (zp1->IsTwilight)
	                    {
	                        zp1->Original[pointIndex].p = zp0->Original[state.Rp0].p + (targetDistance * state.Freedom) / fdotp;
	                        zp1->Current[pointIndex].p = zp1->Original[pointIndex].p;
	                    }

	                    auto currentDistance = Project(zp1->GetCurrent(pointIndex) - zp0->GetCurrent(state.Rp0));
	                    MovePoint(zp1, pointIndex, targetDistance - currentDistance);

	                    state.Rp1 = state.Rp0;
	                    state.Rp2 = pointIndex;
	                    if (opcode == OPCODE_MSIRP1)
	                        state.Rp0 = pointIndex;
	                }
	                break;
	            case OPCODE_IP:
	                {
	                    auto originalBase = zp0->GetOriginal(state.Rp1);
	                    auto currentBase = zp0->GetCurrent(state.Rp1);
	                    auto originalRange = DualProject(zp1->GetOriginal(state.Rp2) - originalBase);
	                    auto currentRange = Project(zp1->GetCurrent(state.Rp2) - currentBase);

	                    for (int i = 0; i < state.Loop; i++)
	                    {
	                        auto pointIndex = stack.Pop();
	                        auto point = zp2->GetCurrent(pointIndex);
	                        auto currentDistance = Project(point - currentBase);
	                        auto originalDistance = DualProject(zp2->GetOriginal(pointIndex) - originalBase);

	                        auto newDistance = 0.0f;
	                        if (originalDistance != 0.0f)
	                        {
	                            // a range of 0.0f is invalid according to the spec (would result in a div by zero)
	                            if (originalRange == 0.0f)
	                                newDistance = originalDistance;
	                            else
	                                newDistance = originalDistance * currentRange / originalRange;
	                        }

	                        MovePoint(zp2, pointIndex, newDistance - currentDistance);
	                    }
	                    state.Loop = 1;
	                }
	                break;
	            case OPCODE_ALIGNRP:
	                {
	                    for (int i = 0; i < state.Loop; i++)
	                    {
	                        auto pointIndex = stack.Pop();
	                        auto p1 = zp1->GetCurrent(pointIndex);
	                        auto p2 = zp0->GetCurrent(state.Rp0);
	                        MovePoint(zp1, pointIndex, -Project(p1 - p2));
	                    }
	                    state.Loop = 1;
	                }
	                break;
	            case OPCODE_ALIGNPTS:
	                {
	                    auto p1 = stack.Pop();
	                    auto p2 = stack.Pop();
	                    auto distance = Project(zp0->GetCurrent(p2) - zp1->GetCurrent(p1)) / 2;
	                    MovePoint(zp1, p1, distance);
	                    MovePoint(zp0, p2, -distance);
	                }
	                break;
	            case OPCODE_UTP: zp0->TouchState[stack.Pop()] &= ~GetTouchState(); break;
	            case OPCODE_IUP0:
	            case OPCODE_IUP1:
	                {
	                    // bail if no contours (empty outline)
	                    if (contoursCount == 0)
	                        break;

	                    TrueTypeVertex* currentPtr = points.Current;
	                    TrueTypeVertex* originalPtr = points.Original;
	                    {
	                        // opcode controls whether we care about X or Y direction
	                        // do some pointer trickery so we can operate on the
	                        // points in a direction-agnostic manner
	                        TouchStateEnum touchMask;
	                        uchar* current;
	                        uchar* original;
	                        if (opcode == OPCODE_IUP0)
	                        {
	                            touchMask = TOUCH_STATE_Y;
	                            current = (uchar*)&currentPtr->p.y;
	                            original = (uchar*)&originalPtr->p.y;
	                        }
	                        else
	                        {
	                            touchMask = TOUCH_STATE_X;
	                            current = (uchar*)&currentPtr->p.x;
	                            original = (uchar*)&originalPtr->p.x;
	                        }

	                        auto point = 0;
	                        for (int i = 0; i < contoursCount; i++)
	                        {
	                            auto endPoint = contours[i];
	                            auto firstPoint = point;
	                            auto firstTouched = -1;
	                            auto lastTouched = -1;

	                            for (; point <= endPoint; point++)
	                            {
	                                // check whether this point has been touched
	                                if ((points.TouchState[point] & touchMask) != 0)
	                                {
	                                    // if this is the first touched point in the contour, note it and continue
	                                    if (firstTouched < 0)
	                                    {
	                                        firstTouched = point;
	                                        lastTouched = point;
	                                        continue;
	                                    }

	                                    // otherwise, interpolate all untouched points
	                                    // between this point and our last touched point
	                                    InterpolatePoints(current, original, lastTouched + 1, point - 1, lastTouched, point);
	                                    lastTouched = point;
	                                }
	                            }

	                            // check if we had any touched points at all in this contour
	                            if (firstTouched >= 0)
	                            {
	                                // there are two cases left to handle:
	                                // 1. there was only one touched point in the whole contour, in
	                                //    which case we want to shift everything relative to that one
	                                // 2. several touched points, in which case handle the gap from the
	                                //    beginning to the first touched point and the gap from the last
	                                //    touched point to the end of the contour
	                                if (lastTouched == firstTouched)
	                                {
	                                    auto delta = *GetPoint(current, lastTouched) - *GetPoint(original, lastTouched);
	                                    if (delta != 0.0f)
	                                    {
	                                        for (int j = firstPoint; j < lastTouched; j++)
	                                            *GetPoint(current, j) += delta;
	                                        for (int j = lastTouched + 1; j <= endPoint; j++)
	                                            *GetPoint(current, j) += delta;
	                                    }
	                                }
	                                else
	                                {
	                                    InterpolatePoints(current, original, lastTouched + 1, endPoint, lastTouched, firstTouched);
	                                    if (firstTouched > 0)
	                                        InterpolatePoints(current, original, firstPoint, firstTouched - 1, lastTouched, firstTouched);
	                                }
	                            }
	                        }
	                    }
	                }
	                break;
	            case OPCODE_ISECT:
	                {
	                    // move point P to the intersection of lines A and B
	                    auto b1 = zp0->GetCurrent(stack.Pop());
	                    auto b0 = zp0->GetCurrent(stack.Pop());
	                    auto a1 = zp1->GetCurrent(stack.Pop());
	                    auto a0 = zp1->GetCurrent(stack.Pop());
	                    auto index = stack.Pop();

	                    // calculate intersection using determinants: https://en.wikipedia.org/wiki/Line%E2%80%93line_intersection#Given_two_points_on_each_line
	                    auto da = a0 - a1;
	                    auto db = b0 - b1;
	                    auto den = (da.x * db.y) - (da.y * db.x);
	                    if (fabs(den) <= Epsilon)
	                    {
	                        // parallel lines; spec says to put the ppoint "into the middle of the two lines"
	                        zp2->Current[index].p = (a0 + a1 + b0 + b1) / 4;
	                    }
	                    else
	                    {
	                        auto t = (a0.x * a1.y) - (a0.y * a1.x);
	                        auto u = (b0.x * b1.y) - (b0.y * b1.x);
	                        auto p = vec2(
	                            (t * db.x) - (da.x * u),
	                            (t * db.y) - (da.y * u)
	                        );
	                        zp2->Current[index].p = p / den;
	                    }
	                    zp2->TouchState[index] = TOUCH_STATE_Both;
	                }
	                break;

	            // ==== STACK MANAGEMENT ====
	            case OPCODE_DUP: stack.Duplicate(); break;
	            case OPCODE_POP: stack.Pop(); break;
	            case OPCODE_CLEAR: stack.Clear(); break;
	            case OPCODE_SWAP: stack.Swap(); break;
	            case OPCODE_DEPTH: stack.Depth(); break;
	            case OPCODE_CINDEX: stack.Copy(); break;
	            case OPCODE_MINDEX: stack.Move(); break;
	            case OPCODE_ROLL: stack.Roll(); break;

	            // ==== FLOW CONTROL ====
	            case OPCODE_IF:
	                {
	                    // value is false; jump to the next else block or endif marker
	                    // otherwise, we don't have to do anything; we'll keep executing this block
	                    if (!stack.PopBool())
	                    {
	                        int indent = 1;
	                        while (indent > 0)
	                        {
	                            opcode = SkipNext(stream);
	                            switch (opcode)
	                            {
	                                case OPCODE_IF: indent++; break;
	                                case OPCODE_EIF: indent--; break;
	                                case OPCODE_ELSE:
	                                    if (indent == 1)
	                                        indent = 0;
	                                    break;
	                            }
	                        }
	                    }
	                }
	                break;
	            case OPCODE_ELSE:
	                {
	                    // assume we hit the true statement of some previous if block
	                    // if we had hit false, we would have jumped over this
	                    int indent = 1;
	                    while (indent > 0)
	                    {
	                        opcode = SkipNext(stream);
	                        switch (opcode)
	                        {
	                            case OPCODE_IF: indent++; break;
	                            case OPCODE_EIF: indent--; break;
	                        }
	                    }
	                }
	                break;
	            case OPCODE_EIF: /* nothing to do */ break;
	            case OPCODE_JROT:
	            case OPCODE_JROF:
	                {	                	
	                    if (stack.PopBool() == (opcode == OPCODE_JROT))
	                        stream->Jump(stack.Pop() - 1);
	                    else
	                        stack.Pop();    // ignore the offset
	                }
	                break;
	            case OPCODE_JMPR: stream->Jump(stack.Pop() - 1); break;

	            // ==== LOGICAL OPS ====
	            case OPCODE_LT:
	                {
	                    auto b = stack.Pop();
	                    auto a = stack.Pop();
	                    stack.Push(a < b);
	                }
	                break;
	            case OPCODE_LTEQ:
	                {
	                    auto b = stack.Pop();
	                    auto a = stack.Pop();
	                    stack.Push(a <= b);
	                }
	                break;
	            case OPCODE_GT:
	                {
	                    auto b = stack.Pop();
	                    auto a = stack.Pop();
	                    stack.Push(a > b);
	                }
	                break;
	            case OPCODE_GTEQ:
	                {
	                    auto b = stack.Pop();
	                    auto a = stack.Pop();
	                    stack.Push(a >= b);
	                }
	                break;
	            case OPCODE_EQ:
	                {
	                    auto b = stack.Pop();
	                    auto a = stack.Pop();
	                    stack.Push(a == b);
	                }
	                break;
	            case OPCODE_NEQ:
	                {
	                    auto b = stack.Pop();
	                    auto a = stack.Pop();
	                    stack.Push(a != b);
	                }
	                break;
	            case OPCODE_AND:
	                {
	                    auto b = stack.PopBool();
	                    auto a = stack.PopBool();
	                    stack.Push(a && b);
	                }
	                break;
	            case OPCODE_OR:
	                {
	                    auto b = stack.PopBool();
	                    auto a = stack.PopBool();
	                    stack.Push(a || b);
	                }
	                break;
	            case OPCODE_NOT: stack.Push(!stack.PopBool()); break;
	            case OPCODE_ODD:
	                {
	                    auto value = (int)Round(stack.PopFloat());
	                    stack.Push(value % 2 != 0);
	                }
	                break;
	            case OPCODE_EVEN:
	                {
	                    auto value = (int)Round(stack.PopFloat());
	                    stack.Push(value % 2 == 0);
	                }
	                break;

	            // ==== ARITHMETIC ====
	            case OPCODE_ADD:
	                {
	                    auto b = stack.Pop();
	                    auto a = stack.Pop();
	                    stack.Push(a + b);
	                }
	                break;
	            case OPCODE_SUB:
	                {
	                    auto b = stack.Pop();
	                    auto a = stack.Pop();
	                    stack.Push(a - b);
	                }
	                break;
	            case OPCODE_DIV:
	                {
	                    auto b = stack.Pop();
	                    assertPrint(b == 0, "Division by zero.");

	                    auto a = stack.Pop();
	                    auto result = ((long)a << 6) / b;
	                    stack.Push((int)result);
	                }
	                break;
	            case OPCODE_MUL:
	                {
	                    auto b = stack.Pop();
	                    auto a = stack.Pop();
	                    auto result = ((long)a * b) >> 6;
	                    stack.Push((int)result);
	                }
	                break;
	            case OPCODE_ABS: stack.Push(abs(stack.Pop())); break;
	            case OPCODE_NEG: stack.Push(-stack.Pop()); break;
	            case OPCODE_FLOOR: stack.Push(stack.Pop() & ~63); break;
	            case OPCODE_CEILING: stack.Push((stack.Pop() + 63) & ~63); break;
	            case OPCODE_MAX: stack.Push(maxInt(stack.Pop(), stack.Pop())); break;
	            case OPCODE_MIN: stack.Push(minInt(stack.Pop(), stack.Pop())); break;

	            // ==== FUNCTIONS ====
	            case OPCODE_FDEF:
	                {
	                    assertPrint(!allowFunctionDefs || inFunction, "Can't define functions here.");

	                    functions[stack.Pop()] = *stream;
	                    functionsCount++;

	                    while (SkipNext(stream) != OPCODE_ENDF);
	                }
	                break;
	            case OPCODE_IDEF:
	                {
	                    assertPrint(!allowFunctionDefs || inFunction, "Can't define functions here.");

	                    instructionDefs[stack.Pop()] = *stream;
	                    while (SkipNext(stream) != OPCODE_ENDF);
	                }
	                break;
	            case OPCODE_ENDF:
	                {
	                    assertPrint(!inFunction, "Found invalid ENDF marker outside of a function definition.");
	                    return;
	                }
	            case OPCODE_CALL:
	            case OPCODE_LOOPCALL:
	                {
	                    callStackSize++;
	                    assertPrint(callStackSize > MaxCallStack, "Stack overflow; infinite recursion?");

	                    InstructionStream function = functions[stack.Pop()];
	                    int count = opcode == OPCODE_LOOPCALL ? stack.Pop() : 1;
	                    for (int i = 0; i < count; i++) {
	                    	callStack[callStackSize] = function;
	                        Execute(true, false);
	                    }
	                    callStackSize--;
	                }
	                break;

	            // ==== ROUNDING ====
	            // we don't have "engine compensation" so the variants are unnecessary
	            case OPCODE_ROUND0:
	            case OPCODE_ROUND1:
	            case OPCODE_ROUND2:
	            case OPCODE_ROUND3: stack.Push(Round(stack.PopFloat())); break;
	            case OPCODE_NROUND0:
	            case OPCODE_NROUND1:
	            case OPCODE_NROUND2:
	            case OPCODE_NROUND3: break;

	            // ==== DELTA EXCEPTIONS ====
	            case OPCODE_DELTAC1:
	            case OPCODE_DELTAC2:
	            case OPCODE_DELTAC3:
	                {
	                    auto last = stack.Pop();
	                    for (int i = 1; i <= last; i++)
	                    {
	                        auto cvtIndex = stack.Pop();
	                        auto arg = stack.Pop();

	                        // upper 4 bits of the 8-bit arg is the relative ppem
	                        // the opcode specifies the base to add to the ppem
	                        auto triggerPpem = (arg >> 4) & 0xF;
	                        triggerPpem += (opcode - OPCODE_DELTAC1) * 16;
	                        triggerPpem += state.DeltaBase;

	                        // if the current ppem matches the trigger, apply the exception
	                        if (ppem == triggerPpem)
	                        {
	                            // the lower 4 bits of the arg is the amount to shift
	                            // it's encoded such that 0 isn't an allowable value (who wants to shift by 0 anyway?)
	                            auto amount = (arg & 0xF) - 8;
	                            if (amount >= 0)
	                                amount++;
	                            amount *= 1 << (6 - state.DeltaShift);

	                            // update the CVT
	                            CheckIndex(cvtIndex, controlValueTableCount);
	                            controlValueTable[cvtIndex] += F26Dot6ToFloat(amount);
	                        }
	                    }
	                }
	                break;
	            case OPCODE_DELTAP1:
	            case OPCODE_DELTAP2:
	            case OPCODE_DELTAP3:
	                {
	                    auto last = stack.Pop();
	                    for (int i = 1; i <= last; i++)
	                    {
	                        auto pointIndex = stack.Pop();
	                        auto arg = stack.Pop();

	                        // upper 4 bits of the 8-bit arg is the relative ppem
	                        // the opcode specifies the base to add to the ppem
	                        auto triggerPpem = (arg >> 4) & 0xF;
	                        triggerPpem += state.DeltaBase;
	                        if (opcode != OPCODE_DELTAP1)
	                            triggerPpem += (opcode - OPCODE_DELTAP2 + 1) * 16;

	                        // if the current ppem matches the trigger, apply the exception
	                        if (ppem == triggerPpem)
	                        {
	                            // the lower 4 bits of the arg is the amount to shift
	                            // it's encoded such that 0 isn't an allowable value (who wants to shift by 0 anyway?)
	                            auto amount = (arg & 0xF) - 8;
	                            if (amount >= 0)
	                                amount++;
	                            amount *= 1 << (6 - state.DeltaShift);

	                            MovePoint(zp0, pointIndex, F26Dot6ToFloat(amount));
	                        }
	                    }
	                }
	                break;

	            // ==== MISCELLANEOUS ====
	            case OPCODE_DEBUG: stack.Pop(); break;
	            case OPCODE_GETINFO:
	                {
	                    auto selector = stack.Pop();
	                    auto result = 0;
	                    if ((selector & 0x1) != 0)
	                    {
	                        // pretend we are MS Rasterizer v35
	                        result = 35;
	                    }

	                    // TODO: rotation and stretching
	                    //if ((selector & 0x2) != 0)
	                    //if ((selector & 0x4) != 0)

	                    // we're always rendering in grayscale
	                    if ((selector & 0x20) != 0)
	                        result |= 1 << 12;

	                    // TODO: ClearType flags

	                    stack.Push(result);
	                }
	                break;

                default:
	                if (opcode >= OPCODE_MIRP) MoveIndirectRelative(opcode - OPCODE_MIRP);
	                else if (opcode >= OPCODE_MDRP) MoveDirectRelative(opcode - OPCODE_MDRP);
	                else {
	                    // check if this is a runtime-defined opcode
	                    int index = (int)opcode;
	                    assertPrint(index > instructionDefsCount || !instructionDefs[index].IsValid(), "Unknown opcode in font program.");

	                    callStackSize++;
	                    assertPrint(callStackSize > MaxCallStack, "Stack overflow; infinite recursion?");

	                    callStack[callStackSize] = instructionDefs[index];
	                    Execute(true, false);
	                    callStackSize--;
	                }
            }
        }
    }

    int CheckIndex(int index, int length) {
        assertPrint(index < 0 || index >= length, "");
        return index;
    }

    float ReadCvt() { return controlValueTable[CheckIndex(stack.Pop(), controlValueTableCount)]; }

    void OnVectorsUpdated() {
        fdotp = state.Freedom * state.Projection;
        if (fabs(fdotp) < Epsilon) fdotp = 1.0f;
    }

    void SetFreedomVectorToAxis(int axis) {
        state.Freedom = axis == 0 ? vec2(0,1) : vec2(1,0);
        OnVectorsUpdated();
    }

    void SetProjectionVectorToAxis(int axis) {
        state.Projection = axis == 0 ? vec2(0,1) : vec2(1,0);
        state.DualProjection = state.Projection;

        OnVectorsUpdated();
    }

    void SetVectorToLine(int mode, bool dual) {
        // mode here should be as follows:
        // 0: SPVTL0
        // 1: SPVTL1
        // 2: SFVTL0
        // 3: SFVTL1
        int index1 = stack.Pop();
        int index2 = stack.Pop();
        Vec2 p1 = zp2->GetCurrent(index1);
        Vec2 p2 = zp1->GetCurrent(index2);

        Vec2 line = p2 - p1;
        if (lenSquaredVec2(line) == 0) {
            // invalid; just set to whatever
            if (mode >= 2) state.Freedom = vec2(1,0);
            else {
                state.Projection = vec2(1,0);
                state.DualProjection = vec2(1,0);
            }
        } else {
            // if mode is 1 or 3, we want a perpendicular vector
            if ((mode & 0x1) != 0) line = vec2(-line.y, line.x);
            line = normVec2(line);

            if (mode >= 2) state.Freedom = line;
            else {
                state.Projection = line;
                state.DualProjection = line;
            }
        }

        // set the dual projection vector using original points
        if (dual) {
            p1 = zp2->GetOriginal(index1);
            p2 = zp2->GetOriginal(index2);
            line = p2 - p1;

            if (lenSquaredVec2(line) == 0) state.DualProjection = vec2(1,0);
            else {
                if ((mode & 0x1) != 0) line = vec2(-line.y, line.x);

                state.DualProjection = normVec2(line);
            }
        }

        OnVectorsUpdated();
    }

    // int ZoneValueToType(int value) {
    //     switch (value) {
    //         case 0: return GLYPH_ZONE_TWILIGHT;
    //         case 1: return GLYPH_ZONE_NORMAL;
    //         default: assertPrint(false, "Invalid zone pointer.");
    //         return -1; // Go away compiler.
    //     }
    // }

    // int GetZoneTypeFromStack() { return ZoneValueToType(stack.Pop()); }

    // Zone* GetZone(int type) { 
    // 	switch(type) {
    // 		case GLYPH_ZONE_TWILIGHT: return &twilight;
    // 		case GLYPH_ZONE_NORMAL: return &points;
    // 		default: assertPrint(false, "Invalid zone type.");
    // 		return 0;
    // 	}
    // }

    Zone* GetZoneFromStack() {
        switch (stack.Pop()) {
            case 0: return &twilight;
            case 1: return &points;
            default: assertPrint(false, "Invalid zone pointer.");
            return 0; // Go away compiler.
        }
    }

    void SetSuperRound(float period) {
        // mode is a bunch of packed flags
        // bits 7-6 are the period multiplier
        int mode = stack.Pop();
        switch (mode & 0xC0) {
            case 0: roundPeriod = period / 2; break;
            case 0x40: roundPeriod = period; break;
            case 0x80: roundPeriod = period * 2; break;
            default: assertPrint(false, "Unknown rounding period multiplier.");
        }

        // bits 5-4 are the phase
        switch (mode & 0x30) {
            case 0: roundPhase = 0; break;
            case 0x10: roundPhase = roundPeriod / 4; break;
            case 0x20: roundPhase = roundPeriod / 2; break;
            case 0x30: roundPhase = roundPeriod * 3 / 4; break;
        }

        // bits 3-0 are the threshold
        if ((mode & 0xF) == 0) roundThreshold = roundPeriod - 1;
        else roundThreshold = ((mode & 0xF) - 4) * roundPeriod / 8;
    }

    void MoveIndirectRelative(int flags) {
        // this instruction tries to make the current distance between a given point
        // and the reference point rp0 be equivalent to the same distance in the original outline
        // there are a bunch of flags that control how that distance is measured
        float cvt = ReadCvt();
        int pointIndex = stack.Pop();

        if (fabs(cvt - state.SingleWidthValue) < state.SingleWidthCutIn) {
            if (cvt >= 0) cvt = state.SingleWidthValue;
            else cvt = -state.SingleWidthValue;
        }

        // if we're looking at the twilight zone we need to prepare the points there
        Vec2 originalReference = zp0->GetOriginal(state.Rp0);
        if (zp1->IsTwilight) {
            Vec2 initialValue = originalReference + state.Freedom * cvt;
            zp1->Original[pointIndex].p = initialValue;
            zp1->Current[pointIndex].p = initialValue;
        }

        Vec2 point = zp1->GetCurrent(pointIndex);
        float originalDistance = DualProject(zp1->GetOriginal(pointIndex) - originalReference);
        float currentDistance = Project(point - zp0->GetCurrent(state.Rp0));

        if (state.AutoFlip && !sameSign(originalDistance, cvt)) cvt = -cvt;

        // if bit 2 is set, round the distance and look at the cut-in value
        float distance = cvt;
        if ((flags & 0x4) != 0) {
            // only perform cut-in tests when both points are in the same zone
            if (zp0->IsTwilight == zp1->IsTwilight && fabs(cvt - originalDistance) > state.ControlValueCutIn) cvt = originalDistance;
            distance = Round(cvt);
        }

        // if bit 3 is set, constrain to the minimum distance
        if ((flags & 0x8) != 0) {
            if (originalDistance >= 0) distance = max(distance, state.MinDistance);
            else distance = min(distance, -state.MinDistance);
        }

        // move the point
        MovePoint(zp1, pointIndex, distance - currentDistance);
        state.Rp1 = state.Rp0;
        state.Rp2 = pointIndex;
        if ((flags & 0x10) != 0) state.Rp0 = pointIndex;
    }

    void MoveDirectRelative(int flags) {
        // determine the original distance between the two reference points
        int pointIndex = stack.Pop();
        Vec2 p1 = zp0->GetOriginal(state.Rp0);
        Vec2 p2 = zp1->GetOriginal(pointIndex);
        float originalDistance = DualProject(p2 - p1);

        // single width cutin test
        if (fabs(originalDistance - state.SingleWidthValue) < state.SingleWidthCutIn) {
            if (originalDistance >= 0) originalDistance = state.SingleWidthValue;
            else originalDistance = -state.SingleWidthValue;
        }

        // if bit 2 is set, perform rounding
        float distance = originalDistance;
        if ((flags & 0x4) != 0) distance = Round(distance);

        // if bit 3 is set, constrain to the minimum distance
        if ((flags & 0x8) != 0) {
            if (originalDistance >= 0) distance = max(distance, state.MinDistance);
            else distance = min(distance, -state.MinDistance);
        }

        // move the point
        originalDistance = Project(zp1->GetCurrent(pointIndex) - zp0->GetCurrent(state.Rp0));
        MovePoint(zp1, pointIndex, distance - originalDistance);
        state.Rp1 = state.Rp0;
        state.Rp2 = pointIndex;
        if ((flags & 0x10) != 0) state.Rp0 = pointIndex;
    }

    Vec2 ComputeDisplacement(int mode, Zone* zone, int* point) {
        // compute displacement of the reference point
        if ((mode & 1) == 0) {
            zone = zp1;
            *point = state.Rp2;
        } else {
            zone = zp0;
            *point = state.Rp1;
        }

        float distance = Project(zone->GetCurrent(*point) - zone->GetOriginal(*point));
        return distance * state.Freedom / fdotp;
    }

    int GetTouchState() {
        int touch = TOUCH_STATE_None;
        if (state.Freedom.x != 0) touch = TOUCH_STATE_X;
        if (state.Freedom.y != 0) touch |= TOUCH_STATE_Y;

        return touch;
    }

    void ShiftPoints(Vec2 displacement) {
        int touch = GetTouchState();
        for (int i = 0; i < state.Loop; i++) {
            int pointIndex = stack.Pop();
            zp2->Current[pointIndex].p += displacement;
            zp2->TouchState[pointIndex] |= touch;
        }
        state.Loop = 1;
    }

    void MovePoint(Zone* zone, int index, float distance) {
        Vec2 point = zone->GetCurrent(index) + ((distance * state.Freedom) / fdotp);
        int touch = GetTouchState();
        zone->Current[index].p = point;
        zone->TouchState[index] |= touch;
    }

    float Round(float value) {
        switch (state.RoundState) {
            case ROUND_MODE_ToGrid: return value >= 0 ? roundFloat(value) : -roundFloat(-value);
            case ROUND_MODE_ToHalfGrid: return value >= 0 ? floor(value) + 0.5f : -(floor(-value) + 0.5f);
            // case ROUND_MODE_ToDoubleGrid: return value >= 0 ? (float)(Math.Round(value * 2, MidpointRounding.AwayFromZero) / 2) : -(float)(Math.Round(-value * 2, MidpointRounding.AwayFromZero) / 2);
            case ROUND_MODE_DownToGrid: return value >= 0 ? floor(value) : -floor(-value);
            case ROUND_MODE_UpToGrid: return value >= 0 ? ceil(value) : -ceil(-value);
            case ROUND_MODE_Super:
            case ROUND_MODE_Super45:
                float result;
                if (value >= 0) {
                    result = value - roundPhase + roundThreshold;
                    result = truncateFloat(result / roundPeriod) * roundPeriod;
                    result += roundPhase;
                    if (result < 0) result = roundPhase;
                } else {
                    result = -value - roundPhase + roundThreshold;
                    result = -truncateFloat(result / roundPeriod) * roundPeriod;
                    result -= roundPhase;
                    if (result > 0) result = -roundPhase;
                }
                return result;

            default: return value;
        }
    }

    float Project(Vec2 point) { return point * state.Projection; }
    float DualProject(Vec2 point) { return point * state.DualProjection; }

    static int SkipNext(InstructionStream* stream) {
        // grab the next opcode, and if it's one of the push instructions skip over its arguments
        int opcode = stream->NextOpCode();
        switch (opcode) {
            case OPCODE_NPUSHB:
            case OPCODE_PUSHB1:
            case OPCODE_PUSHB2:
            case OPCODE_PUSHB3:
            case OPCODE_PUSHB4:
            case OPCODE_PUSHB5:
            case OPCODE_PUSHB6:
            case OPCODE_PUSHB7:
            case OPCODE_PUSHB8: {
                uchar count = opcode == OPCODE_NPUSHB ? stream->NextByte() : opcode - OPCODE_PUSHB1 + 1;
                for (int i = 0; i < count; i++) stream->NextByte();
            } break;
            case OPCODE_NPUSHW:
            case OPCODE_PUSHW1:
            case OPCODE_PUSHW2:
            case OPCODE_PUSHW3:
            case OPCODE_PUSHW4:
            case OPCODE_PUSHW5:
            case OPCODE_PUSHW6:
            case OPCODE_PUSHW7:
            case OPCODE_PUSHW8: {
                uchar count = opcode == OPCODE_NPUSHW ? stream->NextByte() : opcode - OPCODE_PUSHW1 + 1;
                for (int i = 0; i < count; i++) stream->NextWord();
            } break;
        }

        return opcode;
    }

    static float F2Dot14ToFloat(int value) { return (short)value / 16384.0f; }
    static int FloatToF2Dot14(float value) { return (int)(uint)(short)STBTT_ifloor((value * 16384.0f) + 0.5f); }
    static float F26Dot6ToFloat(int value) { return value / 64.0f; }
    static int FloatToF26Dot6(float value) { return (int)STBTT_ifloor((value * 64.0f) + 0.5f); }

    static float* GetPoint(stbtt_uint8* data, int index) { return (float*)(data + sizeof(TrueTypeVertex) * index); }

    static void InterpolatePoints(uchar* current, uchar* original, int start, int end, int ref1, int ref2) {
        if (start > end) return;

        // figure out how much the two reference points
        // have been shifted from their original positions
        float delta1, delta2;
        auto lower = *GetPoint(original, ref1);
        auto upper = *GetPoint(original, ref2);
        if (lower > upper)
        {
            auto temp = lower;
            lower = upper;
            upper = temp;

            delta1 = *GetPoint(current, ref2) - lower;
            delta2 = *GetPoint(current, ref1) - upper;
        }
        else
        {
            delta1 = *GetPoint(current, ref1) - lower;
            delta2 = *GetPoint(current, ref2) - upper;
        }

        auto lowerCurrent = delta1 + lower;
        auto upperCurrent = delta2 + upper;
        auto scale = (upperCurrent - lowerCurrent) / (upper - lower);

        for (int i = start; i <= end; i++)
        {
            // three cases: if it's to the left of the lower reference point or to
            // the right of the upper reference point, do a shift based on that ref point.
            // otherwise, interpolate between the two of them
            auto pos = *GetPoint(original, i);
            if (pos <= lower)
                pos += delta1;
            else if (pos >= upper)
                pos += delta2;
            else
                pos = lowerCurrent + (pos - lower) * scale;
            *GetPoint(current, i) = pos;
        }
    }

};






void stbtt_MakeGlyphBitmapHinted(TrueTypeInterpreter* interpreter, const stbtt_fontinfo *info, unsigned char *output, int out_w, int out_h, int out_stride, float scale_x, float scale_y, float shift_x, float shift_y, int glyph)
{
   int ix0=0,iy0=0;
   stbtt__bitmap gbm;   

   TrueTypeVertex* ttvertices = 0;
   int vertexCount = interpreter->HintGlyph((stbtt_fontinfo *)info, glyph, &ttvertices);
   if(vertexCount == 0) return;

	int xmin, xmax, ymin, ymax;
	int result = interpreter->calcBoundingBox(&xmin, &ymin, &xmax, &ymax);
	if(result) {
		ix0 = ( xmin /** scale_x*/ + shift_x);
		iy0 = (-ymax /** scale_y*/ + shift_y);
	} else {
		return;
	}

   stbtt_vertex* vertices;
	int num_verts = trueTypeGlyphToStb((stbtt_fontinfo*)info, ttvertices, vertexCount, interpreter->finalContours, interpreter->finalContourCount, scale_x, &vertices);

   gbm.pixels = output;
   gbm.w = out_w;
   gbm.h = out_h;
   gbm.stride = out_stride;

   if (gbm.w && gbm.h)
      stbtt_Rasterize(&gbm, 0.35f, vertices, num_verts, scale_x, scale_y, shift_x, shift_y, ix0,iy0, 1, info->userdata);

   STBTT_free(vertices, info->userdata);
}

int stbtt_PackFontRangesHinted(TrueTypeInterpreter* interpreter, stbtt_fontinfo* info, stbtt_pack_context *spc, stbtt_pack_range *ranges, int num_ranges)
{
   int i,j,n, return_value = 1;
   //stbrp_context *context = (stbrp_context *) spc->pack_info;
   stbrp_rect    *rects;

   // flag all characters as NOT packed
   for (i=0; i < num_ranges; ++i)
      for (j=0; j < ranges[i].num_chars; ++j)
         ranges[i].chardata_for_range[j].x0 =
         ranges[i].chardata_for_range[j].y0 =
         ranges[i].chardata_for_range[j].x1 =
         ranges[i].chardata_for_range[j].y1 = 0;

   n = 0;
   for (i=0; i < num_ranges; ++i)
      n += ranges[i].num_chars;
         
   rects = (stbrp_rect *) STBTT_malloc(sizeof(*rects) * n, spc->user_allocator_context);
   if (rects == NULL)
      return 0;

   info->userdata = spc->user_allocator_context;

	// Get rects.
	{
	    int k = 0;
		for (i=0; i < num_ranges; ++i) {
			int height = ranges[i].font_size;
			float scale = stbtt_ScaleForPixelHeight(info, height);

			ranges[i].h_oversample = (unsigned char) spc->h_oversample;
			ranges[i].v_oversample = (unsigned char) spc->v_oversample;

			for (j=0; j < ranges[i].num_chars; ++j) {
				int x0,y0,x1,y1;
   				int codepoint = ranges[i].array_of_unicode_codepoints == NULL ? ranges[i].first_unicode_codepoint_in_range + j : ranges[i].array_of_unicode_codepoints[j];
   				int glyph = stbtt_FindGlyphIndex(info, codepoint);


				TrueTypeVertex* ttvertices = 0;
				int vertexCount = interpreter->HintGlyph(info, glyph, &ttvertices);

				if(vertexCount == 0) {
					rects[k].w = (stbrp_coord) 1 + spc->padding + spc->h_oversample-1;
					rects[k].h = (stbrp_coord) 1 + spc->padding + spc->v_oversample-1;
				} else {
					interpreter->calcBoundingBox(&x0, &y0, &x1, &y1);

					rects[k].w = (stbrp_coord) (x1-x0 + spc->padding + spc->h_oversample-1);
					rects[k].h = (stbrp_coord) (y1-y0 + spc->padding + spc->v_oversample-1);
				}

				++k;
			}

			n = k;
		}
	}
   // n = stbtt_PackFontRangesGatherRects(spc, info, ranges, num_ranges, rects);

   stbtt_PackFontRangesPackRects(spc, rects, n);
  
   // return_value = stbtt_PackFontRangesRenderIntoRects(spc, info, ranges, num_ranges, rects);



   // STBTT_DEF int stbtt_PackFontRangesRenderIntoRects(stbtt_pack_context *spc, stbtt_fontinfo *info, stbtt_pack_range *ranges, int num_ranges, stbrp_rect *rects)
   {
      int i,j,k;

      return_value = 1;

      // save current values
      int old_h_over = spc->h_oversample;
      int old_v_over = spc->v_oversample;

      k = 0;
      for (i=0; i < num_ranges; ++i) {
         float fh = ranges[i].font_size;
         float scale = fh > 0 ? stbtt_ScaleForPixelHeight(info, fh) : stbtt_ScaleForMappingEmToPixels(info, -fh);
         float recip_h,recip_v,sub_x,sub_y;
         spc->h_oversample = ranges[i].h_oversample;
         spc->v_oversample = ranges[i].v_oversample;
         recip_h = 1.0f / spc->h_oversample;
         recip_v = 1.0f / spc->v_oversample;
         sub_x = stbtt__oversample_shift(spc->h_oversample);
         sub_y = stbtt__oversample_shift(spc->v_oversample);
         for (j=0; j < ranges[i].num_chars; ++j) {
            stbrp_rect *r = &rects[k];
            if (r->was_packed) {
               stbtt_packedchar *bc = &ranges[i].chardata_for_range[j];
               int advance, lsb, x0,y0,x1,y1;
               int codepoint = ranges[i].array_of_unicode_codepoints == NULL ? ranges[i].first_unicode_codepoint_in_range + j : ranges[i].array_of_unicode_codepoints[j];
               int glyph = stbtt_FindGlyphIndex(info, codepoint);
               stbrp_coord pad = (stbrp_coord) spc->padding;

               // pad on left and top
               r->x += pad;
               r->y += pad;
               r->w -= pad;
               r->h -= pad;


               stbtt_MakeGlyphBitmapHinted(interpreter, info,
                                             spc->pixels + r->x + r->y*spc->stride_in_bytes,
                                             r->w - spc->h_oversample+1,
                                             r->h - spc->v_oversample+1,
                                             spc->stride_in_bytes,
                                             scale * spc->h_oversample,
                                             scale * spc->v_oversample,
                                             0,0,
                                             glyph);

               {
               	   int xmin, xmax, ymin, ymax;
               	   int result = interpreter->calcBoundingBox(&xmin, &ymin, &xmax, &ymax);

               	   x0 =  xmin;
               	   y0 = -ymax;
               	   x1 =  xmax;
               	   y1 = -ymin;

               	   if(result) {
		               interpreter->getMetrics(&advance);
               	   } else {
		               stbtt_GetGlyphHMetrics(info, glyph, &advance, &lsb);
		               advance *= scale;
               	   }
               }

               if (spc->h_oversample > 1)
                  stbtt__h_prefilter(spc->pixels + r->x + r->y*spc->stride_in_bytes,
                                     r->w, r->h, spc->stride_in_bytes,
                                     spc->h_oversample);

               if (spc->v_oversample > 1)
                  stbtt__v_prefilter(spc->pixels + r->x + r->y*spc->stride_in_bytes,
                                     r->w, r->h, spc->stride_in_bytes,
                                     spc->v_oversample);

               bc->x0       = (stbtt_int16)  r->x;
               bc->y0       = (stbtt_int16)  r->y;
               bc->x1       = (stbtt_int16) (r->x + r->w);
               bc->y1       = (stbtt_int16) (r->y + r->h);
               bc->xadvance =                /*scale **/ advance;
               bc->xoff     =       (float)  x0 * recip_h + sub_x;
               bc->yoff     =       (float)  y0 * recip_v + sub_y;
               bc->xoff2    =                (x0 + r->w) * recip_h + sub_x;
               bc->yoff2    =                (y0 + r->h) * recip_v + sub_y;
            } else {
               return_value = 0; // if any fail, report failure
            }

            ++k;
         }
      }

      // restore original values
      spc->h_oversample = old_h_over;
      spc->v_oversample = old_v_over;
   }


   STBTT_free(rects, spc->user_allocator_context);
   return return_value;
}