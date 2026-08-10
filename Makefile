CC = gcc
CFLAGS = -Wall -Wextra -02 -I./include
LIBS = -lcmark -lws2_32

SRCS = src/main.c src/parser.c src/render.c src/build.c src/serve.c src/init.c src/deploy.c src/rss.c toml.c
OBJS = $(SRCS: .c=.o)

atomik-ssg:	$(SRCS)
	$(CC) $(FLAGS) -o atomik-ssg $(SRCS) $(LIBS)

clean: 
	rm -f atomik-ssg.exe
	rm -rf public/*


.PHONY: clean
