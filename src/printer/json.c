/**
 * @file printer/json.c
 * @author Radek Krejci <rkrejci@cesnet.cz>
 * @brief JSON printer for libyang data structure
 *
 * Copyright (c) 2015 CESNET, z.s.p.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Company nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "../common.h"
#include "../tree.h"

#define INDENT ""
#define LEVEL (level*2)

void json_print_node(FILE *f, int level, struct lyd_node *node);

/* 0 - same, 1 - different */
static int
nscmp(struct lyd_node *node1, struct lyd_node *node2)
{
    struct ly_module *m1, *m2;

    /* we have to cover submodules belonging to the same module */
    if (node1->schema->module->type) {
        m1 = ((struct ly_submodule *)node1->schema->module)->belongsto;
    } else {
        m1 = node1->schema->module;
    }
    if (node2->schema->module->type) {
        m2 = ((struct ly_submodule *)node2->schema->module)->belongsto;
    } else {
        m2 = node2->schema->module;
    }
    if (m1 == m2) {
        /* belongs to the same module */
        return 0;
    } else {
        /* different modules */
        return 1;
    }
}

static void
json_print_container(FILE *f, int level, struct lyd_node *node)
{
    const char *schema;
    struct lyd_node *child;

    if (!node->parent || nscmp(node, node->parent)) {
        /* print "namespace" */
        if (node->schema->module->type) {
            /* submodule, get module */
            schema = ((struct ly_submodule *)node->schema->module)->belongsto->name;
        } else {
            schema = node->schema->module->name;
        }
        fprintf(f, "%*s\"%s:%s\": {\n", LEVEL, INDENT, schema, node->schema->name);
    } else {
        fprintf(f, "%*s\"%s\": {\n", LEVEL, INDENT, node->schema->name);
    }
    LY_TREE_FOR(node->child, child) {
        json_print_node(f, level + 1, child);
    }
    fprintf(f, "%*s}%s\n", LEVEL, INDENT, node->next ? "," : "");
}

static void
json_print_list_internal(FILE *f, int level, struct lyd_node_list *list)
{
    struct lyd_node *child;

    fprintf(f, "%*s{\n", LEVEL, INDENT);

    LY_TREE_FOR(list->child, child) {
        json_print_node(f, level + 1, child);
    }

    fprintf(f, "%*s}%s\n", LEVEL, INDENT, list->lnext ? "," : "");
}

static void
json_print_list(FILE *f, int level, struct lyd_node *node)
{
    const char *schema;
    struct lyd_node_list *list = (struct lyd_node_list *)node;
    struct lyd_node *iter;
    char *delim = "";

    if (list->lprev) {
        /* this list is already printed */
        return;
    }

    if (!node->parent || nscmp(node, node->parent)) {
        /* print "namespace" */
        if (node->schema->module->type) {
            /* submodule, get module */
            schema = ((struct ly_submodule *)node->schema->module)->belongsto->name;
        } else {
            schema = node->schema->module->name;
        }
        fprintf(f, "%*s\"%s:%s\": [\n", LEVEL, INDENT, schema, node->schema->name);
    } else {
        fprintf(f, "%*s\"%s\": [\n", LEVEL, INDENT, node->schema->name);
    }

    for(; list; list = list->lnext) {
        json_print_list_internal(f, level + 1, list);
    }
    for (iter = node->next; iter; iter = iter->next) {
        if (iter->schema != node->schema) {
            delim = ",";
            break;
        }
    }
    fprintf(f, "%*s]%s\n", LEVEL, INDENT, delim);
}

static void
json_print_leaf(FILE *f, int level, struct lyd_node *node)
{
    struct lyd_node_leaf *leaf = (struct lyd_node_leaf *)node;
    const char *schema;

    if (!node->parent || nscmp(node, node->parent)) {
        /* print "namespace" */
        if (node->schema->module->type) {
            /* submodule, get module */
            schema = ((struct ly_submodule *)node->schema->module)->belongsto->name;
        } else {
            schema = node->schema->module->name;
        }
        fprintf(f, "%*s\"%s:%s\": ", LEVEL, INDENT, schema, node->schema->name);
    } else {
        fprintf(f, "%*s\"%s\": ", LEVEL, INDENT, node->schema->name);
    }

    switch (((struct ly_mnode_leaf *)leaf->schema)->type.base) {
    case LY_TYPE_BINARY:
    case LY_TYPE_STRING:
        fprintf(f, "\"%s\"%s\n", leaf->value.string, node->next ? "," : "");
        break;
    default:
        /* TODO */
        fprintf(f, "%s%s\n", "\"TBD\"", node->next ? "," : "");
    }

}

void
json_print_node(FILE *f, int level, struct lyd_node *node)
{
    switch (node->schema->nodetype) {
    case LY_NODE_LIST:
        json_print_list(f, level, node);
        break;
    case LY_NODE_CONTAINER:
        json_print_container(f, level, node);
        break;
    case LY_NODE_LEAF:
        json_print_leaf(f, level, node);
        break;
    default:
        /* TODO: remove when all node types are covered */
        break;
    }

}

int
json_print_data(FILE *f, struct lyd_node *root)
{
    int level = 0;
    struct lyd_node *node;

    /* start */
    fprintf(f, "{\n");

    /* content */
    LY_TREE_FOR(root, node) {
        json_print_node(f, level + 1, node);
    }

    /* end */
    fprintf(f, "}\n");

    return EXIT_SUCCESS;
}
