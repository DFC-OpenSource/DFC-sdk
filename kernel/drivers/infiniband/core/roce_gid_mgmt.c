/*
 * Copyright (c) 2015, Mellanox Technologies inc.  All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "core_priv.h"

#include <linux/in.h>
#include <linux/in6.h>

/* For in6_dev_get/in6_dev_put */
#include <net/addrconf.h>
#include <net/bonding.h>

#include <rdma/ib_cache.h>
#include <rdma/ib_addr.h>

struct workqueue_struct *roce_gid_mgmt_wq;

enum gid_op_type {
	GID_DEL = 0,
	GID_ADD
};

struct  update_gid_event_work {
	struct work_struct work;
	union ib_gid       gid;
	struct ib_gid_attr gid_attr;
	enum gid_op_type gid_op;
};

#define ROCE_NETDEV_CALLBACK_SZ		3
struct netdev_event_work_cmd {
	roce_netdev_callback	cb;
	roce_netdev_filter	filter;
	struct net_device	*ndev;
	struct net_device	*f_ndev;
};

struct netdev_event_work {
	struct work_struct		work;
	struct netdev_event_work_cmd	cmds[ROCE_NETDEV_CALLBACK_SZ];
};

struct roce_rescan_work {
	struct work_struct	work;
	struct ib_device	*ib_dev;
};

static const struct {
	int flag_mask;
	enum ib_gid_type gid_type;
} PORT_CAP_TO_GID_TYPE[] = {
	{IB_PORT_IBOE_V2,   IB_GID_TYPE_IBOE_V2},
	{IB_PORT_IBOE_V1,   IB_GID_TYPE_IBOE_V1},
};

#define CAP_TO_GID_TABLE_SIZE	ARRAY_SIZE(PORT_CAP_TO_GID_TYPE)

unsigned long roce_gid_type_mask_support(struct ib_device *ib_dev, u8 port)
{
	struct ib_port_attr pattr;
	int i;
	int err;
	unsigned int ret_flags = 0;

	err = ib_query_port(ib_dev, port, &pattr);
	if (err) {
		pr_warn("update_gid: ib_query_port() failed for %s, %d\n",
			ib_dev->name, err);
		return 0;
	}

	for (i = 0; i < CAP_TO_GID_TABLE_SIZE; i++)
		if (pattr.port_cap_flags & PORT_CAP_TO_GID_TYPE[i].flag_mask)
			ret_flags |= 1UL << PORT_CAP_TO_GID_TYPE[i].gid_type;

	return ret_flags;
}

static void update_gid(enum gid_op_type gid_op, struct ib_device *ib_dev,
		       u8 port, union ib_gid *gid,
		       struct ib_gid_attr *gid_attr)
{
	int i;
	unsigned long gid_type_mask = roce_gid_type_mask_support(ib_dev, port);

	for (i = 0; i < IB_GID_TYPE_SIZE; i++) {
		if ((1UL << i) & gid_type_mask) {
			gid_attr->gid_type = i;
			switch (gid_op) {
			case GID_ADD:
				roce_add_gid(ib_dev, port,
					     gid, gid_attr);
				break;
			case GID_DEL:
				roce_del_gid(ib_dev, port,
					     gid, gid_attr);
				break;
			}
		}
	}
}

#define IS_NETDEV_BONDING_MASTER(ndev)	\
	(((ndev)->priv_flags & IFF_BONDING) && \
	 ((ndev)->flags & IFF_MASTER))

enum bonding_slave_state {
	BONDING_SLAVE_STATE_ACTIVE	= 1UL << 0,
	BONDING_SLAVE_STATE_INACTIVE	= 1UL << 1,
	BONDING_SLAVE_STATE_NA		= 1UL << 2,
};

static enum bonding_slave_state is_eth_active_slave_of_bonding(struct net_device *idev,
							       struct net_device *upper)
{
	if (upper && IS_NETDEV_BONDING_MASTER(upper)) {
		struct net_device *pdev;

		rcu_read_lock();
		pdev = bond_option_active_slave_get_rcu(netdev_priv(upper));
		rcu_read_unlock();
		if (pdev)
			return idev == pdev ? BONDING_SLAVE_STATE_ACTIVE :
				BONDING_SLAVE_STATE_INACTIVE;
	}

	return BONDING_SLAVE_STATE_NA;
}

