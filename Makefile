
CFLAGS=-I.
TARGET1=bk2comm
TARGET2=append-bin
TARGET3=set_header

all: $(TARGET1) $(TARGET2) $(TARGET3)

$(TARGET1): BK2com.o
	$(CXX) -o $@ BK2com.o -lrt

BK2com.o: BK2com.cpp BK2com.h
	$(CXX) -c -o $@ $< $(CFLAGS)

$(TARGET2): append-bin.c
	$(CC) -o append-bin append-bin.c

$(TARGET3): set_header.c
	$(CC) -o set_header set_header.c

clean:
	rm -f $(TARGET1) $(TARGET2) $(TARGET3) *.o
