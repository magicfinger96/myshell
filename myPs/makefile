# Makefile #

# Compilation #
CC = gcc
EXECUTABLE = myps
CFLAGS = -Wall -W -pedantic -g
LDFLAGS = 
LIBS = 

# Directory #
OBJDIR = bin
SRCDIR = src
INCDIR = include

# Finding #
SRC = $(wildcard $(SRCDIR)/*.c)
OBJS = $(SRC:.c=.o)
OBJS := $(subst src/,,$(OBJS))
OBJS := $(addprefix $(OBJDIR)/, $(OBJS))


# Rules

all : $(EXECUTABLE)

$(EXECUTABLE) : $(OBJS)
	$(CC) $^ $(LDFLAGS) -o $@ 

$(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -I $(INCDIR) -o $@ -c $< 

clean:
	@rm -rf $(OBJDIR)/*.o
	@rm -f $(EXECUTABLE)
	@rm -rf $(SRCDIR)/*.c~
	@rm -rf $(INCDIR)/*.h~
	@rm -rf $(OBJDIR)/*.o~
	@rm -rf *~


