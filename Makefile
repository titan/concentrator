all:
	make -C lib
	make -C src

clean:
	make -C lib clean
	make -C src clean
