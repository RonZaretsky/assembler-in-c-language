/**
 * @file vector.h
 * @author Ron Zaretsky (ronz2512@icloud.com)
 * @brief implemention of vecot data structure in c
 * @version 0.1
 * @date 2023-07-01
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef __VECTOR_H_
#define __VECTOR_H_
#include <stddef.h>

struct vector;

typedef struct vector * Vector;

/**
 * @brief New Vector.
 * 
 * @param ctor Constructor Function
 * @param dtor Distructor Function
 * @return Vector 
 */
Vector new_vector(void * (*ctor)(const void *copy),  void (*dtor)(void *item));

/**
 * @brief Return the address of the first index.
 * 
 * @param v Your vector
 * @return void* const* 
 */
void * const * vector_begin(const Vector v);

/**
 * @brief Returns the address of the last index.
 * 
 * @param v Your vector
 * @return void* const* 
 */
void * const * vector_end(const Vector v);

/**
 * @brief Returns the items count.
 * 
 * @param v Your vector
 * @return size_t 
 */
size_t vector_get_item_count(const Vector v);

/**
 * @brief Item insertion.
 * 
 * @param v Your vector
 * @param copy_item The item you want to insert
 * @return void* 
 */
void * vector_insert(Vector v,  const void *copy_item );

/**
 * @brief Destoys the vector
 * 
 * @param v Your vector
 */
void vector_destroy(Vector * v);

#define VECTOR_FOR_EACH(begin,end,v) for(begin = vector_begin(nums),end = vector_end(nums);begin <= end; begin++)
#endif