/*
-------------------------------------------------------
Name of project: Translator IFJ26
File: test_symbol_table.c

Description: Unit tests (cmocka) of the symbol table - a stack of AVL
             trees (symbol_table.c). Covers scopes, shadowing, AVL
             balancing (all four rotations), NULL arguments and
             memory ownership of keys / function parameters.
-------------------------------------------------------
*/

#include "symbol_table.h"
#include "test_helpers.h"
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static symtable_data_t make_var(symtable_datatype_t type, bool is_mutable, int unique_id) {
    symtable_data_t d;
    memset(&d, 0, sizeof(d));
    d.type = SYM_TYPE_VAR;
    d.var.type = type;
    d.var.is_mutable = is_mutable;
    d.var.unique_id = unique_id;
    return d;
}

static symtable_data_t make_fun(symtable_datatype_t return_type, int param_count,
                                symtable_fun_param_t *params) {
    symtable_data_t d;
    memset(&d, 0, sizeof(d));
    d.type = SYM_TYPE_FUNC;
    d.fun.return_type = return_type;
    d.fun.param_count = param_count;
    d.fun.params = params;
    return d;
}

static symtable_fun_param_t *make_param(const char *name, symtable_datatype_t type,
                                        symtable_fun_param_t *next) {
    symtable_fun_param_t *p = calloc(1, sizeof(*p));
    p->name = strdup(name);
    p->type = type;
    p->next = next;
    return p;
}

// Setup: stack with one (global) scope
static int setup_stack(void **state) {
    test_setup_timeout(state);
    symtable_stack_t *stack = calloc(1, sizeof(*stack));
    assert_true(symtable_init(stack));
    assert_true(symtable_create_avl(stack));
    *state = stack;
    return 0;
}

static int teardown_stack(void **state) {
    symtable_stack_t *stack = *state;
    symtable_stack_delete(stack);
    free(stack);
    test_teardown_timeout(state);
    return 0;
}

// Recursively verifies BST order, stored heights and AVL balance. Returns height.
static int check_avl(symtable_node_t *n, const char *min_excl, const char *max_excl) {
    if (n == NULL) {
        return 0;
    }
    if (min_excl != NULL) {
        assert_true(strcmp(n->key, min_excl) > 0);
    }
    if (max_excl != NULL) {
        assert_true(strcmp(n->key, max_excl) < 0);
    }
    int lh = check_avl(n->left, min_excl, n->key);
    int rh = check_avl(n->right, n->key, max_excl);
    int h = (lh > rh ? lh : rh) + 1;
    assert_int_equal(n->height, h);
    assert_true(lh - rh >= -1 && lh - rh <= 1);
    return h;
}

// ---------------------------------------------------------------------------
// symtable_init / create / pop / delete
// ---------------------------------------------------------------------------

static void test_init_null_returns_false(void **state) {
    (void)state;
    assert_false(symtable_init(NULL));
}

static void test_init_sets_empty_stack(void **state) {
    (void)state;
    symtable_stack_t stack;
    stack.top = (void *)0x1;
    stack.global = (void *)0x1;
    assert_true(symtable_init(&stack));
    assert_null(stack.top);
    assert_null(stack.global);
}

static void test_create_avl_null_returns_false(void **state) {
    (void)state;
    assert_false(symtable_create_avl(NULL));
}

static void test_first_scope_is_global(void **state) {
    symtable_stack_t *stack = *state;
    assert_non_null(stack->top);
    assert_ptr_equal(stack->top, stack->global);
    assert_null(stack->top->next);
    assert_null(stack->top->tree_root);
}

static void test_second_scope_becomes_top_but_not_global(void **state) {
    symtable_stack_t *stack = *state;
    symtable_stack_item_t *global = stack->global;
    assert_true(symtable_create_avl(stack));
    assert_ptr_not_equal(stack->top, global);
    assert_ptr_equal(stack->global, global);
    assert_ptr_equal(stack->top->next, global);
}

static void test_pop_avl_restores_previous_top(void **state) {
    symtable_stack_t *stack = *state;
    symtable_stack_item_t *global = stack->top;
    assert_true(symtable_create_avl(stack));
    symtable_pop_avl(stack);
    assert_ptr_equal(stack->top, global);
    assert_ptr_equal(stack->global, global);
}

