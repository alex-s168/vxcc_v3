#ifndef _SEXPR_H
#define _SEXPR_H

#include <stdio.h>

enum SNodeType {
  S_LIST,
  S_STRING,
  S_SYMBOL,
  S_INTEGER,
  S_FLOAT
};

struct SNode {
  struct SNode *next;
  enum SNodeType type;
  union {
    struct SNode *list;
    char *value;
  };
};

struct SNode *snode_parse(FILE *fp);
void snode_free(struct SNode *node);

void snode_print(struct SNode *node, FILE* out);
struct SNode* snode_expect(struct SNode *node, enum SNodeType ty);
struct SNode* snode_geti(struct SNode* node, size_t i);
struct SNode* snode_geti_expect(struct SNode* node, size_t i);

/** takes something like `(name "alex") (pass 123)`; note that next() is used on @list */
struct SNode* snode_kv_get(struct SNode* list, char const * key);

/** takes something like `(name "alex") (pass 123)`; note that next() is used on @list */
struct SNode* snode_kv_get_expect(struct SNode* list, char const * key);

/** 1 + how many next nodes there are */
size_t snode_num_nodes(struct SNode* sn);

struct SNode* snode_mk(enum SNodeType type, char const* value);
struct SNode* snode_mk_list(struct SNode* inner);

/** creates a (k v) */
struct SNode* snode_mk_kv(char const* key, struct SNode* val);

struct SNode* snode_tail(struct SNode* nd);

/** concatenates b to the tail of a and returns the beginning */
struct SNode* snode_cat(struct SNode* opta, struct SNode* b);

/** creates a (a b c d) */
#define snode_mk_listx(arr, arrlen, serial, serialarg) ({\
	struct SNode* li = NULL; \
	for (size_t i = 0; i < arrlen; i ++) { \
		li = snode_cat(li, serial((serialarg), arr[i])); \
	} \
	snode_mk_list(li); \
})

/** creates a (a b c d) */
#define snode_mk_listxli(begin, next, serial, serialarg) ({\
	struct SNode* li = NULL; \
	for (typeof(begin) i = (begin); i; i = (i->next)) { \
		li = snode_cat(li, serial((serialarg), i)); \
	} \
	snode_mk_list(li); \
})

#endif
