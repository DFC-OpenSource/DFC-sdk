/* 
 * PCINet Driver for Freescale LS2085 (Host side) 
 * 
 * This file is licensed under the terms of the GNU General Public License 
 * version 2. This program is licensed "as is" without any warranty of any 
 * kind, whether express or implied. 
 */ 

#include "config.h"
#include <linux/kernel.h> 
#include <linux/module.h> 
#include <linux/init.h> 
#include <linux/sched.h> 
#include <linux/wait.h> 
#include <linux/interrupt.h> 
#include <linux/irqreturn.h> 
#include <linux/pci.h> 
#include <linux/serial.h> 
#include <linux/serial_core.h> 
#include <linux/etherdevice.h> 
#include <linux/dma-mapping.h> 
#include <linux/mutex.h> 
#include <linux/delay.h> 
#include <linux/timer.h> 
#include <linux/kthread.h> 
#include <linux/jiffies.h> 
#include "pcinet.h" 
#include "ls2_eth.h" 
//#include "fsl_dpaa_fd.h"
#define  DPAA2_ETH_TX_QUEUES 8
unsigned long irq_offset=0x60040;
int i,err,tx_conf_count;
static const char driver_name[] = "fsl-dpaa22-ethernet-nic"; 
struct msix_entry *msix_entries;
struct wqt_dev; 
typedef void (*wqt_irqhandler_t)(struct wqt_dev *); 
static void wqt_rx_refill(unsigned long);
struct wqt_irqhandlers { 
	wqt_irqhandler_t net_start_req_handler; 
	wqt_irqhandler_t net_start_ack_handler; 
	wqt_irqhandler_t net_stop_req_handler; 
	wqt_irqhandler_t net_stop_ack_handler; 
	wqt_irqhandler_t net_rx_packet_handler; 
	wqt_irqhandler_t net_tx_complete_handler; 
}; 
#if FSLU_NVME_INIC_SG
/* TODO Assert it is smaller than DPAA2_ETH_SWA_SIZE */
struct dpaa2_eth_swa {
        struct sk_buff *skb;
        struct scatterlist *scl;
        int num_sg;
        int num_dma_bufs;
};
#endif
static void __iomem *g_immr, *g_netregs,*g_netregs_bkp;
void __iomem *dma_alloc_rx_addr;
void __iomem *dma_alloc_tx_addr;
struct circ_buf_desc __iomem *tx_conf; 
struct wqt_dev {
	/*--------------------------------------------------------------------*/
	/* PCI Infrastructure                                                 */
	/*--------------------------------------------------------------------*/
	struct pci_dev *pdev;
	struct device *dev;
	void __iomem *immr;

	int interrupt_count;
	int ethid;
	struct wqt_irqhandlers handlers;

	//Interup Related
	int flags;
	char rx_name[32];
	char tx_name[32];
	const char *name;

	/*--------------------------------------------------------------------*/ 
	/* Ethernet Device Infrastructure                                     */ 
	/*--------------------------------------------------------------------*/ 
	struct net_device *ndev;
	void __iomem *netregs;
	void __iomem *host_memory_reg;


	/* Circular Buffer Descriptor base */
	struct circ_buf_desc __iomem *rx_base;
	struct circ_buf_desc __iomem *tx_base;
	struct circ_buf_desc __iomem *tx_conf_base;
	struct refill_mem_pool __iomem *mem_pool_base;
	struct refill_mem_pool __iomem *mem_pool_cur;
	struct x86mem_flags __iomem *flags_ptr;

	/* Current SKB index */
	struct circ_buf_desc __iomem *cur_rx;
	struct circ_buf_desc __iomem *cur_tx;
	struct circ_buf_desc __iomem *cur_tx_conf;
	struct circ_buf_desc __iomem *cur_skb_free;
	struct circ_buf_desc __iomem *dirty_tx;
	struct buf_pool_desc __iomem *bman_buf;
	uint64_t tx_free;

	struct tasklet_struct tx_complete_tasklet;
	struct tasklet_struct rx_refill_tasklet;
	struct task_struct *poll_thread;
	spinlock_t net_lock;

	struct mutex net_mutex;
	int net_state;
	struct work_struct net_start_work;
	struct work_struct net_stop_work;
	struct completion net_start_completion;
	struct completion net_stop_completion;
	struct napi_struct napi;
	struct napi_struct napi_skb_free;
	struct timer_list timer;
	bool net_send_rx_packet_dbell;
	void __iomem *qbman_pool;
	uint64_t g_recv_count;
	struct work_struct refill_struct;
	volatile uint16_t wq_flag;
	uint64_t timeout;
	uint64_t write_count;
	uint16_t bpid;
	volatile uint16_t tx_free_flag;
	uint16_t tx_irq_gen;
	uint16_t tx_data_offset;
};

/*----------------------------------------------------------------------------*/
/* Buffer Descriptor Accessor Helpers                                         */
/*----------------------------------------------------------------------------*/

static inline void cbd_write(void __iomem *addr, u32 val)
{
    iowrite32(val, addr);
}

static inline u32 cbd_read(void __iomem *addr)
{
    return ioread32(addr);
}

static inline void cbd_write16(void __iomem *addr, u16 val)
{
    iowrite16(val, addr);
}

static inline u32 cbd_read16(void __iomem *addr)
{
    return ioread16(addr);
}

static inline void cbd_write8(void __iomem *addr, u8 val)
{
    iowrite8(val, addr);
}

static inline u32 cbd_read8(void __iomem *addr)
{
    return ioread8(addr);
}


/*----------------------------------------------------------------------------*/
/* Interrupt Handlers                                                         */
/*----------------------------------------------------------------------------*/

static inline void net_rx_packet_handler(struct wqt_dev *priv)
{
	napi_schedule(&priv->napi);
}
#if 0
static void net_rx_packet_handler_timer(unsigned long arg)
{
	struct wqt_dev *priv = (struct wqt_dev*) arg;
	net_rx_packet_handler(priv);
	net_tx_complete_handler(priv);
	if(netif_queue_stopped(priv->ndev))
		printk("tx queue stopped");
	mod_timer(&priv->timer, jiffies + HZ/100);
}
#endif

/*MSIX interrupt handler (same for all 4 MSIX interrupts, but irq, data are different for each net_device)*/
/*This is only used for physical layer is ready or not*/
static irqreturn_t wqt_intr_msix(int irq, void *data)
{
	struct wqt_dev *priv =(struct wqt_dev*)data;

	if(unlikely(priv->flags_ptr->link_state_update==LINK_STATUS))
	{
		priv->flags_ptr->link_state_update = 0x0;
		if(priv->flags_ptr->link_state== LINK_STATE_UP)
		{
			priv->flags_ptr->link_state=0x0; 
			netif_carrier_on(priv->ndev);
			netif_tx_start_all_queues(priv->ndev);
			//priv->net_state = NET_STATE_RUNNING;
			printk(" %s interface up\n",priv->ndev->name);
		}
		else if(priv->flags_ptr->link_state== LINK_STATE_DOWN)
		{
			priv->flags_ptr->link_state=0x0; 
			netif_tx_stop_all_queues(priv->ndev);
			netif_carrier_off(priv->ndev);
			printk(" %s interface down\n",priv->ndev->name);

		}
		return IRQ_HANDLED;
	}
	cbd_write(&priv->bman_buf->napi_msi, MSI_DISABLE);
#if FSLU_NVME_INIC_DPAA2
	if(likely(priv->wq_flag == 0x1))
	{ 
		priv->wq_flag =0;      
		tasklet_hi_schedule(&priv->rx_refill_tasklet);
	}
#endif
	net_rx_packet_handler(priv);
	return IRQ_HANDLED;
}


/*Generate doorbell interrupt on LS2 for first packet
FIXME: Doorbell is not working, so we will write to LS2 location
LS2 will do polling to know whether interrupt has occured or not*/
static inline void generate_irq_to_ls2(struct wqt_dev *priv) {


	if(priv->flags_ptr->tx_irq_flag==BD_IRQ_MASK)
	{
		return;    
	}
#if FSLU_NVME_INIC_FPGA_IRQ
	uint32_t reg=0;
	reg =ioread32(g_immr+0x3800);
	if(reg & (1<<priv->ethid))
	{
		return ;
	}
	reg = (reg | (1<<priv->ethid));
	iowrite32(reg,(g_immr+0x3800));
#endif
#if FSLU_NVME_INIC_QDMA_IRQ 
	iowrite32(0x201,(g_immr+0x390100));
#endif

	priv->flags_ptr->tx_irq_flag = BD_IRQ_MASK; 
}

/*Request irq and register irq handler for MSIX (We will get MSIX interrupt when we receive a packet)*/
/*Note:
On occurance of first MSIX interrupt, we will disable the IRQ and
run napi
and then enable the MSIX after napi is complete
*/

