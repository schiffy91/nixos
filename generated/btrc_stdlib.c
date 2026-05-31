#include "btrc_stdlib.h"

/* ARC cascade-destroy tracking: avoid reading freed memory */
int __btrc_tracking = 0;
void** __btrc_destroyed = NULL;
int __btrc_destroyed_count = 0;
int __btrc_destroyed_cap = 0;
void __btrc_mark_destroyed(void* ptr) {
    if (__btrc_destroyed_count >= __btrc_destroyed_cap) {
        __btrc_destroyed_cap = __btrc_destroyed_cap ? __btrc_destroyed_cap * 2 : 256;
        __btrc_destroyed = (void**)__btrc_safe_realloc(__btrc_destroyed, sizeof(void*) * __btrc_destroyed_cap);
    }
    __btrc_destroyed[__btrc_destroyed_count++] = ptr;
}
int __btrc_is_destroyed(void* ptr) {
    for (int i = 0; i < __btrc_destroyed_count; i++)
        if (__btrc_destroyed[i] == ptr) return 1;
    return 0;
}

void UiNode_visit(UiNode* self, void (*fn)(void**)) {
    (void)self;
    (void)fn;
}

void btrc_Vector_string_init(btrc_Vector_string* self) {
    self->__rc = 1;
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

btrc_Vector_string* btrc_Vector_string_new(void) {
    btrc_Vector_string* self = ((btrc_Vector_string*)malloc(sizeof(btrc_Vector_string)));
    memset(self, 0, sizeof(btrc_Vector_string));
    btrc_Vector_string_init(self);
    return self;
}

void btrc_Vector_string_destroy(btrc_Vector_string* self) {
    free(self);
}

void btrc_Vector_string_push(btrc_Vector_string* self, char* val) {
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((char**)__btrc_safe_realloc(self->data, (sizeof(char*) * self->cap))));
    }
    (self->data[self->len] = val);
    (self->len++);
}

char* btrc_Vector_string_pop(btrc_Vector_string* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector pop from empty list\n");
        exit(1);
    }
    (self->len--);
    return self->data[self->len];
}

char* btrc_Vector_string_get(btrc_Vector_string* self, int i) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    return self->data[i];
}

void btrc_Vector_string_set(btrc_Vector_string* self, int i, char* val) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    (self->data[i] = val);
}

void btrc_Vector_string_free(btrc_Vector_string* self) {
    for (int i = 0; (i < self->len); (i++)) {
    }
    free(self->data);
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

void btrc_Vector_string_remove(btrc_Vector_string* self, int idx) {
    if ((idx < 0) || (idx >= self->len)) {
        fprintf(stderr, "Vector remove index out of bounds: %d (len=%d)\n", idx, self->len);
        exit(1);
    }
    for (int i = idx; (i < (self->len - 1)); (i++)) {
        (self->data[i] = self->data[(i + 1)]);
    }
    (self->len--);
}

void btrc_Vector_string_reverse(btrc_Vector_string* self) {
    for (int i = 0; (i < (self->len / 2)); (i++)) {
        char* tmp = self->data[i];
        (self->data[i] = self->data[((self->len - 1) - i)]);
        (self->data[((self->len - 1) - i)] = tmp);
    }
}

btrc_Vector_string* btrc_Vector_string_reversed(btrc_Vector_string* self) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        btrc_Vector_string_push(result, self->data[i]);
    }
    return result;
}

void btrc_Vector_string_swap(btrc_Vector_string* self, int i, int j) {
    if ((((i < 0) || (i >= self->len)) || (j < 0)) || (j >= self->len)) {
        fprintf(stderr, "Vector swap index out of bounds\n");
        exit(1);
    }
    char* tmp = self->data[i];
    (self->data[i] = self->data[j]);
    (self->data[j] = tmp);
}

void btrc_Vector_string_clear(btrc_Vector_string* self) {
    for (int i = 0; (i < self->len); (i++)) {
    }
    (self->len = 0);
}

void btrc_Vector_string_fill(btrc_Vector_string* self, char* val) {
    for (int i = 0; (i < self->len); (i++)) {
        (self->data[i] = val);
    }
}

int btrc_Vector_string_size(btrc_Vector_string* self) {
    return self->len;
}

bool btrc_Vector_string_isEmpty(btrc_Vector_string* self) {
    return (self->len == 0);
}

char* btrc_Vector_string_first(btrc_Vector_string* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.first() called on empty list\n");
        exit(1);
    }
    return self->data[0];
}

char* btrc_Vector_string_last(btrc_Vector_string* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.last() called on empty list\n");
        exit(1);
    }
    return self->data[(self->len - 1)];
}

btrc_Vector_string* btrc_Vector_string_slice(btrc_Vector_string* self, int start, int end) {
    if (start < 0) {
        (start = (self->len + start));
    }
    if (end < 0) {
        (end = (self->len + end));
    }
    if (start < 0) {
        (start = 0);
    }
    if (end > self->len) {
        (end = self->len);
    }
    btrc_Vector_string* result = btrc_Vector_string_new();
    for (int i = start; (i < end); (i++)) {
        btrc_Vector_string_push(result, self->data[i]);
    }
    return result;
}

btrc_Vector_string* btrc_Vector_string_take(btrc_Vector_string* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_string_slice(self, 0, n);
}

btrc_Vector_string* btrc_Vector_string_drop(btrc_Vector_string* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_string_slice(self, n, self->len);
}

void btrc_Vector_string_extend(btrc_Vector_string* self, btrc_Vector_string* other) {
    for (int i = 0; (i < other->len); (i++)) {
        btrc_Vector_string_push(self, other->data[i]);
    }
}

void btrc_Vector_string_insert(btrc_Vector_string* self, int idx, char* val) {
    if ((idx < 0) || (idx > self->len)) {
        fprintf(stderr, "Vector insert index out of bounds: %d (size %d)\n", idx, self->len);
        exit(1);
    }
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((char**)__btrc_safe_realloc(self->data, (sizeof(char*) * self->cap))));
    }
    for (int i = self->len; (i > idx); (i--)) {
        (self->data[i] = self->data[(i - 1)]);
    }
    (self->data[idx] = val);
    (self->len++);
}

bool btrc_Vector_string_contains(btrc_Vector_string* self, char* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return true;
        }
    }
    return false;
}

int btrc_Vector_string_indexOf(btrc_Vector_string* self, char* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

int btrc_Vector_string_lastIndexOf(btrc_Vector_string* self, char* val) {
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

int btrc_Vector_string_count(btrc_Vector_string* self, char* val) {
    int c = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            (c++);
        }
    }
    return c;
}

void btrc_Vector_string_removeAll(btrc_Vector_string* self, char* val) {
    int j = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (!__btrc_eq(self->data[i], val)) {
            (self->data[j] = self->data[i]);
            (j++);
        }
    }
    (self->len = j);
}

btrc_Vector_string* btrc_Vector_string_distinct(btrc_Vector_string* self) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (!btrc_Vector_string_contains(result, self->data[i])) {
            btrc_Vector_string_push(result, self->data[i]);
        }
    }
    return result;
}

void btrc_Vector_string_sort(btrc_Vector_string* self) {
    for (int i = 1; (i < self->len); (i++)) {
        char* key = self->data[i];
        int j = (i - 1);
        while ((j >= 0) && __btrc_lt(key, self->data[j])) {
            (self->data[(j + 1)] = self->data[j]);
            (j = (j - 1));
        }
        (self->data[(j + 1)] = key);
    }
}

btrc_Vector_string* btrc_Vector_string_sorted(btrc_Vector_string* self) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_string_push(result, self->data[i]);
    }
    btrc_Vector_string_sort(result);
    return result;
}

char* btrc_Vector_string_min(btrc_Vector_string* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector min on empty list\n");
        exit(1);
    }
    char* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_lt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

char* btrc_Vector_string_max(btrc_Vector_string* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector max on empty list\n");
        exit(1);
    }
    char* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_gt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

char* btrc_Vector_string_join(btrc_Vector_string* self, char* sep) {
    int total = 0;
    int sep_len = ((int)strlen(sep));
    for (int i = 0; (i < self->len); (i++)) {
        (total = (total + ((int)strlen(self->data[i]))));
        if (i < (self->len - 1)) {
            (total = (total + sep_len));
        }
    }
    char* result = ((char*)malloc((total + 1)));
    int pos = 0;
    for (int i = 0; (i < self->len); (i++)) {
        int slen = ((int)strlen(self->data[i]));
        memcpy((result + pos), self->data[i], slen);
        (pos = (pos + slen));
        if (i < (self->len - 1)) {
            memcpy((result + pos), sep, sep_len);
            (pos = (pos + sep_len));
        }
    }
    (result[pos] = '\0');
    return result;
}

char* btrc_Vector_string_joinToString(btrc_Vector_string* self, char* sep) {
    return btrc_Vector_string_join(self, sep);
}

btrc_Vector_string* btrc_Vector_string_filter(btrc_Vector_string* self, __btrc_fn_bool_string pred) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            btrc_Vector_string_push(result, self->data[i]);
        }
    }
    return result;
}

int btrc_Vector_string_findIndex(btrc_Vector_string* self, __btrc_fn_bool_string pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return i;
        }
    }
    return (-1);
}

void btrc_Vector_string_forEach(btrc_Vector_string* self, __btrc_fn_void_string fn) {
    for (int i = 0; (i < self->len); (i++)) {
        fn(self->data[i]);
    }
}

btrc_Vector_string* btrc_Vector_string_map(btrc_Vector_string* self, __btrc_fn_string_string fn) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_string_push(result, fn(self->data[i]));
    }
    return result;
}

bool btrc_Vector_string_any(btrc_Vector_string* self, __btrc_fn_bool_string pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return true;
        }
    }
    return false;
}

bool btrc_Vector_string_all(btrc_Vector_string* self, __btrc_fn_bool_string pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (!pred(self->data[i])) {
            return false;
        }
    }
    return true;
}

char* btrc_Vector_string_reduce(btrc_Vector_string* self, char* init, __btrc_fn_string_string_string fn) {
    char* acc = init;
    for (int i = 0; (i < self->len); (i++)) {
        (acc = fn(acc, self->data[i]));
    }
    return acc;
}

btrc_Vector_string* btrc_Vector_string_copy(btrc_Vector_string* self) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_string_push(result, self->data[i]);
    }
    return result;
}

void btrc_Vector_string_removeAt(btrc_Vector_string* self, int idx) {
    btrc_Vector_string_remove(self, idx);
}

int btrc_Vector_string_iterLen(btrc_Vector_string* self) {
    return self->len;
}

char* btrc_Vector_string_iterGet(btrc_Vector_string* self, int i) {
    return self->data[i];
}

void btrc_Vector_UiNode_init(btrc_Vector_UiNode* self) {
    self->__rc = 1;
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

btrc_Vector_UiNode* btrc_Vector_UiNode_new(void) {
    btrc_Vector_UiNode* self = ((btrc_Vector_UiNode*)malloc(sizeof(btrc_Vector_UiNode)));
    memset(self, 0, sizeof(btrc_Vector_UiNode));
    btrc_Vector_UiNode_init(self);
    return self;
}

void btrc_Vector_UiNode_destroy(btrc_Vector_UiNode* self) {
    free(self);
}

void btrc_Vector_UiNode_push(btrc_Vector_UiNode* self, UiNode* val) {
    (val->__rc++);
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((UiNode**)__btrc_safe_realloc(self->data, (sizeof(UiNode*) * self->cap))));
    }
    (self->data[self->len] = val);
    (self->len++);
}

UiNode* btrc_Vector_UiNode_pop(btrc_Vector_UiNode* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector pop from empty list\n");
        exit(1);
    }
    (self->len--);
    return self->data[self->len];
}

UiNode* btrc_Vector_UiNode_get(btrc_Vector_UiNode* self, int i) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    return self->data[i];
}

void btrc_Vector_UiNode_set(btrc_Vector_UiNode* self, int i, UiNode* val) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    if (self->data[i]) {
        if ((--self->data[i]->__rc) <= 0) {
            UiNode_destroy(self->data[i]);
        }
    }
    (val->__rc++);
    (self->data[i] = val);
}

void btrc_Vector_UiNode_free(btrc_Vector_UiNode* self) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                UiNode_destroy(self->data[i]);
            }
        }
    }
    free(self->data);
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

void btrc_Vector_UiNode_remove(btrc_Vector_UiNode* self, int idx) {
    if ((idx < 0) || (idx >= self->len)) {
        fprintf(stderr, "Vector remove index out of bounds: %d (len=%d)\n", idx, self->len);
        exit(1);
    }
    if (self->data[idx]) {
        if ((--self->data[idx]->__rc) <= 0) {
            UiNode_destroy(self->data[idx]);
        }
    }
    for (int i = idx; (i < (self->len - 1)); (i++)) {
        (self->data[i] = self->data[(i + 1)]);
    }
    (self->len--);
}

void btrc_Vector_UiNode_reverse(btrc_Vector_UiNode* self) {
    for (int i = 0; (i < (self->len / 2)); (i++)) {
        UiNode* tmp = self->data[i];
        (self->data[i] = self->data[((self->len - 1) - i)]);
        (self->data[((self->len - 1) - i)] = tmp);
    }
}

btrc_Vector_UiNode* btrc_Vector_UiNode_reversed(btrc_Vector_UiNode* self) {
    btrc_Vector_UiNode* result = btrc_Vector_UiNode_new();
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        btrc_Vector_UiNode_push(result, self->data[i]);
    }
    return result;
}

void btrc_Vector_UiNode_swap(btrc_Vector_UiNode* self, int i, int j) {
    if ((((i < 0) || (i >= self->len)) || (j < 0)) || (j >= self->len)) {
        fprintf(stderr, "Vector swap index out of bounds\n");
        exit(1);
    }
    UiNode* tmp = self->data[i];
    (self->data[i] = self->data[j]);
    (self->data[j] = tmp);
}

void btrc_Vector_UiNode_clear(btrc_Vector_UiNode* self) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                UiNode_destroy(self->data[i]);
            }
        }
    }
    (self->len = 0);
}

void btrc_Vector_UiNode_fill(btrc_Vector_UiNode* self, UiNode* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                UiNode_destroy(self->data[i]);
            }
        }
        (val->__rc++);
        (self->data[i] = val);
    }
}

int btrc_Vector_UiNode_size(btrc_Vector_UiNode* self) {
    return self->len;
}

bool btrc_Vector_UiNode_isEmpty(btrc_Vector_UiNode* self) {
    return (self->len == 0);
}

UiNode* btrc_Vector_UiNode_first(btrc_Vector_UiNode* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.first() called on empty list\n");
        exit(1);
    }
    return self->data[0];
}

UiNode* btrc_Vector_UiNode_last(btrc_Vector_UiNode* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.last() called on empty list\n");
        exit(1);
    }
    return self->data[(self->len - 1)];
}

btrc_Vector_UiNode* btrc_Vector_UiNode_slice(btrc_Vector_UiNode* self, int start, int end) {
    if (start < 0) {
        (start = (self->len + start));
    }
    if (end < 0) {
        (end = (self->len + end));
    }
    if (start < 0) {
        (start = 0);
    }
    if (end > self->len) {
        (end = self->len);
    }
    btrc_Vector_UiNode* result = btrc_Vector_UiNode_new();
    for (int i = start; (i < end); (i++)) {
        btrc_Vector_UiNode_push(result, self->data[i]);
    }
    return result;
}

btrc_Vector_UiNode* btrc_Vector_UiNode_take(btrc_Vector_UiNode* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_UiNode_slice(self, 0, n);
}

btrc_Vector_UiNode* btrc_Vector_UiNode_drop(btrc_Vector_UiNode* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_UiNode_slice(self, n, self->len);
}

void btrc_Vector_UiNode_extend(btrc_Vector_UiNode* self, btrc_Vector_UiNode* other) {
    for (int i = 0; (i < other->len); (i++)) {
        btrc_Vector_UiNode_push(self, other->data[i]);
    }
}

void btrc_Vector_UiNode_insert(btrc_Vector_UiNode* self, int idx, UiNode* val) {
    if ((idx < 0) || (idx > self->len)) {
        fprintf(stderr, "Vector insert index out of bounds: %d (size %d)\n", idx, self->len);
        exit(1);
    }
    (val->__rc++);
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((UiNode**)__btrc_safe_realloc(self->data, (sizeof(UiNode*) * self->cap))));
    }
    for (int i = self->len; (i > idx); (i--)) {
        (self->data[i] = self->data[(i - 1)]);
    }
    (self->data[idx] = val);
    (self->len++);
}

bool btrc_Vector_UiNode_contains(btrc_Vector_UiNode* self, UiNode* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return true;
        }
    }
    return false;
}

int btrc_Vector_UiNode_indexOf(btrc_Vector_UiNode* self, UiNode* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

int btrc_Vector_UiNode_lastIndexOf(btrc_Vector_UiNode* self, UiNode* val) {
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

int btrc_Vector_UiNode_count(btrc_Vector_UiNode* self, UiNode* val) {
    int c = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            (c++);
        }
    }
    return c;
}

void btrc_Vector_UiNode_removeAll(btrc_Vector_UiNode* self, UiNode* val) {
    int j = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (!__btrc_eq(self->data[i], val)) {
            (self->data[j] = self->data[i]);
            (j++);
        } else if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                UiNode_destroy(self->data[i]);
            }
        }
    }
    (self->len = j);
}

btrc_Vector_UiNode* btrc_Vector_UiNode_distinct(btrc_Vector_UiNode* self) {
    btrc_Vector_UiNode* result = btrc_Vector_UiNode_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (!btrc_Vector_UiNode_contains(result, self->data[i])) {
            btrc_Vector_UiNode_push(result, self->data[i]);
        }
    }
    return result;
}

void btrc_Vector_UiNode_sort(btrc_Vector_UiNode* self) {
    for (int i = 1; (i < self->len); (i++)) {
        UiNode* key = self->data[i];
        int j = (i - 1);
        while ((j >= 0) && __btrc_lt(key, self->data[j])) {
            (self->data[(j + 1)] = self->data[j]);
            (j = (j - 1));
        }
        (self->data[(j + 1)] = key);
    }
}

btrc_Vector_UiNode* btrc_Vector_UiNode_sorted(btrc_Vector_UiNode* self) {
    btrc_Vector_UiNode* result = btrc_Vector_UiNode_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_UiNode_push(result, self->data[i]);
    }
    btrc_Vector_UiNode_sort(result);
    return result;
}

UiNode* btrc_Vector_UiNode_min(btrc_Vector_UiNode* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector min on empty list\n");
        exit(1);
    }
    UiNode* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_lt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

UiNode* btrc_Vector_UiNode_max(btrc_Vector_UiNode* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector max on empty list\n");
        exit(1);
    }
    UiNode* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_gt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

btrc_Vector_UiNode* btrc_Vector_UiNode_filter(btrc_Vector_UiNode* self, __btrc_fn_bool_UiNode pred) {
    btrc_Vector_UiNode* result = btrc_Vector_UiNode_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            btrc_Vector_UiNode_push(result, self->data[i]);
        }
    }
    return result;
}

int btrc_Vector_UiNode_findIndex(btrc_Vector_UiNode* self, __btrc_fn_bool_UiNode pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return i;
        }
    }
    return (-1);
}

void btrc_Vector_UiNode_forEach(btrc_Vector_UiNode* self, __btrc_fn_void_UiNode fn) {
    for (int i = 0; (i < self->len); (i++)) {
        fn(self->data[i]);
    }
}

btrc_Vector_UiNode* btrc_Vector_UiNode_map(btrc_Vector_UiNode* self, __btrc_fn_UiNode_UiNode fn) {
    btrc_Vector_UiNode* result = btrc_Vector_UiNode_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_UiNode_push(result, fn(self->data[i]));
    }
    return result;
}

bool btrc_Vector_UiNode_any(btrc_Vector_UiNode* self, __btrc_fn_bool_UiNode pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return true;
        }
    }
    return false;
}

bool btrc_Vector_UiNode_all(btrc_Vector_UiNode* self, __btrc_fn_bool_UiNode pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (!pred(self->data[i])) {
            return false;
        }
    }
    return true;
}

UiNode* btrc_Vector_UiNode_reduce(btrc_Vector_UiNode* self, UiNode* init, __btrc_fn_UiNode_UiNode_UiNode fn) {
    UiNode* acc = init;
    for (int i = 0; (i < self->len); (i++)) {
        (acc = fn(acc, self->data[i]));
    }
    return acc;
}

btrc_Vector_UiNode* btrc_Vector_UiNode_copy(btrc_Vector_UiNode* self) {
    btrc_Vector_UiNode* result = btrc_Vector_UiNode_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_UiNode_push(result, self->data[i]);
    }
    return result;
}

void btrc_Vector_UiNode_removeAt(btrc_Vector_UiNode* self, int idx) {
    btrc_Vector_UiNode_remove(self, idx);
}

int btrc_Vector_UiNode_iterLen(btrc_Vector_UiNode* self) {
    return self->len;
}

UiNode* btrc_Vector_UiNode_iterGet(btrc_Vector_UiNode* self, int i) {
    return self->data[i];
}

void btrc_Vector_TrayItem_init(btrc_Vector_TrayItem* self) {
    self->__rc = 1;
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

btrc_Vector_TrayItem* btrc_Vector_TrayItem_new(void) {
    btrc_Vector_TrayItem* self = ((btrc_Vector_TrayItem*)malloc(sizeof(btrc_Vector_TrayItem)));
    memset(self, 0, sizeof(btrc_Vector_TrayItem));
    btrc_Vector_TrayItem_init(self);
    return self;
}

void btrc_Vector_TrayItem_destroy(btrc_Vector_TrayItem* self) {
    free(self);
}

void btrc_Vector_TrayItem_push(btrc_Vector_TrayItem* self, TrayItem* val) {
    (val->__rc++);
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((TrayItem**)__btrc_safe_realloc(self->data, (sizeof(TrayItem*) * self->cap))));
    }
    (self->data[self->len] = val);
    (self->len++);
}

TrayItem* btrc_Vector_TrayItem_pop(btrc_Vector_TrayItem* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector pop from empty list\n");
        exit(1);
    }
    (self->len--);
    return self->data[self->len];
}

TrayItem* btrc_Vector_TrayItem_get(btrc_Vector_TrayItem* self, int i) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    return self->data[i];
}

void btrc_Vector_TrayItem_set(btrc_Vector_TrayItem* self, int i, TrayItem* val) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    if (self->data[i]) {
        if ((--self->data[i]->__rc) <= 0) {
            TrayItem_destroy(self->data[i]);
        }
    }
    (val->__rc++);
    (self->data[i] = val);
}

void btrc_Vector_TrayItem_free(btrc_Vector_TrayItem* self) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                TrayItem_destroy(self->data[i]);
            }
        }
    }
    free(self->data);
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

void btrc_Vector_TrayItem_remove(btrc_Vector_TrayItem* self, int idx) {
    if ((idx < 0) || (idx >= self->len)) {
        fprintf(stderr, "Vector remove index out of bounds: %d (len=%d)\n", idx, self->len);
        exit(1);
    }
    if (self->data[idx]) {
        if ((--self->data[idx]->__rc) <= 0) {
            TrayItem_destroy(self->data[idx]);
        }
    }
    for (int i = idx; (i < (self->len - 1)); (i++)) {
        (self->data[i] = self->data[(i + 1)]);
    }
    (self->len--);
}

void btrc_Vector_TrayItem_reverse(btrc_Vector_TrayItem* self) {
    for (int i = 0; (i < (self->len / 2)); (i++)) {
        TrayItem* tmp = self->data[i];
        (self->data[i] = self->data[((self->len - 1) - i)]);
        (self->data[((self->len - 1) - i)] = tmp);
    }
}

btrc_Vector_TrayItem* btrc_Vector_TrayItem_reversed(btrc_Vector_TrayItem* self) {
    btrc_Vector_TrayItem* result = btrc_Vector_TrayItem_new();
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        btrc_Vector_TrayItem_push(result, self->data[i]);
    }
    return result;
}

void btrc_Vector_TrayItem_swap(btrc_Vector_TrayItem* self, int i, int j) {
    if ((((i < 0) || (i >= self->len)) || (j < 0)) || (j >= self->len)) {
        fprintf(stderr, "Vector swap index out of bounds\n");
        exit(1);
    }
    TrayItem* tmp = self->data[i];
    (self->data[i] = self->data[j]);
    (self->data[j] = tmp);
}

void btrc_Vector_TrayItem_clear(btrc_Vector_TrayItem* self) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                TrayItem_destroy(self->data[i]);
            }
        }
    }
    (self->len = 0);
}

void btrc_Vector_TrayItem_fill(btrc_Vector_TrayItem* self, TrayItem* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                TrayItem_destroy(self->data[i]);
            }
        }
        (val->__rc++);
        (self->data[i] = val);
    }
}

int btrc_Vector_TrayItem_size(btrc_Vector_TrayItem* self) {
    return self->len;
}

bool btrc_Vector_TrayItem_isEmpty(btrc_Vector_TrayItem* self) {
    return (self->len == 0);
}

TrayItem* btrc_Vector_TrayItem_first(btrc_Vector_TrayItem* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.first() called on empty list\n");
        exit(1);
    }
    return self->data[0];
}

TrayItem* btrc_Vector_TrayItem_last(btrc_Vector_TrayItem* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.last() called on empty list\n");
        exit(1);
    }
    return self->data[(self->len - 1)];
}

btrc_Vector_TrayItem* btrc_Vector_TrayItem_slice(btrc_Vector_TrayItem* self, int start, int end) {
    if (start < 0) {
        (start = (self->len + start));
    }
    if (end < 0) {
        (end = (self->len + end));
    }
    if (start < 0) {
        (start = 0);
    }
    if (end > self->len) {
        (end = self->len);
    }
    btrc_Vector_TrayItem* result = btrc_Vector_TrayItem_new();
    for (int i = start; (i < end); (i++)) {
        btrc_Vector_TrayItem_push(result, self->data[i]);
    }
    return result;
}

btrc_Vector_TrayItem* btrc_Vector_TrayItem_take(btrc_Vector_TrayItem* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_TrayItem_slice(self, 0, n);
}

btrc_Vector_TrayItem* btrc_Vector_TrayItem_drop(btrc_Vector_TrayItem* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_TrayItem_slice(self, n, self->len);
}

void btrc_Vector_TrayItem_extend(btrc_Vector_TrayItem* self, btrc_Vector_TrayItem* other) {
    for (int i = 0; (i < other->len); (i++)) {
        btrc_Vector_TrayItem_push(self, other->data[i]);
    }
}

void btrc_Vector_TrayItem_insert(btrc_Vector_TrayItem* self, int idx, TrayItem* val) {
    if ((idx < 0) || (idx > self->len)) {
        fprintf(stderr, "Vector insert index out of bounds: %d (size %d)\n", idx, self->len);
        exit(1);
    }
    (val->__rc++);
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((TrayItem**)__btrc_safe_realloc(self->data, (sizeof(TrayItem*) * self->cap))));
    }
    for (int i = self->len; (i > idx); (i--)) {
        (self->data[i] = self->data[(i - 1)]);
    }
    (self->data[idx] = val);
    (self->len++);
}

