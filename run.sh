#initialize directory structure
BINDIR=bin
OBJDIR=obj
if [ ! -d "$BINDIR" ]; then
	mkdir "$BINDIR"
fi
if [ ! -d "$OBJDIR" ]; then
	mkdir "$OBJDIR"
fi

lex -o ./src/lex.yy.c ./parser/shell.l
make

<<<<<<< HEAD
#if [ $? -eq 0 ]; then
#	make run
#	echo nothing to run

#else
#	echo make failed 
#fi
=======
if [ $? -eq 0 ]; then
#	make run
	echo nothing to run
else
	echo make failed 
fi

>>>>>>> e92d59f7976c9d0b78f040f2ef2702ceaefd6201