static int pcidma_request_msi(struct wqt_dev *priv)
{
	int err = 0, vector = priv->ethid;
	snprintf(priv->rx_name, sizeof(priv->rx_name) - 1,"freescale-eth%d", i);
	err = request_irq(msix_entries[vector].vector,wqt_intr_msix, 0, priv->rx_name, priv);
	if (err)
		return err;

	return 0;
}


/*----------------------------------------------------------------------------*/ 
/* Network Startup and Shutdown Helpers                                       */ 
/*----------------------------------------------------------------------------*/ 

/* NOTE: All helper functions prefixed with "do" must be called only from 
 * process context, with priv->net_mutex held. They are expected to sleep */ 

/* NOTE: queues must be stopped before initializing and uninitializing */ 

/*Called from wqt_open()*/
/*reset tx and rx info bases*/
static void do_net_initialize_board(struct wqt_dev *priv) 
{ 
	int i; 
	struct circ_buf_desc __iomem *bdp; 
	struct refill_mem_pool __iomem *mem_pool; 
	cbd_write(&priv->bman_buf->conf_irq,0x4);
	priv->timeout = 0;
	priv->write_count =0;
	priv->wq_flag = 0x1;
	priv->tx_free_flag =0x1;
	priv->tx_irq_gen = 0x0;  
#if FSLU_NVME_INIC_QDMA
        uint64_t addr=0;
        uint64_t *buf_ptr = NULL;
#endif
	/* Fill in RX ring */
	if(priv->ndev->stats.tx_packets==0)
	{
		priv->tx_free =0;  
#if FSLU_NVME_INIC_DPAA2
		for(i=0 , mem_pool = priv->mem_pool_base;i<30;mem_pool++,i++)
		{
			mem_pool->pool_flag = BD_MEM_FREE;    
		}
#endif	
	for (i = 0, bdp = priv->rx_base; i < RX_BD_RING_SIZE; bdp++, i++) { 
		bdp->len_offset_flag=BD_MEM_READY;
#if FSLU_NVME_INIC_QDMA
		buf_ptr = netdev_alloc_frag(DPAA2_ETH_BUF_RAW_SIZE);
		if (unlikely(!buf_ptr) ) {
			printk("buffer allocation failed, do it after some time\n");
			netif_stop_queue(priv->ndev);
			break;
		}
		buf_ptr = PTR_ALIGN(buf_ptr, DPAA2_ETH_RX_BUF_ALIGN);

                addr= pci_map_single(priv->pdev, buf_ptr, DPAA2_ETH_RX_BUFFER_SIZE, PCI_DMA_FROMDEVICE);
                if (!addr) {
                        printk("PCIE:%s pci_map_single() failed\n",__func__);
                }
                addr |= (uint32_t)(addr>>32);
		bdp->addr_lower=(uint32_t)addr;
                //printk("PCIE:%s rx_buf:-%p\n",__func__,addr);
		bdp->addr_higher=0x0;
#endif
#if FSLU_NVME_INIC_DPAA2
bdp->addr_lower=0x0;
bdp->addr_higher=0x0;
#endif
	} 
}
	/* Fill in TX ring */ 
	for (i = 0, bdp = priv->tx_base; i < PH_RING_SIZE; bdp++, i++) { 
		cbd_write(&bdp->len_offset_flag,BD_MEM_READY); 
		cbd_write(&bdp->addr_lower,0x0); 
		cbd_write(&bdp->addr_higher,0x0); 
	}

}


static void do_net_start_queues(struct wqt_dev *priv) 
{ 
	if (priv->net_state == NET_STATE_RUNNING) 
		return; 
	priv->cur_rx = priv->rx_base; 
	priv->cur_tx = priv->tx_base; 
	priv->cur_tx_conf = priv->tx_conf_base; 
	priv->cur_skb_free = priv->tx_base; 
	priv->dirty_tx = priv->tx_base; 
	napi_enable(&priv->napi);
	//netif_carrier_on(priv->ndev);
        //netif_tx_start_all_queues(priv->ndev);
	/* Enable the RX_PACKET and TX_COMPLETE interrupt handlers */ 
	priv->net_state = NET_STATE_RUNNING;
} 

static void do_net_stop_queues(struct wqt_dev *priv) 
{ 
	if (priv->net_state == NET_STATE_STOPPED) 
		return; 


	dev_dbg(priv->dev, "disabling NAPI queue\n"); 
	napi_disable(&priv->napi); 
	dev_dbg(priv->dev, "disabling TX queue\n"); 

	dev_dbg(priv->dev, "carrier off!\n"); 
	//netif_carrier_off(priv->ndev); 
	priv->net_state = NET_STATE_STOPPED; 
} 

/*----------------------------------------------------------------------------*/ 
/* Network Device Operations                                                  */ 
/*----------------------------------------------------------------------------*/ 

/*called when ifconfig up is done*/
static int wqt_open(struct net_device *dev)
{
	struct wqt_dev *priv = netdev_priv(dev);
	struct buf_pool_desc __iomem *bdp_iface;


	/*Stop all the traffic for the network interface and turn off carrier*/
	mutex_lock(&priv->net_mutex);
        netif_tx_stop_all_queues(priv->ndev);
	netif_carrier_off(dev);
	bdp_iface = priv->bman_buf;
	cbd_write8(&bdp_iface->iface_status, IFACE_UP); /*Inform interface status in LS2 by writing into LS2 memory and give doorbell to LS2*/

	/* Reset all tx and rx descriptors to initial conditions*/
	do_net_initialize_board(priv);
	/* Start the network queues */
	do_net_start_queues(priv);
	//mod_timer(&priv->timer, jiffies + HZ);
	mutex_unlock(&priv->net_mutex);
	generate_irq_to_ls2(priv); /*update net_device status on LS2*/
	/*wait for LS2 to make interface up in its linux*/
	while(cbd_read8(&bdp_iface->iface_status) != IFACE_READY)
	{
		msleep(500);
	}
	/*Rest the status flag in LS2 to idle*/
	cbd_write8(&bdp_iface->iface_status, IFACE_IDLE);

	return 0;
}

/*called when ifconfig down is called
	1) Disable network traffic
	2)wait for all the already sent packets to confirm transmit
	tasklet_hi_schedule(&priv->tx_complete_tasklet); will call tx_complete_tasklet
	which will clean the confirmation packets
	*/
static int wqt_stop(struct net_device *dev) 
{
	struct wqt_dev *priv = netdev_priv(dev); 
	struct buf_pool_desc __iomem *bdp_iface;
        int i;
        netif_tx_stop_all_queues(priv->ndev);
	bdp_iface = priv->bman_buf;
        if(priv->tx_free !=0)
        {
         for(i=0;i<200000;i++)
        {
	if(priv->tx_free != dev->stats.tx_packets)
	{
		if(likely(priv->tx_free_flag== 0x1))
		{
			priv->tx_free_flag= 0x4;
			tasklet_hi_schedule(&priv->tx_complete_tasklet);
		}
	}
        else
        {
        break;
        }
        }
        }
	cbd_write8(&bdp_iface->iface_status, IFACE_DOWN);
	generate_irq_to_ls2(priv);


	while(cbd_read8(&bdp_iface->iface_status) != IFACE_STOP) 
	{
		msleep(500);
	}
        
	cbd_write8(&bdp_iface->iface_status, IFACE_IDLE);

	mutex_lock(&priv->net_mutex);
	do_net_stop_queues(priv);

	//del_timer(&priv->timer);
	mutex_unlock(&priv->net_mutex);
	return 0; 
} 

static int wqt_change_mtu(struct net_device *dev, int new_mtu) 
{ 
	if ((new_mtu < 68) || (new_mtu > PH_MAX_MTU))
		return -EINVAL;

	dev->mtu = new_mtu;
	return 0;
}

