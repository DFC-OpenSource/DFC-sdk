
#ifndef __NVME_STATS_H
#define __NVME_STATS_H

enum BlockAcctType {
	BLOCK_ACCT_READ,
	BLOCK_ACCT_WRITE,
	BLOCK_ACCT_FLUSH,
	BLOCK_MAX_IOTYPE,
};

typedef struct BlockAcctStats {
	uint64_t nr_bytes[BLOCK_MAX_IOTYPE];
	uint64_t nr_ops[BLOCK_MAX_IOTYPE];
	uint64_t total_time_ns[BLOCK_MAX_IOTYPE];
	uint64_t wr_highest_sector;
} BlockAcctStats;

/*TODO*/
#define blk_get_stats(x) NULL

#endif /*#ifndef __NVME_STATS_H*/

