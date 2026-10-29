# Default rule (runs when you type 'make')
all: task1 task4 

# Rule to compile the executable
task1: task1.c
	gcc -Wall -g -m32 task1.c -o task1

task4: task4.c
	gcc -m32 -fno-pie -no-pie -fno-stack-protector -g -o task4 task4.c

# Target for clean up (runs when you type 'make clean')
.PHONY: clean
clean:
	rm -f task1 task4