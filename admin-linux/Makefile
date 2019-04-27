
FLAGS = -Wall -Werror -O2 -pedantic

all: admin cliente

admin: admin.c
	gcc $(FLAGS) $<  -o admin

cliente: cliente.c
	gcc $(FLAGS) $<  -o cliente

clean:
	rm -f admin
