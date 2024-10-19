CPPFLAGS = -Wall -Wextra

all: obj exe cliente servidor

debug: CPPFLAGS+= -DDEBUG -g
debug: all

# ================ objetos ================
obj/%.o: src/%.cpp src/%.h
	g++ $(CPPFLAGS) -I src -c $< -o $@

obj/cliente.o: src/cliente.cpp 
	g++ $(CPPFLAGS) -I include -c $< -o $@

obj/servidor.o: src/servidor.cpp 
	g++ $(CPPFLAGS) -I include -c $< -o $@
# ================ objetos ================

# ================ executaveis ================
exe/cliente: obj/cliente.o obj/socket.o
	g++ $^ -o $@ 

exe/servidor: obj/servidor.o obj/socket.o
	g++ $^ -o $@ 
# ================ executaveis ================

obj:
	mkdir -p obj

exe:
	mkdir -p exe
	
clean:
	rm -rf exe/* obj/*

purge: clean
	rm -rf cliente servidor

