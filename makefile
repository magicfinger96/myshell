# Makefile #

# Compilation #
CC = gcc
EXECUTABLE = mysh
CFLAGS = -Wall -Wextra -pedantic -g
LDFLAGS = -lm
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

.PHONY : myPs myLs philo

all : $(EXECUTABLE)\
	myPs\
	myLs\
	philo

$(EXECUTABLE) : $(OBJS)
	$(CC) $^ $(LDFLAGS) -o $@\


$(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -I $(INCDIR) -o $@ -c $< 

test :
	@echo toto

myPs :
	@echo Realisation du make pour PS
	@make -s -C ./myPs clean
	@make -s -C ./myPs all
	@make -s -C ./myPs all
myLs :
	@echo Realisation du make pour LS
	@make -s -C ./myLs clean
	@make -s -C ./myLs all
	@make -s -C ./myLs all

philo :
	@echo Realisation du make pour Philo
	@make -s -C ./philosopherDinner clean
	@make -s -C ./philosopherDinner all

clean:
	@rm -rf $(OBJDIR)/*.o
	@rm -f $(EXECUTABLE)
	@rm -rf $(SRCDIR)/*.c~
	@rm -rf $(INCDIR)/*.h~
	@rm -rf $(OBJDIR)/*.o~
	@rm -rf *~


