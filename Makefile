all : CircuitCompleter

CircuitCompleter : CircuitCompleter.c
	gcc -Wall -Werror -fsanitize=address -std=c11 CircuitCompleter.c -o CircuitCompleter -lm

GrayCodeInputs : GrayCodeInputs.c
	gcc -Wall -Werror -fsanitize=address -std=c11 GrayCodeInputs.c -o GrayCodeInputs -lm

CircuitReducer : CircuitReducer.c
	gcc -Wall -Werror -fsanitize=address -std=c11 CircuitReducer.c -o CircuitReducer -lm
