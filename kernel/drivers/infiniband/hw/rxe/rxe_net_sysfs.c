/*
 * Copyright (c) 2009-2011 Mellanox Technologies Ltd. All rights reserved.
 * Copyright (c) 2009-2011 System Fabric Works, Inc. All rights reserved.
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
 *	- Redistributions of source code must retain the above
 *	  copyright notice, this list of conditions and the following
 *	  disclaimer.
 *
 *	- Redistributions in binary form must reproduce the above
 *	  copyright notice, this list of conditions and the following
 *	  disclaimer in the documentation and/or other materials
 *	  provided with the distribution.
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

#include "rxe.h"
#include "rxe_net.h"

/* Copy argument and remove trailing CR. Return the new length. */
static int sanitize_arg(const char *val, char *intf, int intf_len)
{
	int len;

	if (!val)
		return 0;

	/* Remove newline. */
	for (len = 0; len < intf_len - 1 && val[len] && val[len] != '\n'; len++)
		intf[len] = val[len];
	intf[len] = 0;

	if (len == 0 || (val[len] != 0 && val[len] != '\n'))
		return 0;

	return len;
}

/* Caller must hold net_info_lock */
static void rxe_set_port_state(struct rxe_net_info_list *info_item)
{
	if (info_item->status == IB_PORT_ACTIVE)
		rxe_net_up(info_item->ndev, info_item);
	else
		rxe_net_down(info_item->ndev, info_item); /* down for unknown state */
}

static int rxe_param_set_add(const char *val, const struct kernel_param *kp)
{
	int len;
	struct rxe_net_info_list *info_item;
	char intf[32];

	len = sanitize_arg(val, intf, sizeof(intf));
	if (!len) {
		pr_err("rxe: add: invalid interface name\n");
		return -EINVAL;
	}

	spin_lock_bh(&net_info_lock);
	list_for_each_entry(info_item, &net_info_list, list)
		if (info_item->ndev && (0 == strncmp(intf,
					info_item->ndev->name, (strlen(info_item->ndev->name) > len ) ? strlen(info_item->ndev->name) : len ))) {
			spin_unlock_bh(&net_info_lock);
			if (info_item->rxe)
				pr_info("rxe: already configured on %s\n",
					intf);
			else {
				rxe_net_add(info_item);
				if (info_item->rxe)
					rxe_set_port_state(info_item);
				else
					pr_err("rxe: add appears to have failed"
					       " for %s (index %d)\n",
						intf, info_item->ndev->ifindex);
			}
			return 0;
		}
	spin_unlock_bh(&net_info_lock);

	pr_warn("interface %s not found\n", intf);

	return 0;
}

static void rxe_remove_all(void)
{
	struct rxe_dev *rxe;
	struct rxe_net_info_list *info_item;

	list_for_each_entry(info_item, &net_info_list, list)
		if (info_item->rxe) {
			spin_lock_bh(&net_info_lock);
			rxe = info_item->rxe;
			info_item->rxe = NULL;
			spin_unlock_bh(&net_info_lock);

			rxe_remove(rxe);
		}
}

static int rxe_param_set_remove(const char *val, const struct kernel_param *kp)
{
	int len;
	char intf[32];
	struct rxe_dev *rxe;
	struct rxe_net_info_list *info_item;

	len = sanitize_arg(val, intf, sizeof(intf));
	if (!len) {
		pr_err("rxe: remove: invalid interface name\n");
		return -EINVAL;
	}

	if (strncmp("all", intf, len) == 0) {
		pr_info("rxe_sys: remove all");
		rxe_remove_all();
		return 0;
	}

	spin_lock_bh(&net_info_lock);
	list_for_each_entry(info_item, &net_info_list, list)
		if (!info_item->rxe || !info_item->ndev)
			continue;
		else if (0 == strncmp(intf, info_item->rxe->ib_dev.name, len)) {
			rxe = info_item->rxe;
			info_item->rxe = NULL;
			spin_unlock_bh(&net_info_lock);

			rxe_remove(rxe);

			return 0;
		}
	spin_unlock_bh(&net_info_lock);

	pr_warn("rxe_sys: instance %s not found\n", intf);

	return 0;
}

