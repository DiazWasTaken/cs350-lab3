CC = gcc
DEBUG = -g
DEFINES =
WERROR = 
#WERROR = -Werror 
CFLAGS = $(DEBUG) -Wall -Wextra -Wshadow -Wunreachable-code -Wredundant-decls \
	-Wmissing-declarations -Wold-style-definition \
	-Wmissing-prototypes -Wdeclaration-after-statement \
	-Wno-return-local-addr -Wunsafe-loop-optimizations \
	-Wuninitialized $(WERROR) $(DEFINES)

PROG1 = viktar
PROGS = $(PROG1)


all: $(PROGS)

$(PROG1): $(PROG1).o
	$(CC) $(CFLAGS) -o $@ $^ -lmd

$(PROG1).o: $(PROG1).c $(PROG1).h
	$(CC) $(CFLAGS) -c $< -lmd

clean cls:
	rm -f $(PROGS) *.o *~ \#*

tar:
	tar cvfa bin_file_${LOGNAME}.tar.gz *.[ch] [mM]akefile

git:
	git