#if FSLU_NVME_INIC_DPAA2
static void wqt_tx_complete(unsigned long data)
{

	struct wqt_dev *priv = (struct wqt_dev *)data; 
	struct net_device *dev = priv->ndev;
	struct circ_buf_desc __iomem *free_buf; 
	struct sk_buff **skbh, *skb_tx_conf;
	dma_addr_t paddr_tx_conf;
	struct dpaa2_fas *fas;
	uint32_t status = 0;
        uint64_t flag_addr;
#if FSLU_NVME_INIC_SG
	uint16_t sg=0;
        struct scatterlist *scl;
        int num_sg, num_dma_bufs;
        struct dpaa2_eth_swa *bps;

#endif
//	priv->tx_free_flag = 0x2;
	/*Chking for free packets*/
	free_buf = priv->cur_skb_free;
	while (1) { 
                flag_addr=readq(&free_buf->len_offset_flag);
		if ((( flag_addr & 0xf )!=BD_MEM_FREE)) {
			break; 
		}
                paddr_tx_conf = (uint32_t)(flag_addr>>32);
#if FSLU_NVME_INIC_SG
		sg= paddr_tx_conf & 0x10;
#endif 
        
		paddr_tx_conf |=((uint64_t)(paddr_tx_conf & 0x20)<<27);
		paddr_tx_conf = ((uint64_t)paddr_tx_conf & 0xffffffffffffffc0);
		skbh = phys_to_virt(paddr_tx_conf);
#if !(FSLU_NVME_INIC_SG)
		skb_tx_conf = *skbh;
                dma_unmap_single(&priv->pdev->dev,paddr_tx_conf,skb_tail_pointer(skb_tx_conf)-(unsigned char*)skbh,DMA_TO_DEVICE);
#endif
			fas = (struct dpaa2_fas *)
				((void *)skbh + DPAA2_ETH_SWA_SIZE ); /* priv->buf_layout.private_data_size set to DPAA2_ETH_SWA_SIZE in ls2*/
			status = le32_to_cpu(fas->status);
			if (status & DPAA2_ETH_TXCONF_ERR_MASK) {
				dev_err((const struct device *)dev, "TxConf frame error(s): 0x%08x\n",
						status & DPAA2_ETH_TXCONF_ERR_MASK);
				/* Tx-conf logically pertains to the egress path.
				 * TODO add some specific counters for tx-conf also. */
				dev->stats.tx_errors++;
			}
		if(unlikely(flag_addr & 0xf0))
		{ 
			dev->stats.tx_errors++;
		}	
#if FSLU_NVME_INIC_SG
		if(sg)
		{
                bps = (struct dpaa2_eth_swa *)skbh;
                skb_tx_conf = bps->skb;
                scl = bps->scl;
                 kfree(scl);
        	 kfree(skbh);

		}
		else
		{
			skb_tx_conf = *skbh;
		}
#endif
                writeq(BD_MEM_READY,&free_buf->len_offset_flag);
		priv->tx_free++;
	        dev->stats.tx_dropped++;
mb();
		dev_kfree_skb_irq(skb_tx_conf);
               
		if ((free_buf - priv->tx_base) == PH_RING_SIZE - 1) {
			free_buf = priv->tx_base;
		}
		else {

			free_buf++;
		}

	}


	priv->cur_skb_free = free_buf;

	//priv->tx_free_flag = 0x1;

	return;
}
#endif
#if FSLU_NVME_INIC_QDMA
static void wqt_tx_complete(unsigned long data_t)
{

	struct wqt_dev *priv = (struct wqt_dev *)data_t; 
	struct net_device *dev = priv->ndev;
	struct circ_buf_desc __iomem *free_buf; 
	dma_addr_t paddr_tx_conf;
	struct sk_buff **skbh, *skb_tx_conf;
	uint64_t flag_addr;
	unsigned char *data;
	priv->tx_free_flag = 0x2;

	/*Chking for free packets*/
	free_buf = priv->cur_skb_free;
	while (1) { 
		flag_addr=readq(&free_buf->len_offset_flag);
		if ((( flag_addr & 0xf )!=BD_MEM_FREE)) {
			break; 
		}

		paddr_tx_conf = (uint32_t)(flag_addr>>32);
		paddr_tx_conf |=((uint64_t)(paddr_tx_conf & 0x20)<<27);
		paddr_tx_conf = ((uint64_t)paddr_tx_conf & 0xffffffffffffffc0);
		skbh = phys_to_virt(paddr_tx_conf);
		skb_tx_conf=*skbh;
		writeq(BD_MEM_READY,&free_buf->len_offset_flag);
		priv->tx_free++;
		pci_unmap_single(priv->pdev,virt_to_phys(skb_tx_conf->data),skb_tx_conf->len,PCI_DMA_TODEVICE);
		//dev->stats.tx_dropped++;
		dev_kfree_skb_irq(skb_tx_conf);

		if ((free_buf - priv->tx_base) == PH_RING_SIZE - 1) {
			free_buf = priv->tx_base;
		}
		else {

			free_buf++;
		}

	}

	mb();
	priv->cur_skb_free = free_buf;

	priv->tx_free_flag = 0x1;

	return;
}
#endif
#if FSLU_NVME_INIC_DPAA2
static int wqt_hard_start_xmit(struct sk_buff *skb, struct net_device *dev) 
{ 
	struct wqt_dev *priv = netdev_priv(dev);
	struct circ_buf_desc __iomem *bdp;
	uint8_t *buffer_start;
	struct sk_buff **skbh;
	uint64_t addr;
	uint32_t len_offset_flag = 0;
	//uint64_t queue_mapping =0;
	int err;
	spin_lock(&priv->net_lock);
#if FSLU_NVME_INIC_SG
	struct scatterlist *scl, *crt_scl;
	int num_sg;
	int num_dma_bufs;
#endif
//	if(likely(priv->tx_free_flag== 0x1)) 
//	{ 
//		priv->tx_free_flag= 0x4;
		tasklet_hi_schedule(&priv->tx_complete_tasklet);
//	} 
	if(likely(priv->wq_flag == 0x1))
	{
		priv->wq_flag =0;
		tasklet_schedule(&priv->rx_refill_tasklet);
	}
	bdp = priv->cur_tx;
	/* This should not happen, the queue should be stopped */ 
	if (dev->stats.tx_packets-priv->tx_free >160000) {
		printk("PCIE XMIT flag %u free:%llu tx:- %lu :tx_cur %p free_cur %p :\n",cbd_read(&bdp->len_offset_flag),priv->tx_free,dev->stats.tx_packets,priv->cur_tx,priv->cur_skb_free);
		netif_tx_stop_all_queues(dev); 
		printk("XMIT NET QUEUE STOPPED \n");
		err=NETDEV_TX_OK;
		goto err_alloc_headroom;
	} 
	//queue_mapping = skb_get_queue_mapping(skb);
	if (skb_headroom(skb) < DPAA2_ETH_NEEDED_HEADROOM(priv->tx_data_offset)) {
		struct sk_buff *ns;
		/* ...Empty line to appease checkpatch... */
		ns = skb_realloc_headroom(skb, DPAA2_ETH_NEEDED_HEADROOM(priv->tx_data_offset));
		if (unlikely(!ns)) {
			dev->stats.tx_dropped++;
			err=NETDEV_TX_OK;
			goto err_alloc_headroom;
		}
		dev_kfree_skb(skb);
		skb = ns;
	}
	/* We'll be holding a back-reference to the skb until Tx Confirmation;
	 * we don't want that overwritten by a concurrent Tx with a cloned skb.
	 */
	skb = skb_unshare(skb, GFP_ATOMIC);
	if (unlikely(!skb)) {
		netdev_err(dev, "Out of memory for skb_unshare()");
		printk("out of memory for skb unshare\n");  
		/* skb_unshare() has already freed the skb */
		dev->stats.tx_dropped++;
		spin_unlock(&priv->net_lock);

		return NETDEV_TX_OK;
	}
#if FSLU_NVME_INIC_SG
	if (skb_is_nonlinear(skb)) {
		void *sgt_buf = NULL;
		skb_frag_t *frag;
		int nr_frags = skb_shinfo(skb)->nr_frags;
		struct dpaa_sg_entry *sgt;
		int sgt_buf_size;
		unsigned long paddr_page;
		int i;
		struct dpaa2_eth_swa *bps; 
		struct scatterlist *s;
		/* Create and map scatterlist.
		 * We don't advertise NETIF_F_FRAGLIST, so skb_to_sgvec() will not have
		 * to go beyond nr_frags+1.
		 * TODO We don't support chained scatterlists; we could just fallback
		 * to the old implementation.
		 */
		WARN_ON(PAGE_SIZE / sizeof(struct scatterlist) < nr_frags + 1);
		scl = kcalloc(nr_frags + 1, sizeof(struct scatterlist), GFP_ATOMIC);
		if (unlikely(!scl))
			return -ENOMEM;

		sg_init_table(scl, nr_frags + 1);
		num_sg = skb_to_sgvec(skb, scl, 0, skb->len);

		for_each_sg(scl, s, num_sg, i)
			s->dma_address = (page_to_phys(sg_page(s)) + s->offset)+0x1400000000;
		num_dma_bufs =num_sg; 
		/* Prepare the HW SGT structure */
		sgt_buf_size = 192 + sizeof(struct dpaa_sg_entry) * (1 + num_dma_bufs);
		sgt_buf = kzalloc(sgt_buf_size + DPAA2_ETH_TX_BUF_ALIGN, GFP_ATOMIC);
		if (unlikely(!sgt_buf)) {
			netdev_err(priv->ndev, "failed to allocate SGT buffer\n");
			err = -ENOMEM;
			goto sgt_buf_alloc_failed;
		}
		sgt_buf = PTR_ALIGN(sgt_buf, DPAA2_ETH_TX_BUF_ALIGN);

		/* PTA from egress side is passed as is to the confirmation side so
		 * we need to clear some fields here in order to find consistent values
		 * on TX confirmation. We are clearing FAS (Frame Annotation Status)
		 * field here.
		 */
		memset(sgt_buf +192, 0, 8);

		sgt = (struct dpaa_sg_entry *)(sgt_buf + 192);

		/* Fill in the HW SGT structure.
		 *
		 * sgt_buf is zeroed out, so the following fields are implicit
		 * in all sgt entries:
		 *   - offset is 0
		 *   - format is 'dpaa_sg_single'
		 */
		for_each_sg(scl, crt_scl, num_dma_bufs, i) {
			dpaa2_sg_set_addr(&sgt[i], sg_dma_address(crt_scl));
			dpaa2_sg_set_len(&sgt[i], sg_dma_len(crt_scl));
		}
		dpaa2_sg_set_final(&sgt[i-1], true);

		/* Store the skb backpointer in the SGT buffer.
		 * Fit the scatterlist and the number of buffers alongside the
		 * skb backpointer in the SWA. We'll need all of them on Tx Conf.
		 */
		bps = (struct dpaa2_eth_swa *)sgt_buf;
		bps->skb = skb;
		bps->scl = scl;
		bps->num_sg = num_sg;
		bps->num_dma_bufs = num_dma_bufs;
		addr=virt_to_phys(sgt_buf);
		len_offset_flag = (skb->len) << 16;
		len_offset_flag |= 192 << 4;
		len_offset_flag |= BD_MEM_DIRTY;
		addr |=(1<<4);
		addr |=((addr>>32)<<5);
		//addr |=queue_mapping;
		writeq(((uint64_t)addr<<32)|len_offset_flag,&bdp->len_offset_flag);

	}
	else{
#endif
		buffer_start = PTR_ALIGN(skb->data - priv->tx_data_offset - DPAA2_ETH_TX_BUF_ALIGN, DPAA2_ETH_TX_BUF_ALIGN);
		memset(buffer_start + DPAA2_ETH_SWA_SIZE , 0, 8);
		skbh = (struct sk_buff **)buffer_start;
		*skbh = skb;
                addr=dma_map_single(&priv->pdev->dev,buffer_start,skb_tail_pointer(skb)-(unsigned char*)buffer_start,DMA_TO_DEVICE);
		//addr = virt_to_phys(buffer_start);
		addr |= ((addr>>32)<<5);
		//addr |=queue_mapping; 
		len_offset_flag = (skb->len) << 16;
		len_offset_flag |= (skb->data - buffer_start) << 4;
		len_offset_flag |= BD_MEM_DIRTY;
		writeq(((uint64_t)addr<<32)|len_offset_flag,&bdp->len_offset_flag);
		//cbd_write(&bdp->addr_lower,addr); 
		//cbd_write(&bdp->len_offset_flag,len_offset_flag); 
		wmb();
#if FSLU_NVME_INIC_SG
	}
#endif
#if FSLU_NVME_INIC_POLL
	if( priv->tx_irq_gen == 0)
	{ 
		priv->tx_irq_gen =0x1;
#endif
		generate_irq_to_ls2(priv);
#if FSLU_NVME_INIC_POLL
	}
#endif
	dev->stats.tx_packets++;
	dev->stats.tx_bytes += skb->len;

	if ((bdp - priv->tx_base) == (PH_RING_SIZE - 1)) 
		bdp = priv->tx_base; 
	else 
		bdp++; 

	priv->cur_tx = bdp;
	mb();
	spin_unlock(&priv->net_lock);

	return NETDEV_TX_OK;
#if FSLU_NVME_INIC_SG
sgt_buf_alloc_failed:
	dma_unmap_sg(dev, scl, num_sg, DMA_TO_DEVICE);
dma_map_sg_failed:
	kfree(scl);
#endif
err_alloc_headroom:
	dev_kfree_skb(skb);
         spin_unlock(&priv->net_lock);

	return err;

}
#endif
#if FSLU_NVME_INIC_QDMA
static int wqt_hard_start_xmit(struct sk_buff *skb, struct net_device *dev) 
{ 
	struct wqt_dev *priv = netdev_priv(dev);
	struct circ_buf_desc __iomem *bdp;
	uint8_t *buffer_start;
	struct sk_buff **skbh;
	uint64_t addr,temp_addr;
	uint32_t len_offset_flag = 0;
	//uint64_t queue_mapping =0;
	int err;
	spin_lock(&priv->net_lock);
#if 1
	if(likely(priv->tx_free_flag== 0x1)) 
	{ 
		priv->tx_free_flag= 0x4;
		tasklet_hi_schedule(&priv->tx_complete_tasklet);
	}
#endif
#if 0 
	if(likely(priv->wq_flag == 0x1))
	{
		priv->wq_flag =0;
		tasklet_schedule(&priv->rx_refill_tasklet);
	}
#endif
	bdp = priv->cur_tx;
	/* This should not happen, the queue should be stopped */ 
	if (dev->stats.tx_packets-priv->tx_free >16000) {
		printk("PCIE XMIT flag %p free:%llu tx:- %llu :tx_cur %p free_cur %p :\n",cbd_read(&bdp->len_offset_flag),priv->tx_free,dev->stats.tx_packets,priv->cur_tx,priv->cur_skb_free);
		netif_tx_stop_all_queues(dev); 
		printk("XMIT NET QUEUE STOPPED \n");
		err=NETDEV_TX_OK;
		goto err_alloc_headroom;
	} 
	//queue_mapping = skb_get_queue_mapping(skb);
        if (skb_headroom(skb) < DPAA2_ETH_NEEDED_HEADROOM(priv->tx_data_offset)) {
                struct sk_buff *ns;
                /* ...Empty line to appease checkpatch... */
                ns = skb_realloc_headroom(skb, DPAA2_ETH_NEEDED_HEADROOM(priv->tx_data_offset));
                if (unlikely(!ns)) {
                        printk("%p : tx_dropped (skb_realloc_headroom : fail) = %d\n",skb,dev->stats.tx_dropped);
                        dev->stats.tx_dropped++;
                        goto err_alloc_headroom;
                        //return 0;
                }
                dev_kfree_skb(skb);
                skb = ns;
        }

        /* We'll be holding a back-reference to the skb until Tx Confirmation;
         * we don't want that overwritten by a concurrent Tx with a cloned skb.
         */

  
	skb = skb_unshare(skb, GFP_ATOMIC);
	if (unlikely(!skb)) {
		netdev_err(dev, "Out of memory for skb_unshare()");
		printk("out of memory for skb unshare\n");  
		/* skb_unshare() has already freed the skb */
		dev->stats.tx_dropped++;
		spin_unlock(&priv->net_lock);

		return NETDEV_TX_OK;
	}
        buffer_start = PTR_ALIGN(skb->data - priv->tx_data_offset - DPAA2_ETH_TX_BUF_ALIGN, DPAA2_ETH_TX_BUF_ALIGN);
        memset(buffer_start, 0, 16);
        skbh = (struct sk_buff **)buffer_start;
        *skbh = skb;
        temp_addr=pci_map_single(priv->pdev,skb->data,skb->len,PCI_DMA_TODEVICE);
        if(!temp_addr)
        {
         printk("PCIE:%s pci_map_error \n",__func__);
         goto err_alloc_headroom;
        }
	addr = virt_to_phys(buffer_start);
	addr |= ((addr>>32)<<5);
	//addr |=queue_mapping; 
	len_offset_flag = (skb->len) << 16;
        len_offset_flag |= (skb->data - buffer_start) << 4;
	len_offset_flag |= BD_MEM_DIRTY;
	writeq(((uint64_t)addr<<32)|len_offset_flag,&bdp->len_offset_flag);
	wmb();
#if FSLU_NVME_INIC_POLL
	if( priv->tx_irq_gen == 0)
	{ 
		priv->tx_irq_gen =0x1;
#endif
		generate_irq_to_ls2(priv);
#if FSLU_NVME_INIC_POLL
	}
#endif
	dev->stats.tx_packets++;
	dev->stats.tx_bytes += skb->len;

	if ((bdp - priv->tx_base) == (PH_RING_SIZE - 1)) 
		bdp = priv->tx_base; 
	else 
		bdp++; 

	priv->cur_tx = bdp;
	mb();
	spin_unlock(&priv->net_lock);

	return NETDEV_TX_OK;
err_alloc_headroom:
	dev_kfree_skb(skb);
	spin_unlock(&priv->net_lock);

	return err;

}
#endif