static int rxe_param_set_watermark(const char *val, const struct kernel_param *kp)
{

	char intf[32];
	char valstr[100];
	struct rxe_net_info_list *info_item;
	unsigned long long int watermark_depth;
	int i,size =100;

	memcpy(valstr,val,100);
	printk("valstr len : %u\n",(unsigned int)strlen(valstr));
	
	if(size > strlen(valstr)) {
		size = strlen(valstr);
	}

	for(i=0;i<size;i++) {
		printk("%c",valstr[i]);
	}

        sscanf(valstr,"%s%llu",intf,&watermark_depth);

	printk("intf is : %s\n",intf);
	printk("watermark : %llu\n",watermark_depth);

        spin_lock_bh(&net_info_lock);
        list_for_each_entry(info_item, &net_info_list, list)
                if (info_item->ndev && (0 == strncmp(intf,
                                                info_item->ndev->name, (strlen(info_item->ndev->name))))) {
                        spin_unlock_bh(&net_info_lock);
                        if (info_item->rxe) {
                                printk("roce debug:  (before watermark)  rxe->watermark_depth : %llu \n",\
                                                info_item->rxe->watermark_depth);
                                printk("setting watermark depth for interface %s\n",intf);

                                info_item->rxe->watermark_depth = watermark_depth;
                                printk("roce debug:  (present watermark)  rxe->watermark_depth : %llu \n",\
                                                info_item->rxe->watermark_depth);

                        }

                        else {
                                printk("interface %s not found\n", intf);
                        }
                return 0;
                }
        spin_unlock_bh(&net_info_lock);
        printk("interface %s not found\n", intf);

        return 0;
}

 
static int rxe_param_set_queuing_speed(const char *val, const struct kernel_param *kp)
{

	char intf[32];
	char valstr[100];
	struct rxe_net_info_list *info_item;
	unsigned long long int queuing_speed;
	int i,size =100;

	memcpy(valstr,val,100);
	printk("valstr len : %u\n",(unsigned int)strlen(valstr));
	
	if(size > strlen(valstr)) {
		size = strlen(valstr);
	}

	for(i=0;i<size;i++) {
		printk("%c",valstr[i]);
	}

        sscanf(valstr,"%s%llu",intf,&queuing_speed);

	printk("intf is : %s\n",intf);
	printk("queuing_speed : %llu\n",queuing_speed);

        spin_lock_bh(&net_info_lock);
        list_for_each_entry(info_item, &net_info_list, list)
                if (info_item->ndev && (0 == strncmp(intf,
                                                info_item->ndev->name, (strlen(info_item->ndev->name))))) {
                        spin_unlock_bh(&net_info_lock);
                        if (info_item->rxe) {
                                printk("roce debug:  (before queuing)  rxe->line_speed : %llu BPS rxe->line_accumulation_per_jiffy : %llu Bytes tickrate :%u HZ\n",\
                                                info_item->rxe->line_speed,info_item->rxe->line_accumulation_per_jiffy,HZ);
                                printk("setting queuing speed for interface %s\n",intf);

                                info_item->rxe->line_speed = (queuing_speed/8); // 1250MB  // roce added
                                info_item->rxe->line_accumulation_per_jiffy = (info_item->rxe->line_speed)/HZ; // max permissible data per jiffie // roce added

                                printk("roce debug:  rxe->line_speed : %llu BPS rxe->line_accumulation_per_jiffy : %llu Bytes tickrate :%u HZ\n",\
                                                info_item->rxe->line_speed,info_item->rxe->line_accumulation_per_jiffy,HZ);
                        }

                        else {
                                printk("interface %s not found\n", intf);
                        }
                return 0;
                }
        spin_unlock_bh(&net_info_lock);
        printk("interface %s not found\n", intf);

        return 0;
}
static int rxe_param_set_queuing_type(char *intf, int num)
{

	printk("Checking interface : %s \n",intf);
	struct rxe_net_info_list *info_item;
        spin_lock_bh(&net_info_lock);
        list_for_each_entry(info_item, &net_info_list, list)
                if (info_item->ndev && (0 == strncmp(intf,
                                                info_item->ndev->name, (strlen(info_item->ndev->name))))) {
                        spin_unlock_bh(&net_info_lock);
                        if (info_item->rxe) {
				printk("setting interface : %s with control method : %d\n",intf,num);
				info_item->rxe->control_method = num;
                        }

                        else {
                                printk("interface %s not found\n", intf);
                        }
                return 0;
                }
        spin_unlock_bh(&net_info_lock);
        printk("interface %s not found\n", intf);

        return 0;
}