bool btrc_Vector_TrayItem_contains(btrc_Vector_TrayItem* self, TrayItem* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return true;
        }
    }
    return false;
}

int btrc_Vector_TrayItem_indexOf(btrc_Vector_TrayItem* self, TrayItem* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

int btrc_Vector_TrayItem_lastIndexOf(btrc_Vector_TrayItem* self, TrayItem* val) {
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

int btrc_Vector_TrayItem_count(btrc_Vector_TrayItem* self, TrayItem* val) {
    int c = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            (c++);
        }
    }
    return c;
}

void btrc_Vector_TrayItem_removeAll(btrc_Vector_TrayItem* self, TrayItem* val) {
    int j = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (!__btrc_eq(self->data[i], val)) {
            (self->data[j] = self->data[i]);
            (j++);
        } else if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                TrayItem_destroy(self->data[i]);
            }
        }
    }
    (self->len = j);
}

btrc_Vector_TrayItem* btrc_Vector_TrayItem_distinct(btrc_Vector_TrayItem* self) {
    btrc_Vector_TrayItem* result = btrc_Vector_TrayItem_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (!btrc_Vector_TrayItem_contains(result, self->data[i])) {
            btrc_Vector_TrayItem_push(result, self->data[i]);
        }
    }
    return result;
}

void btrc_Vector_TrayItem_sort(btrc_Vector_TrayItem* self) {
    for (int i = 1; (i < self->len); (i++)) {
        TrayItem* key = self->data[i];
        int j = (i - 1);
        while ((j >= 0) && __btrc_lt(key, self->data[j])) {
            (self->data[(j + 1)] = self->data[j]);
            (j = (j - 1));
        }
        (self->data[(j + 1)] = key);
    }
}

btrc_Vector_TrayItem* btrc_Vector_TrayItem_sorted(btrc_Vector_TrayItem* self) {
    btrc_Vector_TrayItem* result = btrc_Vector_TrayItem_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_TrayItem_push(result, self->data[i]);
    }
    btrc_Vector_TrayItem_sort(result);
    return result;
}

TrayItem* btrc_Vector_TrayItem_min(btrc_Vector_TrayItem* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector min on empty list\n");
        exit(1);
    }
    TrayItem* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_lt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

TrayItem* btrc_Vector_TrayItem_max(btrc_Vector_TrayItem* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector max on empty list\n");
        exit(1);
    }
    TrayItem* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_gt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

btrc_Vector_TrayItem* btrc_Vector_TrayItem_filter(btrc_Vector_TrayItem* self, __btrc_fn_bool_TrayItem pred) {
    btrc_Vector_TrayItem* result = btrc_Vector_TrayItem_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            btrc_Vector_TrayItem_push(result, self->data[i]);
        }
    }
    return result;
}

int btrc_Vector_TrayItem_findIndex(btrc_Vector_TrayItem* self, __btrc_fn_bool_TrayItem pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return i;
        }
    }
    return (-1);
}

void btrc_Vector_TrayItem_forEach(btrc_Vector_TrayItem* self, __btrc_fn_void_TrayItem fn) {
    for (int i = 0; (i < self->len); (i++)) {
        fn(self->data[i]);
    }
}

btrc_Vector_TrayItem* btrc_Vector_TrayItem_map(btrc_Vector_TrayItem* self, __btrc_fn_TrayItem_TrayItem fn) {
    btrc_Vector_TrayItem* result = btrc_Vector_TrayItem_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_TrayItem_push(result, fn(self->data[i]));
    }
    return result;
}

bool btrc_Vector_TrayItem_any(btrc_Vector_TrayItem* self, __btrc_fn_bool_TrayItem pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return true;
        }
    }
    return false;
}

bool btrc_Vector_TrayItem_all(btrc_Vector_TrayItem* self, __btrc_fn_bool_TrayItem pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (!pred(self->data[i])) {
            return false;
        }
    }
    return true;
}

TrayItem* btrc_Vector_TrayItem_reduce(btrc_Vector_TrayItem* self, TrayItem* init, __btrc_fn_TrayItem_TrayItem_TrayItem fn) {
    TrayItem* acc = init;
    for (int i = 0; (i < self->len); (i++)) {
        (acc = fn(acc, self->data[i]));
    }
    return acc;
}

btrc_Vector_TrayItem* btrc_Vector_TrayItem_copy(btrc_Vector_TrayItem* self) {
    btrc_Vector_TrayItem* result = btrc_Vector_TrayItem_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_TrayItem_push(result, self->data[i]);
    }
    return result;
}

void btrc_Vector_TrayItem_removeAt(btrc_Vector_TrayItem* self, int idx) {
    btrc_Vector_TrayItem_remove(self, idx);
}

int btrc_Vector_TrayItem_iterLen(btrc_Vector_TrayItem* self) {
    return self->len;
}

TrayItem* btrc_Vector_TrayItem_iterGet(btrc_Vector_TrayItem* self, int i) {
    return self->data[i];
}

void btrc_Vector_bool_init(btrc_Vector_bool* self) {
    self->__rc = 1;
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

btrc_Vector_bool* btrc_Vector_bool_new(void) {
    btrc_Vector_bool* self = ((btrc_Vector_bool*)malloc(sizeof(btrc_Vector_bool)));
    memset(self, 0, sizeof(btrc_Vector_bool));
    btrc_Vector_bool_init(self);
    return self;
}

void btrc_Vector_bool_destroy(btrc_Vector_bool* self) {
    free(self);
}

void btrc_Vector_bool_push(btrc_Vector_bool* self, bool val) {
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((bool*)__btrc_safe_realloc(self->data, (sizeof(bool) * self->cap))));
    }
    (self->data[self->len] = val);
    (self->len++);
}

bool btrc_Vector_bool_pop(btrc_Vector_bool* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector pop from empty list\n");
        exit(1);
    }
    (self->len--);
    return self->data[self->len];
}

bool btrc_Vector_bool_get(btrc_Vector_bool* self, int i) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    return self->data[i];
}

void btrc_Vector_bool_set(btrc_Vector_bool* self, int i, bool val) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    (self->data[i] = val);
}

void btrc_Vector_bool_free(btrc_Vector_bool* self) {
    for (int i = 0; (i < self->len); (i++)) {
    }
    free(self->data);
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

void btrc_Vector_bool_remove(btrc_Vector_bool* self, int idx) {
    if ((idx < 0) || (idx >= self->len)) {
        fprintf(stderr, "Vector remove index out of bounds: %d (len=%d)\n", idx, self->len);
        exit(1);
    }
    for (int i = idx; (i < (self->len - 1)); (i++)) {
        (self->data[i] = self->data[(i + 1)]);
    }
    (self->len--);
}

void btrc_Vector_bool_reverse(btrc_Vector_bool* self) {
    for (int i = 0; (i < (self->len / 2)); (i++)) {
        bool tmp = self->data[i];
        (self->data[i] = self->data[((self->len - 1) - i)]);
        (self->data[((self->len - 1) - i)] = tmp);
    }
}

btrc_Vector_bool* btrc_Vector_bool_reversed(btrc_Vector_bool* self) {
    btrc_Vector_bool* result = btrc_Vector_bool_new();
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        btrc_Vector_bool_push(result, self->data[i]);
    }
    return result;
}

void btrc_Vector_bool_swap(btrc_Vector_bool* self, int i, int j) {
    if ((((i < 0) || (i >= self->len)) || (j < 0)) || (j >= self->len)) {
        fprintf(stderr, "Vector swap index out of bounds\n");
        exit(1);
    }
    bool tmp = self->data[i];
    (self->data[i] = self->data[j]);
    (self->data[j] = tmp);
}

void btrc_Vector_bool_clear(btrc_Vector_bool* self) {
    for (int i = 0; (i < self->len); (i++)) {
    }
    (self->len = 0);
}

void btrc_Vector_bool_fill(btrc_Vector_bool* self, bool val) {
    for (int i = 0; (i < self->len); (i++)) {
        (self->data[i] = val);
    }
}

int btrc_Vector_bool_size(btrc_Vector_bool* self) {
    return self->len;
}

bool btrc_Vector_bool_isEmpty(btrc_Vector_bool* self) {
    return (self->len == 0);
}

bool btrc_Vector_bool_first(btrc_Vector_bool* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.first() called on empty list\n");
        exit(1);
    }
    return self->data[0];
}

bool btrc_Vector_bool_last(btrc_Vector_bool* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.last() called on empty list\n");
        exit(1);
    }
    return self->data[(self->len - 1)];
}

btrc_Vector_bool* btrc_Vector_bool_slice(btrc_Vector_bool* self, int start, int end) {
    if (start < 0) {
        (start = (self->len + start));
    }
    if (end < 0) {
        (end = (self->len + end));
    }
    if (start < 0) {
        (start = 0);
    }
    if (end > self->len) {
        (end = self->len);
    }
    btrc_Vector_bool* result = btrc_Vector_bool_new();
    for (int i = start; (i < end); (i++)) {
        btrc_Vector_bool_push(result, self->data[i]);
    }
    return result;
}

btrc_Vector_bool* btrc_Vector_bool_take(btrc_Vector_bool* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_bool_slice(self, 0, n);
}

btrc_Vector_bool* btrc_Vector_bool_drop(btrc_Vector_bool* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_bool_slice(self, n, self->len);
}

void btrc_Vector_bool_extend(btrc_Vector_bool* self, btrc_Vector_bool* other) {
    for (int i = 0; (i < other->len); (i++)) {
        btrc_Vector_bool_push(self, other->data[i]);
    }
}

void btrc_Vector_bool_insert(btrc_Vector_bool* self, int idx, bool val) {
    if ((idx < 0) || (idx > self->len)) {
        fprintf(stderr, "Vector insert index out of bounds: %d (size %d)\n", idx, self->len);
        exit(1);
    }
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((bool*)__btrc_safe_realloc(self->data, (sizeof(bool) * self->cap))));
    }
    for (int i = self->len; (i > idx); (i--)) {
        (self->data[i] = self->data[(i - 1)]);
    }
    (self->data[idx] = val);
    (self->len++);
}

bool btrc_Vector_bool_contains(btrc_Vector_bool* self, bool val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return true;
        }
    }
    return false;
}

int btrc_Vector_bool_indexOf(btrc_Vector_bool* self, bool val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

int btrc_Vector_bool_lastIndexOf(btrc_Vector_bool* self, bool val) {
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

int btrc_Vector_bool_count(btrc_Vector_bool* self, bool val) {
    int c = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            (c++);
        }
    }
    return c;
}

void btrc_Vector_bool_removeAll(btrc_Vector_bool* self, bool val) {
    int j = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (!__btrc_eq(self->data[i], val)) {
            (self->data[j] = self->data[i]);
            (j++);
        }
    }
    (self->len = j);
}

btrc_Vector_bool* btrc_Vector_bool_distinct(btrc_Vector_bool* self) {
    btrc_Vector_bool* result = btrc_Vector_bool_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (!btrc_Vector_bool_contains(result, self->data[i])) {
            btrc_Vector_bool_push(result, self->data[i]);
        }
    }
    return result;
}

void btrc_Vector_bool_sort(btrc_Vector_bool* self) {
    for (int i = 1; (i < self->len); (i++)) {
        bool key = self->data[i];
        int j = (i - 1);
        while ((j >= 0) && __btrc_lt(key, self->data[j])) {
            (self->data[(j + 1)] = self->data[j]);
            (j = (j - 1));
        }
        (self->data[(j + 1)] = key);
    }
}

btrc_Vector_bool* btrc_Vector_bool_sorted(btrc_Vector_bool* self) {
    btrc_Vector_bool* result = btrc_Vector_bool_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_bool_push(result, self->data[i]);
    }
    btrc_Vector_bool_sort(result);
    return result;
}

bool btrc_Vector_bool_min(btrc_Vector_bool* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector min on empty list\n");
        exit(1);
    }
    bool m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_lt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

bool btrc_Vector_bool_max(btrc_Vector_bool* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector max on empty list\n");
        exit(1);
    }
    bool m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_gt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

bool btrc_Vector_bool_sum(btrc_Vector_bool* self) {
    bool s = ((bool)0);
    for (int i = 0; (i < self->len); (i++)) {
        (s = (s + self->data[i]));
    }
    return s;
}

btrc_Vector_bool* btrc_Vector_bool_filter(btrc_Vector_bool* self, __btrc_fn_bool_bool pred) {
    btrc_Vector_bool* result = btrc_Vector_bool_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            btrc_Vector_bool_push(result, self->data[i]);
        }
    }
    return result;
}

int btrc_Vector_bool_findIndex(btrc_Vector_bool* self, __btrc_fn_bool_bool pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return i;
        }
    }
    return (-1);
}

void btrc_Vector_bool_forEach(btrc_Vector_bool* self, __btrc_fn_void_bool fn) {
    for (int i = 0; (i < self->len); (i++)) {
        fn(self->data[i]);
    }
}

btrc_Vector_bool* btrc_Vector_bool_map(btrc_Vector_bool* self, __btrc_fn_bool_bool fn) {
    btrc_Vector_bool* result = btrc_Vector_bool_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_bool_push(result, fn(self->data[i]));
    }
    return result;
}

bool btrc_Vector_bool_any(btrc_Vector_bool* self, __btrc_fn_bool_bool pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return true;
        }
    }
    return false;
}

bool btrc_Vector_bool_all(btrc_Vector_bool* self, __btrc_fn_bool_bool pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (!pred(self->data[i])) {
            return false;
        }
    }
    return true;
}

bool btrc_Vector_bool_reduce(btrc_Vector_bool* self, bool init, __btrc_fn_bool_bool_bool fn) {
    bool acc = init;
    for (int i = 0; (i < self->len); (i++)) {
        (acc = fn(acc, self->data[i]));
    }
    return acc;
}

btrc_Vector_bool* btrc_Vector_bool_copy(btrc_Vector_bool* self) {
    btrc_Vector_bool* result = btrc_Vector_bool_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_bool_push(result, self->data[i]);
    }
    return result;
}

void btrc_Vector_bool_removeAt(btrc_Vector_bool* self, int idx) {
    btrc_Vector_bool_remove(self, idx);
}

int btrc_Vector_bool_iterLen(btrc_Vector_bool* self) {
    return self->len;
}

bool btrc_Vector_bool_iterGet(btrc_Vector_bool* self, int i) {
    return self->data[i];
}

void btrc_Vector_int_init(btrc_Vector_int* self) {
    self->__rc = 1;
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

btrc_Vector_int* btrc_Vector_int_new(void) {
    btrc_Vector_int* self = ((btrc_Vector_int*)malloc(sizeof(btrc_Vector_int)));
    memset(self, 0, sizeof(btrc_Vector_int));
    btrc_Vector_int_init(self);
    return self;
}

void btrc_Vector_int_destroy(btrc_Vector_int* self) {
    free(self);
}

void btrc_Vector_int_push(btrc_Vector_int* self, int val) {
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((int*)__btrc_safe_realloc(self->data, (sizeof(int) * self->cap))));
    }
    (self->data[self->len] = val);
    (self->len++);
}

int btrc_Vector_int_pop(btrc_Vector_int* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector pop from empty list\n");
        exit(1);
    }
    (self->len--);
    return self->data[self->len];
}

int btrc_Vector_int_get(btrc_Vector_int* self, int i) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    return self->data[i];
}

void btrc_Vector_int_set(btrc_Vector_int* self, int i, int val) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    (self->data[i] = val);
}

void btrc_Vector_int_free(btrc_Vector_int* self) {
    for (int i = 0; (i < self->len); (i++)) {
    }
    free(self->data);
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

void btrc_Vector_int_remove(btrc_Vector_int* self, int idx) {
    if ((idx < 0) || (idx >= self->len)) {
        fprintf(stderr, "Vector remove index out of bounds: %d (len=%d)\n", idx, self->len);
        exit(1);
    }
    for (int i = idx; (i < (self->len - 1)); (i++)) {
        (self->data[i] = self->data[(i + 1)]);
    }
    (self->len--);
}

void btrc_Vector_int_reverse(btrc_Vector_int* self) {
    for (int i = 0; (i < (self->len / 2)); (i++)) {
        int tmp = self->data[i];
        (self->data[i] = self->data[((self->len - 1) - i)]);
        (self->data[((self->len - 1) - i)] = tmp);
    }
}

btrc_Vector_int* btrc_Vector_int_reversed(btrc_Vector_int* self) {
    btrc_Vector_int* result = btrc_Vector_int_new();
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        btrc_Vector_int_push(result, self->data[i]);
    }
    return result;
}

void btrc_Vector_int_swap(btrc_Vector_int* self, int i, int j) {
    if ((((i < 0) || (i >= self->len)) || (j < 0)) || (j >= self->len)) {
        fprintf(stderr, "Vector swap index out of bounds\n");
        exit(1);
    }
    int tmp = self->data[i];
    (self->data[i] = self->data[j]);
    (self->data[j] = tmp);
}

void btrc_Vector_int_clear(btrc_Vector_int* self) {
    for (int i = 0; (i < self->len); (i++)) {
    }
    (self->len = 0);
}

void btrc_Vector_int_fill(btrc_Vector_int* self, int val) {
    for (int i = 0; (i < self->len); (i++)) {
        (self->data[i] = val);
    }
}

int btrc_Vector_int_size(btrc_Vector_int* self) {
    return self->len;
}

bool btrc_Vector_int_isEmpty(btrc_Vector_int* self) {
    return (self->len == 0);
}

int btrc_Vector_int_first(btrc_Vector_int* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.first() called on empty list\n");
        exit(1);
    }
    return self->data[0];
}

int btrc_Vector_int_last(btrc_Vector_int* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.last() called on empty list\n");
        exit(1);
    }
    return self->data[(self->len - 1)];
}

btrc_Vector_int* btrc_Vector_int_slice(btrc_Vector_int* self, int start, int end) {
    if (start < 0) {
        (start = (self->len + start));
    }
    if (end < 0) {
        (end = (self->len + end));
    }
    if (start < 0) {
        (start = 0);
    }
    if (end > self->len) {
        (end = self->len);
    }
    btrc_Vector_int* result = btrc_Vector_int_new();
    for (int i = start; (i < end); (i++)) {
        btrc_Vector_int_push(result, self->data[i]);
    }
    return result;
}

btrc_Vector_int* btrc_Vector_int_take(btrc_Vector_int* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_int_slice(self, 0, n);
}

btrc_Vector_int* btrc_Vector_int_drop(btrc_Vector_int* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_int_slice(self, n, self->len);
}

void btrc_Vector_int_extend(btrc_Vector_int* self, btrc_Vector_int* other) {
    for (int i = 0; (i < other->len); (i++)) {
        btrc_Vector_int_push(self, other->data[i]);
    }
}

void btrc_Vector_int_insert(btrc_Vector_int* self, int idx, int val) {
    if ((idx < 0) || (idx > self->len)) {
        fprintf(stderr, "Vector insert index out of bounds: %d (size %d)\n", idx, self->len);
        exit(1);
    }
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((int*)__btrc_safe_realloc(self->data, (sizeof(int) * self->cap))));
    }
    for (int i = self->len; (i > idx); (i--)) {
        (self->data[i] = self->data[(i - 1)]);
    }
    (self->data[idx] = val);
    (self->len++);
}

bool btrc_Vector_int_contains(btrc_Vector_int* self, int val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return true;
        }
    }
    return false;
}

int btrc_Vector_int_indexOf(btrc_Vector_int* self, int val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

int btrc_Vector_int_lastIndexOf(btrc_Vector_int* self, int val) {
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

int btrc_Vector_int_count(btrc_Vector_int* self, int val) {
    int c = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            (c++);
        }
    }
    return c;
}

void btrc_Vector_int_removeAll(btrc_Vector_int* self, int val) {
    int j = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (!__btrc_eq(self->data[i], val)) {
            (self->data[j] = self->data[i]);
            (j++);
        }
    }
    (self->len = j);
}

btrc_Vector_int* btrc_Vector_int_distinct(btrc_Vector_int* self) {
    btrc_Vector_int* result = btrc_Vector_int_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (!btrc_Vector_int_contains(result, self->data[i])) {
            btrc_Vector_int_push(result, self->data[i]);
        }
    }
    return result;
}

void btrc_Vector_int_sort(btrc_Vector_int* self) {
    for (int i = 1; (i < self->len); (i++)) {
        int key = self->data[i];
        int j = (i - 1);
        while ((j >= 0) && __btrc_lt(key, self->data[j])) {
            (self->data[(j + 1)] = self->data[j]);
            (j = (j - 1));
        }
        (self->data[(j + 1)] = key);
    }
}

btrc_Vector_int* btrc_Vector_int_sorted(btrc_Vector_int* self) {
    btrc_Vector_int* result = btrc_Vector_int_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_int_push(result, self->data[i]);
    }
    btrc_Vector_int_sort(result);
    return result;
}

int btrc_Vector_int_min(btrc_Vector_int* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector min on empty list\n");
        exit(1);
    }
    int m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_lt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

int btrc_Vector_int_max(btrc_Vector_int* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector max on empty list\n");
        exit(1);
    }
    int m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_gt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

int btrc_Vector_int_sum(btrc_Vector_int* self) {
    int s = ((int)0);
    for (int i = 0; (i < self->len); (i++)) {
        (s = (s + self->data[i]));
    }
    return s;
}

btrc_Vector_int* btrc_Vector_int_filter(btrc_Vector_int* self, __btrc_fn_bool_int pred) {
    btrc_Vector_int* result = btrc_Vector_int_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            btrc_Vector_int_push(result, self->data[i]);
        }
    }
    return result;
}

int btrc_Vector_int_findIndex(btrc_Vector_int* self, __btrc_fn_bool_int pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return i;
        }
    }
    return (-1);
}

void btrc_Vector_int_forEach(btrc_Vector_int* self, __btrc_fn_void_int fn) {
    for (int i = 0; (i < self->len); (i++)) {
        fn(self->data[i]);
    }
}

btrc_Vector_int* btrc_Vector_int_map(btrc_Vector_int* self, __btrc_fn_int_int fn) {
    btrc_Vector_int* result = btrc_Vector_int_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_int_push(result, fn(self->data[i]));
    }
    return result;
}

bool btrc_Vector_int_any(btrc_Vector_int* self, __btrc_fn_bool_int pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return true;
        }
    }
    return false;
}

bool btrc_Vector_int_all(btrc_Vector_int* self, __btrc_fn_bool_int pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (!pred(self->data[i])) {
            return false;
        }
    }
    return true;
}

int btrc_Vector_int_reduce(btrc_Vector_int* self, int init, __btrc_fn_int_int_int fn) {
    int acc = init;
    for (int i = 0; (i < self->len); (i++)) {
        (acc = fn(acc, self->data[i]));
    }
    return acc;
}

btrc_Vector_int* btrc_Vector_int_copy(btrc_Vector_int* self) {
    btrc_Vector_int* result = btrc_Vector_int_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_int_push(result, self->data[i]);
    }
    return result;
}

void btrc_Vector_int_removeAt(btrc_Vector_int* self, int idx) {
    btrc_Vector_int_remove(self, idx);
}

int btrc_Vector_int_iterLen(btrc_Vector_int* self) {
    return self->len;
}

int btrc_Vector_int_iterGet(btrc_Vector_int* self, int i) {
    return self->data[i];
}

void btrc_Vector_float_init(btrc_Vector_float* self) {
    self->__rc = 1;
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

btrc_Vector_float* btrc_Vector_float_new(void) {
    btrc_Vector_float* self = ((btrc_Vector_float*)malloc(sizeof(btrc_Vector_float)));
    memset(self, 0, sizeof(btrc_Vector_float));
    btrc_Vector_float_init(self);
    return self;
}

void btrc_Vector_float_destroy(btrc_Vector_float* self) {
    free(self);
}

void btrc_Vector_float_push(btrc_Vector_float* self, float val) {
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((float*)__btrc_safe_realloc(self->data, (sizeof(float) * self->cap))));
    }
    (self->data[self->len] = val);
    (self->len++);
}

float btrc_Vector_float_pop(btrc_Vector_float* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector pop from empty list\n");
        exit(1);
    }
    (self->len--);
    return self->data[self->len];
}

float btrc_Vector_float_get(btrc_Vector_float* self, int i) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    return self->data[i];
}

void btrc_Vector_float_set(btrc_Vector_float* self, int i, float val) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    (self->data[i] = val);
}

void btrc_Vector_float_free(btrc_Vector_float* self) {
    for (int i = 0; (i < self->len); (i++)) {
    }
    free(self->data);
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

void btrc_Vector_float_remove(btrc_Vector_float* self, int idx) {
    if ((idx < 0) || (idx >= self->len)) {
        fprintf(stderr, "Vector remove index out of bounds: %d (len=%d)\n", idx, self->len);
        exit(1);
    }
    for (int i = idx; (i < (self->len - 1)); (i++)) {
        (self->data[i] = self->data[(i + 1)]);
    }
    (self->len--);
}

void btrc_Vector_float_reverse(btrc_Vector_float* self) {
    for (int i = 0; (i < (self->len / 2)); (i++)) {
        float tmp = self->data[i];
        (self->data[i] = self->data[((self->len - 1) - i)]);
        (self->data[((self->len - 1) - i)] = tmp);
    }
}

btrc_Vector_float* btrc_Vector_float_reversed(btrc_Vector_float* self) {
    btrc_Vector_float* result = btrc_Vector_float_new();
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        btrc_Vector_float_push(result, self->data[i]);
    }
    return result;
}

void btrc_Vector_float_swap(btrc_Vector_float* self, int i, int j) {
    if ((((i < 0) || (i >= self->len)) || (j < 0)) || (j >= self->len)) {
        fprintf(stderr, "Vector swap index out of bounds\n");
        exit(1);
    }
    float tmp = self->data[i];
    (self->data[i] = self->data[j]);
    (self->data[j] = tmp);
}

void btrc_Vector_float_clear(btrc_Vector_float* self) {
    for (int i = 0; (i < self->len); (i++)) {
    }
    (self->len = 0);
}

void btrc_Vector_float_fill(btrc_Vector_float* self, float val) {
    for (int i = 0; (i < self->len); (i++)) {
        (self->data[i] = val);
    }
}

int btrc_Vector_float_size(btrc_Vector_float* self) {
    return self->len;
}

bool btrc_Vector_float_isEmpty(btrc_Vector_float* self) {
    return (self->len == 0);
}

float btrc_Vector_float_first(btrc_Vector_float* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.first() called on empty list\n");
        exit(1);
    }
    return self->data[0];
}

float btrc_Vector_float_last(btrc_Vector_float* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.last() called on empty list\n");
        exit(1);
    }
    return self->data[(self->len - 1)];
}