static bool is_upper_dev_rcu(struct net_device *dev, struct net_device *upper)
{
	struct net_device *_upper = NULL;
	struct list_head *iter;

	rcu_read_lock();
	netdev_for_each_all_upper_dev_rcu(dev, _upper, iter) {
		if (_upper == upper)
			break;
	}

	rcu_read_unlock();
	return _upper == upper;
}

static int _is_eth_port_of_netdev(struct ib_device *ib_dev, u8 port,
				  struct net_device *idev, void *cookie,
				  unsigned long bond_state)
{
	struct net_device *ndev = (struct net_device *)cookie;
	struct net_device *rdev;
	int res;

	if (!idev)
		return 0;

	rcu_read_lock();
	rdev = rdma_vlan_dev_real_dev(ndev);
	if (!rdev)
		rdev = ndev;

	res = ((is_upper_dev_rcu(idev, ndev) &&
	       (is_eth_active_slave_of_bonding(idev, rdev) &
		bond_state)) ||
	       rdev == idev);

	rcu_read_unlock();
	return res;
}

static int is_eth_port_of_netdev(struct ib_device *ib_dev, u8 port,
				 struct net_device *idev, void *cookie)
{
	return _is_eth_port_of_netdev(ib_dev, port, idev, cookie,
				      BONDING_SLAVE_STATE_ACTIVE |
				      BONDING_SLAVE_STATE_NA);
}

static int is_eth_port_inactive_slave(struct ib_device *ib_dev, u8 port,
				      struct net_device *idev, void *cookie)
{
	struct net_device *mdev;
	int res;
	if (!idev)
		return 0;

	rcu_read_lock();
	mdev = netdev_master_upper_dev_get_rcu(idev);
	res = is_eth_active_slave_of_bonding(idev, mdev) ==
		BONDING_SLAVE_STATE_INACTIVE;
	rcu_read_unlock();

	return res;
}

static int pass_all_filter(struct ib_device *ib_dev, u8 port,
			   struct net_device *idev, void *cookie)
{
	return 1;
}

static int upper_device_filter(struct ib_device *ib_dev, u8 port,
			       struct net_device *idev, void *cookie)
{
	struct net_device *ndev = (struct net_device *)cookie;

	return idev == ndev || is_upper_dev_rcu(idev, ndev);
}

static int bonding_slaves_filter(struct ib_device *ib_dev, u8 port,
				 struct net_device *idev, void *cookie)
{
	struct net_device *rdev;
	struct net_device *ndev = (struct net_device *)cookie;
	int res;

	rdev = rdma_vlan_dev_real_dev(ndev);

	ndev = rdev ? rdev : ndev;
	if (!idev || !IS_NETDEV_BONDING_MASTER(ndev))
		return 0;

	rcu_read_lock();
	res = is_upper_dev_rcu(idev, ndev);
	rcu_read_unlock();

	return res;
}

static void netdevice_event_work_handler(struct work_struct *_work)
{
	struct netdev_event_work *work =
		container_of(_work, struct netdev_event_work, work);
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(work->cmds) && work->cmds[i].cb; i++) {
		ib_enum_roce_ports_of_netdev(work->cmds[i].filter,
					     work->cmds[i].f_ndev,
					     work->cmds[i].cb,
					     work->cmds[i].ndev);
		dev_put(work->cmds[i].ndev);
		dev_put(work->cmds[i].f_ndev);
	}

	kfree(work);
}

static void update_gid_ip(enum gid_op_type gid_op,
			  struct ib_device *ib_dev,
			  u8 port, struct net_device *ndev,
			  const struct sockaddr *addr)
{
	union ib_gid gid;
	struct ib_gid_attr gid_attr;

	rdma_ip2gid(addr, &gid);
	memset(&gid_attr, 0, sizeof(gid_attr));
	gid_attr.ndev = ndev;

	update_gid(gid_op, ib_dev, port, &gid, &gid_attr);
}

static void enum_netdev_default_gids(struct ib_device *ib_dev,
				     u8 port, struct net_device *ndev,
				     struct net_device *idev)
{
	unsigned long gid_type_mask;