static void wqt_tx_timeout(struct net_device *dev) 
{ 
	struct wqt_dev *priv = netdev_priv(dev); 
	printk(" %s timeout \n",dev->name);
	if(likely(priv->tx_free_flag== 0x1)) 
	{ 
		priv->tx_free_flag= 0x4;
		tasklet_hi_schedule(&priv->tx_complete_tasklet);
	} 

	dev->stats.tx_errors++; 
} 

static void dpaa2_bp_add_7(struct wqt_dev *priv, uint64_t *buf_array, uint32_t count )
{

	struct buf_pool_desc __iomem *bdp; 
	dma_addr_t pbufAddr ;
	bdp = priv->bman_buf;

	pbufAddr = virt_to_phys(buf_array);

	cbd_write(&bdp->pool_len, count);
	cbd_write(&bdp->pool_addr, pbufAddr);
	cbd_write(&bdp->pool_set, BD_MEM_READY);
}


static struct sk_buff *dpaa2_eth_build_linear_skb(void *fd_vaddr,uint16_t fd_offset,uint32_t fd_length)
{
	struct sk_buff *skb = NULL;

	skb = build_skb(fd_vaddr, DPAA2_ETH_RX_BUFFER_SIZE + SKB_DATA_ALIGN(sizeof(struct skb_shared_info)));
	if (unlikely(!skb)) {
		printk( "build_skb() failed\n");
		return NULL;
	}
	skb_reserve(skb, fd_offset);
	skb_put(skb, fd_length);

