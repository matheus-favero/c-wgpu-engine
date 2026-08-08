GAME_NAME = Teste.out
FILES = src/main.c src/sdl3webgpu.c src/transformations.c
INCLUDE = -I./include
LIBS = -L./lib -lwgpu_native -lSDL3 -lm
RPATH = -Wl,-rpath=./lib
all:
	clang -g $(FILES) $(INCLUDE) $(LIBS) $(RPATH) -o $(GAME_NAME)

run: all
	SDL_VIDEODRIVER=x11 ./$(GAME_NAME)
	$(MAKE) clean

run-valgrind: all
	SDL_VIDEODRIVER=x11 valgrind ./$(GAME_NAME)
	$(MAKE) clean

clean:
	rm -f $(GAME_NAME)