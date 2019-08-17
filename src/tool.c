#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "error.h"
#include "tool.h"

void emptyBuffer()
{
  char c;
  while ((c = getchar()) != '\n' && c != EOF);
}

int existCharAfter(char* string, char target)
{
  char* cursor = string;
  while(*cursor)
    {
      if(*cursor == target)
	return 1;

      cursor++;
    }
  return 0;
}
char* readRestrictedSize(int size)
{
  char* buffer = calloc(size,sizeof(char));
  char* endPosition = NULL;
  
  if(fgets(buffer, size, stdin) != NULL)
    {
      endPosition = strchr(buffer,'\n');
      if(endPosition != NULL)
	{
	  *endPosition = '\0';
	  return buffer;
	}
      else
	{
	  emptyBuffer();
	  return NULL;
	}
    }
  else
    {
      emptyBuffer();
      return NULL;
    }
}

int readEntire(char** entireString)
{
  unsigned int pos = 0;
  unsigned int max = BASE_STRING_SIZE_READING;
  
  char tmpString[LENGTH_BLOCK_READING];
  int isReading = 1;
  char* endPosition = NULL;
  char* position0 = NULL;

  *entireString = calloc(LENGTH_BLOCK_READING, sizeof(char));
  
  while(isReading)
    {
      if(fgets(tmpString,LENGTH_BLOCK_READING,stdin) != NULL)
	{
	  endPosition = strchr(tmpString, '\n');
	  if(endPosition != NULL)
	    {
	      *endPosition = '\0';
	      if(pos + strlen(tmpString) + 1 >= max)
		{
		  max *= 2;
		  *entireString = realloc(*entireString, max * sizeof(char));
		}

	      strcpy(*entireString + pos,tmpString);
	      isReading = 0;
	    }
	  else
	    {
	      position0 = strchr(tmpString,'\0');
	      if(position0 != NULL)
		{
		  if(pos + LENGTH_BLOCK_READING >= max)
		    {
		      max *= 2;
		      *entireString = realloc(*entireString, max * sizeof(char));
		    }
		  
		  strcpy(*entireString + pos,tmpString);
		  *position0 = 'Z';
		  pos += LENGTH_BLOCK_READING-1;
		}
	      else
		{
		  isReading = 0;
		  emptyBuffer();
		  free(*entireString);
		  *entireString = NULL;
		  return READING_USER_INPUT_ERROR;
		}
	    }
	}
      else
	{
	  isReading = 0;
	  emptyBuffer();
	  free(*entireString);
	  *entireString = NULL;
	  return READING_USER_INPUT_ERROR;
	}
    }
  
  return 0;
}

void removeOccurences(char* string, char toRemove)
{
  char *p = string; /* p points to the most current "accepted" char. */
  while (*string) {
    if (*string != toRemove)
      *p++ = *string;

    string++;
  }
  
  *p = '\0';
}

void skipSpace(char** string)
{
  while(*(*string) == ' ')
    {
      (*string)++;
    }
}

int stringIsInteger(const char* string)
{
  unsigned int i;
  for(i = 0; i < strlen(string); i++)
    {
      if(!isdigit(string[i]))
	{
	  return 0;
	}
    }
  return 1;
}

int getMinusPowOf2(int value)
{
  int it = 0;
  while(1)
    {
      int powVal = pow(2,it);
      if(powVal >= value)
	{
	  return powVal;
	}
      it++;
    }
}

void removeUselessSpace(char* line)
{
  char* p = line;
  int nb = 0;
  int hasAlreadyReadSpace = 0;
  int hasReadFirstLetter = 0;
  
  while(*line)
    {
      if(isspace(*line))
	{
	  if(!hasAlreadyReadSpace && hasReadFirstLetter)
	    {
	      *p++ = ' ';
	      hasAlreadyReadSpace = 1;
	      nb++;
	    }
	}
      else
	{
	  *p++ = *line;
	  nb++;
	  hasReadFirstLetter = 1;
	  hasAlreadyReadSpace = 0;
	}

      line++;
    }

  *p = '\0';
  if(nb > 0 && *(p-1) == ' ')
    {
      *(p-1) = '\0';
    }
}