btrc_Vector_float* btrc_Vector_float_slice(btrc_Vector_float* self, int start, int end) {
    if (start < 0) {
        (start = (self->len + start));
    }
    if (end < 0) {
        (end = (self->len + end));
    }
    if (start < 0) {
        (start = 0);
    }
    if (end > self->len) {
        (end = self->len);
    }
    btrc_Vector_float* result = btrc_Vector_float_new();
    for (int i = start; (i < end); (i++)) {
        btrc_Vector_float_push(result, self->data[i]);
    }
    return result;
}

btrc_Vector_float* btrc_Vector_float_take(btrc_Vector_float* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_float_slice(self, 0, n);
}

btrc_Vector_float* btrc_Vector_float_drop(btrc_Vector_float* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_float_slice(self, n, self->len);
}

void btrc_Vector_float_extend(btrc_Vector_float* self, btrc_Vector_float* other) {
    for (int i = 0; (i < other->len); (i++)) {
        btrc_Vector_float_push(self, other->data[i]);
    }
}

void btrc_Vector_float_insert(btrc_Vector_float* self, int idx, float val) {
    if ((idx < 0) || (idx > self->len)) {
        fprintf(stderr, "Vector insert index out of bounds: %d (size %d)\n", idx, self->len);
        exit(1);
    }
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((float*)__btrc_safe_realloc(self->data, (sizeof(float) * self->cap))));
    }
    for (int i = self->len; (i > idx); (i--)) {
        (self->data[i] = self->data[(i - 1)]);
    }
    (self->data[idx] = val);
    (self->len++);
}

bool btrc_Vector_float_contains(btrc_Vector_float* self, float val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return true;
        }
    }
    return false;
}

int btrc_Vector_float_indexOf(btrc_Vector_float* self, float val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

int btrc_Vector_float_lastIndexOf(btrc_Vector_float* self, float val) {
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

int btrc_Vector_float_count(btrc_Vector_float* self, float val) {
    int c = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            (c++);
        }
    }
    return c;
}

void btrc_Vector_float_removeAll(btrc_Vector_float* self, float val) {
    int j = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (!__btrc_eq(self->data[i], val)) {
            (self->data[j] = self->data[i]);
            (j++);
        }
    }
    (self->len = j);
}

btrc_Vector_float* btrc_Vector_float_distinct(btrc_Vector_float* self) {
    btrc_Vector_float* result = btrc_Vector_float_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (!btrc_Vector_float_contains(result, self->data[i])) {
            btrc_Vector_float_push(result, self->data[i]);
        }
    }
    return result;
}

void btrc_Vector_float_sort(btrc_Vector_float* self) {
    for (int i = 1; (i < self->len); (i++)) {
        float key = self->data[i];
        int j = (i - 1);
        while ((j >= 0) && __btrc_lt(key, self->data[j])) {
            (self->data[(j + 1)] = self->data[j]);
            (j = (j - 1));
        }
        (self->data[(j + 1)] = key);
    }
}

btrc_Vector_float* btrc_Vector_float_sorted(btrc_Vector_float* self) {
    btrc_Vector_float* result = btrc_Vector_float_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_float_push(result, self->data[i]);
    }
    btrc_Vector_float_sort(result);
    return result;
}

float btrc_Vector_float_min(btrc_Vector_float* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector min on empty list\n");
        exit(1);
    }
    float m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_lt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

float btrc_Vector_float_max(btrc_Vector_float* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector max on empty list\n");
        exit(1);
    }
    float m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_gt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

float btrc_Vector_float_sum(btrc_Vector_float* self) {
    float s = ((float)0);
    for (int i = 0; (i < self->len); (i++)) {
        (s = (s + self->data[i]));
    }
    return s;
}

btrc_Vector_float* btrc_Vector_float_filter(btrc_Vector_float* self, __btrc_fn_bool_float pred) {
    btrc_Vector_float* result = btrc_Vector_float_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            btrc_Vector_float_push(result, self->data[i]);
        }
    }
    return result;
}

int btrc_Vector_float_findIndex(btrc_Vector_float* self, __btrc_fn_bool_float pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return i;
        }
    }
    return (-1);
}

void btrc_Vector_float_forEach(btrc_Vector_float* self, __btrc_fn_void_float fn) {
    for (int i = 0; (i < self->len); (i++)) {
        fn(self->data[i]);
    }
}

btrc_Vector_float* btrc_Vector_float_map(btrc_Vector_float* self, __btrc_fn_float_float fn) {
    btrc_Vector_float* result = btrc_Vector_float_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_float_push(result, fn(self->data[i]));
    }
    return result;
}

bool btrc_Vector_float_any(btrc_Vector_float* self, __btrc_fn_bool_float pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return true;
        }
    }
    return false;
}

bool btrc_Vector_float_all(btrc_Vector_float* self, __btrc_fn_bool_float pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (!pred(self->data[i])) {
            return false;
        }
    }
    return true;
}

float btrc_Vector_float_reduce(btrc_Vector_float* self, float init, __btrc_fn_float_float_float fn) {
    float acc = init;
    for (int i = 0; (i < self->len); (i++)) {
        (acc = fn(acc, self->data[i]));
    }
    return acc;
}

btrc_Vector_float* btrc_Vector_float_copy(btrc_Vector_float* self) {
    btrc_Vector_float* result = btrc_Vector_float_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_float_push(result, self->data[i]);
    }
    return result;
}

void btrc_Vector_float_removeAt(btrc_Vector_float* self, int idx) {
    btrc_Vector_float_remove(self, idx);
}

int btrc_Vector_float_iterLen(btrc_Vector_float* self) {
    return self->len;
}

float btrc_Vector_float_iterGet(btrc_Vector_float* self, int i) {
    return self->data[i];
}

void btrc_Map_string_string_init(btrc_Map_string_string* self) {
    self->__rc = 1;
    (self->cap = 16);
    (self->len = 0);
    (self->keys = ((char**)__btrc_safe_calloc(16, sizeof(char*))));
    (self->values = ((char**)__btrc_safe_calloc(16, sizeof(char*))));
    (self->occupied = ((bool*)__btrc_safe_calloc(16, sizeof(bool))));
}

btrc_Map_string_string* btrc_Map_string_string_new(void) {
    btrc_Map_string_string* self = ((btrc_Map_string_string*)malloc(sizeof(btrc_Map_string_string)));
    memset(self, 0, sizeof(btrc_Map_string_string));
    btrc_Map_string_string_init(self);
    return self;
}

void btrc_Map_string_string_destroy(btrc_Map_string_string* self) {
    free(self);
}

void btrc_Map_string_string_resize(btrc_Map_string_string* self) {
    int old_cap = self->cap;
    char** old_keys = self->keys;
    char** old_values = self->values;
    bool* old_occupied = self->occupied;
    (self->cap = (self->cap * 2));
    (self->len = 0);
    (self->keys = ((char**)__btrc_safe_calloc(self->cap, sizeof(char*))));
    (self->values = ((char**)__btrc_safe_calloc(self->cap, sizeof(char*))));
    (self->occupied = ((bool*)__btrc_safe_calloc(self->cap, sizeof(bool))));
    for (int i = 0; (i < old_cap); (i++)) {
        if (old_occupied[i]) {
            btrc_Map_string_string_put(self, old_keys[i], old_values[i]);
        }
    }
    free(old_keys);
    free(old_values);
    free(old_occupied);
}

void btrc_Map_string_string_put(btrc_Map_string_string* self, char* key, char* value) {
    if ((self->len * 4) >= (self->cap * 3)) {
        btrc_Map_string_string_resize(self);
    }
    unsigned int idx = (__btrc_hash(key) % self->cap);
    while (self->occupied[idx]) {
        if (__btrc_eq(self->keys[idx], key)) {
            (self->values[idx] = value);
            return;
        }
        (idx = ((idx + 1) % self->cap));
    }
    (self->keys[idx] = key);
    (self->values[idx] = value);
    (self->occupied[idx] = true);
    (self->len++);
}

char* btrc_Map_string_string_get(btrc_Map_string_string* self, char* key) {
    unsigned int idx = (__btrc_hash(key) % self->cap);
    while (self->occupied[idx]) {
        if (__btrc_eq(self->keys[idx], key)) {
            return self->values[idx];
        }
        (idx = ((idx + 1) % self->cap));
    }
    fprintf(stderr, "Map key not found\n");
    exit(1);
    return self->values[0];
}

char* btrc_Map_string_string_getOrDefault(btrc_Map_string_string* self, char* key, char* fallback) {
    unsigned int idx = (__btrc_hash(key) % self->cap);
    while (self->occupied[idx]) {
        if (__btrc_eq(self->keys[idx], key)) {
            return self->values[idx];
        }
        (idx = ((idx + 1) % self->cap));
    }
    return fallback;
}

bool btrc_Map_string_string_has(btrc_Map_string_string* self, char* key) {
    unsigned int idx = (__btrc_hash(key) % self->cap);
    while (self->occupied[idx]) {
        if (__btrc_eq(self->keys[idx], key)) {
            return true;
        }
        (idx = ((idx + 1) % self->cap));
    }
    return false;
}

bool btrc_Map_string_string_contains(btrc_Map_string_string* self, char* key) {
    return btrc_Map_string_string_has(self, key);
}

void btrc_Map_string_string_putIfAbsent(btrc_Map_string_string* self, char* key, char* value) {
    if (!btrc_Map_string_string_has(self, key)) {
        btrc_Map_string_string_put(self, key, value);
    }
}

void btrc_Map_string_string_free(btrc_Map_string_string* self) {
    free(self->keys);
    free(self->values);
    free(self->occupied);
    (self->keys = NULL);
    (self->values = NULL);
    (self->occupied = NULL);
    (self->cap = 0);
    (self->len = 0);
}

void btrc_Map_string_string_remove(btrc_Map_string_string* self, char* key) {
    unsigned int idx = (__btrc_hash(key) % self->cap);
    while (self->occupied[idx]) {
        if (__btrc_eq(self->keys[idx], key)) {
            (self->occupied[idx] = false);
            (self->len--);
            unsigned int j = ((idx + 1) % self->cap);
            while (self->occupied[j]) {
                char* rk = self->keys[j];
                char* rv = self->values[j];
                (self->occupied[j] = false);
                (self->len--);
                btrc_Map_string_string_put(self, rk, rv);
                (j = ((j + 1) % self->cap));
            }
            return;
        }
        (idx = ((idx + 1) % self->cap));
    }
}

void btrc_Map_string_string_clear(btrc_Map_string_string* self) {
    for (int i = 0; (i < self->cap); (i++)) {
        (self->occupied[i] = false);
    }
    (self->len = 0);
}

int btrc_Map_string_string_size(btrc_Map_string_string* self) {
    return self->len;
}

bool btrc_Map_string_string_isEmpty(btrc_Map_string_string* self) {
    return (self->len == 0);
}

btrc_Vector_string* btrc_Map_string_string_keys(btrc_Map_string_string* self) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    for (int i = 0; (i < self->cap); (i++)) {
        if (self->occupied[i]) {
            btrc_Vector_string_push(result, self->keys[i]);
        }
    }
    return result;
}

btrc_Vector_string* btrc_Map_string_string_values(btrc_Map_string_string* self) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    for (int i = 0; (i < self->cap); (i++)) {
        if (self->occupied[i]) {
            btrc_Vector_string_push(result, self->values[i]);
        }
    }
    return result;
}

bool btrc_Map_string_string_containsValue(btrc_Map_string_string* self, char* value) {
    for (int i = 0; (i < self->cap); (i++)) {
        if (self->occupied[i] && __btrc_eq(self->values[i], value)) {
            return true;
        }
    }
    return false;
}

void btrc_Map_string_string_set(btrc_Map_string_string* self, char* key, char* value) {
    btrc_Map_string_string_put(self, key, value);
}

void btrc_Map_string_string_merge(btrc_Map_string_string* self, btrc_Map_string_string* other) {
    for (int i = 0; (i < other->cap); (i++)) {
        if (other->occupied[i]) {
            btrc_Map_string_string_put(self, other->keys[i], other->values[i]);
        }
    }
}

int btrc_Map_string_string_iterLen(btrc_Map_string_string* self) {
    return self->len;
}

char* btrc_Map_string_string_iterGet(btrc_Map_string_string* self, int n) {
    int count = 0;
    for (int i = 0; (i < self->cap); (i++)) {
        if (self->occupied[i]) {
            if (count == n) {
                return self->keys[i];
            }
            (count++);
        }
    }
    fprintf(stderr, "Map iterGet: index out of bounds\n");
    exit(1);
    return self->keys[0];
}

char* btrc_Map_string_string_iterValueAt(btrc_Map_string_string* self, int n) {
    int count = 0;
    for (int i = 0; (i < self->cap); (i++)) {
        if (self->occupied[i]) {
            if (count == n) {
                return self->values[i];
            }
            (count++);
        }
    }
    fprintf(stderr, "Map iterValueAt: index out of bounds\n");
    exit(1);
    return self->values[0];
}

void btrc_Vector_Map_string_string_init(btrc_Vector_Map_string_string* self) {
    self->__rc = 1;
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

btrc_Vector_Map_string_string* btrc_Vector_Map_string_string_new(void) {
    btrc_Vector_Map_string_string* self = ((btrc_Vector_Map_string_string*)malloc(sizeof(btrc_Vector_Map_string_string)));
    memset(self, 0, sizeof(btrc_Vector_Map_string_string));
    btrc_Vector_Map_string_string_init(self);
    return self;
}

void btrc_Vector_Map_string_string_destroy(btrc_Vector_Map_string_string* self) {
    free(self);
}

void btrc_Vector_Map_string_string_push(btrc_Vector_Map_string_string* self, btrc_Map_string_string* val) {
    (val->__rc++);
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((btrc_Map_string_string**)__btrc_safe_realloc(self->data, (sizeof(btrc_Map_string_string*) * self->cap))));
    }
    (self->data[self->len] = val);
    (self->len++);
}

btrc_Map_string_string* btrc_Vector_Map_string_string_pop(btrc_Vector_Map_string_string* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector pop from empty list\n");
        exit(1);
    }
    (self->len--);
    return self->data[self->len];
}

btrc_Map_string_string* btrc_Vector_Map_string_string_get(btrc_Vector_Map_string_string* self, int i) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    return self->data[i];
}

void btrc_Vector_Map_string_string_set(btrc_Vector_Map_string_string* self, int i, btrc_Map_string_string* val) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    if (self->data[i]) {
        if ((--self->data[i]->__rc) <= 0) {
            btrc_Map_string_string_free(self->data[i]);
        }
    }
    (val->__rc++);
    (self->data[i] = val);
}

void btrc_Vector_Map_string_string_free(btrc_Vector_Map_string_string* self) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                btrc_Map_string_string_free(self->data[i]);
            }
        }
    }
    free(self->data);
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

void btrc_Vector_Map_string_string_remove(btrc_Vector_Map_string_string* self, int idx) {
    if ((idx < 0) || (idx >= self->len)) {
        fprintf(stderr, "Vector remove index out of bounds: %d (len=%d)\n", idx, self->len);
        exit(1);
    }
    if (self->data[idx]) {
        if ((--self->data[idx]->__rc) <= 0) {
            btrc_Map_string_string_free(self->data[idx]);
        }
    }
    for (int i = idx; (i < (self->len - 1)); (i++)) {
        (self->data[i] = self->data[(i + 1)]);
    }
    (self->len--);
}

void btrc_Vector_Map_string_string_reverse(btrc_Vector_Map_string_string* self) {
    for (int i = 0; (i < (self->len / 2)); (i++)) {
        btrc_Map_string_string* tmp = self->data[i];
        (self->data[i] = self->data[((self->len - 1) - i)]);
        (self->data[((self->len - 1) - i)] = tmp);
    }
}

btrc_Vector_Map_string_string* btrc_Vector_Map_string_string_reversed(btrc_Vector_Map_string_string* self) {
    btrc_Vector_Map_string_string* result = btrc_Vector_Map_string_string_new();
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        btrc_Vector_Map_string_string_push(result, self->data[i]);
    }
    return result;
}

void btrc_Vector_Map_string_string_swap(btrc_Vector_Map_string_string* self, int i, int j) {
    if ((((i < 0) || (i >= self->len)) || (j < 0)) || (j >= self->len)) {
        fprintf(stderr, "Vector swap index out of bounds\n");
        exit(1);
    }
    btrc_Map_string_string* tmp = self->data[i];
    (self->data[i] = self->data[j]);
    (self->data[j] = tmp);
}

void btrc_Vector_Map_string_string_clear(btrc_Vector_Map_string_string* self) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                btrc_Map_string_string_free(self->data[i]);
            }
        }
    }
    (self->len = 0);
}

void btrc_Vector_Map_string_string_fill(btrc_Vector_Map_string_string* self, btrc_Map_string_string* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                btrc_Map_string_string_free(self->data[i]);
            }
        }
        (val->__rc++);
        (self->data[i] = val);
    }
}

int btrc_Vector_Map_string_string_size(btrc_Vector_Map_string_string* self) {
    return self->len;
}

bool btrc_Vector_Map_string_string_isEmpty(btrc_Vector_Map_string_string* self) {
    return (self->len == 0);
}

btrc_Map_string_string* btrc_Vector_Map_string_string_first(btrc_Vector_Map_string_string* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.first() called on empty list\n");
        exit(1);
    }
    return self->data[0];
}

btrc_Map_string_string* btrc_Vector_Map_string_string_last(btrc_Vector_Map_string_string* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.last() called on empty list\n");
        exit(1);
    }
    return self->data[(self->len - 1)];
}

btrc_Vector_Map_string_string* btrc_Vector_Map_string_string_slice(btrc_Vector_Map_string_string* self, int start, int end) {
    if (start < 0) {
        (start = (self->len + start));
    }
    if (end < 0) {
        (end = (self->len + end));
    }
    if (start < 0) {
        (start = 0);
    }
    if (end > self->len) {
        (end = self->len);
    }
    btrc_Vector_Map_string_string* result = btrc_Vector_Map_string_string_new();
    for (int i = start; (i < end); (i++)) {
        btrc_Vector_Map_string_string_push(result, self->data[i]);
    }
    return result;
}

btrc_Vector_Map_string_string* btrc_Vector_Map_string_string_take(btrc_Vector_Map_string_string* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_Map_string_string_slice(self, 0, n);
}

btrc_Vector_Map_string_string* btrc_Vector_Map_string_string_drop(btrc_Vector_Map_string_string* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_Map_string_string_slice(self, n, self->len);
}

void btrc_Vector_Map_string_string_extend(btrc_Vector_Map_string_string* self, btrc_Vector_Map_string_string* other) {
    for (int i = 0; (i < other->len); (i++)) {
        btrc_Vector_Map_string_string_push(self, other->data[i]);
    }
}

void btrc_Vector_Map_string_string_insert(btrc_Vector_Map_string_string* self, int idx, btrc_Map_string_string* val) {
    if ((idx < 0) || (idx > self->len)) {
        fprintf(stderr, "Vector insert index out of bounds: %d (size %d)\n", idx, self->len);
        exit(1);
    }
    (val->__rc++);
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((btrc_Map_string_string**)__btrc_safe_realloc(self->data, (sizeof(btrc_Map_string_string*) * self->cap))));
    }
    for (int i = self->len; (i > idx); (i--)) {
        (self->data[i] = self->data[(i - 1)]);
    }
    (self->data[idx] = val);
    (self->len++);
}

bool btrc_Vector_Map_string_string_contains(btrc_Vector_Map_string_string* self, btrc_Map_string_string* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return true;
        }
    }
    return false;
}

int btrc_Vector_Map_string_string_indexOf(btrc_Vector_Map_string_string* self, btrc_Map_string_string* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

int btrc_Vector_Map_string_string_lastIndexOf(btrc_Vector_Map_string_string* self, btrc_Map_string_string* val) {
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

int btrc_Vector_Map_string_string_count(btrc_Vector_Map_string_string* self, btrc_Map_string_string* val) {
    int c = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            (c++);
        }
    }
    return c;
}

void btrc_Vector_Map_string_string_removeAll(btrc_Vector_Map_string_string* self, btrc_Map_string_string* val) {
    int j = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (!__btrc_eq(self->data[i], val)) {
            (self->data[j] = self->data[i]);
            (j++);
        } else if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                btrc_Map_string_string_free(self->data[i]);
            }
        }
    }
    (self->len = j);
}

btrc_Vector_Map_string_string* btrc_Vector_Map_string_string_distinct(btrc_Vector_Map_string_string* self) {
    btrc_Vector_Map_string_string* result = btrc_Vector_Map_string_string_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (!btrc_Vector_Map_string_string_contains(result, self->data[i])) {
            btrc_Vector_Map_string_string_push(result, self->data[i]);
        }
    }
    return result;
}

void btrc_Vector_Map_string_string_sort(btrc_Vector_Map_string_string* self) {
    for (int i = 1; (i < self->len); (i++)) {
        btrc_Map_string_string* key = self->data[i];
        int j = (i - 1);
        while ((j >= 0) && __btrc_lt(key, self->data[j])) {
            (self->data[(j + 1)] = self->data[j]);
            (j = (j - 1));
        }
        (self->data[(j + 1)] = key);
    }
}

btrc_Vector_Map_string_string* btrc_Vector_Map_string_string_sorted(btrc_Vector_Map_string_string* self) {
    btrc_Vector_Map_string_string* result = btrc_Vector_Map_string_string_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_Map_string_string_push(result, self->data[i]);
    }
    btrc_Vector_Map_string_string_sort(result);
    return result;
}

btrc_Map_string_string* btrc_Vector_Map_string_string_min(btrc_Vector_Map_string_string* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector min on empty list\n");
        exit(1);
    }
    btrc_Map_string_string* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_lt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

btrc_Map_string_string* btrc_Vector_Map_string_string_max(btrc_Vector_Map_string_string* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector max on empty list\n");
        exit(1);
    }
    btrc_Map_string_string* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_gt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

btrc_Vector_Map_string_string* btrc_Vector_Map_string_string_filter(btrc_Vector_Map_string_string* self, __btrc_fn_bool_Map_string_string pred) {
    btrc_Vector_Map_string_string* result = btrc_Vector_Map_string_string_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            btrc_Vector_Map_string_string_push(result, self->data[i]);
        }
    }
    return result;
}

int btrc_Vector_Map_string_string_findIndex(btrc_Vector_Map_string_string* self, __btrc_fn_bool_Map_string_string pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return i;
        }
    }
    return (-1);
}

void btrc_Vector_Map_string_string_forEach(btrc_Vector_Map_string_string* self, __btrc_fn_void_Map_string_string fn) {
    for (int i = 0; (i < self->len); (i++)) {
        fn(self->data[i]);
    }
}

btrc_Vector_Map_string_string* btrc_Vector_Map_string_string_map(btrc_Vector_Map_string_string* self, __btrc_fn_Map_string_string_Map_string_string fn) {
    btrc_Vector_Map_string_string* result = btrc_Vector_Map_string_string_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_Map_string_string_push(result, fn(self->data[i]));
    }
    return result;
}

bool btrc_Vector_Map_string_string_any(btrc_Vector_Map_string_string* self, __btrc_fn_bool_Map_string_string pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return true;
        }
    }
    return false;
}

bool btrc_Vector_Map_string_string_all(btrc_Vector_Map_string_string* self, __btrc_fn_bool_Map_string_string pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (!pred(self->data[i])) {
            return false;
        }
    }
    return true;
}

btrc_Map_string_string* btrc_Vector_Map_string_string_reduce(btrc_Vector_Map_string_string* self, btrc_Map_string_string* init, __btrc_fn_Map_string_string_Map_string_string_Map_string_string fn) {
    btrc_Map_string_string* acc = init;
    for (int i = 0; (i < self->len); (i++)) {
        (acc = fn(acc, self->data[i]));
    }
    return acc;
}

btrc_Vector_Map_string_string* btrc_Vector_Map_string_string_copy(btrc_Vector_Map_string_string* self) {
    btrc_Vector_Map_string_string* result = btrc_Vector_Map_string_string_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_Map_string_string_push(result, self->data[i]);
    }
    return result;
}

void btrc_Vector_Map_string_string_removeAt(btrc_Vector_Map_string_string* self, int idx) {
    btrc_Vector_Map_string_string_remove(self, idx);
}

int btrc_Vector_Map_string_string_iterLen(btrc_Vector_Map_string_string* self) {
    return self->len;
}

btrc_Map_string_string* btrc_Vector_Map_string_string_iterGet(btrc_Vector_Map_string_string* self, int i) {
    return self->data[i];
}

void btrc_Map_string_bool_init(btrc_Map_string_bool* self) {
    self->__rc = 1;
    (self->cap = 16);
    (self->len = 0);
    (self->keys = ((char**)__btrc_safe_calloc(16, sizeof(char*))));
    (self->values = ((bool*)__btrc_safe_calloc(16, sizeof(bool))));
    (self->occupied = ((bool*)__btrc_safe_calloc(16, sizeof(bool))));
}

btrc_Map_string_bool* btrc_Map_string_bool_new(void) {
    btrc_Map_string_bool* self = ((btrc_Map_string_bool*)malloc(sizeof(btrc_Map_string_bool)));
    memset(self, 0, sizeof(btrc_Map_string_bool));
    btrc_Map_string_bool_init(self);
    return self;
}

void btrc_Map_string_bool_destroy(btrc_Map_string_bool* self) {
    free(self);
}

void btrc_Map_string_bool_resize(btrc_Map_string_bool* self) {
    int old_cap = self->cap;
    char** old_keys = self->keys;
    bool* old_values = self->values;
    bool* old_occupied = self->occupied;
    (self->cap = (self->cap * 2));
    (self->len = 0);
    (self->keys = ((char**)__btrc_safe_calloc(self->cap, sizeof(char*))));
    (self->values = ((bool*)__btrc_safe_calloc(self->cap, sizeof(bool))));
    (self->occupied = ((bool*)__btrc_safe_calloc(self->cap, sizeof(bool))));
    for (int i = 0; (i < old_cap); (i++)) {
        if (old_occupied[i]) {
            btrc_Map_string_bool_put(self, old_keys[i], old_values[i]);
        }
    }
    free(old_keys);
    free(old_values);
    free(old_occupied);
}

void btrc_Map_string_bool_put(btrc_Map_string_bool* self, char* key, bool value) {
    if ((self->len * 4) >= (self->cap * 3)) {
        btrc_Map_string_bool_resize(self);
    }
    unsigned int idx = (__btrc_hash(key) % self->cap);
    while (self->occupied[idx]) {
        if (__btrc_eq(self->keys[idx], key)) {
            (self->values[idx] = value);
            return;
        }
        (idx = ((idx + 1) % self->cap));
    }
    (self->keys[idx] = key);
    (self->values[idx] = value);
    (self->occupied[idx] = true);
    (self->len++);
}

