build: main.c fs.c fs.h
	gcc -Wall -Wextra -o main main.c fs.c

debug: main.c fs.c fs.h
	gcc -g -Wall -Wextra -pedantic -o main main.c fs.c
