
FLAGS 	= -Wall -Werror -O2 -pedantic -g

SRC 		= ./src
BIN 		= ./bin
OBJ 		= ./obj
INCLUDE = ./include


all: server

server: $(OBJ)/main.o $(OBJ)/server.o $(OBJ)/packet.o
	gcc $(FLAGS) $(OBJ)/*.o -I $(INCLUDE) -o $(BIN)/$@

#mascara genérica para compilar os arquivos objetos
$(OBJ)/%.o: $(SRC)/%.c
	gcc -c $(FLAGS) $< -I $(INCLUDE) -o $@

run:
	$(BIN)/server


clean:
	rm -f $(OBJ)/*
	rm -f $(BIN)/*
