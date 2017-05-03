
enum StructType {
	STRUCTTYPE_INT = 0,
	STRUCTTYPE_FLOAT,
	STRUCTTYPE_CHAR,
	STRUCTTYPE_BOOL,

	STRUCTTYPE_VEC3,
	STRUCTTYPE_ENTITY,

	STRUCTTYPE_SIZE,
};

enum ArrayType {
	ARRAYTYPE_CONSTANT, 
	ARRAYTYPE_DYNAMIC, 

	ARRAYTYPE_SIZE, 
};

struct ArrayInfo {
	int type;
	int sizeMode;

	union {
		int size;
		int offset;
	};
};

struct StructMemberInfo {
	char* name;
	int type;
	int offset;
	int arrayCount;
	ArrayInfo arrays[2];
};

StructMemberInfo initMemberInfo(char* name, int type, int offset) {
	StructMemberInfo info;
	info.name = name;
	info.type = type;
	info.offset = offset;
	info.arrayCount = 0;

	return info;
}

StructMemberInfo initMemberInfo(char* name, int type, int offset, ArrayInfo aInfo1, ArrayInfo aInfo2 = {-1}) {
	StructMemberInfo info = initMemberInfo(name, type, offset);
	info.arrayCount = 1;
	if(aInfo2.type != -1) info.arrayCount = 2;

	info.arrays[0] = aInfo1;
	info.arrays[1] = aInfo2;

	return info;
}

StructMemberInfo vec3StructMemberInfos[] = {
	initMemberInfo("x", STRUCTTYPE_FLOAT, offsetof(Vec3, x)),
};

struct StructInfo {
	char* name;
	int size;
	int memberCount;
	StructMemberInfo* list;
};

StructInfo structInfos[] = {
	{ "int", sizeof(int), 0 },
	{ "float", sizeof(float), 0 },
	{ "char", sizeof(char), 0 },
	{ "bool", sizeof(bool), 0 },
	{ "Vec3", sizeof(Vec3), arrayCount(vec3StructMemberInfos), vec3StructMemberInfos }, 
};

bool typeIsPrimitive(int type) {
	return structInfos[type].memberCount == 0;
}

char* castTypeArray(char* base, ArrayInfo aInfo) {
	char* arrayBase = (aInfo.type == ARRAYTYPE_CONSTANT) ? base :*((char**)(base));
	return arrayBase;
}

int getTypeArraySize(char* structBase, ArrayInfo aInfo, char* arrayBase = 0) {
	int arraySize;
	if(aInfo.sizeMode == 0) arraySize = aInfo.size;
	else if(aInfo.sizeMode == 1) arraySize = *(int*)(structBase + aInfo.offset);
	else arraySize = strLen(arrayBase);

	return arraySize;
}

void printType(int structType, char* data, int depth = 0);
void printTypeArray(char* structBase, StructMemberInfo member, char* data, int arrayIndex, int depth) {
	int arrayPrintLimit = 5;

	bool isPrimitive = typeIsPrimitive(member.type);
	ArrayInfo aInfo = member.arrays[arrayIndex];
	int typeSize = structInfos[member.type].size;

	char* arrayBase = castTypeArray(data, aInfo);
	int arraySize = getTypeArraySize(structBase, aInfo, arrayBase);

	printf("[ ");
	
	if(arrayIndex == member.arrayCount-1) {
		int size = min(arraySize, arrayPrintLimit);
		bool isPrimitive = typeIsPrimitive(member.type);

		for(int i = 0; i < size; i++) {
			char* value = arrayBase + (typeSize*i);
			if(!isPrimitive) printf("{ ");
			printType(member.type, value, depth + 1);
			if(!isPrimitive) printf("}, ");
		}
		if(arraySize > arrayPrintLimit) printf("... ");
	} else {
		ArrayInfo aInfo2 = member.arrays[arrayIndex+1];

		int arrayOffset;
		if(aInfo.type == ARRAYTYPE_CONSTANT && aInfo2.type == ARRAYTYPE_CONSTANT) {
			int arraySize2 = getTypeArraySize(data, aInfo2);
			arrayOffset = arraySize2*typeSize;
		} else arrayOffset = sizeof(int*); // Pointers have the same size, so any will suffice.

		for(int i = 0; i < arraySize; i++) {
			printTypeArray(structBase, member, arrayBase + i*arrayOffset, arrayIndex+1, depth+1);
		}
	}

	printf("], ");
}

