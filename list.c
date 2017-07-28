#include "list.h"

linklist init_list(void)
{
	linklist head = calloc(1, sizeof(listnode));
	if(head != NULL)
	{
		head->prev = head->next = head;	
	}

	return head;
}

void list_add(linklist new, linklist head)
{
	new->prev = head;
	new->next = head->next;

	head->next->prev = new;
	head->next = new;
}

void list_add_tail(linklist new, linklist head)
{
	new->prev = head->prev;
	new->next = head;

	head->prev->next = new;
	head->prev = new;
}

void list_del(linklist del)
{
	del->prev->next = del->next;
	del->next->prev = del->prev;

	del->prev = NULL;
	del->next = NULL;
}

// 将节点list移动到head节点的后面
void list_move(linklist list, linklist head)
{
	list_del(list);
	list_add(list, head);
}

// 将节点list移动到head节点的前面
void list_move_tail(linklist list, linklist head)
{
	list_del(list);
	list_add_tail(list, head);
}
