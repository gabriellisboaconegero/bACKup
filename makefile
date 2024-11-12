CPPFLAGS = -Wall -Wextra

all: CPPFLAGS+= -DDOCKER_TEST
all: obj exe exe/cliente exe/servidor

debug2: CPPFLAGS+= -DDEBUG2
debug2: debug

faildebug: CPPFLAGS+= -DSIMULATE_UNSTABLE
faildebug: debug

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
exe/cliente: obj/cliente.o obj/socket.o obj/utils.o
	g++ $^ -o $@ 

exe/servidor: obj/servidor.o obj/socket.o obj/utils.o
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