void printType(int structType, char* data, int depth) {
	int arrayPrintLimit = 5;
	int indent = 2;

	StructInfo* info = structInfos + structType;

	if(depth == 0) printf("{\n");

	if(typeIsPrimitive(structType)) {
		switch(structType) {
			case STRUCTTYPE_INT: printf("%i", *(int*)data); break;
			case STRUCTTYPE_FLOAT: printf("%f", *(float*)data); break;
			case STRUCTTYPE_CHAR: printf("%c", *data); break;
			case STRUCTTYPE_BOOL: printf((*((bool*)data)) ? "true" : "false"); break;
			default: break;
		};

		printf(", ");
		if(depth == 0) printf("\n");
		return;
	}

	for(int i = 0; i < info->memberCount; i++) {
		StructMemberInfo member = info->list[i];

		bool isPrimitive = typeIsPrimitive(member.type);

		if(depth == 0) printf("%*s", indent*(depth+1), "");

		if(member.arrayCount == 0) {
			if(!isPrimitive) printf("{ ");
			printType(member.type, data + member.offset, depth + 1);
			if(!isPrimitive) printf("}, ");
			if(depth == 0) printf("\n");
		} else {
			ArrayInfo aInfo = member.arrays[0];
			printTypeArray(data, member, data + member.offset, 0, depth + 1);

			if(depth == 0) printf("\n");
		}

	}

	if(depth == 0) printf("}\n");

}





void guiPrintIntrospection(Gui* gui, int structType, char* data, int depth = 0);
void guiPrintIntrospection(Gui* gui, char* structBase, StructMemberInfo member, char* data, int arrayIndex, int depth) {
	int arrayPrintLimit = 5;

	bool isPrimitive = typeIsPrimitive(member.type);
	ArrayInfo aInfo = member.arrays[arrayIndex];
	int typeSize = structInfos[member.type].size;

	char* arrayBase = castTypeArray(data, aInfo);
	int arraySize = getTypeArraySize(structBase, aInfo, arrayBase);

	if(arrayIndex == member.arrayCount-1) {
		int size = min(arraySize, arrayPrintLimit);
		bool isPrimitive = typeIsPrimitive(member.type);

		if(aInfo.sizeMode == 2) {
			gui->label(data);
		} else {
			for(int i = 0; i < size; i++) {
				char* value = arrayBase + (typeSize*i);
				printType(member.type, value, depth + 1);
			}
			if(arraySize > arrayPrintLimit) printf("... ");
		}
	} else {
		ArrayInfo aInfo2 = member.arrays[arrayIndex+1];

		int arrayOffset;
		if(aInfo.type == ARRAYTYPE_CONSTANT && aInfo2.type == ARRAYTYPE_CONSTANT) {
			int arraySize2 = getTypeArraySize(data, aInfo2);
			arrayOffset = arraySize2*typeSize;
		} else arrayOffset = sizeof(int*); // Pointers have the same size, so any will suffice.

		for(int i = 0; i < arraySize; i++) {
			guiPrintIntrospection(gui, structBase, member, arrayBase + i*arrayOffset, arrayIndex+1, depth+1);
		}
	}
}

void guiPrintIntrospection(Gui* gui, int structType, char* data, int depth) {
	int arrayPrintLimit = 5;
	int indent = 2;

	StructInfo* info = structInfos + structType;

	if(typeIsPrimitive(structType) == false) gui->label(fillString("%s", info->name), 0);

	gui->startPos.x += 30;
	gui->panelWidth -= 30;

	if(typeIsPrimitive(structType)) {
		switch(structType) {
			case STRUCTTYPE_INT: gui->label(fillString("%i", *(int*)data)); break;
			case STRUCTTYPE_FLOAT: gui->label(fillString("%f", *(float*)data)); break;
			case STRUCTTYPE_CHAR: gui->label(fillString("%c", *data)); break;
			case STRUCTTYPE_BOOL: gui->label(fillString("%b", *(bool*)data)); break;
			default: break;
		};
	} else {

		for(int i = 0; i < info->memberCount; i++) {
			StructMemberInfo member = info->list[i];

			bool isPrimitive = typeIsPrimitive(member.type);
			if(isPrimitive) {
				gui->div(vec2(0,0));
				gui->label(fillString("%s", member.name), 0);
			}

			if(member.arrayCount == 0) {
				guiPrintIntrospection(gui, member.type, data + member.offset, depth + 1);
			} else {
				ArrayInfo aInfo = member.arrays[0];
				guiPrintIntrospection(gui, data, member, data + member.offset, 0, depth + 1);
			}

		}
	}

	gui->startPos.x -= 30;
	gui->panelWidth += 30;

}