static void test_pop_last_avl_clears_global(void **state) {
    symtable_stack_t *stack = *state;
    symtable_pop_avl(stack);
    assert_null(stack->top);
    assert_null(stack->global);
}

static void test_pop_on_empty_stack_is_safe(void **state) {
    (void)state;
    symtable_stack_t stack;
    symtable_init(&stack);
    symtable_pop_avl(&stack);
    symtable_pop_avl(NULL);
    assert_null(stack.top);
}

static void test_stack_delete_clears_everything(void **state) {
    symtable_stack_t *stack = *state;
    assert_true(symtable_insert_item(stack, "a", make_var(SYM_DATATYPE_INT, true, 0)));
    assert_true(symtable_create_avl(stack));
    assert_true(symtable_insert_item(stack, "b", make_var(SYM_DATATYPE_INT, true, 1)));
    symtable_stack_delete(stack);
    assert_null(stack->top);
    assert_null(stack->global);
    // deleting again / deleting NULL must be safe
    symtable_stack_delete(stack);
    symtable_stack_delete(NULL);
}

// ---------------------------------------------------------------------------
// Insert / search
// ---------------------------------------------------------------------------

static void test_insert_and_search_variable(void **state) {
    symtable_stack_t *stack = *state;
    assert_true(symtable_insert_item(stack, "x", make_var(SYM_DATATYPE_DOUBLE, true, 42)));
    symtable_node_t *n = symtable_search(stack, SYM_TYPE_VAR, "x");
    assert_non_null(n);
    assert_string_equal(n->key, "x");
    assert_int_equal(n->data.type, SYM_TYPE_VAR);
    assert_int_equal(n->data.var.type, SYM_DATATYPE_DOUBLE);
    assert_true(n->data.var.is_mutable);
    assert_int_equal(n->data.var.unique_id, 42);
}

static void test_insert_and_search_function(void **state) {
    symtable_stack_t *stack = *state;
    symtable_fun_param_t *p2 = make_param("b", SYM_DATATYPE_STRING, NULL);
    symtable_fun_param_t *p1 = make_param("a", SYM_DATATYPE_INT, p2);
    assert_true(symtable_insert_item(stack, "foo", make_fun(SYM_DATATYPE_UNIT, 2, p1)));

    symtable_node_t *n = symtable_search(stack, SYM_TYPE_FUNC, "foo");
    assert_non_null(n);
    assert_int_equal(n->data.fun.return_type, SYM_DATATYPE_UNIT);
    assert_int_equal(n->data.fun.param_count, 2);
    assert_ptr_equal(n->data.fun.params, p1);
    assert_string_equal(n->data.fun.params->next->name, "b");
}

static void test_search_missing_returns_null(void **state) {
    symtable_stack_t *stack = *state;
    assert_null(symtable_search(stack, SYM_TYPE_VAR, "nothing"));
    symtable_insert_item(stack, "x", make_var(SYM_DATATYPE_INT, false, 0));
    assert_null(symtable_search(stack, SYM_TYPE_VAR, "y"));
}

static void test_search_wrong_symbol_type_returns_null(void **state) {
    symtable_stack_t *stack = *state;
    symtable_insert_item(stack, "x", make_var(SYM_DATATYPE_INT, false, 0));
    assert_null(symtable_search(stack, SYM_TYPE_FUNC, "x"));
    assert_null(symtable_stack_search_current(stack, SYM_TYPE_FUNC, "x"));
}

static void test_duplicate_insert_in_same_scope_fails(void **state) {
    symtable_stack_t *stack = *state;
    assert_true(symtable_insert_item(stack, "x", make_var(SYM_DATATYPE_INT, false, 1)));
    assert_false(symtable_insert_item(stack, "x", make_var(SYM_DATATYPE_STRING, true, 2)));
    // original is untouched
    symtable_node_t *n = symtable_search(stack, SYM_TYPE_VAR, "x");
    assert_non_null(n);
    assert_int_equal(n->data.var.unique_id, 1);
    assert_int_equal(n->data.var.type, SYM_DATATYPE_INT);
}

