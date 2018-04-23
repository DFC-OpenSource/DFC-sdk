#ifndef __NAND_PARAM_STRUCT_H
#define __NAND_PARAM_STRUCT_H

typedef unsigned long long int u64;
typedef unsigned int u32;
typedef unsigned short int u16;
typedef unsigned char u8;

typedef struct nand_details {

    u32    nr_channels; 
    u32    nr_chips_per_channel; 
    u32    nr_chips; 
    u32    nr_blocks_per_chip; 
    u32    nr_pages_per_block; 
    u32    page_main_size;
    u16    page_oob_size; 
    u32    device_type;
    u8     timing_mode;
    u64    device_capacity_in_byte; 
    u16    page_prog_time_us;
    u16    page_read_time_us;
    u16    block_erase_time_us;
    u64    nr_blocks_per_channel; 
    u64    nr_blocks_per_ssd;
    u64    nr_chips_per_ssd;
    u64    nr_pages_per_ssd;
} nand_details;
nand_details g_nand;

/* ONFI */
/* Revision info and features block */
typedef struct ONFI_RevisionInfo {
    u8      sign[4];
    u16     rev_no;
    u16     features;
    u16     opt_cmds;
    u8		adv_cmd;
    u8		resv;
    u16     ext_param_pg_len;
    u8		num_param_pgs;
    u8		resv1[17];
}__attribute__((packed)) ONFI_RevisionInfo;

/* Manufacturer info block */
typedef struct ONFI_ManufactInfo {
    char    dev_manufacturer[12];
    char    dev_model[20];
    u8      jedec_id;
    u16     data_code;
    u8      resv[13];
}__attribute__((packed)) ONFI_ManufactInfo;

/* Memory Organization block */
typedef struct ONFI_MemOrg{
    u32     databytes_per_pg;
    u16     sparebytes_per_pg;
    u8		resv[6];
    u32     pgs_per_blk;
    u32     blks_per_lun;
    u8		lun_count;
    u8		addr_cycles;
    u8		bits_per_cell;
    u16     badblks_per_lun;
    u16     blk_endurance;
    u8		guaranteed_valid_blks;
    u16     guaranteed_blk_endurance;
    u8		prgms_per_pg;
    u8		resv1;
    u8		ecc_bits;
    u8		interleaved_bits;
    u8		interleaved_op_attr;
    u8		resv2[13];
}__attribute__((packed)) ONFI_MemOrg;

/* Electrical parameters block */
typedef struct ONFI_Electrical{
    u8		io_pin_capac_per_chip_en;
    u16     async_timing_mode;
    u16     resv1;
    u16     t_prog;
    u16     t_bers;
    u16     t_r;
    u16     t_ccs;
    u8		nvddr_timing_mode;
    u8		nvddr2_timing_mode;
    u8		nvddr_or_nvddr2_feat;
    u16     clk_inp_pin_capac_typ;
    u16     io_pin_capac_typ;
    u16     inp_capac_typ;
    u8		inp_pin_capac_max;
    u8		drvr_strgth_support;
    u16     t_r_multi_plane;
    u16     t_adl;
    u16     resv2;
    u8		nvddr2_feat;
    u8		nvddr2_warmup_cyc;
    u32     resv3;
}__attribute__((packed)) ONFI_Electrical;
                
/* Vendor block */
typedef struct ONFI_Vendor {
    u16     vendor_rev_num;
    u8		twoplane_pg_rd;
    u8		rd_cache;
    u8		rd_uniq_id;
    u8		prgm_DQ_outimp;
    u8		num_prgm_DQ_outimp;
    u8		prgm_DQ_outimp_feature;
    u8		prgm_RB_pulldwn_strgth;
    u8		prgm_RB_pulldwn_strgth_feature;
    u8		num_prgm_RB_pulldwn_strgth;
    u8		otp_mode;
    u8		otp_pg_st;
    u8		otp_data_prot;
    u8		num_otp_pgs;
    u8		otp_feature;
    u8		rd_retry_opts;
    u32     avai_rd_retry_opts; 
    u8      resv[68];
    u8      param_pg_rev;
    u16     crc;
}__attribute__((packed)) ONFI_Vendor;

/* Extended parameter pages */
typedef struct ONFI_ExtendedParam {
    u16    ext_crc;
    u32    ext_sign;
    u8     resv[10];
    u8     type;
    u8     len;
    u8     resv1[14];
    u8     ecc_bits;
    u8     ecc_size;
    u16    bad_blks_per_lun;
    u16    blk_endurance;
    u8     resv2[10];
}__attribute__((packed)) ONFI_ExtendedParam;