bool btrc_Map_string_bool_get(btrc_Map_string_bool* self, char* key) {
    unsigned int idx = (__btrc_hash(key) % self->cap);
    while (self->occupied[idx]) {
        if (__btrc_eq(self->keys[idx], key)) {
            return self->values[idx];
        }
        (idx = ((idx + 1) % self->cap));
    }
    fprintf(stderr, "Map key not found\n");
    exit(1);
    return self->values[0];
}

bool btrc_Map_string_bool_getOrDefault(btrc_Map_string_bool* self, char* key, bool fallback) {
    unsigned int idx = (__btrc_hash(key) % self->cap);
    while (self->occupied[idx]) {
        if (__btrc_eq(self->keys[idx], key)) {
            return self->values[idx];
        }
        (idx = ((idx + 1) % self->cap));
    }
    return fallback;
}

bool btrc_Map_string_bool_has(btrc_Map_string_bool* self, char* key) {
    unsigned int idx = (__btrc_hash(key) % self->cap);
    while (self->occupied[idx]) {
        if (__btrc_eq(self->keys[idx], key)) {
            return true;
        }
        (idx = ((idx + 1) % self->cap));
    }
    return false;
}

bool btrc_Map_string_bool_contains(btrc_Map_string_bool* self, char* key) {
    return btrc_Map_string_bool_has(self, key);
}

void btrc_Map_string_bool_putIfAbsent(btrc_Map_string_bool* self, char* key, bool value) {
    if (!btrc_Map_string_bool_has(self, key)) {
        btrc_Map_string_bool_put(self, key, value);
    }
}

void btrc_Map_string_bool_free(btrc_Map_string_bool* self) {
    free(self->keys);
    free(self->values);
    free(self->occupied);
    (self->keys = NULL);
    (self->values = NULL);
    (self->occupied = NULL);
    (self->cap = 0);
    (self->len = 0);
}

void btrc_Map_string_bool_remove(btrc_Map_string_bool* self, char* key) {
    unsigned int idx = (__btrc_hash(key) % self->cap);
    while (self->occupied[idx]) {
        if (__btrc_eq(self->keys[idx], key)) {
            (self->occupied[idx] = false);
            (self->len--);
            unsigned int j = ((idx + 1) % self->cap);
            while (self->occupied[j]) {
                char* rk = self->keys[j];
                bool rv = self->values[j];
                (self->occupied[j] = false);
                (self->len--);
                btrc_Map_string_bool_put(self, rk, rv);
                (j = ((j + 1) % self->cap));
            }
            return;
        }
        (idx = ((idx + 1) % self->cap));
    }
}

void btrc_Map_string_bool_clear(btrc_Map_string_bool* self) {
    for (int i = 0; (i < self->cap); (i++)) {
        (self->occupied[i] = false);
    }
    (self->len = 0);
}

int btrc_Map_string_bool_size(btrc_Map_string_bool* self) {
    return self->len;
}

bool btrc_Map_string_bool_isEmpty(btrc_Map_string_bool* self) {
    return (self->len == 0);
}

btrc_Vector_string* btrc_Map_string_bool_keys(btrc_Map_string_bool* self) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    for (int i = 0; (i < self->cap); (i++)) {
        if (self->occupied[i]) {
            btrc_Vector_string_push(result, self->keys[i]);
        }
    }
    return result;
}

btrc_Vector_bool* btrc_Map_string_bool_values(btrc_Map_string_bool* self) {
    btrc_Vector_bool* result = btrc_Vector_bool_new();
    for (int i = 0; (i < self->cap); (i++)) {
        if (self->occupied[i]) {
            btrc_Vector_bool_push(result, self->values[i]);
        }
    }
    return result;
}

bool btrc_Map_string_bool_containsValue(btrc_Map_string_bool* self, bool value) {
    for (int i = 0; (i < self->cap); (i++)) {
        if (self->occupied[i] && __btrc_eq(self->values[i], value)) {
            return true;
        }
    }
    return false;
}

void btrc_Map_string_bool_set(btrc_Map_string_bool* self, char* key, bool value) {
    btrc_Map_string_bool_put(self, key, value);
}

void btrc_Map_string_bool_merge(btrc_Map_string_bool* self, btrc_Map_string_bool* other) {
    for (int i = 0; (i < other->cap); (i++)) {
        if (other->occupied[i]) {
            btrc_Map_string_bool_put(self, other->keys[i], other->values[i]);
        }
    }
}

int btrc_Map_string_bool_iterLen(btrc_Map_string_bool* self) {
    return self->len;
}

char* btrc_Map_string_bool_iterGet(btrc_Map_string_bool* self, int n) {
    int count = 0;
    for (int i = 0; (i < self->cap); (i++)) {
        if (self->occupied[i]) {
            if (count == n) {
                return self->keys[i];
            }
            (count++);
        }
    }
    fprintf(stderr, "Map iterGet: index out of bounds\n");
    exit(1);
    return self->keys[0];
}

bool btrc_Map_string_bool_iterValueAt(btrc_Map_string_bool* self, int n) {
    int count = 0;
    for (int i = 0; (i < self->cap); (i++)) {
        if (self->occupied[i]) {
            if (count == n) {
                return self->values[i];
            }
            (count++);
        }
    }
    fprintf(stderr, "Map iterValueAt: index out of bounds\n");
    exit(1);
    return self->values[0];
}

void Strings_init(Strings* self) {
    self->__rc = 1;
}

Strings* Strings_new(void) {
    Strings* self = ((Strings*)malloc(sizeof(Strings)));
    memset(self, 0, sizeof(Strings));
    Strings_init(self);
    return self;
}

void Strings_destroy(Strings* self) {
    free(self);
}

char* Strings_copy(char* s) {
    int __fstr_1_len = snprintf(NULL, 0, "%s", s);
    char* __fstr_1_buf = __btrc_str_track(((char*)malloc((__fstr_1_len + 1))));
    snprintf(__fstr_1_buf, (__fstr_1_len + 1), "%s", s);
    return __fstr_1_buf;
}

char* Strings_repeat(char* s, int count) {
    int slen = ((int)strlen(s));
    int total = (slen * count);
    char* result = ((char*)malloc((total + 1)));
    for (int i = 0; (i < count); (i++)) {
        memcpy((result + (i * slen)), s, slen);
    }
    (result[total] = '\0');
    return result;
}

char* Strings_join(btrc_Vector_string* items, char* sep) {
    if (items->len == 0) {
        return "";
    }
    int seplen = ((int)strlen(sep));
    int total = 0;
    for (int i = 0; (i < items->len); (i++)) {
        (total = (total + ((int)strlen(btrc_Vector_string_get(items, i)))));
    }
    (total = (total + (seplen * (items->len - 1))));
    char* result = ((char*)malloc((total + 1)));
    int pos = 0;
    int first_len = ((int)strlen(btrc_Vector_string_get(items, 0)));
    memcpy(result, btrc_Vector_string_get(items, 0), first_len);
    (pos = first_len);
    for (int i = 1; (i < items->len); (i++)) {
        memcpy((result + pos), sep, seplen);
        (pos = (pos + seplen));
        int item_len = ((int)strlen(btrc_Vector_string_get(items, i)));
        memcpy((result + pos), btrc_Vector_string_get(items, i), item_len);
        (pos = (pos + item_len));
    }
    (result[pos] = '\0');
    return result;
}

char* Strings_replace(char* s, char* old, char* replacement) {
    if (s == NULL) {
        return "";
    }
    if ((old == NULL) || (replacement == NULL)) {
        return Strings_copy(s);
    }
    int slen = ((int)strlen(s));
    int oldlen = ((int)strlen(old));
    if (oldlen == 0) {
        return Strings_copy(s);
    }
    int replen = ((int)strlen(replacement));
    int cap = ((slen * 2) + 1);
    char* result = ((char*)malloc(cap));
    int rlen = 0;
    int i = 0;
    while (i < slen) {
        if (((i + oldlen) <= slen) && (strncmp((s + i), old, oldlen) == 0)) {
            while ((rlen + replen) >= cap) {
                (cap = (cap * 2));
                (result = ((char*)realloc(result, cap)));
            }
            memcpy((result + rlen), replacement, replen);
            (rlen = (rlen + replen));
            (i = (i + oldlen));
        } else {
            if ((rlen + 1) >= cap) {
                (cap = (cap * 2));
                (result = ((char*)realloc(result, cap)));
            }
            (result[rlen] = s[i]);
            (rlen++);
            (i++);
        }
    }
    (result[rlen] = '\0');
    return result;
}

btrc_Vector_string* Strings_split(char* s, char* delim) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    if ((s == NULL) || (delim == NULL)) {
        return result;
    }
    int dlen = ((int)strlen(delim));
    if (dlen == 0) {
        return result;
    }
    char* p = s;
    while (*p) {
        char* found = strstr(p, delim);
        int seglen = ((found != NULL) ? ((int)(found - p)) : ((int)strlen(p)));
        char* item = ((char*)malloc((seglen + 1)));
        memcpy(item, p, seglen);
        (item[seglen] = '\0');
        btrc_Vector_string_push(result, item);
        if (found == NULL) {
            break;
        }
        (p = (found + dlen));
    }
    return result;
}

bool Strings_isDigit(char c) {
    return ((c >= '0') && (c <= '9'));
}

bool Strings_isAlpha(char c) {
    return (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')));
}

bool Strings_isAlnum(char c) {
    return (Strings_isAlpha(c) || Strings_isDigit(c));
}

bool Strings_isSpace(char c) {
    return ((((c == ' ') || (c == '\t')) || (c == '\n')) || (c == '\r'));
}

int Strings_toInt(char* s) {
    if (s == NULL) {
        return 0;
    }
    char* value = __btrc_str_track(__btrc_trim(s));
    if (__btrc_isEmpty(value)) {
        return 0;
    }
    int sign = 1;
    int i = 0;
    if (__btrc_startsWith(value, "-")) {
        (sign = (-1));
        (i = 1);
    } else if (__btrc_startsWith(value, "+")) {
        (i = 1);
    }
    int result = 0;
    while ((i < ((int)strlen(value))) && Strings_isDigit(value[i])) {
        (result = ((result * 10) + (value[i] - '0')));
        (i++);
    }
    return (result * sign);
}

float Strings_toFloat(char* s) {
    return ((float)atof(s));
}

int Strings_count(char* s, char* sub) {
    int slen = ((int)strlen(s));
    int sublen = ((int)strlen(sub));
    if (sublen == 0) {
        return 0;
    }
    int n = 0;
    int i = 0;
    while ((i + sublen) <= slen) {
        if (strncmp((s + i), sub, sublen) == 0) {
            (n++);
            (i = (i + sublen));
        } else {
            (i++);
        }
    }
    return n;
}

int Strings_find(char* s, char* sub, int start) {
    int slen = ((int)strlen(s));
    int sublen = ((int)strlen(sub));
    if (start < 0) {
        (start = 0);
    }
    if (sublen == 0) {
        return start;
    }
    int i = start;
    while ((i + sublen) <= slen) {
        if (strncmp((s + i), sub, sublen) == 0) {
            return i;
        }
        (i++);
    }
    return (-1);
}

int Strings_rfind(char* s, char* sub) {
    int slen = ((int)strlen(s));
    int sublen = ((int)strlen(sub));
    if (sublen == 0) {
        return slen;
    }
    int i = (slen - sublen);
    while (i >= 0) {
        if (strncmp((s + i), sub, sublen) == 0) {
            return i;
        }
        (i--);
    }
    return (-1);
}

int Strings_compare(char* left, char* right) {
    if ((left == NULL) && (right == NULL)) {
        return 0;
    }
    if (left == NULL) {
        return (-1);
    }
    if (right == NULL) {
        return 1;
    }
    int i = 0;
    while ((left[i] != '\0') && (right[i] != '\0')) {
        if (left[i] < right[i]) {
            return (-1);
        }
        if (left[i] > right[i]) {
            return 1;
        }
        (i++);
    }
    if ((left[i] == '\0') && (right[i] == '\0')) {
        return 0;
    }
    if (left[i] == '\0') {
        return (-1);
    }
    return 1;
}

bool Strings_lessThan(char* left, char* right) {
    return (Strings_compare(left, right) < 0);
}

char* Strings_capitalize(char* s) {
    int slen = ((int)strlen(s));
    char* result = ((char*)malloc((slen + 1)));
    for (int i = 0; (i < slen); (i++)) {
        (result[i] = ((char)tolower(((unsigned char)s[i]))));
    }
    if (slen > 0) {
        (result[0] = ((char)toupper(((unsigned char)s[0]))));
    }
    (result[slen] = '\0');
    return result;
}

char* Strings_title(char* s) {
    int slen = ((int)strlen(s));
    char* result = ((char*)malloc((slen + 1)));
    bool newWord = true;
    for (int i = 0; (i < slen); (i++)) {
        char c = s[i];
        if ((((c == ' ') || (c == '\t')) || (c == '\n')) || (c == '\r')) {
            (result[i] = c);
            (newWord = true);
        } else {
            if (newWord) {
                (result[i] = ((char)toupper(((unsigned char)c))));
            } else {
                (result[i] = ((char)tolower(((unsigned char)c))));
            }
            (newWord = false);
        }
    }
    (result[slen] = '\0');
    return result;
}

char* Strings_swapCase(char* s) {
    int slen = ((int)strlen(s));
    char* result = ((char*)malloc((slen + 1)));
    for (int i = 0; (i < slen); (i++)) {
        char c = s[i];
        if ((c >= 'A') && (c <= 'Z')) {
            (result[i] = ((char)tolower(((unsigned char)c))));
        } else if ((c >= 'a') && (c <= 'z')) {
            (result[i] = ((char)toupper(((unsigned char)c))));
        } else {
            (result[i] = c);
        }
    }
    (result[slen] = '\0');
    return result;
}

char* Strings_padLeft(char* s, int width, char fill) {
    int slen = ((int)strlen(s));
    if (slen >= width) {
        return Strings_copy(s);
    }
    int pad = (width - slen);
    char* result = ((char*)malloc((width + 1)));
    for (int i = 0; (i < pad); (i++)) {
        (result[i] = fill);
    }
    memcpy((result + pad), s, slen);
    (result[width] = '\0');
    return result;
}

char* Strings_padRight(char* s, int width, char fill) {
    int slen = ((int)strlen(s));
    if (slen >= width) {
        return Strings_copy(s);
    }
    int pad = (width - slen);
    char* result = ((char*)malloc((width + 1)));
    memcpy(result, s, slen);
    for (int i = 0; (i < pad); (i++)) {
        (result[(slen + i)] = fill);
    }
    (result[width] = '\0');
    return result;
}

char* Strings_center(char* s, int width, char fill) {
    int slen = ((int)strlen(s));
    if (slen >= width) {
        return Strings_copy(s);
    }
    int total_pad = (width - slen);
    int left_pad = __btrc_div_int(total_pad, 2);
    int right_pad = (total_pad - left_pad);
    char* result = ((char*)malloc((width + 1)));
    for (int i = 0; (i < left_pad); (i++)) {
        (result[i] = fill);
    }
    memcpy((result + left_pad), s, slen);
    for (int i = 0; (i < right_pad); (i++)) {
        (result[((left_pad + slen) + i)] = fill);
    }
    (result[width] = '\0');
    return result;
}

char* Strings_lstrip(char* s) {
    int slen = ((int)strlen(s));
    int start = 0;
    while ((start < slen) && ((((s[start] == ' ') || (s[start] == '\t')) || (s[start] == '\n')) || (s[start] == '\r'))) {
        (start++);
    }
    int newlen = (slen - start);
    char* result = ((char*)malloc((newlen + 1)));
    memcpy(result, (s + start), newlen);
    (result[newlen] = '\0');
    return result;
}

char* Strings_rstrip(char* s) {
    int slen = ((int)strlen(s));
    int end = slen;
    while ((end > 0) && ((((s[(end - 1)] == ' ') || (s[(end - 1)] == '\t')) || (s[(end - 1)] == '\n')) || (s[(end - 1)] == '\r'))) {
        (end--);
    }
    char* result = ((char*)malloc((end + 1)));
    memcpy(result, s, end);
    (result[end] = '\0');
    return result;
}

char* Strings_removePrefix(char* s, char* prefix) {
    if (!__btrc_startsWith(s, prefix)) {
        return Strings_copy(s);
    }
    return __btrc_str_track(__btrc_substring(s, ((int)strlen(prefix)), (((int)strlen(s)) - ((int)strlen(prefix)))));
}

char* Strings_fromInt(int n) {
    char* buf = ((char*)malloc(32));
    snprintf(buf, 32, "%d", n);
    return buf;
}

char* Strings_fromFloat(float f) {
    char* buf = ((char*)malloc(64));
    snprintf(buf, 64, "%g", f);
    return buf;
}

bool Strings_isDigitStr(char* s) {
    int slen = ((int)strlen(s));
    if (slen == 0) {
        return false;
    }
    for (int i = 0; (i < slen); (i++)) {
        if ((s[i] < '0') || (s[i] > '9')) {
            return false;
        }
    }
    return true;
}

bool Strings_isAlphaStr(char* s) {
    int slen = ((int)strlen(s));
    if (slen == 0) {
        return false;
    }
    for (int i = 0; (i < slen); (i++)) {
        char c = s[i];
        if (!(((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')))) {
            return false;
        }
    }
    return true;
}

bool Strings_isBlank(char* s) {
    int slen = ((int)strlen(s));
    for (int i = 0; (i < slen); (i++)) {
        char c = s[i];
        if ((((c != ' ') && (c != '\t')) && (c != '\n')) && (c != '\r')) {
            return false;
        }
    }
    return true;
}

void UnixPlatform_init(UnixPlatform* self) {
    self->__rc = 1;
}

UnixPlatform* UnixPlatform_new(void) {
    UnixPlatform* self = ((UnixPlatform*)malloc(sizeof(UnixPlatform)));
    memset(self, 0, sizeof(UnixPlatform));
    UnixPlatform_init(self);
    return self;
}

void UnixPlatform_destroy(UnixPlatform* self) {
    free(self);
}

int UnixPlatform_pid(void) {
    return ((int)getpid());
}

int UnixPlatform_euid(void) {
    return ((int)geteuid());
}

void Platform_init(Platform* self) {
    self->__rc = 1;
}

Platform* Platform_new(void) {
    Platform* self = ((Platform*)malloc(sizeof(Platform)));
    memset(self, 0, sizeof(Platform));
    Platform_init(self);
    return self;
}

void Platform_destroy(Platform* self) {
    free(self);
}

bool Platform_isUnix(void) {
    return true;
}

bool Platform_isWindows(void) {
    return false;
}

char* Platform_pathSeparator(void) {
    return "/";
}

int Platform_pid(void) {
    return UnixPlatform_pid();
}

int Platform_euid(void) {
    return UnixPlatform_euid();
}

bool Platform_isRoot(void) {
    return (Platform_euid() == 0);
}

void Environment_init(Environment* self) {
    self->__rc = 1;
}

Environment* Environment_new(void) {
    Environment* self = ((Environment*)malloc(sizeof(Environment)));
    memset(self, 0, sizeof(Environment));
    Environment_init(self);
    return self;
}

void Environment_destroy(Environment* self) {
    free(self);
}

char* Environment_get(char* name, char* fallback) {
    char* value = getenv(name);
    if ((value == NULL) || __btrc_isEmpty(value)) {
        return fallback;
    }
    return Strings_copy(value);
}

bool Environment_has(char* name) {
    char* value = getenv(name);
    return ((value != NULL) && (!__btrc_isEmpty(value)));
}

void ProcessStatus_init(ProcessStatus* self, int raw) {
    self->__rc = 1;
    (self->raw = raw);
}

ProcessStatus* ProcessStatus_new(int raw) {
    ProcessStatus* self = ((ProcessStatus*)malloc(sizeof(ProcessStatus)));
    memset(self, 0, sizeof(ProcessStatus));
    ProcessStatus_init(self, raw);
    return self;
}

void ProcessStatus_destroy(ProcessStatus* self) {
    free(self);
}

int ProcessStatus_code(ProcessStatus* self) {
    if (self->raw == (-1)) {
        return 127;
    }
    if (self->raw > 255) {
        return __btrc_div_int(self->raw, 256);
    }
    return self->raw;
}

bool ProcessStatus_ok(ProcessStatus* self) {
    return (ProcessStatus_code(self) == 0);
}

void UnixPipe_init(UnixPipe* self, char* command) {
    self->__rc = 1;
    (self->command = command);
    (self->handle = popen(command, "r"));
}

UnixPipe* UnixPipe_new(char* command) {
    UnixPipe* self = ((UnixPipe*)malloc(sizeof(UnixPipe)));
    memset(self, 0, sizeof(UnixPipe));
    UnixPipe_init(self, command);
    return self;
}

void UnixPipe_destroy(UnixPipe* self) {
    if (self->handle != NULL) {
        pclose(self->handle);
        (self->handle = NULL);
    }
    free(self);
}

bool UnixPipe_ok(UnixPipe* self) {
    return (self->handle != NULL);
}

char* UnixPipe_readAll(UnixPipe* self) {
    if (!UnixPipe_ok(self)) {
        return "";
    }
    int cap = 4096;
    int len = 0;
    char* buffer = ((char*)malloc(cap));
    int ch = fgetc(self->handle);
    while (ch != EOF) {
        if ((len + 2) >= cap) {
            (cap = (cap * 2));
            (buffer = ((char*)realloc(buffer, cap)));
        }
        (buffer[len] = ((char)ch));
        (len++);
        (ch = fgetc(self->handle));
    }
    (buffer[len] = '\0');
    return buffer;
}

ProcessStatus* UnixPipe_close(UnixPipe* self) {
    if (!UnixPipe_ok(self)) {
        return ProcessStatus_new((-1));
    }
    int raw = pclose(self->handle);
    (self->handle = NULL);
    return ProcessStatus_new(raw);
}

void UnixProcess_init(UnixProcess* self) {
    self->__rc = 1;
}

UnixProcess* UnixProcess_new(void) {
    UnixProcess* self = ((UnixProcess*)malloc(sizeof(UnixProcess)));
    memset(self, 0, sizeof(UnixProcess));
    UnixProcess_init(self);
    return self;
}

void UnixProcess_destroy(UnixProcess* self) {
    free(self);
}

ProcessStatus* UnixProcess_system(char* command) {
    return ProcessStatus_new(system(command));
}

UnixPipe* UnixProcess_pipe(char* command) {
    return UnixPipe_new(command);
}

void ShellWords_init(ShellWords* self) {
    self->__rc = 1;
}

ShellWords* ShellWords_new(void) {
    ShellWords* self = ((ShellWords*)malloc(sizeof(ShellWords)));
    memset(self, 0, sizeof(ShellWords));
    ShellWords_init(self);
    return self;
}

void ShellWords_destroy(ShellWords* self) {
    free(self);
}

bool ShellWords_isSafeArgChar(char c) {
    if ((c >= 'a') && (c <= 'z')) {
        return true;
    }
    if ((c >= 'A') && (c <= 'Z')) {
        return true;
    }
    if ((c >= '0') && (c <= '9')) {
        return true;
    }
    return ((((((((c == '_') || (c == '-')) || (c == '.')) || (c == '/')) || (c == ':')) || (c == '=')) || (c == ',')) || (c == '+'));
}

bool ShellWords_isSafeArg(char* raw) {
    int len = ((int)strlen(raw));
    if (len == 0) {
        return false;
    }
    for (int __i_2 = 0; (raw[__i_2] != '\0'); (__i_2++)) {
        char ch = raw[__i_2];
        if (!ShellWords_isSafeArgChar(ch)) {
            return false;
        }
    }
    return true;
}

char* ShellWords_quote(char* raw) {
    if (ShellWords_isSafeArg(raw)) {
        return Strings_copy(raw);
    }
    char* escaped = Strings_replace(raw, "'", "'\\''");
    return __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("'", escaped)), "'"));
}

char* ShellWords_redact(char* text, char* sensitive) {
    if (__btrc_isEmpty(sensitive)) {
        return text;
    }
    return Strings_replace(text, sensitive, "***");
}

void ExecResult_init(ExecResult* self, int code, char* out, char* err, char* command) {
    self->__rc = 1;
    (self->code = code);
    (self->out = out);
    (self->err = err);
    (self->command = command);
}

ExecResult* ExecResult_new(int code, char* out, char* err, char* command) {
    ExecResult* self = ((ExecResult*)malloc(sizeof(ExecResult)));
    memset(self, 0, sizeof(ExecResult));
    ExecResult_init(self, code, out, err, command);
    return self;
}

void ExecResult_destroy(ExecResult* self) {
    free(self);
}

bool ExecResult_ok(ExecResult* self) {
    return (self->code == 0);
}

char* ExecResult_stdout(ExecResult* self) {
    return self->out;
}

char* ExecResult_trimmed(ExecResult* self) {
    return __btrc_str_track(__btrc_trim(self->out));
}

char* ExecResult_stderr(ExecResult* self) {
    return self->err;
}

void Command_init(Command* self, char* executable) {
    self->__rc = 1;
    (self->executable = executable);
    if (self->args != NULL) {
        if ((--self->args->__rc) <= 0) {
            btrc_Vector_string_free(self->args);
        }
    }
    btrc_Vector_string* __list_4 = btrc_Vector_string_new();
    (self->args = __list_4);
    btrc_Vector_string* __list_3 = btrc_Vector_string_new();
    (__list_3->__rc++);
    if (self->env != NULL) {
        if ((--self->env->__rc) <= 0) {
            btrc_Vector_string_free(self->env);
        }
    }
    btrc_Vector_string* __list_6 = btrc_Vector_string_new();
    (self->env = __list_6);
    btrc_Vector_string* __list_5 = btrc_Vector_string_new();
    (__list_5->__rc++);
    (self->useSudo = false);
    (self->captureOutput = true);
    (self->checkStatus = true);
    (self->mergeStderr = true);
    (self->sensitive = "");
}

Command* Command_new(char* executable) {
    Command* self = ((Command*)malloc(sizeof(Command)));
    memset(self, 0, sizeof(Command));
    Command_init(self, executable);
    return self;
}

void Command_destroy(Command* self) {
    if (self->args != NULL) {
        if ((--self->args->__rc) <= 0) {
            btrc_Vector_string_free(self->args);
        }
    }
    if (self->env != NULL) {
        if ((--self->env->__rc) <= 0) {
            btrc_Vector_string_free(self->env);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

Command* Command_arg(Command* self, char* value) {
    btrc_Vector_string_push(self->args, value);
    return self;
}

Command* Command_flag(Command* self, char* name, char* value) {
    btrc_Vector_string_push(self->args, name);
    btrc_Vector_string_push(self->args, value);
    return self;
}

Command* Command_envVar(Command* self, char* name, char* value) {
    int __fstr_8_len = snprintf(NULL, 0, "%s=%s", name, value);
    char* __fstr_8_buf = __btrc_str_track(((char*)malloc((__fstr_8_len + 1))));
    snprintf(__fstr_8_buf, (__fstr_8_len + 1), "%s=%s", name, value);
    btrc_Vector_string_push(self->env, __fstr_8_buf);
    return self;
}

Command* Command_sudo(Command* self, bool enabled) {
    (self->useSudo = enabled);
    return self;
}

Command* Command_capture(Command* self, bool enabled) {
    (self->captureOutput = enabled);
    return self;
}

Command* Command_check(Command* self, bool enabled) {
    (self->checkStatus = enabled);
    return self;
}

Command* Command_mergeError(Command* self, bool enabled) {
    (self->mergeStderr = enabled);
    return self;
}

Command* Command_redact(Command* self, char* value) {
    (self->sensitive = value);
    return self;
}

char* Command_renderEnv(Command* self, char* item) {
    int split = Strings_find(item, "=", 0);
    if (split <= 0) {
        return ShellWords_quote(item);
    }
    char* name = __btrc_str_track(__btrc_substring(item, 0, split));
    char* value = __btrc_str_track(__btrc_substring(item, (split + 1), ((((int)strlen(item)) - split) - 1)));
    return __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(name, "=")), ShellWords_quote(value)));
}

char* Command_render(Command* self) {
    btrc_Vector_string* parts = btrc_Vector_string_new();
    int __n_10 = btrc_Vector_string_iterLen(self->env);
    for (int __i_9 = 0; (__i_9 < __n_10); (__i_9++)) {
        char* item = btrc_Vector_string_iterGet(self->env, __i_9);
        btrc_Vector_string_push(parts, Command_renderEnv(self, item));
    }
    if (self->useSudo) {
        btrc_Vector_string_push(parts, "sudo");
    }
    btrc_Vector_string_push(parts, ShellWords_quote(self->executable));
    int __n_12 = btrc_Vector_string_iterLen(self->args);
    for (int __i_11 = 0; (__i_11 < __n_12); (__i_11++)) {
        char* item = btrc_Vector_string_iterGet(self->args, __i_11);
        btrc_Vector_string_push(parts, ShellWords_quote(item));
    }
    if (self->mergeStderr) {
        btrc_Vector_string_push(parts, "2>&1");
    }
    return btrc_Vector_string_join(parts, " ");
}

void UnixShell_init(UnixShell* self) {
    self->__rc = 1;
    (self->logCommands = false);
    (self->chrootPath = "");
}

UnixShell* UnixShell_new(void) {
    UnixShell* self = ((UnixShell*)malloc(sizeof(UnixShell)));
    memset(self, 0, sizeof(UnixShell));
    UnixShell_init(self);
    return self;
}

void UnixShell_destroy(UnixShell* self) {
    free(self);
}

char* UnixShell_quote(char* raw) {
    return ShellWords_quote(raw);
}

char* UnixShell_redactText(char* text, char* sensitive) {
    return ShellWords_redact(text, sensitive);
}

int UnixShell_statusCode(int rawStatus) {
    ProcessStatus* status = ProcessStatus_new(rawStatus);
    int code = ProcessStatus_code(status);
    if (status != NULL) {
        if ((--status->__rc) <= 0) {
            ProcessStatus_destroy(status);
        }
    }
    return code;
    if (status != NULL) {
        if ((--status->__rc) <= 0) {
            ProcessStatus_destroy(status);
        }
    }
}

void UnixShell_chroot(UnixShell* self, char* path) {
    (self->chrootPath = path);
}

void UnixShell_clearChroot(UnixShell* self) {
    (self->chrootPath = "");
}

ExecResult* UnixShell_run(UnixShell* self, char* command) {
    return UnixShell_runRaw(self, command, true, true, "");
}

ExecResult* UnixShell_runUnchecked(UnixShell* self, char* command) {
    return UnixShell_runRaw(self, command, true, false, "");
}

ExecResult* UnixShell_runCommand(UnixShell* self, Command* command) {
    return UnixShell_runRaw(self, Command_render(command), command->captureOutput, command->checkStatus, command->sensitive);
}

char* UnixShell_capture(UnixShell* self, Command* command) {
    return ExecResult_trimmed(UnixShell_runCommand(self, command));
}

ExecResult* UnixShell_runRaw(UnixShell* self, char* command, bool captureOutput, bool checkStatus, char* sensitive) {
    char* rendered = command;
    if (((int)strlen(self->chrootPath)) > 0) {
        int __fstr_13_len = snprintf(NULL, 0, "nixos-enter --root %s --command %s", ShellWords_quote(self->chrootPath), ShellWords_quote(command));
        char* __fstr_13_buf = __btrc_str_track(((char*)malloc((__fstr_13_len + 1))));
        snprintf(__fstr_13_buf, (__fstr_13_len + 1), "nixos-enter --root %s --command %s", ShellWords_quote(self->chrootPath), ShellWords_quote(command));
        (rendered = __fstr_13_buf);
    }
    if (self->logCommands) {
        char* visible = UnixShell_redactText(rendered, sensitive);
        fprintf(stderr, "LOG: %s\n", visible);
    }
    if (!captureOutput) {
        ProcessStatus* status = UnixProcess_system(rendered);
        int code = ProcessStatus_code(status);
        if (checkStatus && (code != 0)) {
            fprintf(stderr, "Command failed (%d): %s\n", code, UnixShell_redactText(rendered, sensitive));
        }
        return ExecResult_new(code, "", "", rendered);
    }
    UnixPipe* pipe = UnixProcess_pipe(rendered);
    if (!UnixPipe_ok(pipe)) {
        return ExecResult_new(127, "", "popen failed", rendered);
    }
    char* output = UnixPipe_readAll(pipe);
    ProcessStatus* status = UnixPipe_close(pipe);
    int code = ProcessStatus_code(status);
    if (checkStatus && (code != 0)) {
        fprintf(stderr, "Command failed (%d): %s\n", code, UnixShell_redactText(rendered, sensitive));
    }
    return ExecResult_new(code, output, "", rendered);
}

void PowerShell_init(PowerShell* self) {
    self->__rc = 1;
}

PowerShell* PowerShell_new(void) {
    PowerShell* self = ((PowerShell*)malloc(sizeof(PowerShell)));
    memset(self, 0, sizeof(PowerShell));
    PowerShell_init(self);
    return self;
}

void PowerShell_destroy(PowerShell* self) {
    free(self);
}

ExecResult* PowerShell_run(PowerShell* self, char* command) {
    return ExecResult_new(127, "", "PowerShell support is TODO", command);
}

void FileStatus_init(FileStatus* self, char* path) {
    self->__rc = 1;
    (self->path = path);
    struct stat st;
    (self->found = (stat(path, (&st)) == 0));
    (self->mode = (self->found ? ((int)st.st_mode) : 0));
    struct stat lst;
    (self->linkFound = (lstat(path, (&lst)) == 0));
    (self->linkMode = (self->linkFound ? ((int)lst.st_mode) : 0));
}

FileStatus* FileStatus_new(char* path) {
    FileStatus* self = ((FileStatus*)malloc(sizeof(FileStatus)));
    memset(self, 0, sizeof(FileStatus));
    FileStatus_init(self, path);
    return self;
}

void FileStatus_destroy(FileStatus* self) {
    free(self);
}

bool FileStatus_exists(FileStatus* self) {
    return self->found;
}

bool FileStatus_isDir(FileStatus* self) {
    return (self->found && S_ISDIR(self->mode));
}

bool FileStatus_isFile(FileStatus* self) {
    return (self->found && S_ISREG(self->mode));
}

bool FileStatus_isSymlink(FileStatus* self) {
    return (self->linkFound && S_ISLNK(self->linkMode));
}

void Directory_init(Directory* self, char* path) {
    self->__rc = 1;
    (self->path = path);
}

Directory* Directory_new(char* path) {
    Directory* self = ((Directory*)malloc(sizeof(Directory)));
    memset(self, 0, sizeof(Directory));
    Directory_init(self, path);
    return self;
}

void Directory_destroy(Directory* self) {
    free(self);
}

btrc_Vector_string* Directory_entries(Directory* self) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    DIR* dir = opendir(self->path);
    if (dir == NULL) {
        return result;
    }
    struct dirent* entry = readdir(dir);
    while (entry != NULL) {
        char* name = entry->d_name;
        if ((!(strcmp(name, ".") == 0)) && (!(strcmp(name, "..") == 0))) {
            btrc_Vector_string_push(result, Strings_copy(name));
        }
        (entry = readdir(dir));
    }
    closedir(dir);
    return result;
}