static int rxe_param_set_control_method(const char *val, const struct kernel_param *kp)
{
	char type[20],interface[20];
	sscanf(val,"%s%s",interface,type);
	if(strcmp(type,"watermark") == 0) {
		printk("Setting control method : %s\n",type);
		rxe_param_set_queuing_type(interface,2);
	}
	else if(strcmp(type,"bandwidth") == 0) {
		printk("Setting control method : %s\n",type);
		rxe_param_set_queuing_type(interface,1);
	}
	else if(strcmp(type,"none") == 0) {
		printk("Setting control method : %s\n",type);
		rxe_param_set_queuing_type(interface,0);

	}
	else {
		printk("No such method %s\n",type);
		printk("methods are \"watermark\" , \"bandwidth\" , \"none\" \n");
	}
	return 0;
}

static int rxe_param_set_control_value(const char *val, const struct kernel_param *kp)
{

 char intf[32];
        char valstr[100];
        struct rxe_net_info_list *info_item;
        unsigned long long int control_value;
        int i,size =100;

        memcpy(valstr,val,100);
        printk("valstr len : %u\n",(unsigned int)strlen(valstr));

        if(size > strlen(valstr)) {
                size = strlen(valstr);
        }

        for(i=0;i<size;i++) {
                printk("%c",valstr[i]);
        }

        sscanf(valstr,"%s%llu",intf,&control_value);


        spin_lock_bh(&net_info_lock);
        list_for_each_entry(info_item, &net_info_list, list)
                if (info_item->ndev && (0 == strncmp(intf,
                                                info_item->ndev->name, (strlen(info_item->ndev->name))))) {
                        spin_unlock_bh(&net_info_lock);
			if (info_item->rxe) {
				if(info_item->rxe->control_method == 0) {
					printk("cannot set for none profile\n");
				}
				else if(info_item->rxe->control_method == 1) {
					rxe_param_set_queuing_speed(val,kp);
				}
				else if(info_item->rxe->control_method == 2) {
					rxe_param_set_watermark(val,kp);
				}
			}

                        else {
                                printk("interface %s not found\n", intf);
                        }
                return 0;
                }
        spin_unlock_bh(&net_info_lock);
        printk("interface %s not found\n", intf);

        return 0;


}

static struct kernel_param_ops param_ops_control_value = {
        .set = rxe_param_set_control_value,
        .get = NULL,
};
module_param_cb(control_value, &param_ops_control_value, NULL, 0200);


static struct kernel_param_ops param_ops_queuing_control = {
        .set = rxe_param_set_control_method,
        .get = NULL,
};
module_param_cb(control_method, &param_ops_queuing_control, NULL, 0200);


static struct kernel_param_ops param_ops_add = {
	.set = rxe_param_set_add,
	.get = NULL,
};
module_param_cb(add, &param_ops_add, NULL, 0200);

static struct kernel_param_ops param_ops_remove = {
	.set = rxe_param_set_remove,
	.get = NULL,
};
module_param_cb(remove, &param_ops_remove, NULL, 0200);