	return skb;
}
static void wqt_rx_refill(unsigned long data)
{
	int  i,j=0,index=0 ;
	uint32_t *buf_pool_save = NULL;
	struct wqt_dev *priv =(struct wqt_dev *)data;
	struct refill_mem_pool  __iomem *mem_pool;
	uint64_t addr; 
	uint64_t *buf_ptr = NULL;
	mem_pool =priv->mem_pool_cur;
        /*we should not stuck in refill for long time although there is requirment bcz there will be case rx
        descriptors overflow if we don't process them thats wy 35 slight higher than RING 30 */
	while(j++<35)
	{
		if(mem_pool->pool_flag==BD_MEM_READY)
		{
			break;

		} 
                index = (mem_pool - priv->mem_pool_base);
                /*just for better space between different banks taken uint64_t* increment actual 32 bit enough*/
		buf_pool_save = (uint32_t *)(((uint64_t *)priv->qbman_pool) + (index * REFILL_POOL_SIZE)) ;
		mem_pool->pool_addr=virt_to_phys(buf_pool_save);

		for (i = 0; i < REFILL_POOL_SIZE; i++) {
			buf_ptr = netdev_alloc_frag(DPAA2_ETH_BUF_RAW_SIZE);
			if (unlikely(!buf_ptr) ) {
				printk("buffer allocation failed, do it after some time\n");
				netif_tx_stop_all_queues(priv->ndev);
				break;
			}
			buf_ptr = PTR_ALIGN(buf_ptr, DPAA2_ETH_RX_BUF_ALIGN);

			addr = virt_to_phys(buf_ptr);
			addr |= addr>>32;
			*buf_pool_save = addr;
			buf_pool_save++;
		}
		mem_pool->pool_flag=BD_MEM_READY;
		if(index == (REFILL_RING_SIZE-1) )
			mem_pool = priv->mem_pool_base;
		else
			mem_pool++;
	} 
	priv->mem_pool_cur=mem_pool;
	priv->wq_flag = 0x1;

}
#if FSLU_NVME_INIC_DPAA2
static void free_refill_rx_buffers(struct wqt_dev *priv)
{
	int  i,j=0,index=0,count=0 ;
	uint32_t *buf_pool_save = NULL;
	struct refill_mem_pool  __iomem *mem_pool;
	uint64_t addr; 
	mem_pool =priv->mem_pool_base;
	while(j++<REFILL_RING_SIZE)
	{
		if(mem_pool->pool_flag==BD_MEM_READY)
		{
			index = (mem_pool - priv->mem_pool_base);
                        buf_pool_save = (uint32_t *)(((uint64_t *)priv->qbman_pool) + (index * REFILL_POOL_SIZE)) ;


			for (i = 0; i < REFILL_POOL_SIZE; i++) {
				addr=*buf_pool_save;
				addr |=((uint64_t)(addr&0xf)<<32);
				addr =(addr & 0xfffffffffffffff0);
				put_page(virt_to_head_page(phys_to_virt(addr))); 
				buf_pool_save++;
                                count++;   
			}
		}
		if(index == (REFILL_RING_SIZE-1) )
			mem_pool = priv->mem_pool_base;
		else
			mem_pool++;
	} 

}
static void free_bp_rx_buffers(struct wqt_dev *priv)
{
	uint32_t *buf_pool_save = NULL;
	uint64_t addr; 
	int count;
	struct buf_pool_desc __iomem *bdp; 

	bdp = priv->bman_buf;
	count=cbd_read(&bdp->pool_len);
	buf_pool_save = priv->qbman_pool;
	while(count--)
	{


		addr=*buf_pool_save;
		addr |=((uint64_t)(addr&0xf)<<32);
		addr =(addr & 0xfffffffffffffff0);
		put_page(virt_to_head_page(phys_to_virt(addr))); 
		buf_pool_save++;

	} 
}
#endif
/* Free a received FD.
 * Not to be used for Tx conf FDs or on any other paths.
 */
static inline void dpaa2_eth_free_rx_fd(void *vaddr)
{
    put_page(virt_to_head_page(vaddr));
}
#if FSLU_NVME_INIC_QDMA
static void free_rx_buffers(struct wqt_dev *priv)
{
        struct circ_buf_desc __iomem *bdp;
       	uint64_t paddr;
        int i;
	for (i = 0, bdp = priv->rx_base; i < RX_BD_RING_SIZE; bdp++, i++) { 
		if((bdp->len_offset_flag&0xf)==BD_MEM_READY)
		{	
			paddr = bdp->addr_lower;
			paddr |= ((uint64_t)(bdp->addr_lower&0xf)<<32) ;
			paddr = ((uint64_t)paddr& 0xfffffffffffffff0) ;
			pci_unmap_single(priv->pdev, paddr, DPAA2_ETH_RX_BUFFER_SIZE, PCI_DMA_FROMDEVICE);
			dpaa2_eth_free_rx_fd(phys_to_virt(paddr));         
			bdp->len_offset_flag=BD_MEM_FREE;        
		}
		else{
			if((bdp->len_offset_flag&0xf)==BD_MEM_DIRTY)
				printk("PCIE:%s rx packet descriptor is left without process\n",__func__);
		}


	}
}
#endif