void UnixFileSystem_init(UnixFileSystem* self) {
    self->__rc = 1;
}

UnixFileSystem* UnixFileSystem_new(void) {
    UnixFileSystem* self = ((UnixFileSystem*)malloc(sizeof(UnixFileSystem)));
    memset(self, 0, sizeof(UnixFileSystem));
    UnixFileSystem_init(self);
    return self;
}

void UnixFileSystem_destroy(UnixFileSystem* self) {
    free(self);
}

int UnixFileSystem_statusCode(int raw) {
    if (raw == (-1)) {
        return 127;
    }
    if (raw > 255) {
        return __btrc_div_int(raw, 256);
    }
    return raw;
}

int UnixFileSystem_chmodPath(char* path, int mode) {
    return chmod(path, ((mode_t)mode));
}

int UnixFileSystem_mkdirPath(char* path, int mode) {
    return mkdir(path, ((mode_t)mode));
}

int UnixFileSystem_runShell(char* command) {
    return UnixFileSystem_statusCode(system(command));
}

int UnixFileSystem_mkdirp(char* path) {
    char* quoted = PathTools_shellQuote(path);
    int __fstr_14_len = snprintf(NULL, 0, "mkdir -p %s", quoted);
    char* __fstr_14_buf = __btrc_str_track(((char*)malloc((__fstr_14_len + 1))));
    snprintf(__fstr_14_buf, (__fstr_14_len + 1), "mkdir -p %s", quoted);
    return UnixFileSystem_runShell(__fstr_14_buf);
}

int UnixFileSystem_removeRecursive(char* path) {
    char* quoted = PathTools_shellQuote(path);
    int __fstr_15_len = snprintf(NULL, 0, "rm -rf %s", quoted);
    char* __fstr_15_buf = __btrc_str_track(((char*)malloc((__fstr_15_len + 1))));
    snprintf(__fstr_15_buf, (__fstr_15_len + 1), "rm -rf %s", quoted);
    return UnixFileSystem_runShell(__fstr_15_buf);
}

int UnixFileSystem_symlinkPath(char* target, char* linkPath) {
    return symlink(target, linkPath);
}

char* UnixFileSystem_readLink(char* path) {
    char buffer[4096];
    ssize_t length = readlink(path, buffer, 4095);
    if (length < 0) {
        return "";
    }
    (buffer[length] = '\0');
    return Strings_copy(buffer);
}

char* UnixFileSystem_tempDir(char* prefix) {
    char* base = Environment_get("TMPDIR", "/tmp");
    char* templatePath = PathTools_join(base, __btrc_str_track(__btrc_strcat(prefix, ".XXXXXX")));
    char* raw = Strings_copy(templatePath);
    char* result = mkdtemp(raw);
    if (result == NULL) {
        return "";
    }
    return Strings_copy(result);
}

void PathTools_init(PathTools* self) {
    self->__rc = 1;
}

PathTools* PathTools_new(void) {
    PathTools* self = ((PathTools*)malloc(sizeof(PathTools)));
    memset(self, 0, sizeof(PathTools));
    PathTools_init(self);
    return self;
}

void PathTools_destroy(PathTools* self) {
    free(self);
}

char* PathTools_shellQuote(char* raw) {
    return ShellWords_quote(raw);
}

char* PathTools_basename(char* path) {
    int len = ((int)strlen(path));
    if (len == 0) {
        return "";
    }
    int end = (len - 1);
    while ((end > 0) && (path[end] == '/')) {
        (end--);
    }
    int start = end;
    while ((start > 0) && (path[(start - 1)] != '/')) {
        (start--);
    }
    int outLen = ((end - start) + 1);
    char* result = ((char*)malloc((outLen + 1)));
    memcpy(result, (path + start), outLen);
    (result[outLen] = '\0');
    return result;
}

char* PathTools_dirname(char* path) {
    int len = ((int)strlen(path));
    if (len == 0) {
        return ".";
    }
    int end = (len - 1);
    while ((end > 0) && (path[end] == '/')) {
        (end--);
    }
    while ((end > 0) && (path[end] != '/')) {
        (end--);
    }
    if (end == 0) {
        if (path[0] == '/') {
            return "/";
        }
        return ".";
    }
    char* result = ((char*)malloc((end + 1)));
    memcpy(result, path, end);
    (result[end] = '\0');
    return result;
}

char* PathTools_join(char* left, char* right) {
    if (((int)strlen(left)) == 0) {
        return Strings_copy(right);
    }
    if (((int)strlen(right)) == 0) {
        return Strings_copy(left);
    }
    if (left[(((int)strlen(left)) - 1)] == '/') {
        int __fstr_16_len = snprintf(NULL, 0, "%s%s", left, right);
        char* __fstr_16_buf = __btrc_str_track(((char*)malloc((__fstr_16_len + 1))));
        snprintf(__fstr_16_buf, (__fstr_16_len + 1), "%s%s", left, right);
        return __fstr_16_buf;
    }
    int __fstr_17_len = snprintf(NULL, 0, "%s/%s", left, right);
    char* __fstr_17_buf = __btrc_str_track(((char*)malloc((__fstr_17_len + 1))));
    snprintf(__fstr_17_buf, (__fstr_17_len + 1), "%s/%s", left, right);
    return __fstr_17_buf;
}

void FileSystem_init(FileSystem* self) {
    self->__rc = 1;
}

FileSystem* FileSystem_new(void) {
    FileSystem* self = ((FileSystem*)malloc(sizeof(FileSystem)));
    memset(self, 0, sizeof(FileSystem));
    FileSystem_init(self);
    return self;
}

void FileSystem_destroy(FileSystem* self) {
    free(self);
}

bool FileSystem_exists(char* path) {
    FileStatus* status = FileStatus_new(path);
    bool result = FileStatus_exists(status);
    if (status != NULL) {
        if ((--status->__rc) <= 0) {
            FileStatus_destroy(status);
        }
    }
    return result;
    if (status != NULL) {
        if ((--status->__rc) <= 0) {
            FileStatus_destroy(status);
        }
    }
}

bool FileSystem_isDir(char* path) {
    FileStatus* status = FileStatus_new(path);
    bool result = FileStatus_isDir(status);
    if (status != NULL) {
        if ((--status->__rc) <= 0) {
            FileStatus_destroy(status);
        }
    }
    return result;
    if (status != NULL) {
        if ((--status->__rc) <= 0) {
            FileStatus_destroy(status);
        }
    }
}

bool FileSystem_isFile(char* path) {
    FileStatus* status = FileStatus_new(path);
    bool result = FileStatus_isFile(status);
    if (status != NULL) {
        if ((--status->__rc) <= 0) {
            FileStatus_destroy(status);
        }
    }
    return result;
    if (status != NULL) {
        if ((--status->__rc) <= 0) {
            FileStatus_destroy(status);
        }
    }
}

bool FileSystem_isSymlink(char* path) {
    FileStatus* status = FileStatus_new(path);
    bool result = FileStatus_isSymlink(status);
    if (status != NULL) {
        if ((--status->__rc) <= 0) {
            FileStatus_destroy(status);
        }
    }
    return result;
    if (status != NULL) {
        if ((--status->__rc) <= 0) {
            FileStatus_destroy(status);
        }
    }
}

int FileSystem_chmod(char* path, int mode) {
    return UnixFileSystem_chmodPath(path, mode);
}

int FileSystem_mkdir(char* path, int mode) {
    return UnixFileSystem_mkdirPath(path, mode);
}

int FileSystem_mkdirp(char* path) {
    return UnixFileSystem_mkdirp(path);
}

int FileSystem_removeRecursive(char* path) {
    return UnixFileSystem_removeRecursive(path);
}

int FileSystem_symlink(char* target, char* linkPath) {
    return UnixFileSystem_symlinkPath(target, linkPath);
}

char* FileSystem_readLink(char* path) {
    return UnixFileSystem_readLink(path);
}

char* FileSystem_tempDir(char* prefix) {
    return UnixFileSystem_tempDir(prefix);
}

btrc_Vector_string* FileSystem_listDir(char* path) {
    Directory* dir = Directory_new(path);
    btrc_Vector_string* result = Directory_entries(dir);
    if (dir != NULL) {
        if ((--dir->__rc) <= 0) {
            Directory_destroy(dir);
        }
    }
    return result;
    if (dir != NULL) {
        if ((--dir->__rc) <= 0) {
            Directory_destroy(dir);
        }
    }
}

char* FileSystem_readText(char* path) {
    return Path_readAll(path);
}

void FileSystem_writeText(char* path, char* content) {
    Path_writeAll(path, content);
}

void DaemonSpec_init(DaemonSpec* self, char* name, Command* command) {
    self->__rc = 1;
    (self->name = name);
    if (self->command != NULL) {
        if ((--self->command->__rc) <= 0) {
            Command_destroy(self->command);
        }
    }
    (self->command = command);
    (command->__rc++);
    int __fstr_18_len = snprintf(NULL, 0, "/tmp/%s.pid", name);
    char* __fstr_18_buf = __btrc_str_track(((char*)malloc((__fstr_18_len + 1))));
    snprintf(__fstr_18_buf, (__fstr_18_len + 1), "/tmp/%s.pid", name);
    (self->pidFile = __fstr_18_buf);
    int __fstr_19_len = snprintf(NULL, 0, "/tmp/%s.log", name);
    char* __fstr_19_buf = __btrc_str_track(((char*)malloc((__fstr_19_len + 1))));
    snprintf(__fstr_19_buf, (__fstr_19_len + 1), "/tmp/%s.log", name);
    (self->logFile = __fstr_19_buf);
    (self->workingDirectory = "");
    (self->autoRestart = false);
}

DaemonSpec* DaemonSpec_new(char* name, Command* command) {
    DaemonSpec* self = ((DaemonSpec*)malloc(sizeof(DaemonSpec)));
    memset(self, 0, sizeof(DaemonSpec));
    DaemonSpec_init(self, name, command);
    return self;
}

