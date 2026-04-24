all: background

background: background.cpp enemy.cpp enemy.h obstacles.cpp
	g++ background.cpp enemy.cpp -Wall libggfonts.a -lX11 -lGL -lGLU -lm -o project

clean:
	rm -f project

