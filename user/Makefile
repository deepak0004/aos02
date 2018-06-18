SRC=$(wildcard *.c)
DST=$(patsubst %.c,%,$(SRC))

default: $(DST)

$(DST): $(SRC) handler.S
	gcc -o $@ -g $^

clean:
	rm -rf $(DST)