void DaemonSpec_destroy(DaemonSpec* self) {
    if (self->command != NULL) {
        if ((--self->command->__rc) <= 0) {
            Command_destroy(self->command);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

DaemonSpec* DaemonSpec_pid(DaemonSpec* self, char* path) {
    (self->pidFile = path);
    return self;
}

DaemonSpec* DaemonSpec_log(DaemonSpec* self, char* path) {
    (self->logFile = path);
    return self;
}

DaemonSpec* DaemonSpec_cwd(DaemonSpec* self, char* path) {
    (self->workingDirectory = path);
    return self;
}

DaemonSpec* DaemonSpec_restart(DaemonSpec* self, bool enabled) {
    (self->autoRestart = enabled);
    return self;
}

char* DaemonSpec_renderStartCommand(DaemonSpec* self) {
    char* rendered = Command_render(self->command);
    int __fstr_20_len = snprintf(NULL, 0, "cd %s && ", UnixShell_quote(self->workingDirectory));
    char* __fstr_20_buf = __btrc_str_track(((char*)malloc((__fstr_20_len + 1))));
    snprintf(__fstr_20_buf, (__fstr_20_len + 1), "cd %s && ", UnixShell_quote(self->workingDirectory));
    char* prefix = (__btrc_isEmpty(self->workingDirectory) ? "" : __fstr_20_buf);
    int __fstr_21_len = snprintf(NULL, 0, "%snohup %s >> %s 2>&1 & echo $! > %s", prefix, rendered, UnixShell_quote(self->logFile), UnixShell_quote(self->pidFile));
    char* __fstr_21_buf = __btrc_str_track(((char*)malloc((__fstr_21_len + 1))));
    snprintf(__fstr_21_buf, (__fstr_21_len + 1), "%snohup %s >> %s 2>&1 & echo $! > %s", prefix, rendered, UnixShell_quote(self->logFile), UnixShell_quote(self->pidFile));
    return __fstr_21_buf;
}

void DaemonController_init(DaemonController* self) {
    self->__rc = 1;
    if (self->shell != NULL) {
        if ((--self->shell->__rc) <= 0) {
            UnixShell_destroy(self->shell);
        }
    }
    (self->shell = UnixShell_new());
    (UnixShell_new()->__rc++);
}

DaemonController* DaemonController_new(void) {
    DaemonController* self = ((DaemonController*)malloc(sizeof(DaemonController)));
    memset(self, 0, sizeof(DaemonController));
    DaemonController_init(self);
    return self;
}

void DaemonController_destroy(DaemonController* self) {
    if (self->shell != NULL) {
        if ((--self->shell->__rc) <= 0) {
            UnixShell_destroy(self->shell);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

ExecResult* DaemonController_start(DaemonController* self, DaemonSpec* spec) {
    return UnixShell_runRaw(self->shell, DaemonSpec_renderStartCommand(spec), true, true, "");
}

ExecResult* DaemonController_stop(DaemonController* self, DaemonSpec* spec) {
    int __fstr_22_len = snprintf(NULL, 0, "if [ -f %s ]; then kill $(cat %s) && rm -f %s; fi", UnixShell_quote(spec->pidFile), UnixShell_quote(spec->pidFile), UnixShell_quote(spec->pidFile));
    char* __fstr_22_buf = __btrc_str_track(((char*)malloc((__fstr_22_len + 1))));
    snprintf(__fstr_22_buf, (__fstr_22_len + 1), "if [ -f %s ]; then kill $(cat %s) && rm -f %s; fi", UnixShell_quote(spec->pidFile), UnixShell_quote(spec->pidFile), UnixShell_quote(spec->pidFile));
    char* command = __fstr_22_buf;
    return UnixShell_runRaw(self->shell, command, true, false, "");
}

ExecResult* DaemonController_status(DaemonController* self, DaemonSpec* spec) {
    int __fstr_23_len = snprintf(NULL, 0, "test -f %s && kill -0 $(cat %s)", UnixShell_quote(spec->pidFile), UnixShell_quote(spec->pidFile));
    char* __fstr_23_buf = __btrc_str_track(((char*)malloc((__fstr_23_len + 1))));
    snprintf(__fstr_23_buf, (__fstr_23_len + 1), "test -f %s && kill -0 $(cat %s)", UnixShell_quote(spec->pidFile), UnixShell_quote(spec->pidFile));
    char* command = __fstr_23_buf;
    return UnixShell_runRaw(self->shell, command, true, false, "");
}

void AppSpec_init(AppSpec* self, char* name) {
    self->__rc = 1;
    (self->name = name);
    (self->version = "0.0.0");
}

AppSpec* AppSpec_new(char* name) {
    AppSpec* self = ((AppSpec*)malloc(sizeof(AppSpec)));
    memset(self, 0, sizeof(AppSpec));
    AppSpec_init(self, name);
    return self;
}

void AppSpec_destroy(AppSpec* self) {
    free(self);
}

AppSpec* AppSpec_withVersion(AppSpec* self, char* version) {
    (self->version = version);
    return self;
}

void DaemonApp_init(DaemonApp* self, char* name, DaemonSpec* daemon) {
    self->__rc = 1;
    (self->name = name);
    (self->version = "0.0.0");
    if (self->daemon != NULL) {
        if ((--self->daemon->__rc) <= 0) {
            DaemonSpec_destroy(self->daemon);
        }
    }
    (self->daemon = daemon);
    (daemon->__rc++);
}

DaemonApp* DaemonApp_new(char* name, DaemonSpec* daemon) {
    DaemonApp* self = ((DaemonApp*)malloc(sizeof(DaemonApp)));
    memset(self, 0, sizeof(DaemonApp));
    DaemonApp_init(self, name, daemon);
    return self;
}

void DaemonApp_destroy(DaemonApp* self) {
    if (self->daemon != NULL) {
        if ((--self->daemon->__rc) <= 0) {
            DaemonSpec_destroy(self->daemon);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

AppSpec* DaemonApp_withVersion(DaemonApp* self, char* version) {
    return AppSpec_withVersion(((AppSpec*)self), version);
}

void Html_init(Html* self) {
    self->__rc = 1;
}

Html* Html_new(void) {
    Html* self = ((Html*)malloc(sizeof(Html)));
    memset(self, 0, sizeof(Html));
    Html_init(self);
    return self;
}

void Html_destroy(Html* self) {
    free(self);
}

char* Html_escape(char* raw) {
    char* text = Strings_replace(raw, "&", "&amp;");
    (text = Strings_replace(text, "\"", "&quot;"));
    (text = Strings_replace(text, "<", "&lt;"));
    (text = Strings_replace(text, ">", "&gt;"));
    return text;
}

void UiNode_init(UiNode* self, char* tag) {
    self->__rc = 1;
    (self->tag = tag);
    (self->textContent = "");
    (self->rawText = false);
    if (self->attributes != NULL) {
        if ((--self->attributes->__rc) <= 0) {
            btrc_Vector_string_free(self->attributes);
        }
    }
    btrc_Vector_string* __list_25 = btrc_Vector_string_new();
    (self->attributes = __list_25);
    btrc_Vector_string* __list_24 = btrc_Vector_string_new();
    (__list_24->__rc++);
    if (self->children != NULL) {
        if ((--self->children->__rc) <= 0) {
            btrc_Vector_UiNode_free(self->children);
        }
    }
    btrc_Vector_UiNode* __list_27 = btrc_Vector_UiNode_new();
    (self->children = __list_27);
    btrc_Vector_UiNode* __list_26 = btrc_Vector_UiNode_new();
    (__list_26->__rc++);
}

UiNode* UiNode_new(char* tag) {
    UiNode* self = ((UiNode*)malloc(sizeof(UiNode)));
    memset(self, 0, sizeof(UiNode));
    UiNode_init(self, tag);
    return self;
}

void UiNode_destroy(UiNode* self) {
    if (self->attributes != NULL) {
        if ((--self->attributes->__rc) <= 0) {
            btrc_Vector_string_free(self->attributes);
        }
    }
    if (self->children != NULL) {
        if ((--self->children->__rc) <= 0) {
            btrc_Vector_UiNode_free(self->children);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

UiNode* UiNode_text(UiNode* self, char* value) {
    (self->textContent = value);
    (self->rawText = false);
    return self;
}

UiNode* UiNode_raw(UiNode* self, char* value) {
    (self->textContent = value);
    (self->rawText = true);
    return self;
}

UiNode* UiNode_attr(UiNode* self, char* name, char* value) {
    int __fstr_29_len = snprintf(NULL, 0, "%s='%s'", name, Html_escape(value));
    char* __fstr_29_buf = __btrc_str_track(((char*)malloc((__fstr_29_len + 1))));
    snprintf(__fstr_29_buf, (__fstr_29_len + 1), "%s='%s'", name, Html_escape(value));
    btrc_Vector_string_push(self->attributes, __fstr_29_buf);
    return self;
}

UiNode* UiNode_id(UiNode* self, char* value) {
    return UiNode_attr(self, "id", value);
}

UiNode* UiNode_className(UiNode* self, char* value) {
    return UiNode_attr(self, "class", value);
}

UiNode* UiNode_style(UiNode* self, char* value) {
    return UiNode_attr(self, "style", value);
}

UiNode* UiNode_child(UiNode* self, UiNode* node) {
    btrc_Vector_UiNode_push(self->children, node);
    return self;
}

UiNode* UiNode_childrenFrom(UiNode* self, btrc_Vector_UiNode* nodes) {
    int __n_31 = btrc_Vector_UiNode_iterLen(nodes);
    for (int __i_30 = 0; (__i_30 < __n_31); (__i_30++)) {
        UiNode* node = btrc_Vector_UiNode_iterGet(nodes, __i_30);
        btrc_Vector_UiNode_push(self->children, node);
    }
    return self;
}

char* UiNode_renderAttributes(UiNode* self) {
    if (self->attributes->len == 0) {
        return "";
    }
    return __btrc_str_track(__btrc_strcat(" ", btrc_Vector_string_join(self->attributes, " ")));
}

bool UiNode_isVoidElement(UiNode* self) {
    return ((((((strcmp(self->tag, "br") == 0) || (strcmp(self->tag, "hr") == 0)) || (strcmp(self->tag, "img") == 0)) || (strcmp(self->tag, "input") == 0)) || (strcmp(self->tag, "link") == 0)) || (strcmp(self->tag, "meta") == 0));
}

char* UiNode_renderHtml(UiNode* self) {
    char* attrs = UiNode_renderAttributes(self);
    if (UiNode_isVoidElement(self)) {
        int __fstr_32_len = snprintf(NULL, 0, "<%s%s>", self->tag, attrs);
        char* __fstr_32_buf = __btrc_str_track(((char*)malloc((__fstr_32_len + 1))));
        snprintf(__fstr_32_buf, (__fstr_32_len + 1), "<%s%s>", self->tag, attrs);
        return __fstr_32_buf;
    }
    char* body = "";
    if (!__btrc_isEmpty(self->textContent)) {
        (body = (self->rawText ? self->textContent : Html_escape(self->textContent)));
    }
    int __n_34 = btrc_Vector_UiNode_iterLen(self->children);
    for (int __i_33 = 0; (__i_33 < __n_34); (__i_33++)) {
        UiNode* node = btrc_Vector_UiNode_iterGet(self->children, __i_33);
        int __fstr_35_len = snprintf(NULL, 0, "%s%s", body, UiNode_renderHtml(node));
        char* __fstr_35_buf = __btrc_str_track(((char*)malloc((__fstr_35_len + 1))));
        snprintf(__fstr_35_buf, (__fstr_35_len + 1), "%s%s", body, UiNode_renderHtml(node));
        (body = __fstr_35_buf);
    }
    int __fstr_36_len = snprintf(NULL, 0, "<%s%s>%s</%s>", self->tag, attrs, body, self->tag);
    char* __fstr_36_buf = __btrc_str_track(((char*)malloc((__fstr_36_len + 1))));
    snprintf(__fstr_36_buf, (__fstr_36_len + 1), "<%s%s>%s</%s>", self->tag, attrs, body, self->tag);
    return __fstr_36_buf;
}

void UiDocument_init(UiDocument* self, char* title, UiNode* body) {
    self->__rc = 1;
    (self->title = title);
    (self->css = "");
    if (self->body != NULL) {
        if ((--self->body->__rc) <= 0) {
            UiNode_destroy(self->body);
        }
    }
    (self->body = body);
    (body->__rc++);
}

UiDocument* UiDocument_new(char* title, UiNode* body) {
    UiDocument* self = ((UiDocument*)malloc(sizeof(UiDocument)));
    memset(self, 0, sizeof(UiDocument));
    UiDocument_init(self, title, body);
    return self;
}

void UiDocument_destroy(UiDocument* self) {
    if (self->body != NULL) {
        if ((--self->body->__rc) <= 0) {
            UiNode_destroy(self->body);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

UiDocument* UiDocument_style(UiDocument* self, char* css) {
    (self->css = css);
    return self;
}

char* UiDocument_renderHtml(UiDocument* self) {
    int __fstr_37_len = snprintf(NULL, 0, "<style>%s</style>", self->css);
    char* __fstr_37_buf = __btrc_str_track(((char*)malloc((__fstr_37_len + 1))));
    snprintf(__fstr_37_buf, (__fstr_37_len + 1), "<style>%s</style>", self->css);
    char* cssBlock = (__btrc_isEmpty(self->css) ? "" : __fstr_37_buf);
    int __fstr_38_len = snprintf(NULL, 0, "<!doctype html><html><head><meta charset=\"utf-8\"><title>%s</title>%s</head><body>%s</body></html>", Html_escape(self->title), cssBlock, UiNode_renderHtml(self->body));
    char* __fstr_38_buf = __btrc_str_track(((char*)malloc((__fstr_38_len + 1))));
    snprintf(__fstr_38_buf, (__fstr_38_len + 1), "<!doctype html><html><head><meta charset=\"utf-8\"><title>%s</title>%s</head><body>%s</body></html>", Html_escape(self->title), cssBlock, UiNode_renderHtml(self->body));
    return __fstr_38_buf;
}

void UiDocument_writeHtml(UiDocument* self, char* path) {
    FileSystem_writeText(path, UiDocument_renderHtml(self));
}

void HtmlView_init(HtmlView* self, UiDocument* document) {
    self->__rc = 1;
    if (self->document != NULL) {
        if ((--self->document->__rc) <= 0) {
            UiDocument_destroy(self->document);
        }
    }
    (self->document = document);
    (document->__rc++);
}

HtmlView* HtmlView_new(UiDocument* document) {
    HtmlView* self = ((HtmlView*)malloc(sizeof(HtmlView)));
    memset(self, 0, sizeof(HtmlView));
    HtmlView_init(self, document);
    return self;
}

void HtmlView_destroy(HtmlView* self) {
    if (self->document != NULL) {
        if ((--self->document->__rc) <= 0) {
            UiDocument_destroy(self->document);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

char* HtmlView_render(HtmlView* self) {
    return UiDocument_renderHtml(self->document);
}

void HtmlView_write(HtmlView* self, char* path) {
    UiDocument_writeHtml(self->document, path);
}

void NativeView_init(NativeView* self, UiNode* root) {
    self->__rc = 1;
    if (self->root != NULL) {
        if ((--self->root->__rc) <= 0) {
            UiNode_destroy(self->root);
        }
    }
    (self->root = root);
    (root->__rc++);
}

NativeView* NativeView_new(UiNode* root) {
    NativeView* self = ((NativeView*)malloc(sizeof(NativeView)));
    memset(self, 0, sizeof(NativeView));
    NativeView_init(self, root);
    return self;
}

void NativeView_destroy(NativeView* self) {
    if (self->root != NULL) {
        if ((--self->root->__rc) <= 0) {
            UiNode_destroy(self->root);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

void Window_init(Window* self, char* title, int width, int height, HtmlView* html) {
    self->__rc = 1;
    (self->title = title);
    (self->width = width);
    (self->height = height);
    if (self->html != NULL) {
        if ((--self->html->__rc) <= 0) {
            HtmlView_destroy(self->html);
        }
    }
    (self->html = html);
    (html->__rc++);
}

Window* Window_new(char* title, int width, int height, HtmlView* html) {
    Window* self = ((Window*)malloc(sizeof(Window)));
    memset(self, 0, sizeof(Window));
    Window_init(self, title, width, height, html);
    return self;
}

void Window_destroy(Window* self) {
    if (self->html != NULL) {
        if ((--self->html->__rc) <= 0) {
            HtmlView_destroy(self->html);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

void TrayItem_init(TrayItem* self, char* label, char* command) {
    self->__rc = 1;
    (self->label = label);
    (self->command = command);
    (self->enabled = true);
}

TrayItem* TrayItem_new(char* label, char* command) {
    TrayItem* self = ((TrayItem*)malloc(sizeof(TrayItem)));
    memset(self, 0, sizeof(TrayItem));
    TrayItem_init(self, label, command);
    return self;
}

void TrayItem_destroy(TrayItem* self) {
    free(self);
}

TrayItem* TrayItem_disabled(TrayItem* self) {
    (self->enabled = false);
    return self;
}

char* TrayItem_renderLabel(TrayItem* self) {
    int __fstr_39_len = snprintf(NULL, 0, "%s (disabled)", self->label);
    char* __fstr_39_buf = __btrc_str_track(((char*)malloc((__fstr_39_len + 1))));
    snprintf(__fstr_39_buf, (__fstr_39_len + 1), "%s (disabled)", self->label);
    return (self->enabled ? self->label : __fstr_39_buf);
}

void Tray_init(Tray* self, char* title) {
    self->__rc = 1;
    (self->title = title);
    (self->tooltip = title);
    (self->iconPath = "");
    if (self->items != NULL) {
        if ((--self->items->__rc) <= 0) {
            btrc_Vector_TrayItem_free(self->items);
        }
    }
    btrc_Vector_TrayItem* __list_41 = btrc_Vector_TrayItem_new();
    (self->items = __list_41);
    btrc_Vector_TrayItem* __list_40 = btrc_Vector_TrayItem_new();
    (__list_40->__rc++);
}

Tray* Tray_new(char* title) {
    Tray* self = ((Tray*)malloc(sizeof(Tray)));
    memset(self, 0, sizeof(Tray));
    Tray_init(self, title);
    return self;
}

void Tray_destroy(Tray* self) {
    if (self->items != NULL) {
        if ((--self->items->__rc) <= 0) {
            btrc_Vector_TrayItem_free(self->items);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

Tray* Tray_icon(Tray* self, char* path) {
    (self->iconPath = path);
    return self;
}

Tray* Tray_tip(Tray* self, char* text) {
    (self->tooltip = text);
    return self;
}

Tray* Tray_item(Tray* self, char* label, char* command) {
    btrc_Vector_TrayItem_push(self->items, TrayItem_new(label, command));
    return self;
}

Tray* Tray_add(Tray* self, TrayItem* item) {
    btrc_Vector_TrayItem_push(self->items, item);
    return self;
}

void HtmlUiBackend_init(HtmlUiBackend* self, char* opener) {
    self->__rc = 1;
    (self->opener = opener);
}

HtmlUiBackend* HtmlUiBackend_new(char* opener) {
    HtmlUiBackend* self = ((HtmlUiBackend*)malloc(sizeof(HtmlUiBackend)));
    memset(self, 0, sizeof(HtmlUiBackend));
    HtmlUiBackend_init(self, opener);
    return self;
}

void HtmlUiBackend_destroy(HtmlUiBackend* self) {
    free(self);
}

Command* HtmlUiBackend_openCommand(HtmlUiBackend* self, char* path) {
    return Command_check(Command_capture(Command_arg(Command_new(self->opener), path), false), false);
}

ExecResult* HtmlUiBackend_openFile(HtmlUiBackend* self, char* path) {
    UnixShell* shell = UnixShell_new();
    ExecResult* __btrc_ret_42 = UnixShell_runCommand(shell, HtmlUiBackend_openCommand(self, path));
    if (shell != NULL) {
        if ((--shell->__rc) <= 0) {
            UnixShell_destroy(shell);
        }
    }
    return __btrc_ret_42;
    if (shell != NULL) {
        if ((--shell->__rc) <= 0) {
            UnixShell_destroy(shell);
        }
    }
}

ExecResult* HtmlUiBackend_openWindow(HtmlUiBackend* self, Window* window, char* path) {
    HtmlView_write(window->html, path);
    return HtmlUiBackend_openFile(self, path);
}

void NativeUiBackend_init(NativeUiBackend* self, char* name, HtmlUiBackend* htmlBackend) {
    self->__rc = 1;
    (self->name = name);
    if (self->htmlBackend != NULL) {
        if ((--self->htmlBackend->__rc) <= 0) {
            HtmlUiBackend_destroy(self->htmlBackend);
        }
    }
    (self->htmlBackend = htmlBackend);
    (htmlBackend->__rc++);
}

NativeUiBackend* NativeUiBackend_new(char* name, HtmlUiBackend* htmlBackend) {
    NativeUiBackend* self = ((NativeUiBackend*)malloc(sizeof(NativeUiBackend)));
    memset(self, 0, sizeof(NativeUiBackend));
    NativeUiBackend_init(self, name, htmlBackend);
    return self;
}

void NativeUiBackend_destroy(NativeUiBackend* self) {
    if (self->htmlBackend != NULL) {
        if ((--self->htmlBackend->__rc) <= 0) {
            HtmlUiBackend_destroy(self->htmlBackend);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

bool NativeUiBackend_isMac(NativeUiBackend* self) {
    return (strcmp(self->name, "macos") == 0);
}

bool NativeUiBackend_isLinux(NativeUiBackend* self) {
    return (strcmp(self->name, "linux") == 0);
}

bool NativeUiBackend_isWindows(NativeUiBackend* self) {
    return (strcmp(self->name, "windows") == 0);
}

Command* NativeUiBackend_notifyCommand(NativeUiBackend* self, char* title, char* body) {
    if (NativeUiBackend_isMac(self)) {
        char* script = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("display notification ", NativeUi_applescriptString(body))), " with title ")), NativeUi_applescriptString(title)));
        return Command_check(Command_capture(Command_arg(Command_arg(Command_new("osascript"), "-e"), script), false), false);
    }
    if (NativeUiBackend_isLinux(self)) {
        return Command_check(Command_capture(Command_arg(Command_arg(Command_new("notify-send"), title), body), false), false);
    }
    return Command_check(Command_capture(Command_arg(Command_arg(Command_arg(Command_new("powershell"), "-NoProfile"), "-Command"), "Write-Error 'Native notifications are TODO for Windows'"), false), false);
}

Command* NativeUiBackend_alertCommand(NativeUiBackend* self, char* title, char* body) {
    if (NativeUiBackend_isMac(self)) {
        char* script = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("display dialog ", NativeUi_applescriptString(body))), " with title ")), NativeUi_applescriptString(title))), " buttons {\"OK\"} default button \"OK\""));
        return Command_check(Command_capture(Command_arg(Command_arg(Command_new("osascript"), "-e"), script), false), false);
    }
    if (NativeUiBackend_isLinux(self)) {
        return Command_check(Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("zenity"), "--info"), "--title"), title), "--text"), body), false), false);
    }
    return Command_check(Command_capture(Command_arg(Command_arg(Command_arg(Command_new("powershell"), "-NoProfile"), "-Command"), "Write-Error 'Native dialogs are TODO for Windows'"), false), false);
}

ExecResult* NativeUiBackend_notify(NativeUiBackend* self, char* title, char* body) {
    return UnixShell_runCommand(UnixShell_new(), NativeUiBackend_notifyCommand(self, title, body));
}

ExecResult* NativeUiBackend_alert(NativeUiBackend* self, char* title, char* body) {
    return UnixShell_runCommand(UnixShell_new(), NativeUiBackend_alertCommand(self, title, body));
}

ExecResult* NativeUiBackend_openFile(NativeUiBackend* self, char* path) {
    return HtmlUiBackend_openFile(self->htmlBackend, path);
}

ExecResult* NativeUiBackend_openWindow(NativeUiBackend* self, Window* window, char* path) {
    return HtmlUiBackend_openWindow(self->htmlBackend, window, path);
}

void LinuxUiBuilder_init(LinuxUiBuilder* self) {
    self->__rc = 1;
}

LinuxUiBuilder* LinuxUiBuilder_new(void) {
    LinuxUiBuilder* self = ((LinuxUiBuilder*)malloc(sizeof(LinuxUiBuilder)));
    memset(self, 0, sizeof(LinuxUiBuilder));
    LinuxUiBuilder_init(self);
    return self;
}

void LinuxUiBuilder_destroy(LinuxUiBuilder* self) {
    free(self);
}

HtmlUiBackend* LinuxUiBuilder_html(void) {
    return HtmlUiBackend_new("xdg-open");
}

NativeUiBackend* LinuxUiBuilder_native(void) {
    return NativeUiBackend_new("linux", LinuxUiBuilder_html());
}

void MacUiBuilder_init(MacUiBuilder* self) {
    self->__rc = 1;
}

MacUiBuilder* MacUiBuilder_new(void) {
    MacUiBuilder* self = ((MacUiBuilder*)malloc(sizeof(MacUiBuilder)));
    memset(self, 0, sizeof(MacUiBuilder));
    MacUiBuilder_init(self);
    return self;
}

void MacUiBuilder_destroy(MacUiBuilder* self) {
    free(self);
}

HtmlUiBackend* MacUiBuilder_html(void) {
    return HtmlUiBackend_new("open");
}

NativeUiBackend* MacUiBuilder_native(void) {
    return NativeUiBackend_new("macos", MacUiBuilder_html());
}

void WindowsUiBuilder_init(WindowsUiBuilder* self) {
    self->__rc = 1;
}

WindowsUiBuilder* WindowsUiBuilder_new(void) {
    WindowsUiBuilder* self = ((WindowsUiBuilder*)malloc(sizeof(WindowsUiBuilder)));
    memset(self, 0, sizeof(WindowsUiBuilder));
    WindowsUiBuilder_init(self);
    return self;
}

void WindowsUiBuilder_destroy(WindowsUiBuilder* self) {
    free(self);
}

HtmlUiBackend* WindowsUiBuilder_html(void) {
    return HtmlUiBackend_new("powershell");
}

NativeUiBackend* WindowsUiBuilder_native(void) {
    return NativeUiBackend_new("windows", WindowsUiBuilder_html());
}

void Ui_init(Ui* self) {
    self->__rc = 1;
}

Ui* Ui_new(void) {
    Ui* self = ((Ui*)malloc(sizeof(Ui)));
    memset(self, 0, sizeof(Ui));
    Ui_init(self);
    return self;
}

void Ui_destroy(Ui* self) {
    free(self);
}

UiNode* Ui_node(char* tag) {
    return UiNode_new(tag);
}

UiNode* Ui_text(char* value) {
    return UiNode_text(UiNode_new("span"), value);
}

UiNode* Ui_rawHtml(char* value) {
    return UiNode_raw(UiNode_new("div"), value);
}

UiNode* Ui_div(void) {
    return UiNode_new("div");
}

UiNode* Ui_button(char* label) {
    return UiNode_text(UiNode_new("button"), label);
}

UiNode* Ui_input(char* name, char* value) {
    return UiNode_attr(UiNode_attr(UiNode_new("input"), "name", name), "value", value);
}

UiDocument* Ui_document(char* title, UiNode* body) {
    return UiDocument_new(title, body);
}

void NativeUi_init(NativeUi* self) {
    self->__rc = 1;
}

NativeUi* NativeUi_new(void) {
    NativeUi* self = ((NativeUi*)malloc(sizeof(NativeUi)));
    memset(self, 0, sizeof(NativeUi));
    NativeUi_init(self);
    return self;
}

void NativeUi_destroy(NativeUi* self) {
    free(self);
}

char* NativeUi_applescriptString(char* raw) {
    char* escaped = Strings_replace(raw, "\\", "\\\\");
    (escaped = Strings_replace(escaped, "\"", "\\\""));
    return __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("\"", escaped)), "\""));
}

NativeUiBackend* NativeUi_detect(void) {
    ExecResult* result = UnixShell_runUnchecked(UnixShell_new(), "uname -s");
    char* kernel = __btrc_str_track(__btrc_trim(ExecResult_stdout(result)));
    if (strcmp(kernel, "Darwin") == 0) {
        return MacUiBuilder_native();
    }
    if ((__btrc_strContains(kernel, "MINGW") || __btrc_strContains(kernel, "MSYS")) || __btrc_strContains(kernel, "CYGWIN")) {
        return WindowsUiBuilder_native();
    }
    return LinuxUiBuilder_native();
}

void UiRuntime_init(UiRuntime* self) {
    self->__rc = 1;
}

UiRuntime* UiRuntime_new(void) {
    UiRuntime* self = ((UiRuntime*)malloc(sizeof(UiRuntime)));
    memset(self, 0, sizeof(UiRuntime));
    UiRuntime_init(self);
    return self;
}

void UiRuntime_destroy(UiRuntime* self) {
    free(self);
}

static void* __btrc_spawn_wrapper_1(void* __arg) {
    __btrc_spawn_env_1* __env = ((__btrc_spawn_env_1*)__arg);
    Command* command = __env->command;
    ExecResult* result = UnixShell_runCommand(UnixShell_new(), command);
    void* __result = ((void*)((intptr_t)result->code));
    if (command != NULL) {
        if ((--command->__rc) <= 0) {
            Command_destroy(command);
        }
    }
    free(__env);
    return __result;
}

__btrc_thread_t* UiRuntime_runCommandAsync(Command* command) {
    __btrc_spawn_env_1* __se1 = ((__btrc_spawn_env_1*)malloc(sizeof(__btrc_spawn_env_1)));
    __se1->command = command;
    if (command) {
        (command->__rc++);
    }
    return __btrc_thread_spawn((void*(*)(void*))__btrc_spawn_wrapper_1, ((void*)__se1));
}

static void* __btrc_spawn_wrapper_2(void* __arg) {
    __btrc_spawn_env_2* __env = ((__btrc_spawn_env_2*)__arg);
    NativeUiBackend* backend = __env->backend;
    char* body = __env->body;
    char* title = __env->title;
    ExecResult* result = NativeUiBackend_notify(backend, title, body);
    void* __result = ((void*)((intptr_t)result->code));
    if (backend != NULL) {
        if ((--backend->__rc) <= 0) {
            NativeUiBackend_destroy(backend);
        }
    }
    free(__env);
    return __result;
}

__btrc_thread_t* UiRuntime_notifyAsync(NativeUiBackend* backend, char* title, char* body) {
    __btrc_spawn_env_2* __se2 = ((__btrc_spawn_env_2*)malloc(sizeof(__btrc_spawn_env_2)));
    __se2->backend = backend;
    if (backend) {
        (backend->__rc++);
    }
    __se2->body = body;
    __se2->title = title;
    return __btrc_thread_spawn((void*(*)(void*))__btrc_spawn_wrapper_2, ((void*)__se2));
}

void Signal_init(Signal* self) {
    self->__rc = 1;
    if (self->events != NULL) {
        if ((--self->events->__rc) <= 0) {
            btrc_Vector_string_free(self->events);
        }
    }
    btrc_Vector_string* __list_44 = btrc_Vector_string_new();
    (self->events = __list_44);
    btrc_Vector_string* __list_43 = btrc_Vector_string_new();
    (__list_43->__rc++);
}

Signal* Signal_new(void) {
    Signal* self = ((Signal*)malloc(sizeof(Signal)));
    memset(self, 0, sizeof(Signal));
    Signal_init(self);
    return self;
}

void Signal_destroy(Signal* self) {
    if (self->events != NULL) {
        if ((--self->events->__rc) <= 0) {
            btrc_Vector_string_free(self->events);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

Signal* Signal_emit(Signal* self, char* value) {
    btrc_Vector_string_push(self->events, value);
    return self;
}

bool Signal_hasEvents(Signal* self) {
    return (self->events->len > 0);
}

char* Signal_latest(Signal* self) {
    return btrc_Vector_string_last(self->events);
}

void Signal_clear(Signal* self) {
    btrc_Vector_string_clear(self->events);
}

void CliArgs_init(CliArgs* self, int argc, char** argv) {
    self->__rc = 1;
    (self->program = ((argc > 0) ? Strings_copy(argv[0]) : ""));
    if (self->values != NULL) {
        if ((--self->values->__rc) <= 0) {
            btrc_Vector_string_free(self->values);
        }
    }
    btrc_Vector_string* __list_46 = btrc_Vector_string_new();
    (self->values = __list_46);
    btrc_Vector_string* __list_45 = btrc_Vector_string_new();
    (__list_45->__rc++);
    for (int i = 1; (i < argc); (i++)) {
        btrc_Vector_string_push(self->values, Strings_copy(argv[i]));
    }
}

CliArgs* CliArgs_new(int argc, char** argv) {
    CliArgs* self = ((CliArgs*)malloc(sizeof(CliArgs)));
    memset(self, 0, sizeof(CliArgs));
    CliArgs_init(self, argc, argv);
    return self;
}

void CliArgs_destroy(CliArgs* self) {
    if (self->values != NULL) {
        if ((--self->values->__rc) <= 0) {
            btrc_Vector_string_free(self->values);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

int CliArgs_count(CliArgs* self) {
    return self->values->len;
}

char* CliArgs_get(CliArgs* self, int index) {
    return btrc_Vector_string_get(self->values, index);
}

char* CliArgs_command(CliArgs* self) {
    if (self->values->len == 0) {
        return "";
    }
    return btrc_Vector_string_get(self->values, 0);
}

bool CliArgs_has(CliArgs* self, char* flag) {
    int __n_48 = btrc_Vector_string_iterLen(self->values);
    for (int __i_47 = 0; (__i_47 < __n_48); (__i_47++)) {
        char* value = btrc_Vector_string_iterGet(self->values, __i_47);
        if (strcmp(value, flag) == 0) {
            return true;
        }
    }
    return false;
}

char* CliArgs_valueAfter(CliArgs* self, char* flag, char* fallback) {
    for (int i = 0; (i < (self->values->len - 1)); (i++)) {
        if (strcmp(btrc_Vector_string_get(self->values, i), flag) == 0) {
            return btrc_Vector_string_get(self->values, (i + 1));
        }
    }
    return fallback;
}

bool CliArgs_commandIs(CliArgs* self, char* name) {
    return (strcmp(CliArgs_command(self), name) == 0);
}

char* CliArgs_valueAfterPrefix(CliArgs* self, char* prefix, char* fallback) {
    int __n_50 = btrc_Vector_string_iterLen(self->values);
    for (int __i_49 = 0; (__i_49 < __n_50); (__i_49++)) {
        char* value = btrc_Vector_string_iterGet(self->values, __i_49);
        if (__btrc_startsWith(value, prefix)) {
            return __btrc_str_track(__btrc_substring(value, ((int)strlen(prefix)), (((int)strlen(value)) - ((int)strlen(prefix)))));
        }
    }
    return fallback;
}

void CliCommand_init(CliCommand* self, char* name) {
    self->__rc = 1;
    (self->name = name);
    if (self->aliases != NULL) {
        if ((--self->aliases->__rc) <= 0) {
            btrc_Vector_string_free(self->aliases);
        }
    }
    btrc_Vector_string* __list_52 = btrc_Vector_string_new();
    (self->aliases = __list_52);
    btrc_Vector_string* __list_51 = btrc_Vector_string_new();
    (__list_51->__rc++);
}

CliCommand* CliCommand_new(char* name) {
    CliCommand* self = ((CliCommand*)malloc(sizeof(CliCommand)));
    memset(self, 0, sizeof(CliCommand));
    CliCommand_init(self, name);
    return self;
}

void CliCommand_destroy(CliCommand* self) {
    if (self->aliases != NULL) {
        if ((--self->aliases->__rc) <= 0) {
            btrc_Vector_string_free(self->aliases);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

void CliCommand_alias(CliCommand* self, char* name) {
    btrc_Vector_string_push(self->aliases, name);
}

bool CliCommand_matches(CliCommand* self, char* value) {
    if (strcmp(self->name, value) == 0) {
        return true;
    }
    int __n_54 = btrc_Vector_string_iterLen(self->aliases);
    for (int __i_53 = 0; (__i_53 < __n_54); (__i_53++)) {
        char* alias = btrc_Vector_string_iterGet(self->aliases, __i_53);
        if (strcmp(alias, value) == 0) {
            return true;
        }
    }
    return false;
}

void Console_init(Console* self) {
    self->__rc = 1;
}

Console* Console_new(void) {
    Console* self = ((Console*)malloc(sizeof(Console)));
    memset(self, 0, sizeof(Console));
    Console_init(self);
    return self;
}

void Console_destroy(Console* self) {
    free(self);
}

void Console_log(char* msg) {
    printf("%s\n", msg);
}

void Console_error(char* msg) {
    fprintf(stderr, "%s\n", msg);
}

void Console_write(char* msg) {
    printf("%s", msg);
}

void Console_writeLine(char* msg) {
    printf("%s\n", msg);
}

void DateTime_init(DateTime* self, int year, int month, int day, int hour, int minute, int second) {
    self->__rc = 1;
    (self->year = year);
    (self->month = month);
    (self->day = day);
    (self->hour = hour);
    (self->minute = minute);
    (self->second = second);
}

DateTime* DateTime_new(int year, int month, int day, int hour, int minute, int second) {
    DateTime* self = ((DateTime*)malloc(sizeof(DateTime)));
    memset(self, 0, sizeof(DateTime));
    DateTime_init(self, year, month, day, hour, minute, second);
    return self;
}

void DateTime_destroy(DateTime* self) {
    free(self);
}

DateTime* DateTime_now(void) {
    time_t t = time(NULL);
    struct tm* tm = localtime((&t));
    return DateTime_new((tm->tm_year + 1900), (tm->tm_mon + 1), tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
}

void DateTime_display(DateTime* self) {
    printf("%04d-%02d-%02d %02d:%02d:%02d", self->year, self->month, self->day, self->hour, self->minute, self->second);
}

char* DateTime_format(DateTime* self) {
    char buf[64];
    snprintf(buf, 64, "%04d-%02d-%02d %02d:%02d:%02d", self->year, self->month, self->day, self->hour, self->minute, self->second);
    return Strings_copy(buf);
}

char* DateTime_dateString(DateTime* self) {
    char buf[32];
    snprintf(buf, 32, "%04d-%02d-%02d", self->year, self->month, self->day);
    return Strings_copy(buf);
}

char* DateTime_timeString(DateTime* self) {
    char buf[32];
    snprintf(buf, 32, "%02d:%02d:%02d", self->hour, self->minute, self->second);
    return Strings_copy(buf);
}

void Timer_init(Timer* self) {
    self->__rc = 1;
    (self->start_time = 0);
    (self->end_time = 0);
    (self->running = false);
}

Timer* Timer_new(void) {
    Timer* self = ((Timer*)malloc(sizeof(Timer)));
    memset(self, 0, sizeof(Timer));
    Timer_init(self);
    return self;
}

void Timer_destroy(Timer* self) {
    free(self);
}

void Timer_start(Timer* self) {
    (self->start_time = clock());
    (self->running = true);
}

void Timer_stop(Timer* self) {
    (self->end_time = clock());
    (self->running = false);
}

float Timer_elapsed(Timer* self) {
    clock_t end = (self->running ? clock() : self->end_time);
    return __btrc_div_double(((float)(end - self->start_time)), ((float)CLOCKS_PER_SEC));
}

void Timer_reset(Timer* self) {
    (self->start_time = 0);
    (self->end_time = 0);
    (self->running = false);
}

void Error_init(Error* self, char* message, int code) {
    self->__rc = 1;
    (self->message = message);
    (self->code = code);
}

Error* Error_new(char* message, int code) {
    Error* self = ((Error*)malloc(sizeof(Error)));
    memset(self, 0, sizeof(Error));
    Error_init(self, message, code);
    return self;
}

void Error_destroy(Error* self) {
    free(self);
}

char* Error_toString(Error* self) {
    return self->message;
}

void ValueError_init(ValueError* self, char* message) {
    self->__rc = 1;
    (self->message = message);
    (self->code = 1);
}

ValueError* ValueError_new(char* message) {
    ValueError* self = ((ValueError*)malloc(sizeof(ValueError)));
    memset(self, 0, sizeof(ValueError));
    ValueError_init(self, message);
    return self;
}

void ValueError_destroy(ValueError* self) {
    free(self);
}

char* ValueError_toString(ValueError* self) {
    return Error_toString(((Error*)self));
}

void IOError_init(IOError* self, char* message) {
    self->__rc = 1;
    (self->message = message);
    (self->code = 2);
}

IOError* IOError_new(char* message) {
    IOError* self = ((IOError*)malloc(sizeof(IOError)));
    memset(self, 0, sizeof(IOError));
    IOError_init(self, message);
    return self;
}

void IOError_destroy(IOError* self) {
    free(self);
}

char* IOError_toString(IOError* self) {
    return Error_toString(((Error*)self));
}

void TypeError_init(TypeError* self, char* message) {
    self->__rc = 1;
    (self->message = message);
    (self->code = 3);
}

TypeError* TypeError_new(char* message) {
    TypeError* self = ((TypeError*)malloc(sizeof(TypeError)));
    memset(self, 0, sizeof(TypeError));
    TypeError_init(self, message);
    return self;
}

void TypeError_destroy(TypeError* self) {
    free(self);
}

char* TypeError_toString(TypeError* self) {
    return Error_toString(((Error*)self));
}

void IndexError_init(IndexError* self, char* message) {
    self->__rc = 1;
    (self->message = message);
    (self->code = 4);
}

IndexError* IndexError_new(char* message) {
    IndexError* self = ((IndexError*)malloc(sizeof(IndexError)));
    memset(self, 0, sizeof(IndexError));
    IndexError_init(self, message);
    return self;
}

void IndexError_destroy(IndexError* self) {
    free(self);
}

char* IndexError_toString(IndexError* self) {
    return Error_toString(((Error*)self));
}

void KeyError_init(KeyError* self, char* message) {
    self->__rc = 1;
    (self->message = message);
    (self->code = 5);
}

KeyError* KeyError_new(char* message) {
    KeyError* self = ((KeyError*)malloc(sizeof(KeyError)));
    memset(self, 0, sizeof(KeyError));
    KeyError_init(self, message);
    return self;
}

void KeyError_destroy(KeyError* self) {
    free(self);
}

char* KeyError_toString(KeyError* self) {
    return Error_toString(((Error*)self));
}

void HttpRequest_init(HttpRequest* self) {
    self->__rc = 1;
    (self->method = "GET");
    (self->path = "/");
    (self->query = "");
    if (self->headers != NULL) {
        if ((--self->headers->__rc) <= 0) {
            btrc_Map_string_string_free(self->headers);
        }
    }
    (self->headers = btrc_Map_string_string_new());
    (btrc_Map_string_string_new()->__rc++);
    (self->body = "");
}

HttpRequest* HttpRequest_new(void) {
    HttpRequest* self = ((HttpRequest*)malloc(sizeof(HttpRequest)));
    memset(self, 0, sizeof(HttpRequest));
    HttpRequest_init(self);
    return self;
}

void HttpRequest_destroy(HttpRequest* self) {
    if (self->headers != NULL) {
        if ((--self->headers->__rc) <= 0) {
            btrc_Map_string_string_free(self->headers);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

char* HttpRequest_header(HttpRequest* self, char* name) {
    return btrc_Map_string_string_getOrDefault(self->headers, __btrc_str_track(__btrc_toLower(name)), "");
}

bool HttpRequest_isGet(HttpRequest* self) {
    return (strcmp(self->method, "GET") == 0);
}

bool HttpRequest_isPost(HttpRequest* self) {
    return (strcmp(self->method, "POST") == 0);
}

void HttpResponse_init(HttpResponse* self, int status, char* contentType, char* body) {
    self->__rc = 1;
    (self->status = status);
    (self->contentType = contentType);
    (self->body = body);
    if (self->extraHeaders != NULL) {
        if ((--self->extraHeaders->__rc) <= 0) {
            btrc_Vector_string_free(self->extraHeaders);
        }
    }
    btrc_Vector_string* __list_56 = btrc_Vector_string_new();
    (self->extraHeaders = __list_56);
    btrc_Vector_string* __list_55 = btrc_Vector_string_new();
    (__list_55->__rc++);
}

HttpResponse* HttpResponse_new(int status, char* contentType, char* body) {
    HttpResponse* self = ((HttpResponse*)malloc(sizeof(HttpResponse)));
    memset(self, 0, sizeof(HttpResponse));
    HttpResponse_init(self, status, contentType, body);
    return self;
}

void HttpResponse_destroy(HttpResponse* self) {
    if (self->extraHeaders != NULL) {
        if ((--self->extraHeaders->__rc) <= 0) {
            btrc_Vector_string_free(self->extraHeaders);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

HttpResponse* HttpResponse_addHeader(HttpResponse* self, char* line) {
    btrc_Vector_string_push(self->extraHeaders, line);
    return self;
}

HttpResponse* HttpResponse_text(char* body) {
    return HttpResponse_new(200, "text/plain; charset=utf-8", body);
}

HttpResponse* HttpResponse_html(char* body) {
    return HttpResponse_new(200, "text/html; charset=utf-8", body);
}

HttpResponse* HttpResponse_json(char* body) {
    return HttpResponse_new(200, "application/json; charset=utf-8", body);
}

HttpResponse* HttpResponse_js(char* body) {
    return HttpResponse_new(200, "application/javascript; charset=utf-8", body);
}

HttpResponse* HttpResponse_css(char* body) {
    return HttpResponse_new(200, "text/css; charset=utf-8", body);
}

HttpResponse* HttpResponse_notFound(void) {
    return HttpResponse_new(404, "text/plain; charset=utf-8", "Not Found");
}

HttpResponse* HttpResponse_error(int code, char* message) {
    return HttpResponse_new(code, "text/plain; charset=utf-8", message);
}

void HttpServer_init(HttpServer* self, int port) {
    self->__rc = 1;
    (self->port = port);
    (self->fd = (-1));
    (self->bindAny = false);
}

HttpServer* HttpServer_new(int port) {
    HttpServer* self = ((HttpServer*)malloc(sizeof(HttpServer)));
    memset(self, 0, sizeof(HttpServer));
    HttpServer_init(self, port);
    return self;
}

void HttpServer_destroy(HttpServer* self) {
    free(self);
}

bool HttpServer_start(HttpServer* self) {
    signal(SIGPIPE, SIG_IGN);
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        return false;
    }
    int yes = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (&yes), sizeof(yes));
    struct sockaddr_in addr;
    memset((&addr), 0, sizeof(addr));
    (addr.sin_family = AF_INET);
    (addr.sin_port = htons(((unsigned short)self->port)));
    if (self->bindAny) {
        (addr.sin_addr.s_addr = htonl(INADDR_ANY));
    } else {
        (addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK));
    }
    if (bind(s, ((struct sockaddr*)(&addr)), sizeof(addr)) < 0) {
        close(s);
        return false;
    }
    if (listen(s, 64) < 0) {
        close(s);
        return false;
    }
    (self->fd = s);
    return true;
}

int HttpServer_acceptConn(HttpServer* self) {
    struct sockaddr_in cli;
    socklen_t clilen = sizeof(cli);
    int c = accept(self->fd, ((struct sockaddr*)(&cli)), (&clilen));
    if (c >= 0) {
        struct timeval tv;
        (tv.tv_sec = 15);
        (tv.tv_usec = 0);
        setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, (&tv), sizeof(tv));
    }
    return c;
}

void HttpServer_closeConn(HttpServer* self, int c) {
    close(c);
}

int HttpServer_headerContentLength(char* head, int headerEnd) {
    char* lower = __btrc_str_track(__btrc_toLower(__btrc_str_track(__btrc_substring(head, 0, headerEnd))));
    int idx = Strings_find(lower, "content-length:", 0);
    if (idx < 0) {
        return 0;
    }
    int i = (idx + 15);
    int len = ((int)strlen(lower));
    while ((i < len) && (lower[i] == ' ')) {
        (i++);
    }
    int val = 0;
    while (((i < len) && (lower[i] >= '0')) && (lower[i] <= '9')) {
        (val = ((val * 10) + (lower[i] - '0')));
        (i++);
    }
    return val;
}

char* HttpServer_readRequestRaw(HttpServer* self, int c) {
    char* data = "";
    char buf[8193];
    bool haveHeaders = false;
    int headerEnd = (-1);
    int contentLen = 0;
    while (true) {
        int n = ((int)recv(c, buf, 8192, 0));
        if (n <= 0) {
            break;
        }
        (buf[n] = '\0');
        (data = __btrc_str_track(__btrc_strcat(data, Strings_copy(buf))));
        if (!haveHeaders) {
            int he = Strings_find(data, "\r\n\r\n", 0);
            if (he >= 0) {
                (haveHeaders = true);
                (headerEnd = he);
                (contentLen = HttpServer_headerContentLength(data, he));
            }
        }
        if (haveHeaders) {
            int bodyStart = (headerEnd + 4);
            int have = (((int)strlen(data)) - bodyStart);
            if (have >= contentLen) {
                break;
            }
        }
    }
    return data;
}

HttpRequest* HttpServer_readRequest(HttpServer* self, int c) {
    return HttpServer_parse(self, HttpServer_readRequestRaw(self, c));
}

HttpRequest* HttpServer_parse(HttpServer* self, char* raw) {
    HttpRequest* req = HttpRequest_new();
    if (((int)strlen(raw)) == 0) {
        return req;
    }
    int he = Strings_find(raw, "\r\n\r\n", 0);
    char* head = ((he >= 0) ? __btrc_str_track(__btrc_substring(raw, 0, he)) : raw);
    if (he >= 0) {
        int bodyStart = (he + 4);
        (req->body = __btrc_str_track(__btrc_substring(raw, bodyStart, (((int)strlen(raw)) - bodyStart))));
    }
    btrc_Vector_string* lines = Strings_split(head, "\r\n");
    if (lines->len > 0) {
        btrc_Vector_string* parts = Strings_split(btrc_Vector_string_get(lines, 0), " ");
        if (parts->len >= 2) {
            (req->method = btrc_Vector_string_get(parts, 0));
            char* target = btrc_Vector_string_get(parts, 1);
            int q = Strings_find(target, "?", 0);
            if (q >= 0) {
                (req->path = __btrc_str_track(__btrc_substring(target, 0, q)));
                (req->query = __btrc_str_track(__btrc_substring(target, (q + 1), (((int)strlen(target)) - (q + 1)))));
            } else {
                (req->path = target);
            }
        }
    }
    for (int i = 1; (i < lines->len); (i++)) {
        char* line = btrc_Vector_string_get(lines, i);
        int colon = Strings_find(line, ":", 0);
        if (colon > 0) {
            char* name = __btrc_str_track(__btrc_toLower(__btrc_str_track(__btrc_trim(__btrc_str_track(__btrc_substring(line, 0, colon))))));
            char* value = __btrc_str_track(__btrc_trim(__btrc_str_track(__btrc_substring(line, (colon + 1), (((int)strlen(line)) - (colon + 1))))));
            btrc_Map_string_string_put(req->headers, name, value);
        }
    }
    return req;
    if (req != NULL) {
        if ((--req->__rc) <= 0) {
            HttpRequest_destroy(req);
        }
    }
}

char* HttpServer_reason(HttpServer* self, int status) {
    if (status == 200) {
        return "OK";
    }
    if (status == 201) {
        return "Created";
    }
    if (status == 204) {
        return "No Content";
    }
    if (status == 400) {
        return "Bad Request";
    }
    if (status == 401) {
        return "Unauthorized";
    }
    if (status == 403) {
        return "Forbidden";
    }
    if (status == 404) {
        return "Not Found";
    }
    if (status == 405) {
        return "Method Not Allowed";
    }
    if (status == 500) {
        return "Internal Server Error";
    }
    return "OK";
}

void HttpServer_sendAll(HttpServer* self, int c, char* data) {
    char* p = data;
    int total = ((int)strlen(data));
    int sent = 0;
    while (sent < total) {
        int n = ((int)send(c, (p + sent), (total - sent), 0));
        if (n <= 0) {
            break;
        }
        (sent = (sent + n));
    }
}

void HttpServer_respond(HttpServer* self, int c, HttpResponse* resp) {
    int blen = ((int)strlen(resp->body));
    char* r = HttpServer_reason(self, resp->status);
    int __fstr_57_len = snprintf(NULL, 0, "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %d\r\nCache-Control: no-store\r\nConnection: close\r\n", resp->status, r, resp->contentType, blen);
    char* __fstr_57_buf = __btrc_str_track(((char*)malloc((__fstr_57_len + 1))));
    snprintf(__fstr_57_buf, (__fstr_57_len + 1), "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %d\r\nCache-Control: no-store\r\nConnection: close\r\n", resp->status, r, resp->contentType, blen);
    char* head = __fstr_57_buf;
    int __n_59 = btrc_Vector_string_iterLen(resp->extraHeaders);
    for (int __i_58 = 0; (__i_58 < __n_59); (__i_58++)) {
        char* h = btrc_Vector_string_iterGet(resp->extraHeaders, __i_58);
        (head = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(head, h)), "\r\n")));
    }
    (head = __btrc_str_track(__btrc_strcat(head, "\r\n")));
    HttpServer_sendAll(self, c, __btrc_str_track(__btrc_strcat(head, resp->body)));
}

void HttpClientResponse_init(HttpClientResponse* self, int status, char* body, char* error) {
    self->__rc = 1;
    (self->status = status);
    (self->body = body);
    (self->error = error);
}

HttpClientResponse* HttpClientResponse_new(int status, char* body, char* error) {
    HttpClientResponse* self = ((HttpClientResponse*)malloc(sizeof(HttpClientResponse)));
    memset(self, 0, sizeof(HttpClientResponse));
    HttpClientResponse_init(self, status, body, error);
    return self;
}

void HttpClientResponse_destroy(HttpClientResponse* self) {
    free(self);
}

bool HttpClientResponse_ok(HttpClientResponse* self) {
    return ((self->status >= 200) && (self->status < 300));
}

void HttpClient_init(HttpClient* self) {
    self->__rc = 1;
    (self->timeoutSecs = 120);
}

HttpClient* HttpClient_new(void) {
    HttpClient* self = ((HttpClient*)malloc(sizeof(HttpClient)));
    memset(self, 0, sizeof(HttpClient));
    HttpClient_init(self);
    return self;
}

void HttpClient_destroy(HttpClient* self) {
    free(self);
}

HttpClient* HttpClient_timeout(HttpClient* self, int secs) {
    (self->timeoutSecs = secs);
    return self;
}

HttpClientResponse* HttpClient_request(HttpClient* self, char* method, char* url, btrc_Vector_string* headers, char* body) {
    char* dir = FileSystem_tempDir("btrchttp");
    if (__btrc_isEmpty(dir)) {
        return HttpClientResponse_new(0, "", "could not create temp dir");
    }
    char* outFile = PathTools_join(dir, "resp.bin");
    char* bodyFile = PathTools_join(dir, "req.bin");
    bool hasBody = (!__btrc_isEmpty(body));
    if (hasBody) {
        FileSystem_writeText(bodyFile, body);
    }
    Command* cmd = Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("curl"), "-sS"), "--max-time"), Strings_fromInt(self->timeoutSecs)), "-X"), method), "-o"), outFile), "-w"), "%{http_code}");
    int __n_61 = btrc_Vector_string_iterLen(headers);
    for (int __i_60 = 0; (__i_60 < __n_61); (__i_60++)) {
        char* h = btrc_Vector_string_iterGet(headers, __i_60);
        Command_arg(Command_arg(cmd, "-H"), h);
    }
    if (hasBody) {
        Command_arg(Command_arg(cmd, "--data-binary"), __btrc_str_track(__btrc_strcat("@", bodyFile)));
    }
    Command_mergeError(Command_check(Command_capture(Command_arg(cmd, url), true), false), false);
    ExecResult* result = UnixShell_runCommand(UnixShell_new(), cmd);
    int status = Strings_toInt(__btrc_str_track(__btrc_trim(ExecResult_stdout(result))));
    char* respBody = "";
    if (FileSystem_exists(outFile)) {
        (respBody = FileSystem_readText(outFile));
    }
    char* err = "";
    if (status == 0) {
        int __fstr_62_len = snprintf(NULL, 0, "curl failed (exit %d)", result->code);
        char* __fstr_62_buf = __btrc_str_track(((char*)malloc((__fstr_62_len + 1))));
        snprintf(__fstr_62_buf, (__fstr_62_len + 1), "curl failed (exit %d)", result->code);
        (err = __fstr_62_buf);
    }
    FileSystem_removeRecursive(dir);
    return HttpClientResponse_new(status, respBody, err);
}

HttpClientResponse* HttpClient_post(HttpClient* self, char* url, btrc_Vector_string* headers, char* body) {
    return HttpClient_request(self, "POST", url, headers, body);
}

HttpClientResponse* HttpClient_get(HttpClient* self, char* url, btrc_Vector_string* headers) {
    return HttpClient_request(self, "GET", url, headers, "");
}

void Browser_init(Browser* self) {
    self->__rc = 1;
}

Browser* Browser_new(void) {
    Browser* self = ((Browser*)malloc(sizeof(Browser)));
    memset(self, 0, sizeof(Browser));
    Browser_init(self);
    return self;
}

void Browser_destroy(Browser* self) {
    free(self);
}

void Browser_open(char* url) {
    ExecResult* u = UnixShell_runUnchecked(UnixShell_new(), "uname -s");
    char* kernel = __btrc_str_track(__btrc_trim(ExecResult_stdout(u)));
    char* opener = "xdg-open";
    if (strcmp(kernel, "Darwin") == 0) {
        (opener = "open");
    }
    if ((__btrc_strContains(kernel, "MINGW") || __btrc_strContains(kernel, "MSYS")) || __btrc_strContains(kernel, "CYGWIN")) {
        (opener = "start");
    }
    Command* cmd = Command_mergeError(Command_check(Command_capture(Command_arg(Command_new(opener), url), false), false), false);
    UnixShell_runCommand(UnixShell_new(), cmd);
}

void File_init(File* self, char* path, char* mode) {
    self->__rc = 1;
    (self->path = path);
    (self->mode = mode);
    (self->handle = fopen(path, mode));
    (self->is_open = (self->handle != NULL));
}

File* File_new(char* path, char* mode) {
    File* self = ((File*)malloc(sizeof(File)));
    memset(self, 0, sizeof(File));
    File_init(self, path, mode);
    return self;
}

void File_destroy(File* self) {
    File_close(self);
    free(self);
}

bool File_ok(File* self) {
    return self->is_open;
}

char* File_read(File* self) {
    if (!self->is_open) {
        return "";
    }
    fseek(self->handle, 0, SEEK_END);
    long size = ftell(self->handle);
    fseek(self->handle, 0, SEEK_SET);
    char* buf = ((char*)malloc((size + 1)));
    long n = ((long)fread(buf, 1, size, self->handle));
    (buf[n] = '\0');
    return buf;
}

char* File_readLine(File* self) {
    if (!self->is_open) {
        return "";
    }
    char buf[4096];
    if (fgets(buf, 4096, self->handle) != NULL) {
        int len = ((int)strlen(buf));
        if ((len > 0) && (buf[(len - 1)] == '\n')) {
            (buf[(len - 1)] = '\0');
        }
        return Strings_copy(buf);
    }
    return "";
}

btrc_Vector_string* File_readLines(File* self) {
    btrc_Vector_string* lines = btrc_Vector_string_new();
    if (!self->is_open) {
        return lines;
    }
    char buf[4096];
    while (fgets(buf, 4096, self->handle) != NULL) {
        int len = ((int)strlen(buf));
        if ((len > 0) && (buf[(len - 1)] == '\n')) {
            (buf[(len - 1)] = '\0');
        }
        btrc_Vector_string_push(lines, Strings_copy(buf));
    }
    return lines;
}

void File_setHandle(File* self, FILE* h) {
    (self->handle = h);
    (self->is_open = true);
}

void File_write(File* self, char* text) {
    if (!self->is_open) {
        return;
    }
    fputs(text, self->handle);
}

void File_writeLine(File* self, char* text) {
    if (!self->is_open) {
        return;
    }
    fputs(text, self->handle);
    fputc('\n', self->handle);
}

void File_close(File* self) {
    if (self->is_open) {
        if (((int)strlen(self->path)) > 0) {
            fclose(self->handle);
        }
        (self->is_open = false);
    }
}

bool File_eof(File* self) {
    if (!self->is_open) {
        return true;
    }
    return (feof(self->handle) != 0);
}

void File_flush(File* self) {
    if (self->is_open) {
        fflush(self->handle);
    }
}

void Path_init(Path* self) {
    self->__rc = 1;
}

Path* Path_new(void) {
    Path* self = ((Path*)malloc(sizeof(Path)));
    memset(self, 0, sizeof(Path));
    Path_init(self);
    return self;
}

void Path_destroy(Path* self) {
    free(self);
}

bool Path_exists(char* path) {
    FILE* f = fopen(path, "r");
    if (f != NULL) {
        fclose(f);
        return true;
    }
    return false;
}

char* Path_readAll(char* path) {
    File* f = File_new(path, "r");
    if (!File_ok(f)) {
        char* __btrc_ret_63 = "";
        if (f != NULL) {
            if ((--f->__rc) <= 0) {
                File_destroy(f);
            }
        }
        return __btrc_ret_63;
    }
    char* content = File_read(f);
    File_close(f);
    if (f != NULL) {
        if ((--f->__rc) <= 0) {
            File_destroy(f);
        }
    }
    return content;
    if (f != NULL) {
        if ((--f->__rc) <= 0) {
            File_destroy(f);
        }
    }
}

void Path_writeAll(char* path, char* content) {
    File* f = File_new(path, "w");
    if (!File_ok(f)) {
        if (f != NULL) {
            if ((--f->__rc) <= 0) {
                File_destroy(f);
            }
        }
        return;
    }
    File_write(f, content);
    File_close(f);
    if (f != NULL) {
        if ((--f->__rc) <= 0) {
            File_destroy(f);
        }
    }
}

void JsonObject_init(JsonObject* self) {
    self->__rc = 1;
    if (self->values != NULL) {
        if ((--self->values->__rc) <= 0) {
            btrc_Map_string_string_free(self->values);
        }
    }
    (self->values = btrc_Map_string_string_new());
    (btrc_Map_string_string_new()->__rc++);
    if (self->quoted != NULL) {
        if ((--self->quoted->__rc) <= 0) {
            btrc_Map_string_bool_free(self->quoted);
        }
    }
    (self->quoted = btrc_Map_string_bool_new());
    (btrc_Map_string_bool_new()->__rc++);
}

JsonObject* JsonObject_new(void) {
    JsonObject* self = ((JsonObject*)malloc(sizeof(JsonObject)));
    memset(self, 0, sizeof(JsonObject));
    JsonObject_init(self);
    return self;
}

void JsonObject_destroy(JsonObject* self) {
    if (self->values != NULL) {
        if ((--self->values->__rc) <= 0) {
            btrc_Map_string_string_free(self->values);
        }
    }
    if (self->quoted != NULL) {
        if ((--self->quoted->__rc) <= 0) {
            btrc_Map_string_bool_free(self->quoted);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

char* JsonObject_escape(char* text) {
    if (text == NULL) {
        return "";
    }
    char* escaped = Strings_replace(text, "\\", "\\\\");
    (escaped = Strings_replace(escaped, "\"", "\\\""));
    (escaped = Strings_replace(escaped, "\n", "\\n"));
    return escaped;
}

char* JsonObject_unescape(char* text) {
    char* result = "";
    bool escaped = false;
    for (int i = 0; (i < ((int)strlen(text))); (i++)) {
        char* current = __btrc_str_track(__btrc_substring(text, i, 1));
        if (escaped) {
            if (strcmp(current, "n") == 0) {
                (result = __btrc_str_track(__btrc_strcat(result, "\n")));
            } else if (strcmp(current, "r") == 0) {
                (result = __btrc_str_track(__btrc_strcat(result, "\r")));
            } else if (strcmp(current, "t") == 0) {
                (result = __btrc_str_track(__btrc_strcat(result, "\t")));
            } else {
                (result = __btrc_str_track(__btrc_strcat(result, current)));
            }
            (escaped = false);
            continue;
        }
        if (strcmp(current, "\\") == 0) {
            (escaped = true);
            continue;
        }
        (result = __btrc_str_track(__btrc_strcat(result, current)));
    }
    if (escaped) {
        (result = __btrc_str_track(__btrc_strcat(result, "\\")));
    }
    return result;
}

void JsonObject_setString(JsonObject* self, char* key, char* value) {
    btrc_Map_string_string_put(self->values, key, value);
    btrc_Map_string_bool_put(self->quoted, key, true);
}

void JsonObject_setRaw(JsonObject* self, char* key, char* value) {
    btrc_Map_string_string_put(self->values, key, value);
    btrc_Map_string_bool_put(self->quoted, key, false);
}

void JsonObject_setBool(JsonObject* self, char* key, bool value) {
    btrc_Map_string_string_put(self->values, key, (value ? "true" : "false"));
    btrc_Map_string_bool_put(self->quoted, key, false);
}

void JsonObject_setInt(JsonObject* self, char* key, int value) {
    btrc_Map_string_string_put(self->values, key, Strings_fromInt(value));
    btrc_Map_string_bool_put(self->quoted, key, false);
}

bool JsonObject_has(JsonObject* self, char* key) {
    return btrc_Map_string_string_has(self->values, key);
}

char* JsonObject_getString(JsonObject* self, char* key, char* fallback) {
    if (!btrc_Map_string_string_has(self->values, key)) {
        return fallback;
    }
    return btrc_Map_string_string_get(self->values, key);
}

bool JsonObject_getBool(JsonObject* self, char* key, bool fallback) {
    if (!btrc_Map_string_string_has(self->values, key)) {
        return fallback;
    }
    char* value = btrc_Map_string_string_get(self->values, key);
    if (strcmp(value, "true") == 0) {
        return true;
    }
    if (strcmp(value, "false") == 0) {
        return false;
    }
    return fallback;
}

int JsonObject_getInt(JsonObject* self, char* key, int fallback) {
    if (!btrc_Map_string_string_has(self->values, key)) {
        return fallback;
    }
    return Strings_toInt(btrc_Map_string_string_get(self->values, key));
}

char* JsonObject_stringify(JsonObject* self) {
    btrc_Vector_string* fields = btrc_Vector_string_new();
    int __n_65 = btrc_Map_string_string_iterLen(self->values);
    for (int __i_64 = 0; (__i_64 < __n_65); (__i_64++)) {
        char* key = btrc_Map_string_string_iterGet(self->values, __i_64);
        char* value = btrc_Map_string_string_iterValueAt(self->values, __i_64);
        char* escapedKey = JsonObject_escape(key);
        char* field = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("\"", escapedKey)), "\":"));
        if (btrc_Map_string_bool_getOrDefault(self->quoted, key, true)) {
            char* escapedValue = JsonObject_escape(value);
            (field = __btrc_str_track(__btrc_strcat(field, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("\"", escapedValue)), "\"")))));
        } else {
            (field = __btrc_str_track(__btrc_strcat(field, value)));
        }
        btrc_Vector_string_push(fields, field);
    }
    return __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("{", btrc_Vector_string_join(fields, ","))), "}"));
}

int JsonObject_skipSpaces(char* text, int i) {
    int len = ((int)strlen(text));
    while ((i < len) && ((((text[i] == ' ') || (text[i] == '\n')) || (text[i] == '\t')) || (text[i] == '\r'))) {
        (i++);
    }
    return i;
}

char* JsonObject_slice(char* text, int start, int end) {
    int len = (end - start);
    char* result = ((char*)malloc((len + 1)));
    memcpy(result, (text + start), len);
    (result[len] = '\0');
    return result;
}

int JsonObject_stringEnd(char* text, int start) {
    int len = ((int)strlen(text));
    bool escaped = false;
    int i = start;
    while (i < len) {
        if ((!escaped) && (text[i] == ((char)34))) {
            return i;
        }
        (escaped = ((!escaped) && (text[i] == '\\')));
        if (text[i] != '\\') {
            (escaped = false);
        }
        (i++);
    }
    return len;
}

JsonObject* JsonObject_parse(char* text) {
    JsonObject* obj = JsonObject_new();
    int len = ((int)strlen(text));
    int i = 0;
    while (i < len) {
        (i = JsonObject_skipSpaces(text, i));
        if (i >= len) {
            break;
        }
        if (text[i] != ((char)34)) {
            (i++);
            continue;
        }
        (i++);
        int keyStart = i;
        (i = JsonObject_stringEnd(text, keyStart));
        char* key = JsonObject_unescape(JsonObject_slice(text, keyStart, i));
        (i++);
        (i = JsonObject_skipSpaces(text, i));
        if ((i < len) && (text[i] == ':')) {
            (i++);
        }
        (i = JsonObject_skipSpaces(text, i));
        if (i >= len) {
            break;
        }
        if (text[i] == ((char)34)) {
            (i++);
            int valueStart = i;
            (i = JsonObject_stringEnd(text, valueStart));
            char* value = JsonObject_unescape(JsonObject_slice(text, valueStart, i));
            JsonObject_setString(obj, key, value);
            (i++);
        } else {
            int valueStart = i;
            while (((i < len) && (text[i] != ',')) && (text[i] != '}')) {
                (i++);
            }
            int valueEnd = i;
            while ((valueEnd > valueStart) && ((((text[(valueEnd - 1)] == ' ') || (text[(valueEnd - 1)] == '\n')) || (text[(valueEnd - 1)] == '\t')) || (text[(valueEnd - 1)] == '\r'))) {
                (valueEnd--);
            }
            char* value = JsonObject_slice(text, valueStart, valueEnd);
            JsonObject_setRaw(obj, key, value);
        }
    }
    return obj;
    if (obj != NULL) {
        if ((--obj->__rc) <= 0) {
            JsonObject_destroy(obj);
        }
    }
}

JsonObject* JsonObject_readFile(char* path) {
    return JsonObject_parse(Path_readAll(path));
}

void JsonObject_writeFile(JsonObject* self, char* path) {
    Path_writeAll(path, JsonObject_stringify(self));
}

void Json_init(Json* self) {
    self->__rc = 1;
}

Json* Json_new(void) {
    Json* self = ((Json*)malloc(sizeof(Json)));
    memset(self, 0, sizeof(Json));
    Json_init(self);
    return self;
}

void Json_destroy(Json* self) {
    free(self);
}

char* Json_esc(char* s) {
    if (s == NULL) {
        return "";
    }
    char* r = Strings_replace(s, "\\", "\\\\");
    (r = Strings_replace(r, "\"", "\\\""));
    (r = Strings_replace(r, "\n", "\\n"));
    (r = Strings_replace(r, "\r", "\\r"));
    (r = Strings_replace(r, "\t", "\\t"));
    return r;
}

char* Json_str(char* s) {
    return __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("\"", Json_esc(s))), "\""));
}

