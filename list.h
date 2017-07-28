#ifndef __LIST_H
#define __LIST_H

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#ifndef LIST_NODE_DATATYPE
#define LIST_NODE_DATATYPE int
#endif

typedef LIST_NODE_DATATYPE datatype;

typedef struct node
{
	datatype data;
	struct node *prev;
	struct node *next;
}listnode, *linklist;


// 初始化链表
linklist init_list(void);

// 添加节点
void list_add(linklist new, linklist head);
void list_add_tail(linklist new, linklist head);

// 删除节点
void list_del(linklist del);

// 移动节点
void list_move(linklist list, linklist head);
void list_move_tail(linklist list, linklist head);

#endif
