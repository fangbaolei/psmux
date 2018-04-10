CC = g++
XX = g++
CFLAGS = -Wall -O -g 
TARGET = psmux
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
%.o:%.cpp
	$(XX) $(CFLAGS) -c $< -o $@

SOURCES = $(wildcard *.c *.cpp)
OBJS = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCES)))

$(TARGET) : $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) -lpthread
	chmod a+x $(TARGET)

clean:
	 rm -rf *.o psmux
