#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct Node{
    char *value;
    struct Node *prev;
    struct Node *next;
} Node;

typedef struct List{
    Node *head;
    Node *tail;
} List;

static char *xstrdup(const char *s){
    size_t n = strlen(s)+1;
    char *p = (char *)malloc(n);
    if (!p){ perror("malloc"); exit(1);}
    memcpy(p, s, n);
    return p;
}

void list_init(List *list){
    list -> head = NULL;
    list -> tail = NULL;
}

Node *list_insert_back(List *list, const char *s){
    Node *node = (Node *)malloc(sizeof(Node));
    if (!node){ perror("malloc"); exit(1);}
    node -> value = xstrdup(s);
    node -> prev = list -> tail;
    node -> next = NULL;

    if (list -> tail) list -> tail -> next = node;
    else list -> head = node;

    list -> tail = node;
    return node;
}

Node *list_find(List *list, const char *s){
    for (Node *cur = list -> head; cur; cur = cur -> next){
        if (strcmp(cur -> value, s) == 0) return cur;
    }
    return NULL;
}

int list_delete_node(List *list, Node *node){
    if (!node) return 0;

    if (node -> prev) node -> prev -> next = node -> next;
    else list -> head = node -> next;

    if (node -> next) node -> next -> prev = node -> prev;
    else list -> tail = node -> prev;

    free(node -> value);
    free(node);
    return 1;
}

int list_delete_value(List *list, const char *s){
    Node *node = list_find(list, s);
    return list_delete_node(list, node);
}

void list_free(List *list){
    Node *cur = list -> head;
    while (cur){
        Node *next = cur -> next;
        free(cur -> value);
        free(cur);
        cur = next;
    }
    list -> head = NULL;
    list -> tail = NULL;
}

void list_print(const List *list){
    printf("[");
    for (Node *cur = list -> head; cur; cur = cur -> next){
        printf("\"%s\"", cur -> value);
        if (cur -> next) printf(", ");
    }
    printf("]\n");
}

int main(int argc, const char* argv[]){
    printf("Hello, world!\n");

    List list;
    list_init(&list);

    list_insert_back(&list, "bmw");
    list_insert_back(&list, "audi");
    list_insert_back(&list, "lexus");
    list_print(&list);

    assert(list_find(&list, "audi") != NULL);
    assert(list_find(&list, "volvo") == NULL);

    assert(list_delete_value(&list, "audi") == 1);
    assert(list_find(&list, "audi") == NULL);
    list_print(&list);

    assert(list_delete_value(&list, "audi") == 0);

    list_free(&list);
    return 0;
}