#define DEBUG_NOTE_LENGTH 50
#define DEBUG_NOTE_DURATION 3

struct Asset;
struct DebugState {
	// Asset* assets;
	// int assetCount;

	i64 lastTimeStamp;
	f64 dt;
	f64 time;

	f64 debugTime;
	f64 debugRenderTime;

	bool showMenu;
	bool showStats;
	bool showConsole;
	bool showHud;

	Input* input;

	Timer* timer;
	Timings timings[120][TIMER_INFO_SIZE];
	Statistic statistics[120][TIMER_INFO_SIZE];
	int cycleIndex;
	bool stopCollating;

	bool setPause;
	bool setPlay;
	bool noCollating;
	int lastCycleIndex;
	GraphSlot* savedBuffer;
	int savedBufferIndex;
	int savedBufferMax;
	int savedBufferCount;
	int lastBufferIndex;

	GraphSlot graphSlots[16][8]; // threads, stackIndex
	int graphSlotCount[16];

	//

	int mode;
	int graphSortingIndex;
	
	double timelineCamPos;
	double timelineCamSize;

	float lineGraphCamPos;
	float lineGraphCamSize;
	float lineGraphHeight;
	int lineGraphHighlight;

	//

	GuiInput gInput;
	Gui* gui;
	Gui* gui2;
	float guiAlpha;

	Input* recordedInput;
	int inputCapacity;
	bool recordingInput;
	int inputIndex;

	char* snapShotMemory[8];
	int snapShotCount;
	int snapShotMemoryIndex;

	bool playbackInput;
	int playbackIndex;
	bool playbackSwapMemory;
	bool playbackPause;
	bool playbackBreak;
	int playbackBreakIndex;

	Console console;

	char* notificationStack[10];
	float notificationTimes[10];
	int notificationCount;

	char* infoStack[50];
	int infoStackCount;
};

void addDebugNote(char* string, float duration = DEBUG_NOTE_DURATION) {
	DebugState* ds = globalDebugState;

	assert(strLen(string) < DEBUG_NOTE_LENGTH);
	if(ds->notificationCount >= arrayCount(ds->notificationStack)) return;

	int count = ds->notificationCount;
	strClear(ds->notificationStack[count]);
	ds->notificationTimes[count] = duration;
	strCpy(ds->notificationStack[count], string);
	ds->notificationCount++;
}

void addDebugInfo(char* string) {
	DebugState* ds = globalDebugState;

	if(ds->infoStackCount >= arrayCount(ds->infoStack)) return;
	ds->infoStack[ds->infoStackCount++] = string;
}

void addDebugInfoInt  (int v)   { addDebugInfo(fillString("%i", v)); }
void addDebugInfoFloat(float v) { addDebugInfo(fillString("%f", v)); }
void addDebugInfoVec2i(Vec2i v) { addDebugInfo(fillString("{%i, %i}", v.x, v.y)); }
void addDebugInfoVec2 (Vec2 v)  { addDebugInfo(fillString("{%f, %f}", v.x, v.y)); }
void addDebugInfoVec3 (Vec3 v)  { addDebugInfo(fillString("{%f, %f, %f}", v.x, v.y, v.z)); }
void addDebugInfoVec4 (Vec4 v)  { addDebugInfo(fillString("{%f, %f, %f, %f}", v.r, v.g, v.g, v.a)); }



struct Asset {
	int index;
	int folderIndex;
	char* filePath;
	FILETIME lastWriteTime;
};

