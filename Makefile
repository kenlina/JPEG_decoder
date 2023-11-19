CXX = g++

TARGET = my_jpeg

SRC = main.cpp qdbmp.cpp

all: $(TARGET) print_usage print_output

$(TARGET):
	$(CXX) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) my_photo.bmp

print_usage:
	@echo "\nUsage: \n	./$(TARGET) your_jpeg_file\n"

print_output:
	@echo "\nOutput file name is 'my_photo.bmp' \n"