#if FSLU_NVME_INIC_DPAA2
static int wqt_rx_napi(struct napi_struct *napi, int budget) 
{ 
	struct wqt_dev *priv = container_of(napi, struct wqt_dev, napi); 
	struct net_device *dev = priv->ndev; 
	void *vaddr = NULL;
	int received = 0;
	uint32_t status = 0;
	uint32_t len_offset_flag = 0;
	struct sk_buff *skb = NULL;
	struct circ_buf_desc __iomem *bdp = NULL;
	uint64_t paddr;
	struct dpaa2_fas *fas = NULL;
	bdp = priv->cur_rx;

	while (received < budget) {


		if(likely(priv->wq_flag == 0x1))
		{ 
			priv->wq_flag =0;      
			tasklet_hi_schedule(&priv->rx_refill_tasklet);
		}
		len_offset_flag = bdp->len_offset_flag;  
		/* Check if we are out of buffers to process */ 
		if ((len_offset_flag & 0xf) != BD_MEM_DIRTY) {
			break; 
		}

		paddr = bdp->addr_lower;
		paddr |= ((uint64_t)(bdp->addr_lower&0xf)<<32) ;
		paddr = ((uint64_t)paddr& 0xfffffffffffffff0) ;
		bdp->len_offset_flag = BD_MEM_READY;
		vaddr = phys_to_virt(paddr);
		fas = (struct dpaa2_fas *)
			(vaddr +  DPAA2_ETH_SWA_SIZE ); /* priv->buf_layout.private_data_size set to DPAA2_ETH_SWA_SIZE */
		status = le32_to_cpu(fas->status);
		if (status & DPAA2_ETH_RX_ERR_MASK) {
			printk("%s Rx frame error(s) - 0x%08x\n", __func__, status & DPAA2_ETH_RX_ERR_MASK);
			/* TODO when we grow up and get to run in Rx softirq,
			 * we won't need this. Besides, on RT we'd only need
			 * migrate_disable().
			 */
			dev->stats.rx_errors++;
			dpaa2_eth_free_rx_fd(vaddr);
			goto free_fd; //return;
		} else if (status & DPAA2_ETH_RX_UNSUPP_MASK) {
			printk("PCIE:%s UNSUP MASK \n",__func__);
			/* TODO safety net; to be removed as we support more and
			 * more of these, e.g. rx multicast
			 */
			printk("%s Unsupported feature in bitmask - 0x%08x\n", __func__, status & DPAA2_ETH_RX_UNSUPP_MASK);
		}

		skb = dpaa2_eth_build_linear_skb(vaddr,(uint16_t )( (len_offset_flag >> 4) & 0x0FFF),(len_offset_flag>>16));

		if (unlikely(!skb)) {
			netdev_err(priv->ndev, "error building skb\n");
			dpaa2_eth_free_rx_fd(vaddr);
			dev->stats.rx_dropped++;
			printk("PCIE: error in building skb \n");
			goto free_fd; //return 0;
		}
		skb->protocol = eth_type_trans(skb, priv->ndev);
		skb->ip_summed = CHECKSUM_UNNECESSARY;
		prefetch(skb->data);
		//dev->stats.rx_bytes += (len_offset_flag>>16);
		dev->stats.rx_bytes +=skb->len;
		dev->stats.rx_packets++;

#if 0 
		if (dev->features & NETIF_F_GRO)
			napi_gro_receive(napi, skb);
		else
			if(netif_receive_skb(skb)== NET_RX_DROP){
				printk("NET_RX_DROP \n");
				goto free_fd;
			}
#endif
		if (unlikely(netif_receive_skb(skb) == NET_RX_DROP)) {
			/* Nothing to do here, the stack updates the dropped counter */
			printk("NET_RX_DROP \n");
			goto free_fd;
		}
free_fd:
		received++;

		/* Update the bdp */ 
		if ((bdp - priv->rx_base) == (RX_BD_RING_SIZE - 1)) {
			bdp = priv->rx_base; 
		} else {
			bdp++;
		}
	}

	priv->cur_rx = bdp; 

	if (received < budget) { 
		/* Enable MSI */
		napi_complete(napi);
		cbd_write(&priv->bman_buf->napi_msi, MSI_ENABLE);
	}
	return received; 
}
#endif 
#if FSLU_NVME_INIC_QDMA
static int wqt_rx_napi(struct napi_struct *napi, int budget) 
{
	struct wqt_dev *priv = container_of(napi, struct wqt_dev, napi); 
	struct net_device *dev = priv->ndev; 
	void *vaddr = NULL;
	int received = 0;
	uint32_t status = 0;
	uint32_t len_offset_flag = 0;
	struct sk_buff *skb = NULL;
	struct circ_buf_desc __iomem *bdp = NULL;
	uint64_t paddr;
	struct dpaa2_fas *fas = NULL;
	bdp = priv->cur_rx;
        int len=0,offset=0;
        uint64_t alloc_addr;

	while (received < budget) {

		len_offset_flag = bdp->len_offset_flag;  
		/* Check if we are out of buffers to process */ 
		if ((len_offset_flag & 0xf) != BD_MEM_DIRTY) {
			break; 
		}

                offset=(uint16_t )( (len_offset_flag >> 4) & 0x0FFF);
                len=(len_offset_flag>>16);
		paddr = bdp->addr_lower;
		paddr |= ((uint64_t)(bdp->addr_lower&0xf)<<32) ;
		paddr = ((uint64_t)paddr& 0xfffffffffffffff0) ;
                pci_unmap_single(priv->pdev, paddr, DPAA2_ETH_RX_BUFFER_SIZE, PCI_DMA_FROMDEVICE);
		vaddr = phys_to_virt(paddr);

/*refill buffer for next time */
		alloc_addr = netdev_alloc_frag(DPAA2_ETH_BUF_RAW_SIZE);
		if (unlikely(!alloc_addr) ) {
			printk("PCIE %s buffer allocation failed, do it after some time\n",__func__);
			netif_tx_stop_all_queues(priv->ndev);
			break;
		}
		alloc_addr = PTR_ALIGN(alloc_addr, DPAA2_ETH_RX_BUF_ALIGN);
                alloc_addr = pci_map_single(priv->pdev, alloc_addr, DPAA2_ETH_RX_BUFFER_SIZE, PCI_DMA_FROMDEVICE);
                if (!alloc_addr) {
                        printk("PCIE:%s pci_map_single() failed\n",__func__);
			netif_tx_stop_all_queues(priv->ndev);
                }
		alloc_addr |= (uint32_t)(alloc_addr>>32);
		bdp->addr_lower = (uint32_t)alloc_addr;
		bdp->len_offset_flag = BD_MEM_READY;
/*done*/

#if 1
		fas = (struct dpaa2_fas *)
			(vaddr +  DPAA2_ETH_SWA_SIZE ); /* priv->buf_layout.private_data_size set to DPAA2_ETH_SWA_SIZE */
		status = le32_to_cpu(fas->status);
		if (status & DPAA2_ETH_RX_ERR_MASK) {
			printk("%s Rx frame error(s) - 0x%08x\n", __func__, status & DPAA2_ETH_RX_ERR_MASK);
			/* TODO when we grow up and get to run in Rx softirq,
			 * we won't need this. Besides, on RT we'd only need
			 * migrate_disable().
			 */
			dev->stats.rx_errors++;
			dpaa2_eth_free_rx_fd(vaddr);
			goto free_fd; //return;
		} else if (status & DPAA2_ETH_RX_UNSUPP_MASK) {
			printk("PCIE:%s UNSUP MASK \n",__func__);
			/* TODO safety net; to be removed as we support more and
			 * more of these, e.g. rx multicast
			 */
			printk("%s Unsupported feature in bitmask - 0x%08x\n", __func__, status & DPAA2_ETH_RX_UNSUPP_MASK);
		}
#endif
		skb = dpaa2_eth_build_linear_skb(vaddr,offset,len);
		if (unlikely(!skb)) {
			netdev_err(priv->ndev, "error building skb\n");
			dpaa2_eth_free_rx_fd(vaddr);
			dev->stats.rx_dropped++;
			printk("PCIE: error in building skb \n");
			goto free_fd; //return 0;
		}
		skb->protocol = eth_type_trans(skb, priv->ndev);
		skb->ip_summed = CHECKSUM_UNNECESSARY;
		prefetch(skb->data);
		dev->stats.rx_bytes +=skb->len;
		dev->stats.rx_packets++;
 #if 0 
if (dev->features & NETIF_F_GRO)
                napi_gro_receive(napi, skb);
        else
                if(netif_receive_skb(skb)== NET_RX_DROP){
                 printk("NET_RX_DROP \n");
                goto free_fd;
                  }
#endif
       		if (unlikely(netif_receive_skb(skb) == NET_RX_DROP)) {
			/* Nothing to do here, the stack updates the dropped counter */
			printk("NET_RX_DROP \n");
			goto free_fd;
		}
		free_fd:
		received++;

		/* Update the bdp */ 
		if ((bdp - priv->rx_base) == (RX_BD_RING_SIZE - 1)) {
			bdp = priv->rx_base; 
		} else {
			bdp++;
		}
	}

	priv->cur_rx = bdp; 

	if (received < budget) { 
		/* Enable MSI */
		napi_complete(napi);
		cbd_write(&priv->bman_buf->napi_msi, MSI_ENABLE);
	}
	return received; 
} 
#endif
#if 0
static inline u16 dpaa2_eth_select_queue(struct net_device *dev,
                                         struct sk_buff *skb,
                                         void *accel_priv,
                                         select_queue_fallback_t fallback)
{
        if (likely(!preemptible()))
                return smp_processor_id();

        return skb_get_rxhash(skb) % dev->real_num_tx_queues;
}
#endif
static const struct net_device_ops wqt_net_ops = { 
    .ndo_open	 = wqt_open, 
    .ndo_stop	 = wqt_stop, 
    .ndo_change_mtu	 = wqt_change_mtu, 
    .ndo_start_xmit	 = wqt_hard_start_xmit, 
    .ndo_tx_timeout	 = wqt_tx_timeout, 
    //.ndo_select_queue = dpaa2_eth_select_queue,
}; 

/*----------------------------------------------------------------------------*/ 
/* PCI Subsystem                                                              */ 
/*----------------------------------------------------------------------------*/ 
#define NUM_NET_DEV 2
static struct net_device *wqt_net_devices[NUM_NET_DEV];
static struct wqt_dev *g_priv[NUM_NET_DEV];