char* Json_getString(char* json, char* key) {
    return Json_getStringFrom(json, key, 0);
}

char* Json_getStringAfter(char* json, char* anchor, char* key) {
    int a = Strings_find(json, anchor, 0);
    if (a < 0) {
        return "";
    }
    return Json_getStringFrom(json, key, a);
}

char* Json_getStringFrom(char* json, char* key, int from) {
    char* needle = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("\"", key)), "\""));
    int k = Strings_find(json, needle, from);
    if (k < 0) {
        return "";
    }
    int i = (k + ((int)strlen(needle)));
    int len = ((int)strlen(json));
    while ((i < len) && (((((json[i] == ' ') || (json[i] == ':')) || (json[i] == '\t')) || (json[i] == '\n')) || (json[i] == '\r'))) {
        (i++);
    }
    if ((i >= len) || (json[i] != '"')) {
        return "";
    }
    (i++);
    char* out = "";
    while (i < len) {
        char c = json[i];
        if (c == '\\') {
            (i++);
            if (i >= len) {
                break;
            }
            char n = json[i];
            if (n == 'n') {
                (out = __btrc_str_track(__btrc_strcat(out, "\n")));
            } else if (n == 'r') {
                (out = __btrc_str_track(__btrc_strcat(out, "\r")));
            } else if (n == 't') {
                (out = __btrc_str_track(__btrc_strcat(out, "\t")));
            } else if (n == '"') {
                (out = __btrc_str_track(__btrc_strcat(out, "\"")));
            } else if (n == '\\') {
                (out = __btrc_str_track(__btrc_strcat(out, "\\")));
            } else if (n == '/') {
                (out = __btrc_str_track(__btrc_strcat(out, "/")));
            } else {
                (out = __btrc_str_track(__btrc_strcat(out, __btrc_str_track(__btrc_substring(json, i, 1)))));
            }
            (i++);
        } else if (c == '"') {
            break;
        } else {
            (out = __btrc_str_track(__btrc_strcat(out, __btrc_str_track(__btrc_substring(json, i, 1)))));
            (i++);
        }
    }
    return out;
}

