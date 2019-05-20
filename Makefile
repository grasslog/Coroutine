all : main

main : main.c coroutine.c
		gcc -g -wall -o $@ $^

clean:
		rm main