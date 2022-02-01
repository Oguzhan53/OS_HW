all: program1 program2

program1: part2.c
	gcc -o  makeFileSystem -Wall -ggdb3  part2.c  -lrt -lm
program2: part3.c
	gcc -o  fileSystemOper -Wall -ggdb3  part3.c  -lrt -lm
clean: 
	  $(RM) makeFileSystem
	  $(RM) fileSystemOper
