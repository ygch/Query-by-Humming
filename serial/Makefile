All: spring

CC = g++

CLFAGS+= -O3

spring: spring.o util.o main.o asm.o
	$(CC) -o $@ $^

%.o: %.cpp
	$(CC) $(CLFAGS) -o $@ -c $^

clean:
	$(RM) *.o *.s *.i spring
