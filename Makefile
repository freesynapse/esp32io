all : esp32io serial_write

esp32io : esp32io.cpp serial.h array.h plotter.h types.h plotter_utils.h thread.h gui.h config_parser.h
	g++ esp32io.cpp -ggdb -o esp32io -Iraylib/src/ -Lraylib/src/ -lraylib -lX11 -lpthread
#	g++ esp32io.cpp -O3 -o esp32io -Iraylib/src/ -Lraylib/src/ -lraylib -lX11 -lpthread

serial_write : serial_write.cpp types.h
	g++ serial_write.cpp -ggdb -o serial_write
