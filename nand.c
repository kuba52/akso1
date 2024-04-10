#include <malloc.h>
#include "nand.h"
#include <memory.h>
#include <errno.h>


typedef struct item {
    nand_t* val;
    size_t port;
    struct item* next;
} output_item;

typedef struct list{
    output_item* begin;
} gate_list;

typedef struct status {
    int* status;
    struct status* next;
} status_item;

typedef struct status_item* status_list;

struct nand {
    nand_t** gates_in;
    gate_list* gates_out;
    bool** input;
    ssize_t depth;
    size_t size;
    bool isEvalating;
    bool result;
    int* type;
    ssize_t output_connections;
    int status;
};

//Type flags
#define GATE 1
#define SIGNAL 0
#define UNUSED -9

//Evaluation status flags
#define DONE 2
#define NOT_READY -1


nand_t* nand_new(unsigned n) {
    nand_t* nand = (nand_t *)malloc(sizeof(nand_t));
    if (!nand) {
        errno = ENOMEM;
        return NULL;
    }

    nand_t **gates_in = (nand_t **) malloc(n * sizeof(nand_t *));
    if (!gates_in) {
        free(nand);
        errno = ENOMEM;
        return NULL;
    }

    gate_list* gates_out = (gate_list *) malloc(sizeof (gate_list));
    if (!gates_out) {
        free(gates_in);
        free(nand);
        errno = ENOMEM;
        return NULL;
    }

    bool **input = (bool **) malloc(n * sizeof(bool*));
    if (!input) {
        free(gates_out);
        free(gates_in);
        free(nand);
        errno = ENOMEM;
        return NULL;
    }

    int *type = (int *) malloc(n * sizeof(int));
    if (!type) {
        free(input);
        free(gates_out);
        free(gates_in);
        free(nand);
        errno = ENOMEM;
        return NULL;

    }

    for (unsigned i = 0; i < n; i++) {
        input[i] = false;
        type[i] = UNUSED;
        gates_in[i] = NULL;
    }
    gates_out->begin = NULL;
    nand->gates_in = gates_in;
    nand->gates_out = gates_out;
    nand->input = input;
    nand->depth = 0;
    nand->size = n;
    nand->isEvalating = false;
    nand->type = type;
    nand->output_connections = 0;
    nand->status = NOT_READY;
    return nand;
}

void nand_delete(nand_t *g) {
    if (!g) return;

    output_item * out_iter = g->gates_out->begin;
    output_item* tmp = out_iter;
    while (out_iter) {
       if (out_iter->val) {
           out_iter->val->gates_in[out_iter->port] = NULL;
           out_iter->val->type[out_iter->port] = UNUSED;
           tmp = out_iter;
           out_iter = out_iter->next;
           free(tmp);
       }
    }

    for (size_t i = 0; i < g->size; i++) {
        if (g->type[i] == GATE) {
            nand_t * old_nand = g->gates_in[i];
            if (old_nand) {
                out_iter = old_nand->gates_out->begin;

                if (out_iter->val == g && out_iter->port == i) {
                    old_nand->gates_out->begin = out_iter->next;
                    free(out_iter);
                    old_nand->output_connections--;

                } else {

                    while (out_iter && out_iter->next && (out_iter->next->val != g || out_iter->next->port != i))
                        out_iter = out_iter->next;

                    if (out_iter && out_iter->next && out_iter->next->val == g && out_iter->next->port == i) {
                        output_item *t = out_iter->next;
                        out_iter->next = t->next;
                        free(t);
                        old_nand->output_connections--;
                    }
                }

            }
        }

    }
    free(g->gates_in);
    free(g->gates_out);
    free(g->input);
    free(g->type);
    free(g);
    g = NULL;

}

int nand_connect_nand(nand_t *g_out, nand_t *g_in, unsigned k) {
    if (!g_out || !g_in || k > g_in->size) {
        errno = EINVAL;
        return -1;
    }

    if (g_in->type[k] == GATE) {
        nand_t * old_nand = g_in->gates_in[k];
        if (!old_nand) {
            errno = EINVAL;
            return -1;
        }

        output_item * old_out_iter = old_nand->gates_out->begin;


        if (old_out_iter->val == g_in && old_out_iter->port == k) {
            old_nand->gates_out->begin = old_out_iter->next;
            free(old_out_iter);
            old_nand->output_connections--;

        } else {

            while (old_out_iter && old_out_iter->next
            && (old_out_iter->next->val != g_in || old_out_iter->next->port != k))

                old_out_iter = old_out_iter->next;

            if (old_out_iter && old_out_iter->next && old_out_iter->next->val == g_in
            && old_out_iter->next->port == k) {
                output_item *t = old_out_iter->next;
                old_out_iter->next = t->next;
                free(t);
                old_nand->output_connections--;
            }
        }


    }

    output_item* tmp = (output_item *) malloc(sizeof(output_item));
    if (!tmp)  {
        errno = ENOMEM;
        return -1;
    }
    tmp->val = g_in;
    tmp->port = k;
    tmp->next = g_out->gates_out->begin;
    g_out->gates_out->begin = tmp;
    g_in->type[k] = GATE;
    g_in->input[k] = NULL;
    g_in->gates_in[k] = g_out;
    g_out->output_connections++;
    return 0;

}