	rcu_read_lock();
	if (!idev ||
	    ((idev != ndev && !is_upper_dev_rcu(idev, ndev)) ||
	     is_eth_active_slave_of_bonding(idev,
					    netdev_master_upper_dev_get_rcu(idev)) ==
	     BONDING_SLAVE_STATE_INACTIVE)) {
		rcu_read_unlock();
		return;
	}
	rcu_read_unlock();

	gid_type_mask = roce_gid_type_mask_support(ib_dev, port);

	roce_gid_cache_set_default_gid(ib_dev, port, idev, gid_type_mask,
				       ROCE_GID_CACHE_DEFAULT_MODE_SET);
}

static void bond_delete_netdev_default_gids(struct ib_device *ib_dev,
					    u8 port, struct net_device *ndev,
					    struct net_device *idev)
{
	struct net_device *rdev = rdma_vlan_dev_real_dev(ndev);

	if (!idev)
		return;

	if (!rdev)
		rdev = ndev;

	rcu_read_lock();

	if (is_upper_dev_rcu(idev, ndev) &&
	    is_eth_active_slave_of_bonding(idev, rdev) ==
	    BONDING_SLAVE_STATE_INACTIVE) {
		unsigned long gid_type_mask;

		rcu_read_unlock();

		gid_type_mask = roce_gid_type_mask_support(ib_dev, port);

		roce_gid_cache_set_default_gid(ib_dev, port, idev,
					       gid_type_mask,
					       ROCE_GID_CACHE_DEFAULT_MODE_DELETE);
	} else {
		rcu_read_unlock();
	}
}

static void enum_netdev_ipv4_ips(struct ib_device *ib_dev,
				 u8 port, struct net_device *ndev)
{
	struct in_device *in_dev;

	if (ndev->reg_state >= NETREG_UNREGISTERING)
		return;

	in_dev = in_dev_get(ndev);
	if (!in_dev)
		return;

	for_ifa(in_dev) {
		struct sockaddr_in ip;

		ip.sin_family = AF_INET;
		ip.sin_addr.s_addr = ifa->ifa_address;
		update_gid_ip(GID_ADD, ib_dev, port, ndev,
			      (struct sockaddr *)&ip);
	}
	endfor_ifa(in_dev);

	in_dev_put(in_dev);
}

#if IS_ENABLED(CONFIG_IPV6)
static void enum_netdev_ipv6_ips(struct ib_device *ib_dev,
				 u8 port, struct net_device *ndev)
{
	struct inet6_ifaddr *ifp;
	struct inet6_dev *in6_dev;
	struct sin6_list {
		struct list_head	list;
		struct sockaddr_in6	sin6;
	};
	struct sin6_list *sin6_iter;
	struct sin6_list *sin6_temp;
	struct ib_gid_attr gid_attr = {.ndev = ndev};
	LIST_HEAD(sin6_list);

	if (ndev->reg_state >= NETREG_UNREGISTERING)
		return;

	in6_dev = in6_dev_get(ndev);
	if (!in6_dev)
		return;

	read_lock_bh(&in6_dev->lock);
	list_for_each_entry(ifp, &in6_dev->addr_list, if_list) {
		struct sin6_list *entry = kzalloc(sizeof(*entry), GFP_ATOMIC);

		if (!entry) {
			pr_warn("roce_gid_mgmt: couldn't allocate entry for IPv6 update\n");
			continue;
		}

		entry->sin6.sin6_family = AF_INET6;
		entry->sin6.sin6_addr = ifp->addr;
		list_add_tail(&entry->list, &sin6_list);
	}
	read_unlock_bh(&in6_dev->lock);

	in6_dev_put(in6_dev);

	list_for_each_entry_safe(sin6_iter, sin6_temp, &sin6_list, list) {
		union ib_gid	gid;

		rdma_ip2gid((const struct sockaddr *)&sin6_iter->sin6, &gid);
		update_gid(GID_ADD, ib_dev, port, &gid, &gid_attr);
		list_del(&sin6_iter->list);
		kfree(sin6_iter);
	}
}
#endif

static void add_netdev_ips(struct ib_device *ib_dev, u8 port,
			   struct net_device *idev, void *cookie)
{
	struct net_device *ndev = (struct net_device *)cookie;

	enum_netdev_default_gids(ib_dev, port, ndev, idev);
	enum_netdev_ipv4_ips(ib_dev, port, ndev);
#if IS_ENABLED(CONFIG_IPV6)
	enum_netdev_ipv6_ips(ib_dev, port, ndev);
#endif
}