/*PCI device probe
1) Handshake with LS2 using inbound and outbound regions of LS2
2) Register netdev devices to net core
*/
static int wqt_probe(struct pci_dev *dev, const struct pci_device_id *id) 
{ 
	struct net_device *ndev;
	struct wqt_dev *priv;
	int ret;
	dma_addr_t phys_addr,phys_addr1;
	struct buf_pool_desc __iomem *bdp_ptr;
	int phys_len;
	int i,num_vectors;
	int j;


	/* Hardware Initialization */ 
	/*pci core provides dev*/
	ret = pci_enable_device(dev);
	if (ret)
		goto out_pci_enable_dev;

	/*Set X86 as host (root complex) for PCI device dev (PEX3 of LS2)*/
	pci_set_master(dev);


	/*Finding MSIX capability*/
	/**/
	if (pci_find_capability(dev, PCI_CAP_ID_MSIX)) {  
		num_vectors = NUM_NET_DEV; 
		msix_entries =kcalloc(num_vectors,sizeof(struct msix_entry),GFP_KERNEL);
		if (msix_entries) {
			for (i = 0; i < num_vectors; i++)
				msix_entries[i].entry = i;
			/*Enable one MSIX interrupt for each network device (one MSIX for one devicre, total 4)*/
			err = pci_enable_msix(dev,msix_entries,num_vectors);
			if(err!=0)
			{                  
				ret=err;
				goto err_enable_msi;
			}
		}
		else 
		{
			ret = -ENOMEM;
			goto err_calloc;  
		}
	}
	/*Request PCI regions of LS2
	Before accessing PCI device memory,we have to 
	make a request, if request is success , then regions can be accessed*/
	ret = pci_request_regions(dev, driver_name);
	if (ret) {
		goto out_pci_request_regions;
	}

	/*Get the physical address of BAR0 of LS2 in x86 memory map*/
	phys_addr1=pci_resource_start(dev,0);

	/*remap BAR0 to kernel virtual address space*/
	g_immr = pci_ioremap_bar(dev, 0);
	if (!g_immr) {
		ret = -ENOMEM;
		goto out_ioremap_immr;
	}
	/*BAR1  is reserved for MSIX*/
	/*Get the physical address of BAR2 of LS2 in x86 memory map*/
	phys_addr=pci_resource_start(dev,2);
	/*Length is for checking only*/
	phys_len= pci_resource_len(dev,2);
	
	/*remap BAR2 to kernel virtual address space*/
	g_netregs = pci_ioremap_bar(dev, 2);

	if (!g_netregs) {
		ret = -ENOMEM;
		goto out_ioremap_netregs;
	}

	/*TODO*/
	if (!dma_set_mask(&dev->dev,DMA_BIT_MASK(64))) {
		printk("PCIE:Unable to set 64 bit DMA mask try 32\n");
	}
	else if(!dma_set_mask(&dev->dev,DMA_BIT_MASK(32)))
	{
		printk("PCIE:Unable to set 32 bit DMA mask \n");
	}
	else{
		printk("PCIE:dma set mask fail\n");
		ret = -ENODEV;
		goto out_set_dma_mask;
	}
	g_netregs_bkp = g_netregs;

	/*pci_set_drvdata() is used to store wqt_net_devices , used at module exit */
	pci_set_drvdata(dev, wqt_net_devices);
	for ( i = 0 ; i < NUM_NET_DEV; i++) {
		uint32_t *buf_pool_save;
		uint64_t  *buf;
		dma_addr_t addr;
		/*allocate memory for struct net_device, mq: Number of Queues*/
		//ndev = alloc_etherdev_mq(sizeof(*priv), DPAA2_ETH_TX_QUEUES);
		/*For single Queue net device allocation*/
		ndev=alloc_etherdev(sizeof(*priv));
                if (!ndev) {
			ret = -ENOMEM;
			goto out_alloc_ndev;
		}
                SET_NETDEV_DEV(ndev, &dev->dev);
		priv = netdev_priv(ndev);
		priv->pdev = dev; /*acess pci_device from private structure*/
		priv->dev = &dev->dev; /**/
		priv->ndev = ndev;  /*acess net_device from private structure*/
		priv->immr = g_immr; /*Outbound for X86 (BAR0)*/
		priv->netregs =(void*)((unsigned long)g_netregs + (i*INTERFACE_REG));  /*BAR2 , X86 Outbound*/
		priv->ethid = i; /*give an id*/

                /*use GFP_DMA is must here*/
		dma_alloc_rx_addr  = kzalloc((2*1024*1024),GFP_KERNEL|GFP_DMA); /*Allocate Space for flags RX descriptors (using 2MB space by partitioning*/
		if (!dma_alloc_rx_addr) {
			printk("PCIE:rx_desc failed \n");
			ret = -ENOMEM;
			goto out_no_mem;
		}
                priv->host_memory_reg=dma_alloc_rx_addr;
 		dma_alloc_rx_addr = PTR_ALIGN(dma_alloc_rx_addr,0x40);
		printk("PCIE:x86 desc base address %p \n",(void *)virt_to_phys(dma_alloc_rx_addr));
		priv->flags_ptr = dma_alloc_rx_addr; /*location of flags, using 64 bytes from that location*/
		priv->mem_pool_base = (void __iomem*)((unsigned long)dma_alloc_rx_addr+0x40);   /*Used for refilling RX buffers (from offset 0x40 to 0x3ff)*/
		priv->mem_pool_cur = priv->mem_pool_base; /*mem_pool_cur is current location to be used for refill*/
		priv->rx_base =(void __iomem*)((unsigned long)dma_alloc_rx_addr + 0x400);  /*rx descriptors base address*/
		priv->bman_buf = priv->netregs ; /*bman_buf is base address for Tx info (flags and descriptors)*/ 
                #if 0  /*Mostly to be removed*/
                dma_addr_t dma_handle=0;  
		dma_alloc_tx_addr  = dma_zalloc_coherent(0,(3*1024*1024),&dma_handle,GFP_KERNEL);
		if (!dma_alloc_tx_addr) {
			printk("PCIE:tx_desc failed \n");
			ret = -ENOMEM;
			goto out_no_mem;
		}
		printk("PCIE:x86 tx_desc base address %p dma_handle:-%p \n",virt_to_phys(dma_alloc_tx_addr),dma_handle);
		priv->tx_conf_base = ((unsigned long)dma_alloc_tx_addr + 0x400);
		cbd_write(&priv->bman_buf->tx_conf_desc_base,(uint32_t)dma_handle);
                #endif 
		priv->tx_base = priv->netregs + PCINET_TXBD_BASE; /*Base address for TX descriptors*/
		bdp_ptr=priv->bman_buf; /*store tx info location in local variable bdp_ptr for easy programming below*/
		cbd_write(&priv->bman_buf->rx_desc_base,virt_to_phys(dma_alloc_rx_addr)); /*Equivalent to priv->bman_buf->rx_desc_base = dma_alloc_rx_addr ,inside tx info (present in LS2 memory), writing rx info address  */
#if FSLU_NVME_INIC_DPAA2
		priv->qbman_pool  = kmalloc( BUFF_POOL_SIZE * sizeof(uint32_t)  , GFP_KERNEL|GFP_DMA); /*Allocate space for array of buffer pointers*/
		if (!priv->qbman_pool) {
			ret = -ENOMEM;
			goto out_no_mem;
		}
		buf_pool_save = (uint32_t *)priv->qbman_pool; /*buf_pool_save  is local variable for easy coding*/
		/*For each rx buffer pointer, allocate a buffer*/
		for (j = 0; j < BUFF_POOL_SIZE; j++) {
			/* Allocate buffer visible to WRIOP + skb shared info +
			 * alignment padding
			 */
			buf = netdev_alloc_frag(DPAA2_ETH_BUF_RAW_SIZE);
			if (unlikely(!buf)) {
				printk("buffer allocation failed\n");
				goto out_no_mem;
			}
			buf = PTR_ALIGN(buf,DPAA2_ETH_RX_BUF_ALIGN);
			addr = virt_to_phys(buf);
			addr |= addr>>32;
			*buf_pool_save = addr;
			buf_pool_save++;
		}

		/*Share all rx buffer pointers to LS2 DPAA, dpaa2_bp_add_7, does not mean seven */
		/*REAME TODO : Buffer size is static  which is based on MTU */
			/*This will release rx buffers to dpaa2*/
		dpaa2_bp_add_7(priv,(uint64_t *)priv->qbman_pool, BUFF_POOL_SIZE);
#endif	

		
		tasklet_init(&priv->tx_complete_tasklet, wqt_tx_complete,(unsigned long)priv); /*Initialize tx_conf tasklet*/
		tasklet_init(&priv->rx_refill_tasklet, wqt_rx_refill,(unsigned long)priv); /*Initialize rx_buffer_refill tasklet*/
		tasklet_disable(&priv->tx_complete_tasklet); /*Just disable tasklets, later we will enable it*/
		tasklet_disable(&priv->rx_refill_tasklet);
		spin_lock_init(&priv->net_lock);

		mutex_init(&priv->net_mutex); /*Used for net_device up and down purpose*/
		priv->net_state = NET_STATE_STOPPED; /*Initiallt net device status is stopped*/
                printk("getting mac address from ls2\n");
		/*Getting MAC address from LS2*/
		while(cbd_read8(&bdp_ptr->mac_id[6]) != MAC_SET);
		ndev->dev_addr[0] = cbd_read8(&bdp_ptr->mac_id[0]);
		ndev->dev_addr[1] = cbd_read8(&bdp_ptr->mac_id[1]);
		ndev->dev_addr[2] = cbd_read8(&bdp_ptr->mac_id[2]);
		ndev->dev_addr[3] = cbd_read8(&bdp_ptr->mac_id[3]);
		ndev->dev_addr[4] = cbd_read8(&bdp_ptr->mac_id[4]);
		ndev->dev_addr[5] = cbd_read8(&bdp_ptr->mac_id[5]);

		/*Getting buffer pool id , which is filled in LS2 linux probe function*/
		priv->bpid = cbd_read16(&bdp_ptr->bpid);
		/*Getting device id , which is filled in LS2 linux probe function*/
		snprintf(ndev->name, IFNAMSIZ, "ni%d", cbd_read16(&bdp_ptr->obj_id)); /*Names on LS2 and X86 should match (Just of convinience) */
		/*If name is not valid, then network stack will provide some name*/
		if (!dev_valid_name(ndev->name)) {
			dev_warn(&ndev->dev,
					"netdevice name \"%s\" cannot be used, reverting to default..\n",
					ndev->name);

			dev_alloc_name(ndev, "eth%d");
			printk("PCIE:netdev name setting to default \n");
			dev_warn(&ndev->dev, "using name \"%s\"\n", ndev->name);
		}
		/*store net_device operations into net_device*/
		ndev->netdev_ops        = &wqt_net_ops;
		/*Watchhdog reset timer*/
		ndev->watchdog_timeo    = HZ / 4;
		/*We do not support multicast*/
		ndev->flags            &= ~IFF_MULTICAST;  /* No multicast support */
		/* Default MTU when device is made "ifconfig up" */
		ndev->mtu               = PH_MTU;
		/*Add napi for rx function*/
		netif_napi_add(ndev, &priv->napi, wqt_rx_napi, RX_NAPI_WEIGHT);

		/* Features */
                ndev->features = NETIF_F_RXCSUM |
                            NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM | NETIF_F_HIGHDMA;
#if FSLU_NVME_INIC_SG
		//ndev->needed_headroom = DPAA2_ETH_NEEDED_HEADROOM(192);
		/*We support scatter gather support for TX Only (Not for RX)*/
		ndev->features |= NETIF_F_SG;

#endif
		ndev->hw_features |= ndev->features; 
#if 0
		/*Register net device*/
		ret = register_netdev(ndev);
		if (ret)
			goto out_register_netdev;
#endif
		/*Register 4 irqs , 4 handlers (one handler to one irq) for MSIX*/
		pcidma_request_msi(priv);
		/**/
		wqt_net_devices[i] = ndev; /*Need to use wqt_net_devices[i] at module_exit*/
		g_priv[i]= priv;  /*globally store the addresses of private structures*/
		dev_info(priv->dev, "using ethernet device %s\n", ndev->name);
		tasklet_enable(&priv->tx_complete_tasklet); /*Now enable tasklets*/
		tasklet_enable(&priv->rx_refill_tasklet);
		priv->tx_data_offset=cbd_read(&priv->bman_buf->tx_data_offset);
                printk("PCIE:%s priv->tx_data_offset:%u\n",__func__,priv->tx_data_offset);
#if FSLU_NVME_INIC_QDMA

	cbd_write(&priv->bman_buf->pool_set, BD_MEM_READY);
#endif
	} // END OF FOR LOOP 

	for( i = 0; i < NUM_NET_DEV; i++) {
		ndev = wqt_net_devices[i];
		/*Register net device*/
		ret = register_netdev(ndev);
		if (ret)
		{
                  j=i;
			while(j<0)
			{
				ndev = wqt_net_devices[j];
				priv = netdev_priv(ndev);
				free_bp_rx_buffers(priv);
				/*pool cleaned*/

				kfree(priv->qbman_pool);/*free array of buffer adress*/
                                if(j!=i)
				unregister_netdev(ndev);
				kfree(priv->host_memory_reg);/*free host memory used for flags and rx descriptors*/
				free_irq(msix_entries[j].vector, priv); /*free msix irq lines*/
				free_netdev(ndev);
                                j--;
			}		
	goto out_register_netdev;
		}

	}
	return 0; 

out_register_netdev: 
out_no_mem:
out_alloc_ndev:
out_set_dma_mask: 
	iounmap(g_netregs); 
out_ioremap_netregs: 
	iounmap(g_immr); 
out_ioremap_immr: 
	pci_release_regions(dev); 
out_pci_request_regions: 
	pci_disable_msix(dev);
err_enable_msi:
	kfree(msix_entries);
err_calloc:
	pci_disable_device(dev); 
out_pci_enable_dev: 
	return ret; 
} 
#if FSLU_NVME_INIC_DPAA2
static void wqt_remove(struct pci_dev *dev) 
{
 
	struct net_device *ndev;
	struct wqt_dev *priv;
	int i;
	for( i = 0; i < NUM_NET_DEV; i++) {
		ndev = wqt_net_devices[i];
		priv = netdev_priv(ndev);

                free_refill_rx_buffers(priv);/*free refill ring buffers*/ 
             
                /*since buffer banks are freed same array of address location is used to fetch rx bufer
                address given to dpaa2 from ls2*/
                /*pull buffers from qbman and free it*/
                priv->bman_buf->pull_rx_buffers=BP_PULL_SET;
                generate_irq_to_ls2(priv);
                while(priv->bman_buf->pull_rx_buffers!=BP_PULL_DONE)
                {
                msleep(100);
                }
                free_bp_rx_buffers(priv);
                priv->bman_buf->pull_rx_buffers=BP_PULL_CLEAR;
                /*pool cleaned*/
      
                kfree(priv->qbman_pool);/*free array of buffer adress*/

		unregister_netdev(ndev);
                kfree(priv->host_memory_reg);/*free host memory used for flags and rx descriptors*/
		free_irq(msix_entries[i].vector, priv); /*free msix irq lines*/
		free_netdev(ndev);
	}


	iounmap(g_netregs);/*unmap both the bars*/
	iounmap(g_immr);
	pci_disable_msix(dev);/*disable msix support*/
	pci_release_regions(dev);/*release device memory regions*/
	pci_disable_device(dev);/*disable pci device enaabled in probe call*/
}
#endif
#if FSLU_NVME_INIC_QDMA
static void wqt_remove(struct pci_dev *dev) 
{
 
	struct net_device *ndev;
	struct wqt_dev *priv;
	int i;
	for( i = 0; i < NUM_NET_DEV; i++) {
		ndev = wqt_net_devices[i];
		priv = netdev_priv(ndev);
          
		unregister_netdev(ndev);

                /*clean all the rx buffers created and address stored in rx buffers*/
                free_rx_buffers(priv);


                kfree(priv->host_memory_reg);/*free host memory used for flags and rx descriptors*/
		free_irq(msix_entries[i].vector, priv); /*free msix irq lines*/
		free_netdev(ndev);
	}


	iounmap(g_netregs);/*unmap both the bars*/
	iounmap(g_immr);
	pci_disable_msix(dev);/*disable msix support*/
	pci_release_regions(dev);/*release device memory regions*/
	pci_disable_device(dev);/*disable pci device enaabled in probe call*/
}
#endif
#define PCI_DEVID_FSL_MPC8349EMDS 0x0953
#define PCI_VENDOR_ID_FREESCALE 0x1957
#define PCI_CLASS_ETHERNET_CONTROLLER 0x020000

