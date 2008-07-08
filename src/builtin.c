#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <ohm/ohm-fact.h>
#include <dres/dres.h>
#include "dres-debug.h"

#define BUILTIN_HANDLER(b) \
    static int dres_builtin_##b(dres_t *dres, \
                                char *name, dres_action_t *action, void **ret)

#if 0
BUILTIN_HANDLER(assign);
#endif
BUILTIN_HANDLER(dres);
BUILTIN_HANDLER(resolve);
BUILTIN_HANDLER(echo);
BUILTIN_HANDLER(shell);
BUILTIN_HANDLER(fail);
BUILTIN_HANDLER(unknown);

#define BUILTIN(b) { .name = #b, .handler = dres_builtin_##b }

static dres_handler_t builtins[] = {
#if 0
    { .name = DRES_BUILTIN_ASSIGN, .handler = dres_builtin_assign },
#endif
    BUILTIN(dres),
    BUILTIN(resolve),
    BUILTIN(echo),
    BUILTIN(shell),
    BUILTIN(fail),
    { .name = DRES_BUILTIN_UNKNOWN, .handler = dres_builtin_unknown },
    { .name = NULL, .handler = NULL }
};


/*****************************************************************************
 *                            *** builtin handlers ***                       *
 *****************************************************************************/

/********************
 * dres_fallback_handler
 ********************/
int
dres_fallback_handler(dres_t *dres,
                      int (*handler)(dres_t *,
                                     char *, dres_action_t *, void **))
{
    dres->fallback.name    = "fallback";
    dres->fallback.handler = handler;
    return 0;
}


/********************
 * dres_register_builtins
 ********************/
int
dres_register_builtins(dres_t *dres)
{
    dres_handler_t *h;
    int             status;

    for (h = builtins; h->name; h++)
        if ((status = dres_register_handler(dres, h->name, h->handler)) != 0)
            return status;

    return 0;
}


#if 0
/********************
 * dres_builtin_assign
 ********************/
BUILTIN_HANDLER(assign)
{
    OhmFactStore *store = ohm_fact_store_get_fact_store();
    GSList       *list;
    
    char     *prefix;
    char      rval[64], factname[64];
    OhmFact **facts;
    int       nfact, i;
    
    if (DRES_ID_TYPE(action->lvalue.variable) != DRES_TYPE_FACTVAR || !ret)
        return EINVAL;
    
    if (action->immediate != DRES_ID_NONE) {
        *ret = NULL;
        return 0;              /* handled in action.c:assign_result */
    }
    
    prefix = dres_get_prefix(dres);
    dres_name(dres, action->rvalue.variable, rval, sizeof(rval));
    snprintf(factname, sizeof(factname), "%s%s", prefix, rval + 1);
    
    if ((list = ohm_fact_store_get_facts_by_name(store, factname)) != NULL) {
        nfact = g_slist_length(list);
        if ((facts = ALLOC_ARR(OhmFact *, nfact + 1)) == NULL)
            return ENOMEM;
        for (i = 0; i < nfact && list != NULL; i++, list = g_slist_next(list))
            facts[i] = g_object_ref((OhmFact *)list->data);
        facts[i] = NULL;
    }
    
    *ret = facts;
    return 0;
}
#endif


/********************
 * dres_builtin_dres
 ********************/
BUILTIN_HANDLER(dres)
{
    dres_call_t *call = action->call;
    char         goal[64];
    int          status;
    
    if (action->call->args == NULL)
        return EINVAL;
    
    goal[0] = '\0';
    dres_print_value(dres, &call->args->value, goal, sizeof(goal));
    
    DEBUG(DBG_RESOLVE, "DRES recursing for goal %s", goal);
    depth++;
    dres_scope_push(dres, call->locals);
    status = dres_update_goal(dres, goal, NULL);
    dres_scope_pop(dres);
    depth--;
    DEBUG(DBG_RESOLVE, "DRES back from goal %s", goal);

    *ret = NULL;
    return status;
}


/********************
 * dres_builtin_resolve
 ********************/
BUILTIN_HANDLER(resolve)
{
    return dres_builtin_dres(dres, name, action, ret);
}


/********************
 * dres_builtin_echo
 ********************/
BUILTIN_HANDLER(echo)
{
    dres_call_t  *call = action->call;
    dres_arg_t   *arg;
    dres_value_t *value;
    char          var[128], buf[1024];
    
    
    for (arg = call->args; arg != NULL; arg = arg->next) {
        switch (arg->value.type) {
        case DRES_TYPE_INTEGER:
        case DRES_TYPE_DOUBLE:
        case DRES_TYPE_STRING:
            value = &arg->value;
            break;
        case DRES_TYPE_DRESVAR:
            dres_name(dres, arg->value.v.id, var, sizeof(var));
            value = dres_scope_getvar(dres->scope, var + 1);
            break;
        case DRES_TYPE_FACTVAR:
        default:
            value = NULL;
        }
        
        if (value != NULL) {
            buf[0] = '\0';
            dres_print_value(dres, value, buf, sizeof(buf));
            printf("%s ", buf);
        }
        else
            printf("??? ");
    }

    printf("\n");
    
    if (ret != NULL)
        *ret = NULL;

    return 0;
}


/********************
 * dres_builtin_shell
 ********************/
BUILTIN_HANDLER(shell)
{
    return dres_builtin_unknown(dres, name, action, ret);
}


/********************
 * dres_builtin_fail
 ********************/
BUILTIN_HANDLER(fail)
{
    *ret = NULL;
    return EINVAL;
}


/********************
 * dres_builtin_unknown
 ********************/
BUILTIN_HANDLER(unknown)
{
    if (dres->fallback.handler != NULL)
        return dres->fallback.handler(dres, name, action, ret);
    else {
        if (action == NULL)
            return 0;
    
        DEBUG(DBG_RESOLVE, "unknown action %s", name);
    
        printf("*** unknown action %s", name);
        dres_dump_action(dres, action);
        
        return EINVAL;
    }
}





/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