void Math_init(Math* self) {
    self->__rc = 1;
}

Math* Math_new(void) {
    Math* self = ((Math*)malloc(sizeof(Math)));
    memset(self, 0, sizeof(Math));
    Math_init(self);
    return self;
}

void Math_destroy(Math* self) {
    free(self);
}

float Math_PI(void) {
    return 3.14159265358979323846f;
}

float Math_E(void) {
    return 2.71828182845904523536f;
}

float Math_TAU(void) {
    return 6.28318530717958647692f;
}

float Math_INF(void) {
    float zero = 0.0f;
    return __btrc_div_double(1.0f, zero);
}

int Math_abs(int x) {
    if (x < 0) {
        return (-x);
    }
    return x;
}

float Math_fabs(float x) {
    if (x < 0.0f) {
        return (-x);
    }
    return x;
}

int Math_max(int a, int b) {
    if (a > b) {
        return a;
    }
    return b;
}

int Math_min(int a, int b) {
    if (a < b) {
        return a;
    }
    return b;
}

float Math_fmax(float a, float b) {
    if (a > b) {
        return a;
    }
    return b;
}

float Math_fmin(float a, float b) {
    if (a < b) {
        return a;
    }
    return b;
}

int Math_clamp(int x, int lo, int hi) {
    if (x < lo) {
        return lo;
    }
    if (x > hi) {
        return hi;
    }
    return x;
}

float Math_power(float base, int exp) {
    float result = 1.0f;
    bool negative = false;
    if (exp < 0) {
        (negative = true);
        (exp = (-exp));
    }
    for (int i = 0; (i < exp); (i++)) {
        (result = (result * base));
    }
    if (negative) {
        return __btrc_div_double(1.0f, result);
    }
    return result;
}

float Math_sqrt(float x) {
    return sqrt(x);
}

int Math_factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return (n * Math_factorial((n - 1)));
}

int Math_gcd(int a, int b) {
    while (b != 0) {
        int temp = b;
        (b = __btrc_mod_int(a, b));
        (a = temp);
    }
    return a;
}

int Math_lcm(int a, int b) {
    return (__btrc_div_int(Math_abs(a), Math_gcd(a, b)) * Math_abs(b));
}

int Math_fibonacci(int n) {
    if (n <= 0) {
        return 0;
    }
    if (n == 1) {
        return 1;
    }
    int a = 0;
    int b = 1;
    for (int i = 2; (i < (n + 1)); (i++)) {
        int temp = (a + b);
        (a = b);
        (b = temp);
    }
    return b;
}

bool Math_isPrime(int n) {
    if (n < 2) {
        return false;
    }
    if (n < 4) {
        return true;
    }
    if (__btrc_mod_int(n, 2) == 0) {
        return false;
    }
    int i = 3;
    while ((i * i) <= n) {
        if (__btrc_mod_int(n, i) == 0) {
            return false;
        }
        (i = (i + 2));
    }
    return true;
}

bool Math_isEven(int n) {
    return (__btrc_mod_int(n, 2) == 0);
}

bool Math_isOdd(int n) {
    return (__btrc_mod_int(n, 2) != 0);
}

int Math_sum(btrc_Vector_int* items) {
    int total = 0;
    for (int i = 0; (i < items->len); (i++)) {
        (total = (total + btrc_Vector_int_get(items, i)));
    }
    return total;
}

float Math_fsum(btrc_Vector_float* items) {
    float total = 0.0f;
    for (int i = 0; (i < items->len); (i++)) {
        (total = (total + btrc_Vector_float_get(items, i)));
    }
    return total;
}

float Math_sin(float x) {
    return sin(x);
}

float Math_cos(float x) {
    return cos(x);
}

float Math_tan(float x) {
    return tan(x);
}

float Math_asin(float x) {
    return asin(x);
}

float Math_acos(float x) {
    return acos(x);
}

float Math_atan(float x) {
    return atan(x);
}

float Math_atan2(float y, float x) {
    return atan2(y, x);
}

float Math_ceil(float x) {
    return ceil(x);
}

float Math_floor(float x) {
    return floor(x);
}

int Math_round(float x) {
    return ((int)round(x));
}

int Math_truncate(float x) {
    return ((int)trunc(x));
}

float Math_log(float x) {
    return log(x);
}

float Math_log10(float x) {
    return log10(x);
}

float Math_log2(float x) {
    return log2(x);
}

float Math_exp(float x) {
    return exp(x);
}

float Math_toRadians(float degrees) {
    return __btrc_div_double((degrees * 3.14159265358979323846f), 180.0f);
}

float Math_toDegrees(float radians) {
    return __btrc_div_double((radians * 180.0f), 3.14159265358979323846f);
}

float Math_fclamp(float val, float lo, float hi) {
    if (val < lo) {
        return lo;
    }
    if (val > hi) {
        return hi;
    }
    return val;
}

int Math_sign(int x) {
    if (x > 0) {
        return 1;
    }
    if (x < 0) {
        return (-1);
    }
    return 0;
}

float Math_fsign(float x) {
    if (x > 0.0f) {
        return 1.0f;
    }
    if (x < 0.0f) {
        return (-1.0f);
    }
    return 0.0f;
}

void UnixPattern_init(UnixPattern* self) {
    self->__rc = 1;
}

UnixPattern* UnixPattern_new(void) {
    UnixPattern* self = ((UnixPattern*)malloc(sizeof(UnixPattern)));
    memset(self, 0, sizeof(UnixPattern));
    UnixPattern_init(self);
    return self;
}

void UnixPattern_destroy(UnixPattern* self) {
    free(self);
}

bool UnixPattern_matches(char* pattern, char* text) {
    return (fnmatch(pattern, text, 0) == 0);
}

void Pattern_init(Pattern* self) {
    self->__rc = 1;
}

Pattern* Pattern_new(void) {
    Pattern* self = ((Pattern*)malloc(sizeof(Pattern)));
    memset(self, 0, sizeof(Pattern));
    Pattern_init(self);
    return self;
}

void Pattern_destroy(Pattern* self) {
    free(self);
}

bool Pattern_matches(char* pattern, char* text) {
    return UnixPattern_matches(pattern, text);
}

bool Pattern_anyMatches(btrc_Vector_string* patterns, char* text) {
    int __n_67 = btrc_Vector_string_iterLen(patterns);
    for (int __i_66 = 0; (__i_66 < __n_67); (__i_66++)) {
        char* pattern = btrc_Vector_string_iterGet(patterns, __i_66);
        if (Pattern_matches(pattern, text)) {
            return true;
        }
    }
    return false;
}

void Random_init(Random* self) {
    self->__rc = 1;
    (self->seeded = false);
}

Random* Random_new(void) {
    Random* self = ((Random*)malloc(sizeof(Random)));
    memset(self, 0, sizeof(Random));
    Random_init(self);
    return self;
}

void Random_destroy(Random* self) {
    free(self);
}

void Random_seed(Random* self, int s) {
    srand(s);
    (self->seeded = true);
}

void Random_seedTime(Random* self) {
    srand(((unsigned int)time(NULL)));
    (self->seeded = true);
}

int Random_randint(Random* self, int lo, int hi) {
    if (!self->seeded) {
        Random_seedTime(self);
    }
    return (lo + (rand() % ((hi - lo) + 1)));
}

float Random_random(Random* self) {
    if (!self->seeded) {
        Random_seedTime(self);
    }
    return __btrc_div_double(((float)rand()), ((float)RAND_MAX));
}

float Random_uniform(Random* self, float lo, float hi) {
    return (lo + (Random_random(self) * (hi - lo)));
}

int Random_choice(Random* self, btrc_Vector_int* items) {
    int idx = Random_randint(self, 0, (items->len - 1));
    return btrc_Vector_int_get(items, idx);
}

void Random_shuffle(Random* self, btrc_Vector_int* items) {
    for (int i = (items->len - 1); (i > 0); (i--)) {
        int j = Random_randint(self, 0, i);
        int tmp = btrc_Vector_int_get(items, i);
        btrc_Vector_int_set(items, i, btrc_Vector_int_get(items, j));
        btrc_Vector_int_set(items, j, tmp);
    }
}

void UnixPamPassword_init(UnixPamPassword* self) {
    self->__rc = 1;
}

UnixPamPassword* UnixPamPassword_new(void) {
    UnixPamPassword* self = ((UnixPamPassword*)malloc(sizeof(UnixPamPassword)));
    memset(self, 0, sizeof(UnixPamPassword));
    UnixPamPassword_init(self);
    return self;
}

void UnixPamPassword_destroy(UnixPamPassword* self) {
    free(self);
}

bool UnixPamPassword_change(char* user, char* oldPassword, char* newPassword) {
    struct passwd* pw = getpwnam(user);
    if (pw == NULL) {
        return false;
    }
    int fd = (-1);
    pid_t pid = forkpty((&fd), NULL, NULL, NULL);
    if (pid < 0) {
        return false;
    }
    if (pid == 0) {
        setgid(pw->pw_gid);
        setuid(pw->pw_uid);
        setenv("HOME", pw->pw_dir, 1);
        setenv("USER", user, 1);
        setenv("LOGNAME", user, 1);
        setenv("PATH", "/run/wrappers/bin:/run/current-system/sw/bin:/usr/bin:/bin", 1);
        execlp("passwd", "passwd", ((char*)NULL));
        _exit(127);
    }
    char* responses[3];
    (responses[0] = oldPassword);
    (responses[1] = newPassword);
    (responses[2] = newPassword);
    int response = 0;
    time_t deadline = (time(NULL) + 30);
    char buffer[4096];
    while ((time(NULL) < deadline) && (response < 3)) {
        fd_set readfds;
        FD_ZERO((&readfds));
        FD_SET(fd, (&readfds));
        struct timeval timeout;
        (timeout.tv_sec = 5);
        (timeout.tv_usec = 0);
        int ready = select((fd + 1), (&readfds), NULL, NULL, (&timeout));
        if (ready < 0) {
            break;
        }
        if (ready == 0) {
            continue;
        }
        ssize_t n = read(fd, buffer, sizeof(buffer));
        if (n <= 0) {
            break;
        }
        bool prompt = false;
        for (ssize_t i = 0; (i < n); (i++)) {
            if (buffer[i] == ':') {
                (prompt = true);
            }
        }
        if (prompt) {
            write(fd, responses[response], strlen(responses[response]));
            write(fd, "\n", 1);
            (response++);
            usleep(200000);
        }
    }
    close(fd);
    int status = 0;
    waitpid(pid, (&status), 0);
    return (WIFEXITED(status) && (WEXITSTATUS(status) == 0));
}

void Toml_init(Toml* self) {
    self->__rc = 1;
}

Toml* Toml_new(void) {
    Toml* self = ((Toml*)malloc(sizeof(Toml)));
    memset(self, 0, sizeof(Toml));
    Toml_init(self);
    return self;
}

void Toml_destroy(Toml* self) {
    free(self);
}

char* Toml_stripInlineComment(char* raw) {
    bool inString = false;
    bool escaped = false;
    int len = ((int)strlen(raw));
    for (int i = 0; (i < len); (i++)) {
        char c = raw[i];
        if (inString) {
            if ((!escaped) && (c == ((char)34))) {
                (inString = false);
            }
            (escaped = ((!escaped) && (c == '\\')));
            if (c != '\\') {
                (escaped = false);
            }
            continue;
        }
        if (c == ((char)34)) {
            (inString = true);
            (escaped = false);
            continue;
        }
        if (c == '#') {
            return __btrc_str_track(__btrc_trim(__btrc_str_track(__btrc_substring(raw, 0, i))));
        }
    }
    return __btrc_str_track(__btrc_trim(raw));
}

char* Toml_unquote(char* raw) {
    char* value = __btrc_str_track(__btrc_trim(raw));
    if ((__btrc_startsWith(value, "\"") && __btrc_endsWith(value, "\"")) && (((int)strlen(value)) >= 2)) {
        return __btrc_str_track(__btrc_substring(value, 1, (((int)strlen(value)) - 2)));
    }
    if ((__btrc_startsWith(value, "'") && __btrc_endsWith(value, "'")) && (((int)strlen(value)) >= 2)) {
        return __btrc_str_track(__btrc_substring(value, 1, (((int)strlen(value)) - 2)));
    }
    return value;
}

char* Toml_key(char* line) {
    char* cleaned = Toml_stripInlineComment(line);
    int pos = Strings_find(cleaned, "=", 0);
    if (pos < 0) {
        return "";
    }
    return Toml_unquote(__btrc_str_track(__btrc_trim(__btrc_str_track(__btrc_substring(cleaned, 0, pos)))));
}

char* Toml_value(char* line) {
    char* cleaned = Toml_stripInlineComment(line);
    int pos = Strings_find(cleaned, "=", 0);
    if (pos < 0) {
        return "";
    }
    return Toml_unquote(__btrc_str_track(__btrc_trim(__btrc_str_track(__btrc_substring(cleaned, (pos + 1), ((((int)strlen(cleaned)) - pos) - 1))))));
}

char* Toml_sectionName(char* line) {
    char* cleaned = Toml_stripInlineComment(line);
    if ((!__btrc_startsWith(cleaned, "[")) || (!__btrc_endsWith(cleaned, "]"))) {
        return "";
    }
    if (__btrc_startsWith(cleaned, "[[")) {
        return "";
    }
    return __btrc_str_track(__btrc_trim(__btrc_str_track(__btrc_substring(cleaned, 1, (((int)strlen(cleaned)) - 2)))));
}

char* Toml_tableArrayName(char* line) {
    char* cleaned = Toml_stripInlineComment(line);
    if ((!__btrc_startsWith(cleaned, "[[")) || (!__btrc_endsWith(cleaned, "]]"))) {
        return "";
    }
    return __btrc_str_track(__btrc_trim(__btrc_str_track(__btrc_substring(cleaned, 2, (((int)strlen(cleaned)) - 4)))));
}

btrc_Map_string_string* Toml_sectionMap(char* content, char* section) {
    btrc_Map_string_string* result = btrc_Map_string_string_new();
    bool inSection = false;
    int __n_69 = btrc_Vector_string_iterLen(Strings_split(content, "\n"));
    for (int __i_68 = 0; (__i_68 < __n_69); (__i_68++)) {
        char* line = btrc_Vector_string_iterGet(Strings_split(content, "\n"), __i_68);
        char* trimmed = __btrc_str_track(__btrc_trim(line));
        char* name = Toml_sectionName(trimmed);
        if (strcmp(name, section) == 0) {
            (inSection = true);
            continue;
        }
        if ((!__btrc_isEmpty(name)) || (!__btrc_isEmpty(Toml_tableArrayName(trimmed)))) {
            (inSection = false);
            continue;
        }
        if (!inSection) {
            continue;
        }
        char* field = Toml_key(trimmed);
        if (!__btrc_isEmpty(field)) {
            btrc_Map_string_string_put(result, field, Toml_value(trimmed));
        }
    }
    return result;
}

btrc_Vector_Map_string_string* Toml_tableArrayBlocks(char* content, char* table) {
    btrc_Vector_Map_string_string* blocks = btrc_Vector_Map_string_string_new();
    btrc_Map_string_string* current = btrc_Map_string_string_new();
    bool inTable = false;
    int __n_71 = btrc_Vector_string_iterLen(Strings_split(content, "\n"));
    for (int __i_70 = 0; (__i_70 < __n_71); (__i_70++)) {
        char* line = btrc_Vector_string_iterGet(Strings_split(content, "\n"), __i_70);
        char* trimmed = __btrc_str_track(__btrc_trim(line));
        char* name = Toml_tableArrayName(trimmed);
        if (strcmp(name, table) == 0) {
            if (inTable) {
                btrc_Vector_Map_string_string_push(blocks, current);
            }
            (current = btrc_Map_string_string_new());
            (inTable = true);
            continue;
        }
        if ((!__btrc_isEmpty(Toml_sectionName(trimmed))) || (!__btrc_isEmpty(name))) {
            if (inTable) {
                btrc_Vector_Map_string_string_push(blocks, current);
            }
            (inTable = false);
            continue;
        }
        if (!inTable) {
            continue;
        }
        char* field = Toml_key(trimmed);
        if (!__btrc_isEmpty(field)) {
            btrc_Map_string_string_put(current, field, Toml_value(trimmed));
        }
    }
    if (inTable) {
        btrc_Vector_Map_string_string_push(blocks, current);
    }
    return blocks;
}

