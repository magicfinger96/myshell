#ifndef __TOOL__
#define __TOOL__

#define BASE_STRING_SIZE_READING 30
#define LENGTH_BLOCK_READING 30

void emptyBuffer();
int readEntire(char** entireString);
char* readRestrictedSize(int size);
void removeOccurences(char* string, char toRemove);
int existCharAfter(char* string, char target);
void skipSpace(char** string);
void removeUselessSpace(char* line);
int stringIsInteger(const char* string);
int getMinusPowOf2(int value);

#endif
