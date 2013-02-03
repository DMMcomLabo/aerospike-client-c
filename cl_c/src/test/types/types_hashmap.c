
#include "../test.h"
#include <citrusleaf/as_hashmap.h>
#include <citrusleaf/as_linkedlist.h>
#include <citrusleaf/as_integer.h>
#include <citrusleaf/as_string.h>

/******************************************************************************
 * TEST CASES
 *****************************************************************************/

TEST( types_hashmap_empty, "as_hashmap is empty" ) {
    as_hashmap * hm = as_hashmap_new(0);

    assert_int_eq( as_hashmap_size(hm), 0 );

    as_hashmap_free(hm);
}

TEST( types_hashmap_ops, "as_hashmap ops" ) {

    as_val * a = (as_val *) as_string_new(strdup("a"));
    as_val * b = (as_val *) as_string_new(strdup("b"));
    as_val * c = (as_val *) as_string_new(strdup("c"));
    as_integer * v = NULL;

    as_hashmap * hm = as_hashmap_new(10);
    assert_int_eq( as_hashmap_size(hm), 0 );

    // Setting the values

    as_hashmap_set(hm, as_val_ref(a), (as_val *) as_integer_new(1));
    as_hashmap_set(hm, as_val_ref(b), (as_val *) as_integer_new(2));
    as_hashmap_set(hm, as_val_ref(c), (as_val *) as_integer_new(3));
    assert_int_eq( as_hashmap_size(hm), 3 );

    // check individual values

    v = (as_integer *) as_hashmap_get(hm, a);
    assert_int_eq( v->value, 1 );

    v = (as_integer *) as_hashmap_get(hm, b);
    assert_int_eq( v->value, 2 );

    v = (as_integer *) as_hashmap_get(hm, c);
    assert_int_eq( v->value, 3 );

    // Resetting the values
    
    as_hashmap_set(hm, as_val_ref(a), (as_val *) as_integer_new(4));
    as_hashmap_set(hm, as_val_ref(b), (as_val *) as_integer_new(5));
    as_hashmap_set(hm, as_val_ref(c), (as_val *) as_integer_new(6));
    assert_int_eq( as_hashmap_size(hm), 3 );

    // check individual values

    v = (as_integer *) as_hashmap_get(hm, a);
    assert_int_eq( v->value, 4 );

    v = (as_integer *) as_hashmap_get(hm, b);
    assert_int_eq( v->value, 5 );

    v = (as_integer *) as_hashmap_get(hm, c);
    assert_int_eq( v->value, 6 );

    // Clear the map

    as_hashmap_clear(hm);
    assert_int_eq( as_hashmap_size(hm), 0 );

    // Resetting the values

    as_hashmap_set(hm, as_val_ref(a), (as_val *) as_integer_new(7));
    as_hashmap_set(hm, as_val_ref(b), (as_val *) as_integer_new(8));
    as_hashmap_set(hm, as_val_ref(c), (as_val *) as_integer_new(9));
    assert_int_eq( as_hashmap_size(hm), 3 );

    v = (as_integer *) as_hashmap_get(hm, a);
    assert_int_eq( v->value, 7 );

    v = (as_integer *) as_hashmap_get(hm, b);
    assert_int_eq( v->value, 8 );

    v = (as_integer *) as_hashmap_get(hm, c);
    assert_int_eq( v->value, 9 );

    as_hashmap_free(hm);
}