static void test_keys_are_case_sensitive(void **state) {
    symtable_stack_t *stack = *state;
    assert_true(symtable_insert_item(stack, "abc", make_var(SYM_DATATYPE_INT, false, 1)));
    assert_true(symtable_insert_item(stack, "ABC", make_var(SYM_DATATYPE_INT, false, 2)));
    assert_int_equal(symtable_search(stack, SYM_TYPE_VAR, "abc")->data.var.unique_id, 1);
    assert_int_equal(symtable_search(stack, SYM_TYPE_VAR, "ABC")->data.var.unique_id, 2);
}

static void test_insert_copies_the_key(void **state) {
    symtable_stack_t *stack = *state;
    char buf[8] = "name";
    assert_true(symtable_insert_item(stack, buf, make_var(SYM_DATATYPE_INT, false, 0)));
    strcpy(buf, "XXXX");
    assert_non_null(symtable_search(stack, SYM_TYPE_VAR, "name"));
    assert_null(symtable_search(stack, SYM_TYPE_VAR, "XXXX"));
}

static void test_null_arguments(void **state) {
    symtable_stack_t *stack = *state;
    symtable_data_t d = make_var(SYM_DATATYPE_INT, false, 0);
    assert_false(symtable_insert_item(NULL, "x", d));
    assert_false(symtable_insert_item(stack, NULL, d));
    assert_null(symtable_search(NULL, SYM_TYPE_VAR, "x"));
    assert_null(symtable_search(stack, SYM_TYPE_VAR, NULL));
    assert_null(symtable_stack_search_current(NULL, SYM_TYPE_VAR, "x"));
    assert_null(symtable_stack_search_current(stack, SYM_TYPE_VAR, NULL));
}

static void test_search_on_empty_stack_returns_null(void **state) {
    (void)state;
    symtable_stack_t stack;
    symtable_init(&stack);
    assert_null(symtable_search(&stack, SYM_TYPE_VAR, "x"));
    assert_null(symtable_stack_search_current(&stack, SYM_TYPE_VAR, "x"));
}

static void test_insert_on_stack_without_scope_fails_gracefully(void **state) {
    (void)state;
    // symtable_init() without symtable_create_avl(): top == NULL
    symtable_stack_t stack;
    symtable_init(&stack);
    assert_false(symtable_insert_item(&stack, "x", make_var(SYM_DATATYPE_INT, false, 0)));
}

static void test_empty_key_is_handled(void **state) {
    symtable_stack_t *stack = *state;
    assert_true(symtable_insert_item(stack, "", make_var(SYM_DATATYPE_INT, false, 0)));
    assert_non_null(symtable_search(stack, SYM_TYPE_VAR, ""));
}

// ---------------------------------------------------------------------------
// Scopes and shadowing
// ---------------------------------------------------------------------------

static void test_search_looks_through_outer_scopes(void **state) {
    symtable_stack_t *stack = *state;
    symtable_insert_item(stack, "outer", make_var(SYM_DATATYPE_INT, false, 1));
    symtable_create_avl(stack);
    symtable_create_avl(stack);
    assert_non_null(symtable_search(stack, SYM_TYPE_VAR, "outer"));
}

static void test_search_current_ignores_outer_scopes(void **state) {
    symtable_stack_t *stack = *state;
    symtable_insert_item(stack, "outer", make_var(SYM_DATATYPE_INT, false, 1));
    symtable_create_avl(stack);
    assert_null(symtable_stack_search_current(stack, SYM_TYPE_VAR, "outer"));
    symtable_insert_item(stack, "inner", make_var(SYM_DATATYPE_INT, false, 2));
    assert_non_null(symtable_stack_search_current(stack, SYM_TYPE_VAR, "inner"));
}

static void test_shadowing_in_inner_scope(void **state) {
    symtable_stack_t *stack = *state;
    symtable_insert_item(stack, "x", make_var(SYM_DATATYPE_INT, false, 1));
    symtable_create_avl(stack);
    // same name in a nested scope is allowed (spec 2.2)
    assert_true(symtable_insert_item(stack, "x", make_var(SYM_DATATYPE_STRING, true, 2)));
    symtable_node_t *inner = symtable_search(stack, SYM_TYPE_VAR, "x");
    assert_int_equal(inner->data.var.unique_id, 2);
    assert_int_equal(inner->data.var.type, SYM_DATATYPE_STRING);

    symtable_pop_avl(stack);
    symtable_node_t *outer = symtable_search(stack, SYM_TYPE_VAR, "x");
    assert_int_equal(outer->data.var.unique_id, 1);
    assert_int_equal(outer->data.var.type, SYM_DATATYPE_INT);
}