static void del_netdev_ips(struct ib_device *ib_dev, u8 port,
			   struct net_device *idev, void *cookie)
{
	struct net_device *ndev = (struct net_device *)cookie;

	roce_del_all_netdev_gids(ib_dev, port, ndev);
}

static void del_netdev_upper_ips(struct ib_device *ib_dev, u8 port,
				 struct net_device *idev, void *cookie)
{
	struct net_device *ndev = (struct net_device *)cookie;
	struct upper_list {
		struct list_head list;
		struct net_device *upper;
	};
	struct net_device *upper;
	struct list_head *iter;
	struct upper_list *upper_iter;
	struct upper_list *upper_temp;
	LIST_HEAD(upper_list);

	rcu_read_lock();
	netdev_for_each_all_upper_dev_rcu(ndev, upper, iter) {
		struct upper_list *entry = kmalloc(sizeof(*entry),
						   GFP_ATOMIC);

		if (!entry) {
			pr_info("roce_gid_mgmt: couldn't allocate entry to delete ndev\n");
			continue;
		}

		list_add_tail(&entry->list, &upper_list);
		dev_hold(upper);
		entry->upper = upper;
	}
	rcu_read_unlock();

	roce_del_all_netdev_gids(ib_dev, port, ndev);
	list_for_each_entry_safe(upper_iter, upper_temp, &upper_list,
				 list) {
		roce_del_all_netdev_gids(ib_dev, port,
					 upper_iter->upper);
		dev_put(upper_iter->upper);
		list_del(&upper_iter->list);
		kfree(upper_iter);
	}
}

static void del_netdev_default_ips_join(struct ib_device *ib_dev, u8 port,
					struct net_device *idev, void *cookie)
{
	struct net_device *mdev;

	rcu_read_lock();
	mdev = netdev_master_upper_dev_get_rcu(idev);
	if (mdev)
		dev_hold(mdev);
	rcu_read_unlock();

	if (mdev) {
		bond_delete_netdev_default_gids(ib_dev, port, mdev, idev);
		dev_put(mdev);
	}
}

static void del_netdev_default_ips(struct ib_device *ib_dev, u8 port,
				   struct net_device *idev, void *cookie)
{
	struct net_device *ndev = (struct net_device *)cookie;

	bond_delete_netdev_default_gids(ib_dev, port, ndev, idev);
}

