cardinality: 
	gcc -lm -std=c99 -Wall -O3 *.c zlib/*.c tests/*.c libs/*.c -o cardinality
