
#CROSS_COMPILE ?=
#CC=$(CROSS_COMPILE)gcc
#CC ?=
mw :
	$(CC) -o mw mem_write.c
	ln -s mw mw.b
	ln -s mw mw.w
	ln -s mw mw.l
	ln -s mw mw.d

clean:
	rm -f mw*

