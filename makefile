regex: common.o NFA.o main.o postfix.o
	gcc -o main *.o -g

main: main.c
	gcc -c main.c -g

common: common.h common.c
	gcc -c common.c -g

postfix: postfix.h postfix.c
	gcc -c postfix.c -g

nfa: NFA.c NFA.h
	gcc -c NFA.c -g

run: regex
	./main

clean:
	rm main *.o