all: othello

clean:
	rm -f *.o

distclean: clean
	rm -f othello *~

othello: othello.o
	$(CC) -o $@ $^
