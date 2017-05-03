
#CROSS_COMPILE ?=
#CC=$(CROSS_COMPILE)gcc

md :
	$(CC) -o md mem_disp.c
	ln -s md md.b
	ln -s md md.w
	ln -s md md.l
	ln -s md md.d
clean:
	rm -f md*