/* The list of devices that this module will support */
static struct pci_device_id wqt_ids[] = {
{.vendor=PCI_VENDOR_ID_FREESCALE,
.device=PCI_DEVID_FSL_MPC8349EMDS,
.class=PCI_CLASS_ETHERNET_CONTROLLER,
.class_mask=0xffffff,
.subvendor = PCI_ANY_ID,
.subdevice = PCI_ANY_ID, },
  { 0, }
};


MODULE_DEVICE_TABLE(pci, wqt_ids);

static struct pci_driver wqt_pci_driver = {
    .name     = (char *)driver_name,
    .id_table = wqt_ids,
    .probe    = wqt_probe,
    .remove   = wqt_remove,
};

/*----------------------------------------------------------------------------*/ 
/* Module Init / Exit                                                         */ 
/*----------------------------------------------------------------------------*/ 

static int __init wqt_init(void) {
    int ret;

    ret = pci_register_driver(&wqt_pci_driver);
    if (ret)
        goto out_pci_register_driver;

    return 0;

out_pci_register_driver:
    return ret;
}

static void __exit wqt_exit(void)
{
    pci_unregister_driver(&wqt_pci_driver);
}

MODULE_AUTHOR("");
MODULE_DESCRIPTION("PCINet Driver for LS2085 (Host side)");
MODULE_LICENSE("GPL");

module_init(wqt_init);
module_exit(wqt_exit);