TEST( types_hashmap_map, "as_hashmap w/ map ops" ) {

    as_val * a = (as_val *) as_string_new(strdup("a"));
    as_val * b = (as_val *) as_string_new(strdup("b"));
    as_val * c = (as_val *) as_string_new(strdup("c"));
    as_integer * v = NULL;

    as_hashmap * hm = as_hashmap_new(10);
    assert_int_eq( as_hashmap_size(hm), 0 );

    as_map m;
    as_map_init(&m, hm, &as_hashmap_map);

    // Setting the values

    as_map_set(&m, as_val_ref(a), (as_val *) as_integer_new(1));
    as_map_set(&m, as_val_ref(b), (as_val *) as_integer_new(2));
    as_map_set(&m, as_val_ref(c), (as_val *) as_integer_new(3));
    assert_int_eq( as_map_size(&m), 3 );

    // check individual values

    v = (as_integer *) as_map_get(&m, a);
    assert_int_eq( v->value, 1 );

    v = (as_integer *) as_map_get(&m, b);
    assert_int_eq( v->value, 2 );

    v = (as_integer *) as_map_get(&m, c);
    assert_int_eq( v->value, 3 );

    // Resetting the values
    
    as_map_set(&m, as_val_ref(a), (as_val *) as_integer_new(4));
    as_map_set(&m, as_val_ref(b), (as_val *) as_integer_new(5));
    as_map_set(&m, as_val_ref(c), (as_val *) as_integer_new(6));
    assert_int_eq( as_map_size(&m), 3 );

    // check individual values

    v = (as_integer *) as_map_get(&m, a);
    assert_int_eq( v->value, 4 );

    v = (as_integer *) as_map_get(&m, b);
    assert_int_eq( v->value, 5 );

    v = (as_integer *) as_map_get(&m, c);
    assert_int_eq( v->value, 6 );

    // Clear the map

    as_map_clear(&m);
    assert_int_eq( as_map_size(&m), 0 );

    // Resetting the values

    as_map_set(&m, as_val_ref(a), (as_val *) as_integer_new(7));
    as_map_set(&m, as_val_ref(b), (as_val *) as_integer_new(8));
    as_map_set(&m, as_val_ref(c), (as_val *) as_integer_new(9));
    assert_int_eq( as_map_size(&m), 3 );

    v = (as_integer *) as_map_get(&m, a);
    assert_int_eq( v->value, 7 );

    v = (as_integer *) as_map_get(&m, b);
    assert_int_eq( v->value, 8 );

    v = (as_integer *) as_map_get(&m, c);
    assert_int_eq( v->value, 9 );

    as_map_destroy(&m);

    as_hashmap_free(hm);
}

TEST( types_hashmap_iterator, "as_hashmap w/ iterator ops" ) {

    as_hashmap * hm = as_hashmap_new(10);
    assert_int_eq( as_hashmap_size(hm), 0 );

    as_map m;
    as_map_init(&m, hm, &as_hashmap_map);

    as_map_set(&m, (as_val *) as_string_new(strdup("a")), (as_val *) as_integer_new(1));
    as_map_set(&m, (as_val *) as_string_new(strdup("b")), (as_val *) as_integer_new(2));
    as_map_set(&m, (as_val *) as_string_new(strdup("c")), (as_val *) as_integer_new(3));
    assert_int_eq( as_map_size(&m), 3 );

    as_iterator * i  = as_map_iterator(&m);

    int count = 0;
    while ( as_iterator_has_next(i) ) {
        as_pair * p = (as_pair *) as_iterator_next(i);
        as_integer * a = (as_integer *) as_pair_2(p);
        as_integer * e = (as_integer *) as_map_get(&m, as_pair_1(p));
        assert( a->value == e->value );
        count ++;
    }

    as_iterator_free(i);

    assert_int_eq( as_map_size(&m), count );

    as_map_destroy(&m);
    as_hashmap_free(hm);
}

TEST( types_hashmap_stack, "as_hashmap via stack allocation" ) {

    as_linkedlist * tail = NULL;

    as_linkedlist a;
    tail = as_linkedlist_init(&a, (as_val *) as_integer_new(1), tail);

    as_linkedlist b;
    tail = as_linkedlist_init(&b, (as_val *) as_integer_new(2), tail);

    as_linkedlist c;
    tail = as_linkedlist_init(&c, (as_val *) as_integer_new(3), tail);

    as_list l;
    as_list_init(&l, &c, &as_linkedlist_list);

    assert_int_eq( as_list_size(&l), 3 );

    as_iterator * i = NULL;
    as_integer * v = NULL;

    i  = as_list_iterator(&l);
    
    assert_true( as_iterator_has_next(i) );

    v = (as_integer *) as_iterator_next(i);
    assert_int_eq( as_integer_toint(v), 3 );

    v = (as_integer *) as_iterator_next(i);
    assert_int_eq( as_integer_toint(v), 2 );

    v = (as_integer *) as_iterator_next(i);
    assert_int_eq( as_integer_toint(v), 1 );

    assert_false( as_iterator_has_next(i) );
}

/******************************************************************************
 * TEST SUITE
 *****************************************************************************/

SUITE( types_hashmap, "as_hashmap" ) {
    suite_add( types_hashmap_empty );
    suite_add( types_hashmap_ops );
    suite_add( types_hashmap_map );
    suite_add( types_hashmap_iterator );
    suite_add( types_hashmap_stack );
}