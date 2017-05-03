#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <linux/types.h>

#define usage_string(name)\
"usage:\n"\
"%s <addr> <value>\n"\
,name

#define print_usage(name) fprintf(stderr, usage_string(name));
#define usage() print_usage(argv[0])

int main(int argc, char **argv)
{

	unsigned long long int base, base_page, page_off, width = 4;
    unsigned long long int value;
	char *mem;
	int fd;

	if(argc < 3) {
		usage();
		return -1;
	}

	base = strtoul(argv[1], NULL, 16);
	value = strtoul(argv[2], NULL, 16);


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

	if((fd = open("/dev/mem", O_RDWR)) < 0) {
		fprintf(stderr, "error while opening /dev/mem: %s\n", strerror(errno));
		return -1;
	}

	page_off = base % getpagesize();
	base_page = base - page_off;

	mem = mmap(0, getpagesize(), PROT_WRITE, MAP_SHARED, fd, base_page);
	if((int)mem == -1) {
		perror("mmap failed");
		close(fd);
		return -1;
	}

	if(width == 1)
		*((volatile uint8_t*)(mem + page_off)) = (uint8_t)value;
	else if(width == 2)
		*((volatile uint16_t*)(mem + page_off)) = (uint16_t)value;
	else if(width == 4)
		*((volatile uint32_t*)(mem + page_off)) = (uint32_t)value;
	else if(width == 8)
		*((volatile uint64_t*)(mem + page_off)) = (uint64_t)value;

	munmap(mem, getpagesize());
	close(fd);

	return 0;
}

