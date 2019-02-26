dash: DashGL/dashgl.c
	gcc -c -o DashGL/dashgl.o DashGL/dashgl.c -lepoxy -lpng -lm

main: main.c
	gcc `pkg-config --cflags gtk+-3.0` main.c DashGL/dashgl.o `pkg-config --libs gtk+-3.0` \
	-lepoxy -lm -lpng