int nand_connect_signal(bool const *s, nand_t *g_in, unsigned k) {
    if (!g_in || k > g_in->size) {
        errno = EINVAL;
        return  -1;
    }

    if (g_in->type[k] == GATE) {
        nand_t *old_nand = g_in->gates_in[k];
        if (!old_nand) {
            errno = EINVAL;
            return -1;
        }
        output_item *it = old_nand->gates_out->begin;

        if (it && it->val == g_in && it->port == k) {
            old_nand->gates_out->begin = it->next;
            free(it);
            old_nand->output_connections--;

        } else {

            while (it && it->next && (it->next->val != g_in || it->next->port != k)) it = it->next;

            if (it && it->next && it->next->val == g_in && it->next->port == k) {
                output_item *t = it->next;
                it->next = t->next;
                free(t);
                old_nand->output_connections--;
            }
        }

        g_in->gates_in[k] = NULL;

    }

    g_in->input[k] = (bool *)s;
    g_in->type[k] = SIGNAL;
    return 0;

}

bool evaluate(nand_t * g, status_item** status_ptrList) {
    if (!g) {
        errno = EINVAL;
        return false;
    }

    if (g->isEvalating) {
        errno = ECANCELED;
        return false;
    }

    g->isEvalating = true;
    g->result = false;
    g->depth = 0;

    for (size_t i = 0; i < g->size; i++) {
        int type = g->type[i];

        if (type == UNUSED) {
            errno = ECANCELED;
            g->isEvalating = false;
            return false;
        }

        if (type == GATE) {
            nand_t *gate = g->gates_in[i];
            if (!gate) {
                errno = ECANCELED;
                g->isEvalating = false;
                return false;
            }
            if (gate->status == NOT_READY){
                bool res = evaluate(gate, status_ptrList);
                if (!res) {
                    errno = ECANCELED;
                    g->isEvalating = false;
                    return false;
                }
            }
            if (gate->result == false) g->result = true;
            if (gate->depth > g->depth) g->depth = gate->depth;
        }

        if (type == SIGNAL && *(g->input[i]) == false) g->result = true;
    }

    g->isEvalating = false;
    g->depth ++;

    status_item * tmp = (status_item *) malloc(sizeof (status_item));
    if (!tmp) {
        errno = ENOMEM;
        return false;
    }
    tmp->status = &(g->status);
    tmp->next = (*status_ptrList);
    (*status_ptrList) = tmp;
    g->status = DONE;
    return true;
}

ssize_t nand_evaluate(nand_t **g, bool *s, size_t m) {
    if (m == 0) {
        errno = EINVAL;
        return -1;
    }
    status_item** status_ptrList = (status_item**)malloc(sizeof (status_item*));
    if (!status_ptrList) {
        errno = ENOMEM;
        return -1;
    }
    *status_ptrList = NULL;
    ssize_t max_depth = 0;
    for (size_t j = 0; j < m; j++) {
        nand_t * gate = g[j];
        if (!gate) {
            errno = EINVAL;
            max_depth = -1;
            break;
        }
        bool res = evaluate(gate, status_ptrList);
        if (!res) {
            max_depth = -1;
            break;
        }
        if (gate->depth > max_depth) max_depth = gate->depth;
        s[j] = gate->result;

    }

    status_item* undone_iter = (*status_ptrList);
    while (undone_iter != NULL) {
        if (undone_iter->status)
            *(undone_iter->status) = NOT_READY;
        status_item* tmp = undone_iter;
        undone_iter = undone_iter->next;
        free(tmp);
    }
    free(status_ptrList);

    return max_depth;
}

ssize_t nand_fan_out(nand_t const *g) {
    if (!g) {
        errno = EINVAL;
        return -1;
    }
    return g->output_connections;
}

void* nand_input(nand_t const *g, unsigned k) {
    if (!g || g->size < k) {
        errno = EINVAL;
        return NULL;
    }

    if (g->type[k] == GATE){
        return g->gates_in[k];
    }
    else if (g->type[k] == SIGNAL) {
        return g->input[k];
    }
    else {
        errno = 0;
        return NULL;
    }

}

nand_t* nand_output(nand_t const *g, ssize_t k) {
    if (!g) {
        return NULL;
    }

    ssize_t len = 0;
    output_item * iter = g->gates_out->begin;
    while (iter && len < k) {
        iter = iter->next;
        len++;
    }
    if (!iter || len != k) {
        return NULL;
    }

    return iter->val;

}