static void test_popped_scope_symbols_disappear(void **state) {
    symtable_stack_t *stack = *state;
    symtable_create_avl(stack);
    symtable_insert_item(stack, "local", make_var(SYM_DATATYPE_INT, false, 1));
    assert_non_null(symtable_search(stack, SYM_TYPE_VAR, "local"));
    symtable_pop_avl(stack);
    assert_null(symtable_search(stack, SYM_TYPE_VAR, "local"));
}

static void test_search_skips_other_symbol_type_in_inner_scope(void **state) {
    symtable_stack_t *stack = *state;
    // global function "f", local variable "f" -> searching for the function still finds the global
    // one
    symtable_insert_item(stack, "f", make_fun(SYM_DATATYPE_INT, 0, NULL));
    symtable_create_avl(stack);
    symtable_insert_item(stack, "f", make_var(SYM_DATATYPE_INT, false, 1));
    assert_non_null(symtable_search(stack, SYM_TYPE_FUNC, "f"));
    assert_non_null(symtable_search(stack, SYM_TYPE_VAR, "f"));
}

static void test_global_scope_is_reachable_after_many_pushes(void **state) {
    symtable_stack_t *stack = *state;
    symtable_insert_item(stack, "g", make_fun(SYM_DATATYPE_UNIT, 0, NULL));
    for (int i = 0; i < 50; i++) {
        assert_true(symtable_create_avl(stack));
    }
    assert_non_null(symtable_search(stack, SYM_TYPE_FUNC, "g"));
    for (int i = 0; i < 50; i++) {
        symtable_pop_avl(stack);
    }
    assert_ptr_equal(stack->top, stack->global);
}

// ---------------------------------------------------------------------------
// AVL behaviour
// ---------------------------------------------------------------------------

static void insert_keys(symtable_stack_t *stack, const char *const *keys, int count) {
    for (int i = 0; i < count; i++) {
        assert_true(
            symtable_insert_item(stack, (char *)keys[i], make_var(SYM_DATATYPE_INT, false, i)));
    }
}

static void test_avl_rotation_right_right(void **state) {
    symtable_stack_t *stack = *state;
    const char *keys[] = {"a", "b", "c"};
    insert_keys(stack, keys, 3);
    symtable_node_t *root = stack->top->tree_root;
    assert_string_equal(root->key, "b");
    assert_int_equal(root->height, 2);
    check_avl(root, NULL, NULL);
}

static void test_avl_rotation_left_left(void **state) {
    symtable_stack_t *stack = *state;
    const char *keys[] = {"c", "b", "a"};
    insert_keys(stack, keys, 3);
    assert_string_equal(stack->top->tree_root->key, "b");
    check_avl(stack->top->tree_root, NULL, NULL);
}

static void test_avl_rotation_left_right(void **state) {
    symtable_stack_t *stack = *state;
    const char *keys[] = {"c", "a", "b"};
    insert_keys(stack, keys, 3);
    assert_string_equal(stack->top->tree_root->key, "b");
    check_avl(stack->top->tree_root, NULL, NULL);
}

static void test_avl_rotation_right_left(void **state) {
    symtable_stack_t *stack = *state;
    const char *keys[] = {"a", "c", "b"};
    insert_keys(stack, keys, 3);
    assert_string_equal(stack->top->tree_root->key, "b");
    check_avl(stack->top->tree_root, NULL, NULL);
}

static void test_avl_sorted_insert_stays_balanced(void **state) {
    symtable_stack_t *stack = *state;
    char key[16];
    for (int i = 0; i < 1000; i++) {
        snprintf(key, sizeof(key), "k%05d", i);
        assert_true(symtable_insert_item(stack, key, make_var(SYM_DATATYPE_INT, false, i)));
    }
    int h = check_avl(stack->top->tree_root, NULL, NULL);
    assert_true(h <= 14); // 1.44 * log2(1000) ~ 14.4 is the AVL upper bound
    assert_true(h >= 10); // and at least ceil(log2(1001))
    for (int i = 0; i < 1000; i += 37) {
        snprintf(key, sizeof(key), "k%05d", i);
        symtable_node_t *n = symtable_search(stack, SYM_TYPE_VAR, key);
        assert_non_null(n);
        assert_int_equal(n->data.var.unique_id, i);
    }
}

