﻿//MIT, 2015, Michael Popoloski's SharpFont


#define Default_Array_Size 100

void assertPrint(bool condition, char* string) {
	printf(string);
	STBTT_assert(!condition);
}
void assertPrint(bool condition) { return assertPrint(condition, ""); }







struct SharpFontInterpreter {
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

	struct Zone {
	    stbtt_vertex* Current;
	    int CurrentCount;
	    stbtt_vertex* Original;
	    int OriginalCount;
	    int* TouchState;
	    int TouchStateCount;
	    bool IsTwilight;

	    Zone() {};
	    Zone(stbtt_vertex* points, int pointsCount, bool isTwilight) {
	        IsTwilight = isTwilight;
	        Current = points;
	        CurrentCount = pointsCount;
	        memCpy(Original, points, pointsCount);
	        OriginalCount = pointsCount;
	        TouchState = mallocArray(int, pointsCount);
	        TouchStateCount = pointsCount;
	    }

	    // Vec2 GetCurrent(int index) { return Current[index].P; }
	    // Vec2 GetOriginal(int index) { return Original[index].P; }

	    Vec2 GetCurrent(int index) { return vec2(0,0); }
	    Vec2 GetOriginal(int index) { return vec2(0,0); }
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

        void Reset() {
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

	    InstructionStream(uchar* instructions, int count) {
	        this->instructions = instructions;
	        this->instructionsCount = count;
	        ip = 0;
	    }

	    stbtt_uint8 NextByte() {
	        assertPrint(Done());
	        return instructions[ip++];
	    }

	    int NextOpCode() { return NextByte(); }
	    int NextWord() { return (short)(ushort)(NextByte() << 8 | NextByte()); }
	    void Jump(int offset) { ip += offset; }
	};

	struct ExecutionStack {
	    int* s;
	    int sMaxCount;
	    int count;

