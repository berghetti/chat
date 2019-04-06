FLAGS = -Wall -pedantic

all: servidor cliente

servidor: servidor.c
	gcc -o servidor servidor.c $(FLAGS)

cliente: cliente.c
	gcc -o cliente cliente.c $(FLAGS)
