CC = gcc
CFLAGS = -Wall -Wextra -02
LIBS = -lcmark -lws2_32

atomik-ssg:	main.c toml.c
	$(CC) $(FLAGS) -o atomik-ssg main.c toml.c $(LIBS)

clean: 
	rm -f atomik-ssg.exe
	rm -rf public/*


.PHONY: clean
