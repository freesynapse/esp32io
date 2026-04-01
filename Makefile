all : esp32io serial_write raylib_test

esp32io : esp32io.cpp serial.h array.h plotter.h types.h plotter_utils.h
	g++ esp32io.cpp -ggdb -o esp32io -Iraylib/src/ -Lraylib/src/ -lraylib -lX11 -lpthread

serial_write : serial_write.cpp types.h
	g++ serial_write.cpp -ggdb -o serial_write

raylib_test : raylib_test.cpp
	g++ raylib_test.cpp -ggdb -o raylib_test -Iraylib/src/ -Lraylib/src/ -lraylib -lX11

