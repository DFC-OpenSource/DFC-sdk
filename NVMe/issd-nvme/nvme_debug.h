
#ifndef __NVME_DEBUG_H
#define __NVME_DEBUG_H

typedef enum nvme_log_lvls {
	NVME_LOG_DBG,
	NVME_LOG_INFO,
	NVME_LOG_WARN,
	NVME_LOG_ERR,
} NvmeLogLvls;

/*Function declarations*/
static inline void hexDump_32Bit (uint32_t *buf, uint16_t size)
{
	uint16_t n32 = 0;
	for (; n32 < size && buf; n32++) {
		printf ("%08x%s", *buf++, (n32+1)%1?" ":"\r\n");
	}
}

#define hexDump_SQEntry(buf) hexDump_32Bit (buf, 16)

#endif /*#ifndef __NVME_DEBUG_H*/

