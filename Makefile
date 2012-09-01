TARGET	=	exec

CC		=	gcc
CFLAGS	= 	-g -Wall -I.
LINKER	=	gcc -o
LFLAGS	= 	-Wall -I. -lm -lpthread

LEXER	=	lex -o

BINDIR	=	bin
OBJDIR	=	obj
SRCDIR	=	src
LEXDIR	=	parser

PROGRAMSOURCES 	:= 	$(wildcard $(SRCDIR)/*.c)
#PARSERSOURCES	:=	$(wildcard $(LEXDIR)/*.l)
OBJECTS 		:= 	$(PROGRAMSOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

$(BINDIR)/$(TARGET) : 	$(OBJECTS)
	$(LINKER) $@ $(LFLAGS) $(OBJECTS)
 
$(OBJECTS)			:	$(OBJDIR)/%.o :$(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY	: 	run
run		:
	./$(BINDIR)/$(TARGET)
print:
	@echo $(OBJECTS)