	    ExecutionStack() {}; // Void default constructor to please c++.
	    ExecutionStack(int maxStack) {
	        s = mallocArray(int, maxStack);
	        sMaxCount = maxStack;
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




    GraphicsState state;
    GraphicsState cvtState;
    ExecutionStack stack;
    InstructionStream* functions;
    int functionsCount;
    InstructionStream* instructionDefs;
    int instructionDefsCount;
    float* controlValueTable;
    int controlValueTableCount;

    int* storage;
    int storageCount;
    ushort* contours;
    int contoursCount;
    float scale;
    int ppem;
    int callStackSize;
    float fdotp;
    float roundThreshold;
    float roundPhase;
    float roundPeriod;
    Zone zp0, zp1, zp2;	
    Zone points, twilight;


    // SharpFontInterpreter(int maxStack, int maxStorage, int maxFunctions, int maxInstructionDefs, int maxTwilightPoints) {
    //     stack = ExecutionStack(maxStack);
    //     storage = mallocArray(int, maxStorage);
    //     functions = mallocArray(InstructionStream, maxFunctions);
    //     instructionDefs = mallocArray(InstructionStream, (maxInstructionDefs > 0 ? 256 : 0));
    //     state = GraphicsState();
    //     cvtState = GraphicsState();
    //     // twilight = new Zone(new stb_vertex[maxTwilightPoints], isTwilight: true);
    // }

    void init(int maxStack, int maxStorage, int maxFunctions, int maxInstructionDefs, int maxTwilightPoints) {
        stack = ExecutionStack(maxStack);
        storage = mallocArray(int, maxStorage);
        functions = mallocArray(InstructionStream, maxFunctions);
        instructionDefs = mallocArray(InstructionStream, (maxInstructionDefs > 0 ? 256 : 0));
        state = GraphicsState();
        cvtState = GraphicsState();
        // twilight = new Zone(new stb_vertex[maxTwilightPoints], isTwilight: true);
    }


    void InitializeFunctionDefs(uchar* instructions, int count) {
        Execute(InstructionStream(instructions, count), false, true);
    }

    void SetControlValueTable(int* cvt, int cvtCount, float scale, float ppem, uchar* cvProgram, int cvProgramCount) {
        if (this->scale == scale || cvt == 0) return;

        if (controlValueTable == 0) controlValueTable = mallocArray(float, cvtCount);
        //copy cvt and apply scale
        for (int i = cvtCount - 1; i >= 0; --i)
            controlValueTable[i] = cvt[i] * scale;

        this->scale = scale;
        this->ppem = roundInt(ppem);
        // zp0 = zp1 = zp2 = points;
        state.Reset();
        stack.Clear();

        if (cvProgram != 0) {
            Execute(InstructionStream(cvProgram, cvProgramCount), false, false);

            // save off the CVT graphics state so that we can restore it for each glyph we hint
            if ((state.InstructionControl & CONTROL_FLAG_UseDefaultGraphicsState) != 0) cvtState.Reset();
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

    void HintGlyph(stbtt_vertex* glyphPoints, int glyphPointsCount, ushort* contours, int contoursCount, uchar* instructions, int instructionsCount) {
        if (instructions == 0 || instructionsCount == 0) return;

        // check if the CVT program disabled hinting
        if ((state.InstructionControl & CONTROL_FLAG_InhibitGridFitting) != 0) return;

        // TODO: composite glyphs
        // TODO: round the phantom points?

        // save contours and points
        this->contours = contours;
        this->contoursCount = contoursCount;
        zp0 = zp1 = zp2 = points = Zone(glyphPoints, glyphPointsCount, false);

        // reset all of our shared state
        state = cvtState;
        callStackSize = 0;
#if DEBUG
        debugList.Clear();
#endif
        stack.Clear();
        OnVectorsUpdated();

        // normalize the round state settings
        switch (state.RoundState) {
            case ROUND_MODE_Super: SetSuperRound(1.0f); break;
            case ROUND_MODE_Super45: SetSuperRound(Sqrt2Over2); break;
        }

        Execute(InstructionStream(instructions, instructionsCount), false, false);
    }

#if DEBUG
    System.Collections.Generic.List<OpCode> debugList = new System.Collections.Generic.List<OpCode>();
#endif
    void Execute(InstructionStream stream, bool inFunction, bool allowFunctionDefs)
    {
        // dispatch each instruction in the stream
        while (!stream.Done())
        {
            int opcode = stream.NextOpCode();
#if DEBUG
            debugList.Add(opcode);
#endif
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
                    uchar count = opcode == OPCODE_NPUSHB ? stream.NextByte() : opcode - OPCODE_PUSHB1 + 1;
                    for (int i = count - 1; i >= 0; --i) stack.Push(stream.NextByte());
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
	                uchar count = opcode == OPCODE_NPUSHW ? stream.NextByte() : opcode - OPCODE_PUSHW1 + 1;
	                for (int i = count - 1; i >= 0; --i) stack.Push(stream.NextWord());
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
	            case OPCODE_GC0: stack.Push(Project(zp2.GetCurrent(stack.Pop()))); break;
	            case OPCODE_GC1: stack.Push(DualProject(zp2.GetOriginal(stack.Pop()))); break;
	            case OPCODE_SCFS:
	                {
	            //         var value = stack.PopFloat();
	            //         var index = stack.Pop();
	            //         var point = zp2.GetCurrent(index);
	            //         MovePoint(zp2, index, value - Project(point));

	            //         // moving twilight points moves their "original" value also
	            //         if (zp2.IsTwilight)
	            //             zp2.Original[index].P = zp2.Current[index].P;
	                }
	                break;
	            case OPCODE_MD0:
	                {
	            //         var p1 = zp1.GetOriginal(stack.Pop());
	            //         var p2 = zp0.GetOriginal(stack.Pop());
	            //         stack.Push(DualProject(p2 - p1));
	                }
	                break;
	            case OPCODE_MD1:
	                {
	            //         var p1 = zp1.GetCurrent(stack.Pop());
	            //         var p2 = zp0.GetCurrent(stack.Pop());
	            //         stack.Push(Project(p2 - p1));
	                }
	                break;
	            case OPCODE_MPS: // MPS should return point size, but we assume DPI so it's the same as pixel size
	            case OPCODE_MPPEM: stack.Push(ppem); break;
	            case OPCODE_AA: /* deprecated instruction */ stack.Pop(); break;

	            // ==== POINT MODIFICATION ====
	            case OPCODE_FLIPPT:
	                {
	            //         for (int i = 0; i < state.Loop; i++)
	            //         {
	            //             var index = stack.Pop();
	            //             //review here again!
	            //             points.Current[index].onCurve = !points.Current[index].onCurve;
	            //             //if (points.Current[index].onCurve)
	            //             //    points.Current[index].onCurve = false;
	            //             //else
	            //             //    points.Current[index].onCurve = true;
	            //         }
	            //         state.Loop = 1;
	                }
	                break;
	            case OPCODE_FLIPRGON:
	                {
	            //         var end = stack.Pop();
	            //         for (int i = stack.Pop(); i <= end; i++)
	            //             //points.Current[i].Type = PointType.OnCurve;
	            //             points.Current[i].onCurve = true;
	                }
	                break;
	            case OPCODE_FLIPRGOFF:
	                {
	            //         var end = stack.Pop();
	            //         for (int i = stack.Pop(); i <= end; i++)
	            //             //points.Current[i].Type = PointType.Quadratic;
	            //             points.Current[i].onCurve = false;
	                }
	                break;
	            case OPCODE_SHP0:
	            case OPCODE_SHP1:
	                {
	            //         Zone zone;
	            //         int point;
	            //         var displacement = ComputeDisplacement((int)opcode, out zone, out point);
	            //         ShiftPoints(displacement);
	                }
	                break;
	            case OPCODE_SHPIX: ShiftPoints(stack.PopFloat() * state.Freedom); break;
	            case OPCODE_SHC0:
	            case OPCODE_SHC1:
	                {
	            //         Zone zone;
	            //         int point;
	            //         var displacement = ComputeDisplacement((int)opcode, out zone, out point);
	            //         var touch = GetTouchState();
	            //         var contour = stack.Pop();
	            //         var start = contour == 0 ? 0 : contours[contour - 1] + 1;
	            //         var count = zp2.IsTwilight ? zp2.Current.Length : contours[contour] + 1;

	            //         for (int i = start; i < count; i++)
	            //         {
	            //             // don't move the reference point
	            //             if (zone.Current != zp2.Current || point != i)
	            //             {
	            //                 zp2.Current[i].P += displacement;
	            //                 zp2.TouchState[i] |= touch;
	            //             }
	            //         }
	                }
	                break;
	            case OPCODE_SHZ0:
	            case OPCODE_SHZ1:
	                {
	            //         Zone zone;
	            //         int point;
	            //         var displacement = ComputeDisplacement((int)opcode, out zone, out point);
	            //         var count = 0;
	            //         if (zp2.IsTwilight)
	            //             count = zp2.Current.Length;
	            //         else if (contours.Length > 0)
	            //             count = contours[contours.Length - 1] + 1;

	            //         for (int i = 0; i < count; i++)
	            //         {
	            //             // don't move the reference point
	            //             if (zone.Current != zp2.Current || point != i)
	            //                 zp2.Current[i].P += displacement;
	            //         }
	                }
	                break;
	            case OPCODE_MIAP0:
	            case OPCODE_MIAP1:
	                {
	            //         var distance = ReadCvt();
	            //         var pointIndex = stack.Pop();

	            //         // this instruction is used in the CVT to set up twilight points with original values
	            //         if (zp0.IsTwilight)
	            //         {
	            //             var original = state.Freedom * distance;
	            //             zp0.Original[pointIndex].P = original;
	            //             zp0.Current[pointIndex].P = original;
	            //         }

	            //         // current position of the point along the projection vector
	            //         var point = zp0.GetCurrent(pointIndex);
	            //         var currentPos = Project(point);
	            //         if (opcode == OPCODE_MIAP1)
	            //         {
	            //             // only use the CVT if we are above the cut-in point
	            //             if (Math.Abs(distance - currentPos) > state.ControlValueCutIn)
	            //                 distance = currentPos;
	            //             distance = Round(distance);
	            //         }

	            //         MovePoint(zp0, pointIndex, distance - currentPos);
	            //         state.Rp0 = pointIndex;
	            //         state.Rp1 = pointIndex;
	                }
	                break;
	            case OPCODE_MDAP0:
	            case OPCODE_MDAP1:
	                {
	            //         var pointIndex = stack.Pop();
	            //         var point = zp0.GetCurrent(pointIndex);
	            //         var distance = 0.0f;
	            //         if (opcode == OPCODE_MDAP1)
	            //         {
	            //             distance = Project(point);
	            //             distance = Round(distance) - distance;
	            //         }

	            //         MovePoint(zp0, pointIndex, distance);
	            //         state.Rp0 = pointIndex;
	            //         state.Rp1 = pointIndex;
	                }
	                break;
	            case OPCODE_MSIRP0:
	            case OPCODE_MSIRP1:
	                {
	            //         var targetDistance = stack.PopFloat();
	            //         var pointIndex = stack.Pop();

	            //         // if we're operating on the twilight zone, initialize the points
	            //         if (zp1.IsTwilight)
	            //         {
	            //             zp1.Original[pointIndex].P = zp0.Original[state.Rp0].P + targetDistance * state.Freedom / fdotp;
	            //             zp1.Current[pointIndex].P = zp1.Original[pointIndex].P;
	            //         }

	            //         var currentDistance = Project(zp1.GetCurrent(pointIndex) - zp0.GetCurrent(state.Rp0));
	            //         MovePoint(zp1, pointIndex, targetDistance - currentDistance);

	            //         state.Rp1 = state.Rp0;
	            //         state.Rp2 = pointIndex;
	            //         if (opcode == OPCODE_MSIRP1)
	            //             state.Rp0 = pointIndex;
	                }
	                break;
	            case OPCODE_IP:
	                {
	            //         var originalBase = zp0.GetOriginal(state.Rp1);
	            //         var currentBase = zp0.GetCurrent(state.Rp1);
	            //         var originalRange = DualProject(zp1.GetOriginal(state.Rp2) - originalBase);
	            //         var currentRange = Project(zp1.GetCurrent(state.Rp2) - currentBase);

	            //         for (int i = 0; i < state.Loop; i++)
	            //         {
	            //             var pointIndex = stack.Pop();
	            //             var point = zp2.GetCurrent(pointIndex);
	            //             var currentDistance = Project(point - currentBase);
	            //             var originalDistance = DualProject(zp2.GetOriginal(pointIndex) - originalBase);

	            //             var newDistance = 0.0f;
	            //             if (originalDistance != 0.0f)
	            //             {
	            //                 // a range of 0.0f is invalid according to the spec (would result in a div by zero)
	            //                 if (originalRange == 0.0f)
	            //                     newDistance = originalDistance;
	            //                 else
	            //                     newDistance = originalDistance * currentRange / originalRange;
	            //             }

	            //             MovePoint(zp2, pointIndex, newDistance - currentDistance);
	            //         }
	            //         state.Loop = 1;
	                }
	                break;
	            case OPCODE_ALIGNRP:
	                {
	            //         for (int i = 0; i < state.Loop; i++)
	            //         {
	            //             var pointIndex = stack.Pop();
	            //             var p1 = zp1.GetCurrent(pointIndex);
	            //             var p2 = zp0.GetCurrent(state.Rp0);
	            //             MovePoint(zp1, pointIndex, -Project(p1 - p2));
	            //         }
	            //         state.Loop = 1;
	                }
	                break;
	            case OPCODE_ALIGNPTS:
	                {
	            //         var p1 = stack.Pop();
	            //         var p2 = stack.Pop();
	            //         var distance = Project(zp0.GetCurrent(p2) - zp1.GetCurrent(p1)) / 2;
	            //         MovePoint(zp1, p1, distance);
	            //         MovePoint(zp0, p2, -distance);
	                }
	                break;
	            case OPCODE_UTP: zp0.TouchState[stack.Pop()] &= ~GetTouchState(); break;
	            case OPCODE_IUP0:
	            case OPCODE_IUP1:
	            //     unsafe
	                {
	            //         // bail if no contours (empty outline)
	            //         if (contours.Length == 0)
	            //             break;

	            //         fixed (stb_vertex* currentPtr = points.Current)
	            //         fixed (stb_vertex* originalPtr = points.Original)
	            //         {
	            //             // opcode controls whether we care about X or Y direction
	            //             // do some pointer trickery so we can operate on the
	            //             // points in a direction-agnostic manner
	            //             TouchState touchMask;
	            //             byte* current;
	            //             byte* original;
	            //             if (opcode == OPCODE_IUP0)
	            //             {
	            //                 touchMask = TOUCH_STATE_Y;
	            //                 current = (byte*)&currentPtr->P.Y;
	            //                 original = (byte*)&originalPtr->P.Y;
	            //             }
	            //             else
	            //             {
	            //                 touchMask = TOUCH_STATE_X;
	            //                 current = (byte*)&currentPtr->P.X;
	            //                 original = (byte*)&originalPtr->P.X;
	            //             }

	            //             var point = 0;
	            //             for (int i = 0; i < contours.Length; i++)
	            //             {
	            //                 var endPoint = contours[i];
	            //                 var firstPoint = point;
	            //                 var firstTouched = -1;
	            //                 var lastTouched = -1;

	            //                 for (; point <= endPoint; point++)
	            //                 {
	            //                     // check whether this point has been touched
	            //                     if ((points.TouchState[point] & touchMask) != 0)
	            //                     {
	            //                         // if this is the first touched point in the contour, note it and continue
	            //                         if (firstTouched < 0)
	            //                         {
	            //                             firstTouched = point;
	            //                             lastTouched = point;
	            //                             continue;
	            //                         }

	            //                         // otherwise, interpolate all untouched points
	            //                         // between this point and our last touched point
	            //                         InterpolatePoints(current, original, lastTouched + 1, point - 1, lastTouched, point);
	            //                         lastTouched = point;
	            //                     }
	            //                 }

	            //                 // check if we had any touched points at all in this contour
	            //                 if (firstTouched >= 0)
	            //                 {
	            //                     // there are two cases left to handle:
	            //                     // 1. there was only one touched point in the whole contour, in
	            //                     //    which case we want to shift everything relative to that one
	            //                     // 2. several touched points, in which case handle the gap from the
	            //                     //    beginning to the first touched point and the gap from the last
	            //                     //    touched point to the end of the contour
	            //                     if (lastTouched == firstTouched)
	            //                     {
	            //                         var delta = *GetPoint(current, lastTouched) - *GetPoint(original, lastTouched);
	            //                         if (delta != 0.0f)
	            //                         {
	            //                             for (int j = firstPoint; j < lastTouched; j++)
	            //                                 *GetPoint(current, j) += delta;
	            //                             for (int j = lastTouched + 1; j <= endPoint; j++)
	            //                                 *GetPoint(current, j) += delta;
	            //                         }
	            //                     }
	            //                     else
	            //                     {
	            //                         InterpolatePoints(current, original, lastTouched + 1, endPoint, lastTouched, firstTouched);
	            //                         if (firstTouched > 0)
	            //                             InterpolatePoints(current, original, firstPoint, firstTouched - 1, lastTouched, firstTouched);
	            //                     }
	            //                 }
	            //             }
	            //         }
	                }
	                break;
	            case OPCODE_ISECT:
	                {
	            //         // move point P to the intersection of lines A and B
	            //         var b1 = zp0.GetCurrent(stack.Pop());
	            //         var b0 = zp0.GetCurrent(stack.Pop());
	            //         var a1 = zp1.GetCurrent(stack.Pop());
	            //         var a0 = zp1.GetCurrent(stack.Pop());
	            //         var index = stack.Pop();

	            //         // calculate intersection using determinants: https://en.wikipedia.org/wiki/Line%E2%80%93line_intersection#Given_two_points_on_each_line
	            //         var da = a0 - a1;
	            //         var db = b0 - b1;
	            //         var den = (da.X * db.Y) - (da.Y * db.X);
	            //         if (Math.Abs(den) <= Epsilon)
	            //         {
	            //             // parallel lines; spec says to put the ppoint "into the middle of the two lines"
	            //             zp2.Current[index].P = (a0 + a1 + b0 + b1) / 4;
	            //         }
	            //         else
	            //         {
	            //             var t = (a0.X * a1.Y) - (a0.Y * a1.X);
	            //             var u = (b0.X * b1.Y) - (b0.Y * b1.X);
	            //             var p = new Vec2(
	            //                 (t * db.X) - (da.X * u),
	            //                 (t * db.Y) - (da.Y * u)
	            //             );
	            //             zp2.Current[index].P = p / den;
	            //         }
	            //         zp2.TouchState[index] = TOUCH_STATE_Both;
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
	            //         // value is false; jump to the next else block or endif marker
	            //         // otherwise, we don't have to do anything; we'll keep executing this block
	            //         if (!stack.PopBool())
	            //         {
	            //             int indent = 1;
	            //             while (indent > 0)
	            //             {
	            //                 opcode = SkipNext(ref stream);
	            //                 switch (opcode)
	            //                 {
	            //                     case OPCODE_IF: indent++; break;
	            //                     case OPCODE_EIF: indent--; break;
	            //                     case OPCODE_ELSE:
	            //                         if (indent == 1)
	            //                             indent = 0;
	            //                         break;
	            //                 }
	            //             }
	            //         }
	                }
	                break;
	            case OPCODE_ELSE:
	                {
	            //         // assume we hit the true statement of some previous if block
	            //         // if we had hit false, we would have jumped over this
	            //         int indent = 1;
	            //         while (indent > 0)
	            //         {
	            //             opcode = SkipNext(ref stream);
	            //             switch (opcode)
	            //             {
	            //                 case OPCODE_IF: indent++; break;
	            //                 case OPCODE_EIF: indent--; break;
	            //             }
	            //         }
	                }
	                break;
	            case OPCODE_EIF: /* nothing to do */ break;
	            case OPCODE_JROT:
	            case OPCODE_JROF:
	                {
	            //         if (stack.PopBool() == (opcode == OPCODE_JROT))
	            //             stream.Jump(stack.Pop() - 1);
	            //         else
	            //             stack.Pop();    // ignore the offset
	                }
	                break;
	            case OPCODE_JMPR: stream.Jump(stack.Pop() - 1); break;

	            // ==== LOGICAL OPS ====
	            case OPCODE_LT:
	                {
	            //         var b = stack.Pop();
	            //         var a = stack.Pop();
	            //         stack.Push(a < b);
	                }
	                break;
	            case OPCODE_LTEQ:
	                {
	            //         var b = stack.Pop();
	            //         var a = stack.Pop();
	            //         stack.Push(a <= b);
	                }
	                break;
	            case OPCODE_GT:
	                {
	            //         var b = stack.Pop();
	            //         var a = stack.Pop();
	            //         stack.Push(a > b);
	                }
	                break;
	            case OPCODE_GTEQ:
	                {
	            //         var b = stack.Pop();
	            //         var a = stack.Pop();
	            //         stack.Push(a >= b);
	                }
	                break;
	            case OPCODE_EQ:
	                {
	            //         var b = stack.Pop();
	            //         var a = stack.Pop();
	            //         stack.Push(a == b);
	                }
	                break;
	            case OPCODE_NEQ:
	                {
	            //         var b = stack.Pop();
	            //         var a = stack.Pop();
	            //         stack.Push(a != b);
	                }
	                break;
	            case OPCODE_AND:
	                {
	            //         var b = stack.PopBool();
	            //         var a = stack.PopBool();
	            //         stack.Push(a && b);
	                }
	                break;
	            case OPCODE_OR:
	                {
	            //         var b = stack.PopBool();
	            //         var a = stack.PopBool();
	            //         stack.Push(a || b);
	                }
	                break;
	            case OPCODE_NOT: stack.Push(!stack.PopBool()); break;
	            case OPCODE_ODD:
	                {
	            //         var value = (int)Round(stack.PopFloat());
	            //         stack.Push(value % 2 != 0);
	                }
	                break;
	            case OPCODE_EVEN:
	                {
	            //         var value = (int)Round(stack.PopFloat());
	            //         stack.Push(value % 2 == 0);
	                }
	                break;

	            // ==== ARITHMETIC ====
	            case OPCODE_ADD:
	                {
	            //         var b = stack.Pop();
	            //         var a = stack.Pop();
	            //         stack.Push(a + b);
	                }
	                break;
	            case OPCODE_SUB:
	                {
	            //         var b = stack.Pop();
	            //         var a = stack.Pop();
	            //         stack.Push(a - b);
	                }
	                break;
	            case OPCODE_DIV:
	                {
	            //         var b = stack.Pop();
	            //         assertPrint(b == 0, "Division by zero.");

	            //         var a = stack.Pop();
	            //         var result = ((long)a << 6) / b;
	            //         stack.Push((int)result);
	                }
	                break;
	            case OPCODE_MUL:
	                {
	            //         var b = stack.Pop();
	            //         var a = stack.Pop();
	            //         var result = ((long)a * b) >> 6;
	            //         stack.Push((int)result);
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

	                    functions[stack.Pop()] = stream;
	                    while (SkipNext(&stream) != OPCODE_ENDF);
	                }
	                break;
	            case OPCODE_IDEF:
	                {
	                    assertPrint(!allowFunctionDefs || inFunction, "Can't define functions here.");

	                    instructionDefs[stack.Pop()] = stream;
	                    while (SkipNext(&stream) != OPCODE_ENDF);
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
	            //         callStackSize++;
	            //         assertPrint(callStackSize > MaxCallStack, "Stack overflow; infinite recursion?");

	            //         var function = functions[stack.Pop()];
	            //         var count = opcode == OPCODE_LOOPCALL ? stack.Pop() : 1;
	            //         for (int i = 0; i < count; i++)
	            //             Execute(function, true, false);
	            //         callStackSize--;
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
	            //         var last = stack.Pop();
	            //         for (int i = 1; i <= last; i++)
	            //         {
	            //             var cvtIndex = stack.Pop();
	            //             var arg = stack.Pop();

	            //             // upper 4 bits of the 8-bit arg is the relative ppem
	            //             // the opcode specifies the base to add to the ppem
	            //             var triggerPpem = (arg >> 4) & 0xF;
	            //             triggerPpem += (opcode - OPCODE_DELTAC1) * 16;
	            //             triggerPpem += state.DeltaBase;

	            //             // if the current ppem matches the trigger, apply the exception
	            //             if (ppem == triggerPpem)
	            //             {
	            //                 // the lower 4 bits of the arg is the amount to shift
	            //                 // it's encoded such that 0 isn't an allowable value (who wants to shift by 0 anyway?)
	            //                 var amount = (arg & 0xF) - 8;
	            //                 if (amount >= 0)
	            //                     amount++;
	            //                 amount *= 1 << (6 - state.DeltaShift);

	            //                 // update the CVT
	            //                 CheckIndex(cvtIndex, controlValueTable.Length);
	            //                 controlValueTable[cvtIndex] += F26Dot6ToFloat(amount);
	            //             }
	            //         }
	                }
	                break;
	            case OPCODE_DELTAP1:
	            case OPCODE_DELTAP2:
	            case OPCODE_DELTAP3:
	                {
	            //         var last = stack.Pop();
	            //         for (int i = 1; i <= last; i++)
	            //         {
	            //             var pointIndex = stack.Pop();
	            //             var arg = stack.Pop();

	            //             // upper 4 bits of the 8-bit arg is the relative ppem
	            //             // the opcode specifies the base to add to the ppem
	            //             var triggerPpem = (arg >> 4) & 0xF;
	            //             triggerPpem += state.DeltaBase;
	            //             if (opcode != OPCODE_DELTAP1)
	            //                 triggerPpem += (opcode - OPCODE_DELTAP2 + 1) * 16;

	            //             // if the current ppem matches the trigger, apply the exception
	            //             if (ppem == triggerPpem)
	            //             {
	            //                 // the lower 4 bits of the arg is the amount to shift
	            //                 // it's encoded such that 0 isn't an allowable value (who wants to shift by 0 anyway?)
	            //                 var amount = (arg & 0xF) - 8;
	            //                 if (amount >= 0)
	            //                     amount++;
	            //                 amount *= 1 << (6 - state.DeltaShift);

	            //                 MovePoint(zp0, pointIndex, F26Dot6ToFloat(amount));
	            //             }
	            //         }
	                }
	                break;

	            // ==== MISCELLANEOUS ====
	            case OPCODE_DEBUG: stack.Pop(); break;
	            case OPCODE_GETINFO:
	                {
	            //         var selector = stack.Pop();
	            //         var result = 0;
	            //         if ((selector & 0x1) != 0)
	            //         {
	            //             // pretend we are MS Rasterizer v35
	            //             result = 35;
	            //         }

	            //         // TODO: rotation and stretching
	            //         //if ((selector & 0x2) != 0)
	            //         //if ((selector & 0x4) != 0)

	            //         // we're always rendering in grayscale
	            //         if ((selector & 0x20) != 0)
	            //             result |= 1 << 12;

	            //         // TODO: ClearType flags

	            //         stack.Push(result);
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

	                    Execute(instructionDefs[index], true, false);
	                    callStackSize--;
	                }
            }
        }
    }

    int CheckIndex(int index, int length) {
        assertPrint(index < 0 || index >= length);
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
        Vec2 p1 = zp2.GetCurrent(index1);
        Vec2 p2 = zp1.GetCurrent(index2);

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
            p1 = zp2.GetOriginal(index1);
            p2 = zp2.GetOriginal(index2);
            line = p2 - p1;

            if (lenSquaredVec2(line) == 0) state.DualProjection = vec2(1,0);
            else {
                if ((mode & 0x1) != 0) line = vec2(-line.y, line.x);

                state.DualProjection = normVec2(line);
            }
        }

        OnVectorsUpdated();
    }

    Zone GetZoneFromStack() {
        switch (stack.Pop()) {
            case 0: return twilight;
            case 1: return points;
            default: assertPrint(false, "Invalid zone pointer.");
            return twilight; // Go away compiler.
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
        Vec2 originalReference = zp0.GetOriginal(state.Rp0);
        if (zp1.IsTwilight) {
            Vec2 initialValue = originalReference + state.Freedom * cvt;
            // zp1.Original[pointIndex].P = initialValue;
            // zp1.Current[pointIndex].P = initialValue;
        }

        Vec2 point = zp1.GetCurrent(pointIndex);
        float originalDistance = DualProject(zp1.GetOriginal(pointIndex) - originalReference);
        float currentDistance = Project(point - zp0.GetCurrent(state.Rp0));

        if (state.AutoFlip && !sameSign(originalDistance, cvt)) cvt = -cvt;

        // if bit 2 is set, round the distance and look at the cut-in value
        float distance = cvt;
        if ((flags & 0x4) != 0) {
            // only perform cut-in tests when both points are in the same zone
            if (zp0.IsTwilight == zp1.IsTwilight && fabs(cvt - originalDistance) > state.ControlValueCutIn) cvt = originalDistance;
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
        Vec2 p1 = zp0.GetOriginal(state.Rp0);
        Vec2 p2 = zp1.GetOriginal(pointIndex);
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
        originalDistance = Project(zp1.GetCurrent(pointIndex) - zp0.GetCurrent(state.Rp0));
        MovePoint(zp1, pointIndex, distance - originalDistance);
        state.Rp1 = state.Rp0;
        state.Rp2 = pointIndex;
        if ((flags & 0x10) != 0) state.Rp0 = pointIndex;
    }

    Vec2 ComputeDisplacement(int mode, Zone* zone, int* point) {
        // compute displacement of the reference point
        if ((mode & 1) == 0) {
            *zone = zp1;
            *point = state.Rp2;
        } else {
            *zone = zp0;
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
            // zp2.Current[pointIndex].P += displacement;
            zp2.TouchState[pointIndex] |= touch;
        }
        state.Loop = 1;
    }

    void MovePoint(Zone zone, int index, float distance) {
        // Vec2 point = zone.GetCurrent(index) + distance * state.Freedom / fdotp;
        int touch = GetTouchState();
        // zone.Current[index].P = point;
        zone.TouchState[index] |= touch;
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

    float* GetPoint(stbtt_uint8* data, int index) { return (float*)(data + sizeof(stbtt_vertex) * index); }

    const float Sqrt2Over2 = (float)(sqrt(2) / 2);

    const int MaxCallStack = 128;
    const float Epsilon = 0.000001f;

    // static void InterpolatePoints(uchar* current, uchar* original, int start, int end, int ref1, int ref2) {
    //     if (start > end) return;

    //     // figure out how much the two reference points
    //     // have been shifted from their original positions
    //     float delta1, delta2;
    //     var lower = *GetPoint(original, ref1);
    //     var upper = *GetPoint(original, ref2);
    //     if (lower > upper)
    //     {
    //         var temp = lower;
    //         lower = upper;
    //         upper = temp;

    //         delta1 = *GetPoint(current, ref2) - lower;
    //         delta2 = *GetPoint(current, ref1) - upper;
    //     }
    //     else
    //     {
    //         delta1 = *GetPoint(current, ref1) - lower;
    //         delta2 = *GetPoint(current, ref2) - upper;
    //     }

    //     var lowerCurrent = delta1 + lower;
    //     var upperCurrent = delta2 + upper;
    //     var scale = (upperCurrent - lowerCurrent) / (upper - lower);

    //     for (int i = start; i <= end; i++)
    //     {
    //         // three cases: if it's to the left of the lower reference point or to
    //         // the right of the upper reference point, do a shift based on that ref point.
    //         // otherwise, interpolate between the two of them
    //         var pos = *GetPoint(original, i);
    //         if (pos <= lower)
    //             pos += delta1;
    //         else if (pos >= upper)
    //             pos += delta2;
    //         else
    //             pos = lowerCurrent + (pos - lower) * scale;
    //         *GetPoint(current, i) = pos;
    //     }
    // }

};






struct TrueTypeInterpreter {
	stbtt_fontinfo* info;
    // Typeface currentTypeFace;
    SharpFontInterpreter _interpreter;

    bool UseVerticalHinting;

    void SetTypeFace(stbtt_fontinfo* info) {
        this->info = info;

        int max = 100;
        _interpreter.init(max,max,max,max,max);

        // the fpgm table optionally contains a program to run at initialization time 
        if (info->fpgm != 0) {
        	uchar* instructions = info->data + info->fpgm;
            _interpreter.InitializeFunctionDefs(instructions, 100);
        }
    }

    // void HintGlyph(ushort glyphIndex, float glyphSizeInPixel, stbtt_vertex* vertices, int* verticesCount) {

    //     Glyph glyph = currentTypeFace.GetGlyphByIndex(glyphIndex);
    //     //-------------------------------------------
    //     //1. start with original points/contours from glyph 
    //     int horizontalAdv = currentTypeFace.GetHAdvanceWidthFromGlyphIndex(glyphIndex);
    //     int hFrontSideBearing = currentTypeFace.GetHFrontSideBearingFromGlyphIndex(glyphIndex);

    //     return HintGlyph(horizontalAdv,
    //         hFrontSideBearing,
    //         glyph.MinX,
    //         glyph.MaxY,
    //         glyph.GlyphPoints,
    //         glyph.EndPoints,
    //         glyph.GlyphInstructions,
    //         glyphSizeInPixel);

    // }

    // void HintGlyph(int horizontalAdv, int hFrontSideBearing, int minX, int maxY, stb_vertex[] glyphPoints, ushort[] contourEndPoints, byte[] instructions, float glyphSizeInPixel) {

    //     //get glyph for its matrix

    //     //TODO: review here again

    //     int verticalAdv = 0;
    //     int vFrontSideBearing = 0;
    //     var pp1 = new stb_vertex((minX - hFrontSideBearing), 0, true);
    //     var pp2 = new stb_vertex(pp1.X + horizontalAdv, 0, true);
    //     var pp3 = new stb_vertex(0, maxY + vFrontSideBearing, true);
    //     var pp4 = new stb_vertex(0, pp3.Y - verticalAdv, true);
    //     //-------------------------

    //     //2. use a clone version extend org with 4 elems
    //     int orgLen = glyphPoints.Length;
    //     stb_vertex[] newGlyphPoints = Utils.CloneArray(glyphPoints, 4);
    //     // add phantom points; these are used to define the extents of the glyph,
    //     // and can be modified by hinting instructions
    //     newGlyphPoints[orgLen] = pp1;
    //     newGlyphPoints[orgLen + 1] = pp2;
    //     newGlyphPoints[orgLen + 2] = pp3;
    //     newGlyphPoints[orgLen + 3] = pp4;

    //     //3. scale all point to target pixel size
    //     float pxScale = currentTypeFace.CalculateToPixelScale(glyphSizeInPixel);
    //     for (int i = orgLen + 3; i >= 0; --i)
    //     {
    //         newGlyphPoints[i].ApplyScale(pxScale);
    //     }

    //     //----------------------------------------------
    //     //test : agg's vertical hint
    //     //apply large scale on horizontal axis only 
    //     //translate and then scale back
    //     float agg_x_scale = 1000;
    //     //
    //     if (UseVerticalHinting)
    //     {
    //         ApplyScaleOnlyOnXAxis(newGlyphPoints, agg_x_scale);
    //     }

    //     //4.  
    //     _interpreter.SetControlValueTable(currentTypeFace.ControlValues,
    //         pxScale,
    //         glyphSizeInPixel,
    //         currentTypeFace.PrepProgramBuffer);
    //     //--------------------------------------------------
    //     //5. hint
    //     _interpreter.HintGlyph(newGlyphPoints, contourEndPoints, instructions);

    //     //6. scale back
    //     if (UseVerticalHinting)
    //     {
    //         ApplyScaleOnlyOnXAxis(newGlyphPoints, 1f / agg_x_scale);
    //     }
    //     return newGlyphPoints;

    // }

    // static void ApplyScaleOnlyOnXAxis(stb_vertex[] glyphPoints, float xscale)
    // {
    //     //TODO: review performance here
    //     for (int i = glyphPoints.Length - 1; i >= 0; --i)
    //     {
    //         glyphPoints[i].ApplyScaleOnlyOnXAxis(xscale);
    //     }

    // }

};


