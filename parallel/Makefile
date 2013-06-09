All: spring

CC = nvcc


LDFLAGS+= -lcutil -lm -lcublas
CFLAGS+= -O3 
NVCCFLAGS= -arch sm_13 --ptxas-options=-v

spring: datalist.o util.o main.o asm.o
	$(CC)  $(LDFLAGS) -o $@ $^

%.o : %.cu
	$(CC) $(NVCCFLAGS) $(CFLAGS) -o $@ -c $^ 

%.o: %.cpp
	g++ $(CLFAGS) -o $@ -c $^

clean:
	$(RM) *.o *.s *.i spring
