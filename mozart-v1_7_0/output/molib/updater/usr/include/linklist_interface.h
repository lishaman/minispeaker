#ifndef _LIST_H_
#define _LIST_H_

#include <stdbool.h>


/**
 * @brief node struct
 */
typedef struct node{    //节点结构
	/**
	 * @brief  user data on the node.
	 */
	void *data;
	/**
	 * @brief next node position.
	 */
	struct node *next;
}node;

/**
 * @brief linklist struct
 */
typedef struct{    //链表结构
	/**
	 * @brief head of the list.
	 */
	struct node *head;
	/**
	 * @brief tail of the list.
	 */
	struct node *tail;
	/**
	 * @brief the length of the list.
	 */
	long len;
}List;

/**
 * @brief list init.
 *
 * @param list [in][out] the list to be inited.
 */
extern void  list_init(List *list);

/**
 * @brief is the list is empty?
 *
 * @param list [in][out] the list that to be operated.
 *
 * @return true if empty, false otherwise.
 */
extern bool  is_empty(List *list);

/**
 * @brief insert a new node to the tail of the list.
 *
 * @param list [in][out] the list that to be operated.
 * @param data [in] user data.
 */
extern void  list_insert(List *list, void *data);    		 			//默认采用尾插法

/**
 * @brief insert a new node to the head of the list.
 *
 * @param list [in][out] the list that to be operated.
 * @param data [in] user data.
 */
extern void  list_insert_at_head(List *list, void *data);    			//头插法

/**
 * @brief insert a new node to the tail of the list.
 *
 * @param list [in][out] the list that to be operated.
 * @param data [in] user data.
 */
extern void  list_insert_at_tail(List *list, void *data);    			//尾插法

/**
 * @brief insert a new node to the random postion of the list.
 *
 * @param list [in][out] the list that to be operated.
 * @param data [in] user data.
 * @param idx [in] index of the list node.
 */
extern void  list_insert_at_index(List *list, void *data, long idx);	//随插法

/**
 * @brief  delete a node from the list.
 *
 * @param list [in][out] the list that to be operated.
 * @param key [in] the keyword that delete a node.
 * @param compare [in] the rule of compare nodes.
 *
 * @return the deleted user data.
 */
extern void *list_delete(List *list, void *key, int (*compare)(const void *, const void *));
extern void *list_delete_at_index(List *list, long idx);

/**
 * @brief  search the list.
 *
 * @param list [in][out] the list that to be operated.
 * @param key [in] the keyword that delete a node.
 * @param compare [in] the rule of compare nodes.
 *
 * @return user data if found, NULL otherwise.
 */
extern void *list_search(List *list, void *key, int (*compare)(const void *, const void *));

/**
 * @brief sort the list
 *
 * @param list [in][out] the list that to be operated.
 * @param compare [in] the rule of compare nodes.
 */
extern void  list_sort(List *list, int (*compare)(const void *, const void *));

/**
 * @brief traverse the list.
 *
 * @param list [in][out] the list that to be operated.
 * @param handle [in] the handler to traverse the list.
 */
extern void  list_traverse(List *list, void (*handle)(void *));

/**
 * @brief reverse the list.
 *
 * @param list [in][out] the list that to be operated.
 */
extern void  list_reverse(List *list);

/**
 * @brief get the length of the list.
 *
 * @param list [in][out] the list that to be operated.
 *
 * @return the length of the list.
 */
extern long  list_get_length(List *list);

/**
 * @brief get the idx's node's user data.
 *
 * @param list [in][out] the list that to be operated.
 *
 * @param idx [in] index of the list node.
 *
 * @return user data if get it, NULL otherwise.
 */
extern void *list_get_element(List *list, int idx);

/**
 * @brief destroy a initted list.
 *
 * @param list [in][out] the list that to be operated.
 * @param destroy [in] handler the user data destroyed from every list node.
 */
extern void  list_destroy(List *list, void (*destroy)(void *data));

#endif
