/*
 *  Copyright (C) 2015, Clarive Software, All Rights Reserved
 *
 *  This file is part of clax.
 *
 *  Clax is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Clax is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Clax.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CLAX_UTIL_H
#define CLAX_UTIL_H

#include <stdio.h>
#include <string.h>

#define sizeof_struct_member(type, member) sizeof(((type *)0)->member)

#define TRY if ((
#define GOTO ) < 0) {goto error;}

#define MIN(a,b) ((a) < (b) ? (a) : (b))

typedef struct {
    char *key;
    char *val;
} clax_kv_list_item_t;

typedef struct {
    clax_kv_list_item_t **items;
    size_t size;
} clax_kv_list_t;

void clax_kv_list_init(clax_kv_list_t *list);
void clax_kv_list_free(clax_kv_list_t *list);
int clax_kv_list_push(clax_kv_list_t *list, char *key, char *val);
char *clax_kv_list_find_all(clax_kv_list_t *list, char *key, size_t *start);
char *clax_kv_list_find(clax_kv_list_t *list, char *key);
clax_kv_list_item_t *clax_kv_list_find_item(clax_kv_list_t *list, char *key);
clax_kv_list_item_t *clax_kv_list_at(clax_kv_list_t *list, size_t index);
clax_kv_list_item_t *clax_kv_list_next(clax_kv_list_t *list, size_t *start);

char *clax_buf2str(const unsigned char *buf, size_t len);

#endif