static void test_avl_reverse_and_zigzag_insert_stays_balanced(void **state) {
    symtable_stack_t *stack = *state;
    char key[16];
    for (int i = 500; i > 0; i--) {
        snprintf(key, sizeof(key), "r%05d", i);
        assert_true(symtable_insert_item(stack, key, make_var(SYM_DATATYPE_INT, false, i)));
    }
    for (int i = 0; i < 250; i++) {
        snprintf(key, sizeof(key), "z%05d", (i % 2) ? 1000 - i : i);
        assert_true(symtable_insert_item(stack, key, make_var(SYM_DATATYPE_INT, false, i)));
    }
    check_avl(stack->top->tree_root, NULL, NULL);
}

static void test_avl_pseudo_random_insert_all_found(void **state) {
    symtable_stack_t *stack = *state;
    char key[16];
    unsigned int seed = 12345u;
    int inserted = 0;
    for (int i = 0; i < 600; i++) {
        seed = seed * 1103515245u + 12345u;
        snprintf(key, sizeof(key), "n%u", (seed >> 8) % 100000u);
        if (symtable_insert_item(stack, key, make_var(SYM_DATATYPE_INT, false, i))) {
            inserted++;
        }
    }
    assert_true(inserted > 500);
    check_avl(stack->top->tree_root, NULL, NULL);
}

static void test_function_params_are_freed_with_scope(void **state) {
    // Run under valgrind (make valgrind) to verify there are no leaks:
    // the table owns the parameter list and its names.
    symtable_stack_t *stack = *state;
    symtable_create_avl(stack);
    symtable_fun_param_t *p =
        make_param("a", SYM_DATATYPE_INT, make_param("b", SYM_DATATYPE_INT, NULL));
    assert_true(symtable_insert_item(stack, "f", make_fun(SYM_DATATYPE_INT, 2, p)));
    symtable_pop_avl(stack);
}

#define T(fn) cmocka_unit_test_setup_teardown(fn, setup_stack, teardown_stack)
#define TN(fn) cmocka_unit_test_setup_teardown(fn, test_setup_timeout, test_teardown_timeout)

int main(void) {
    const struct CMUnitTest tests[] = {
        TN(test_init_null_returns_false),
        TN(test_init_sets_empty_stack),
        TN(test_create_avl_null_returns_false),
        T(test_first_scope_is_global),
        T(test_second_scope_becomes_top_but_not_global),
        T(test_pop_avl_restores_previous_top),
        T(test_pop_last_avl_clears_global),
        TN(test_pop_on_empty_stack_is_safe),
        T(test_stack_delete_clears_everything),
        T(test_insert_and_search_variable),
        T(test_insert_and_search_function),
        T(test_search_missing_returns_null),
        T(test_search_wrong_symbol_type_returns_null),
        T(test_duplicate_insert_in_same_scope_fails),
        T(test_keys_are_case_sensitive),
        T(test_insert_copies_the_key),
        T(test_null_arguments),
        TN(test_search_on_empty_stack_returns_null),
        TN(test_insert_on_stack_without_scope_fails_gracefully),
        T(test_empty_key_is_handled),
        T(test_search_looks_through_outer_scopes),
        T(test_search_current_ignores_outer_scopes),
        T(test_shadowing_in_inner_scope),
        T(test_popped_scope_symbols_disappear),
        T(test_search_skips_other_symbol_type_in_inner_scope),
        T(test_global_scope_is_reachable_after_many_pushes),
        T(test_avl_rotation_right_right),
        T(test_avl_rotation_left_left),
        T(test_avl_rotation_left_right),
        T(test_avl_rotation_right_left),
        T(test_avl_sorted_insert_stays_balanced),
        T(test_avl_reverse_and_zigzag_insert_stays_balanced),
        T(test_avl_pseudo_random_insert_all_found),
        T(test_function_params_are_freed_with_scope),
    };
    return cmocka_run_group_tests_name("symbol_table", tests, NULL, NULL);
}
