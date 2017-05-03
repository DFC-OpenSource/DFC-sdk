#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <linux/types.h>

#define usage_string(name)\
"usage:\n"\
"%s <mem_base> [size]\n"\
,name

#define print_usage(name) fprintf(stderr, usage_string(name));
#define usage() print_usage(argv[0])

/*
 * Print data buffer in hex and ascii form to the terminal.
 *
 * data reads are buffered so that each memory address is only read once.
 * Useful when displaying the contents of volatile registers.
 *
 * parameters:
 *    addr: Starting address to display at start of line
 *    data: pointer to data buffer
 *    width: data value width.  May be 1, 2, or 4.
 *    count: number of values to display
 *    linelen: Number of values to print per line; specify 0 for default length
 */
#define MAX_LINE_LENGTH_BYTES (64)
#define DEFAULT_LINE_LENGTH_BYTES (16)
int print_buffer (ulong addr, void* data, uint width, uint count, uint linelen)
{
	__u8 linebuf[MAX_LINE_LENGTH_BYTES];
	__u64 *ulp = (void*)linebuf;
	__u32 *uip = (void*)linebuf;
	__u16 *usp = (void*)linebuf;
	__u8 *ucp = (void*)linebuf;
	int i;

	if (linelen*width > MAX_LINE_LENGTH_BYTES)
		linelen = MAX_LINE_LENGTH_BYTES / width;
	if (linelen < 1)
		linelen = DEFAULT_LINE_LENGTH_BYTES / width;

	while (count) {
		printf("%08lx:", addr);

		/* check for overflow condition */
		if (count < linelen)
			linelen = count;

		/* Copy from memory into linebuf and print hex values */
		for (i = 0; i < linelen; i++) {
			if (width == 8) {
				ulp[i] = *(volatile __u64 *)data;
				printf(" %016llx", ulp[i]);
			} else if (width == 4) {
				uip[i] = *(volatile __u32 *)data;
				printf(" %08x", uip[i]);
			} else if (width == 2) {
				usp[i] = *(volatile __u16 *)data;
				printf(" %04x", usp[i]);
			} else {
				ucp[i] = *(volatile __u8 *)data;
				printf(" %02x", ucp[i]);
			}
			data += width;
		}

		/* Print data in ASCII characters */
		printf("    ");
		for (i = 0; i < linelen * width; i++)
			fputc(isprint(ucp[i]) && (ucp[i] < 0x80) ? ucp[i] : '.', stdout);
		fputc ('\n', stdout);

		/* update references */
		addr += linelen * width;
		count -= linelen;
	}

	return 0;
}

int main(int argc, char **argv)
{

	unsigned long long int base, base_page, page_off, size = 256, width = 8;
	char *mem;
	int fd;

	if(argc < 2) {
		usage();
		return -1;
	}

	if(argc == 3)
		size = strtoul(argv[2], NULL, 16);

	if( *(argv[0] + strlen(argv[0]) - 2) == '.') {
		switch( *(argv[0] + strlen(argv[0]) - 1)) {
		case 'b':
			width = 1;
			break;
		case 'w':
			width = 2;
			break;
		case 'l':
			width = 4;
			break;
		case 'd':
			width = 8;
			break;
		}
	}

	base = strtoul(argv[1], NULL, 16);

	if((fd = open("/dev/mem", O_RDONLY)) < 0) {
		fprintf(stderr, "error while opening /dev/mem: %s\n", strerror(errno));
		return -1;
	}

	page_off = base % getpagesize();
	base_page = base - page_off;

	if(size > getpagesize() - page_off) {
		fprintf(stderr, "this size and offset not supported now\n");
		return -1;
	}
    int j =0;
	mem = mmap(0, getpagesize(), PROT_READ, MAP_SHARED, fd, base_page);
	if((int)mem == -1) {
		perror("mmap failed");
		close(fd);
		return -1;
	}
#if 1 
	print_buffer(base, mem + page_off, width, size/width, 0);
#else
	for(j=0;j<6;j++){
		printf("%08x  ",*(__u32 *)mem+page_off);
	}
    printf("\n");
#endif
	
	munmap(mem, getpagesize());
	close(fd);
	return 0;
}
