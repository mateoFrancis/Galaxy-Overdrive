all: background

background: background.cpp
	g++ background.cpp -Wall libggfonts.a -lX11 -lGL -lGLU -lm -oproject

clean:
	rm -f background 


