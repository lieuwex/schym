#include "linked_list.h"
#include "util.h"

typedef struct Item Item;
struct Item {
	Node *value;
	Item *next;
};

struct LinkedList {
	Item *top;
};

static Item *ll_end(const LinkedList *list) {
	Item *item = list->top;
	if (item == NULL) {
		return NULL;
	}

	while (item->next != NULL) {
		item = item->next;
	}
	return item;
}
static void item_free(Item *item) {
	free(item);
}

LinkedList *ll_make(void) {
	LinkedList *res = malloc(1, sizeof(LinkedList));
	res->top = NULL;
	return res;
}

LinkedList *ll_copy(const LinkedList *list) {
	LinkedList *res = ll_make();

	Item *item = list->top;
	while (item != NULL) {
		ll_push_back(res, item->value);
	}

	return res;
}

Node *ll_pop_front(LinkedList *list) {
	Item *top = list->top;
	list->top = top->next;
	Node *res = top->value;
	item_free(top);
	return res;
}
Node *ll_pop_back(LinkedList *list) {
	Item *prev = NULL;
	Item *curr = list->top;

	while (curr->next != NULL) {
		prev = curr;
		curr = curr->next;
	}

	prev->next = NULL;
	Node *res = curr->value;
	item_free(curr);
	return res;
}

void ll_push_front(LinkedList *list, Node *node) {
	Item *item = malloc(1, sizeof(Item));
	item->next = list->top;
	item->value = node;
	list->top = item;
}
void ll_push_back(LinkedList *list, Node *node) {
	Item *end = ll_end(list);

	Item *item = malloc(1, sizeof(Item));
	item->value = node;

	if (end == NULL) {
		list->top = item;
	} else {
		end->next = item;
	}
}

Node *ll_front(const LinkedList *list) {
	return list->top->value;
}
Node *ll_back(const LinkedList *list) {
	Item *end = ll_end(list);
	if (end == NULL) {
		return NULL;
	}
	return end->value;
}

void ll_free(LinkedList *list) {
	while (list->top != NULL) {
		ll_pop_front(list);
	}
	free(list);
}