static int netdevice_event(struct notifier_block *this, unsigned long event,
			   void *ptr)
{
	static const struct netdev_event_work_cmd add_cmd = {
		.cb = add_netdev_ips, .filter = is_eth_port_of_netdev};
	static const struct netdev_event_work_cmd del_cmd = {
		.cb = del_netdev_ips, .filter = pass_all_filter};
	static const struct netdev_event_work_cmd bonding_default_del_cmd_join = {
		.cb = del_netdev_default_ips_join, .filter = is_eth_port_inactive_slave};
	static const struct netdev_event_work_cmd bonding_default_del_cmd = {
		.cb = del_netdev_default_ips, .filter = is_eth_port_inactive_slave};
	static const struct netdev_event_work_cmd default_del_cmd = {
		.cb = del_netdev_default_ips, .filter = pass_all_filter};
	static const struct netdev_event_work_cmd bonding_event_ips_del_cmd = {
		.cb = del_netdev_ips, .filter = bonding_slaves_filter};
	static const struct netdev_event_work_cmd upper_ips_del_cmd = {
		.cb = del_netdev_upper_ips, .filter = upper_device_filter};
	struct net_device *ndev = netdev_notifier_info_to_dev(ptr);
	struct netdev_event_work *ndev_work;
	struct netdev_event_work_cmd cmds[ROCE_NETDEV_CALLBACK_SZ] = { {NULL} };
	unsigned int i;

	if (ndev->type != ARPHRD_ETHER)
		return NOTIFY_DONE;

	switch (event) {
	case NETDEV_REGISTER:
	case NETDEV_UP:
		cmds[0] = bonding_default_del_cmd_join;
		cmds[1] = add_cmd;
		break;

	case NETDEV_UNREGISTER:
		if (ndev->reg_state < NETREG_UNREGISTERED)
			cmds[0] = del_cmd;
		else
			return NOTIFY_DONE;
		break;

	case NETDEV_CHANGEADDR:
		cmds[0] = default_del_cmd;
		cmds[1] = add_cmd;
		break;

	case NETDEV_CHANGEUPPER:
		{
			struct netdev_changeupper_info *changeupper_info =
				container_of(ptr, struct netdev_changeupper_info, info);

			if (changeupper_info->event ==
			    NETDEV_CHANGEUPPER_UNLINK) {
				cmds[0] = upper_ips_del_cmd;
				cmds[0].ndev = changeupper_info->upper;
				cmds[1] = add_cmd;
			} else if (changeupper_info->event ==
				   NETDEV_CHANGEUPPER_LINK) {
				cmds[0] = bonding_default_del_cmd;
				cmds[0].ndev = changeupper_info->upper;
				cmds[1] = add_cmd;
				cmds[1].ndev = changeupper_info->upper;
				cmds[1].f_ndev = changeupper_info->upper;
			}
		}
	break;

	case NETDEV_BONDING_FAILOVER:
		cmds[0] = bonding_event_ips_del_cmd;
		cmds[1] = bonding_default_del_cmd_join;
		cmds[2] = add_cmd;
		break;

	default:
		return NOTIFY_DONE;
	}

	ndev_work = kmalloc(sizeof(*ndev_work), GFP_KERNEL);
	if (!ndev_work) {
		pr_warn("roce_gid_mgmt: can't allocate work for netdevice_event\n");
		return NOTIFY_DONE;
	}

	memcpy(ndev_work->cmds, cmds, sizeof(ndev_work->cmds));
	for (i = 0; i < ARRAY_SIZE(ndev_work->cmds) && ndev_work->cmds[i].cb; i++) {
		if (!ndev_work->cmds[i].ndev)
			ndev_work->cmds[i].ndev = ndev;
		if (!ndev_work->cmds[i].f_ndev)
			ndev_work->cmds[i].f_ndev = ndev;
		dev_hold(ndev_work->cmds[i].ndev);
		dev_hold(ndev_work->cmds[i].f_ndev);
	}
	INIT_WORK(&ndev_work->work, netdevice_event_work_handler);

	queue_work(roce_gid_mgmt_wq, &ndev_work->work);

	return NOTIFY_DONE;
}

static void callback_for_addr_gid_device_scan(struct ib_device *device,
					      u8 port,
					      struct net_device *idev,
					      void *cookie)
{
	struct update_gid_event_work *parsed = cookie;

	return update_gid(parsed->gid_op, device,
			  port, &parsed->gid,
			  &parsed->gid_attr);
}

static void update_gid_event_work_handler(struct work_struct *_work)
{
	struct update_gid_event_work *work =
		container_of(_work, struct update_gid_event_work, work);

	ib_enum_roce_ports_of_netdev(is_eth_port_of_netdev, work->gid_attr.ndev,
				     callback_for_addr_gid_device_scan, work);

	dev_put(work->gid_attr.ndev);
	kfree(work);
}

static int addr_event(struct notifier_block *this, unsigned long event,
		      struct sockaddr *sa, struct net_device *ndev)
{
	struct update_gid_event_work *work;
	enum gid_op_type gid_op;

	if (ndev->type != ARPHRD_ETHER)
		return NOTIFY_DONE;

	switch (event) {
	case NETDEV_UP:
		gid_op = GID_ADD;
		break;

	case NETDEV_DOWN:
		gid_op = GID_DEL;
		break;

	default:
		return NOTIFY_DONE;
	}

	work = kmalloc(sizeof(*work), GFP_ATOMIC);
	if (!work) {
		pr_warn("roce_gid_mgmt: Couldn't allocate work for addr_event\n");
		return NOTIFY_DONE;
	}

	INIT_WORK(&work->work, update_gid_event_work_handler);

	rdma_ip2gid(sa, &work->gid);
	work->gid_op = gid_op;

	memset(&work->gid_attr, 0, sizeof(work->gid_attr));
	dev_hold(ndev);
	work->gid_attr.ndev   = ndev;

	queue_work(roce_gid_mgmt_wq, &work->work);

	return NOTIFY_DONE;
}