typedef struct ONFI_Param_Pg {
    ONFI_RevisionInfo    onfi_rev_info; /* Revision info and features block */
    ONFI_ManufactInfo    onfi_manuf_info; /* Manufacturer info block */
    ONFI_MemOrg          onfi_mem_org; /* Memory Organization block */
    ONFI_Electrical      onfi_elec_param; /* Electrical parameters block */
    ONFI_Vendor          onfi_vendor; /* Vendor block */
}__attribute__((packed)) ONFI_Param_Pg; 

/* ONFI parameter page data structure */
typedef struct ONFI {
    ONFI_Param_Pg onfi_param_pg;
//    ONFI_Param_Pg resv1[59];
    ONFI_ExtendedParam   extended_param; /* Extended_param pages */
//    ONFI_ExtendedParam   resv2[59];
}__attribute__((packed)) onfi; 

#if 0
/*JEDEC*/
/* Revision info and features block */
typedef struct JEDEC_RevisionInfo {
    u8      sign[4];
    u16     rev_no;
    u16     features;
    u8      opt_cmds[3];
    u16     sec_cmds;
    u8		num_param_pgs;
    u8		resv[18];
}__attribute__((packed)) JEDEC_RevisionInfo;

/* Manufacturer info block */
typedef struct JEDEC_ManufactInfo {
    char    dev_manufacturer[12];
    char    dev_model[20];
    u8      jedec_id[6];
    u8      resv[10];
}__attribute__((packed)) JEDEC_ManufactInfo;

/* Memory Organization block */
typedef struct JEDEC_MemOrg{
    u32     databytes_per_pg;
    u16     sparebytes_per_pg;
    u32     databytes_per_partpg;
    u16     sparebytes_per_partpg;
    u32     pgs_per_blk;
    u32     blks_per_lun;
    u8		lun_count;
    u8		addr_cycles;
    u8		bits_per_cell;
	u8      programs_per_page;
	u8      multi_plane_addr;
	u8      multi_plane_op_attr;
	u8      reserved3[38];
}__attribute__((packed)) JEDEC_MemOrg;

/* Electrical parameters block */
typedef struct JEDEC_Electrical{
	u16     async_sdr_speed_grade;
	u16     ddr2_speed_grade;
	u16     sync_ddr_speed_grade;
	u8      async_sdr_features;
	u8      resv;
	u8      sync_ddr_features;
    u16     t_prog;
    u16     t_bers;
    u16     t_r;
    u16     t_r_multi_plane;
    u16     t_ccs;
    u16     io_pin_capac_typ;
    u16     inp_capac_typ;
    u16     clk_pin_capac_typ;
    u8		drvr_strgth_support;
    u16     t_adl;
	u8      resv1[36];
}__attribute__((packed)) JEDEC_Electrical;
                
/* ECC and endurance block */
typedef struct JEDEC_ECC_endurance {
	u8      guaranteed_valid_blocks;
	u16     guaranteed_block_endurance;
    u8      ecc_bits;
	u8      codeword_size;
	u16     badblk_per_lun;
	u16     blk_endurance;
	u8      resv[2];
	u8      resv1[53];
}__attribute__((packed)) JEDEC_ECC_endurance;

/* Vendor block and CRC for parameter page */
typedef struct JEDEC_Vendor {
    u16	    vendor_rev_num;
    u8		rd_retry_opts;
    u32	    avai_rd_retry_opts; 
    u8	    resv[83];
    u16	    crc;
}__attribute__((packed)) JEDEC_Vendor;

typedef struct JEDEC_Param_Pg {
    JEDEC_RevisionInfo    jedec_rev_info; /* Revision info and features block */
    JEDEC_ManufactInfo    jedec_manuf_info; /* Manufacturer info block */
    JEDEC_MemOrg          jedec_mem_org; /* Memory Organization block */
    JEDEC_Electrical      jedec_elec_param; /* Electrical parameters block */
    JEDEC_ECC_endurance   jedec_ecc_endurance; /* ECC and endurance block */
    u8                    resv[148]; 
    JEDEC_Vendor          jedec_vendor; /* Vendor block and CRC */
}__attribute__((packed)) JEDEC_Param_Pg; 

/* JEDEC parameter page data structure */
typedef struct JEDEC {
    JEDEC_Param_Pg jedec_param_pg;
    JEDEC_Param_Pg resv1[35];
}__attribute__((packed)) jedec; 
#endif

#endif /* __NAND_STRUCT_H */