static void enum_all_gids_of_dev_cb(struct ib_device *ib_dev,
				    u8 port,
				    struct net_device *idev,
				    void *cookie)
{
	struct net *net;
	struct net_device *ndev;

	/* Lock the rtnl to make sure the netdevs does not move under
	 * our feet
	 */
	rtnl_lock();
	for_each_net(net)
		for_each_netdev(net, ndev)
			if (is_eth_port_of_netdev(ib_dev, port, idev, ndev))
				add_netdev_ips(ib_dev, port, idev, ndev);
	rtnl_unlock();
}

/* This function will rescan all of the network devices in the system
 * and add their gids, as needed, to the relevant RoCE devices. Will
 * take rtnl and the IB device list mutexes. Must not be called from
 * ib_wq or deadlock will happen. */
static void enum_all_gids_of_dev(struct ib_device *ib_dev)
{
	ib_dev_roce_ports_of_netdev(ib_dev, pass_all_filter, NULL,
				    enum_all_gids_of_dev_cb, NULL);
}

static int inetaddr_event(struct notifier_block *this, unsigned long event,
			  void *ptr)
{
	struct sockaddr_in	in;
	struct net_device	*ndev;
	struct in_ifaddr	*ifa = ptr;

	in.sin_family = AF_INET;
	in.sin_addr.s_addr = ifa->ifa_address;
	ndev = ifa->ifa_dev->dev;

	return addr_event(this, event, (struct sockaddr *)&in, ndev);
}

#if IS_ENABLED(CONFIG_IPV6)
static int inet6addr_event(struct notifier_block *this, unsigned long event,
			   void *ptr)
{
	struct sockaddr_in6	in6;
	struct net_device	*ndev;
	struct inet6_ifaddr	*ifa6 = ptr;

	in6.sin6_family = AF_INET6;
	in6.sin6_addr = ifa6->addr;
	ndev = ifa6->idev->dev;

	return addr_event(this, event, (struct sockaddr *)&in6, ndev);
}
#endif

static struct notifier_block nb_netdevice = {
	.notifier_call = netdevice_event
};

static struct notifier_block nb_inetaddr = {
	.notifier_call = inetaddr_event
};

#if IS_ENABLED(CONFIG_IPV6)
static struct notifier_block nb_inet6addr = {
	.notifier_call = inet6addr_event
};
#endif

static void roce_rescan_device_work_handler(struct work_struct *_work)
{
	struct roce_rescan_work *work =
		container_of(_work, struct roce_rescan_work, work);

	enum_all_gids_of_dev(work->ib_dev);
	kfree(work);
}

/* Caller must flush system workqueue before removing the ib_device */
int roce_rescan_device(struct ib_device *ib_dev)
{
	struct roce_rescan_work *work = kmalloc(sizeof(*work), GFP_KERNEL);

	if (!work)
		return -ENOMEM;

	work->ib_dev = ib_dev;
	INIT_WORK(&work->work, roce_rescan_device_work_handler);
	schedule_work(&work->work);

	return 0;
}

int __init roce_gid_mgmt_init(void)
{
	roce_gid_mgmt_wq = alloc_ordered_workqueue("roce_gid_mgmt_wq", 0);

	if (!roce_gid_mgmt_wq) {
		pr_warn("roce_gid_mgmt: can't allocate work queue\n");
		return -ENOMEM;
	}

	register_inetaddr_notifier(&nb_inetaddr);
#if IS_ENABLED(CONFIG_IPV6)
	register_inet6addr_notifier(&nb_inet6addr);
#endif
	/* We relay on the netdevice notifier to enumerate all
	 * existing devices in the system. Register to this notifier
	 * last to make sure we will not miss any IP add/del
	 * callbacks.
	 */
	register_netdevice_notifier(&nb_netdevice);

	return 0;
}

void __exit roce_gid_mgmt_cleanup(void)
{
#if IS_ENABLED(CONFIG_IPV6)
	unregister_inet6addr_notifier(&nb_inet6addr);
#endif
	unregister_inetaddr_notifier(&nb_inetaddr);
	unregister_netdevice_notifier(&nb_netdevice);
	/* Ensure all gid deletion tasks complete before we go down,
	 * to avoid any reference to free'd memory. By the time
	 * ib-core is removed, all physical devices have been removed,
	 * so no issue with remaining hardware contexts.
	 */
	synchronize_rcu();
	drain_workqueue(roce_gid_mgmt_wq);
	destroy_workqueue(roce_gid_mgmt_wq);
}
