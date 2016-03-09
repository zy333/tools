extern "C"
{
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}
#include <assert.h>
#include <openssl/md5.h>
#define MAX_QUERYID_LEN 64

static ngx_int_t g_module_ctx_index = -1;
unsigned long g_qid_high;
unsigned long g_qid_low;

struct ngx_http_ezubo_common_loc_conf_t {
    ngx_flag_t ezubo_common_enabled;
};

struct ngx_http_ezubo_common_ctx_t {
    ngx_str_t logid;
};

static void* ngx_http_ezubo_common_create_loc_conf(ngx_conf_t* cf);
static char* ngx_http_ezubo_common_merge_loc_conf(ngx_conf_t* cf, void* parent, void* child);
static ngx_int_t ngx_http_ezubo_common_add_variables(ngx_conf_t* cf);
static ngx_int_t ngx_http_get_variable_ezubo_common_module_ctx(ngx_http_request_t* r,ngx_http_variable_value_t* v, uintptr_t data);
static ngx_int_t ngx_http_ezubo_common_init(ngx_conf_t* cf);
static ngx_int_t ngx_http_ezubo_common_handler(ngx_http_request_t* r);
static ngx_int_t get_logid(ngx_http_request_t* r, ngx_http_ezubo_common_ctx_t* ctx);
static ngx_int_t calculate_own_logid(ngx_http_request_t* r,ngx_http_ezubo_common_ctx_t* ctx);
static ngx_int_t ngx_http_ezubo_common_process_init(ngx_cycle_t* cycle);
int creat_sign_md64(char* psrc,int slen,unsigned int* sign1,unsigned int*sign2);
static ngx_int_t ngx_http_ezubo_common_module_init(ngx_cycle_t* cycle);
static ngx_int_t ngx_http_get_variable_ezubo_common_ctx_param (ngx_http_request_t* r,
        ngx_http_variable_value_t* v, uintptr_t data);

static ngx_command_t ngx_http_ezubo_common_commands[] = {
    {
        ngx_string("ezubo_common_enabled"),
        NGX_HTTP_MAIN_CONF |NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_FLAG,
        ngx_conf_set_flag_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_ezubo_common_loc_conf_t, ezubo_common_enabled),
        NULL
    },
};

static ngx_http_variable_t ngx_http_ezubo_common_vars[] = {
    {
        ngx_string("ezubo_common_module_ctx"), 
        0,
        ngx_http_get_variable_ezubo_common_module_ctx, 0,
        NGX_HTTP_VAR_NOHASH | NGX_HTTP_VAR_INDEXED, 0
    },

    {
        ngx_string("ezubo_common_logid"), //qid
        0,
        ngx_http_get_variable_ezubo_common_ctx_param, offsetof(ngx_http_ezubo_common_ctx_t,logid),
        NGX_HTTP_VAR_NOCACHEABLE, 0
    },

};

/*module context*/
static ngx_http_module_t  ngx_http_ezubo_common_module_ctx = {
    ngx_http_ezubo_common_add_variables,             /* preconfiguration */
    ngx_http_ezubo_common_init,            /* postconfiguration */

    NULL,                              /* create main configuration */
    NULL,                              /* init main configuration */

    NULL,                                /* create server configuration */
    NULL,                              /* merge server configuration */

    ngx_http_ezubo_common_create_loc_conf, /* create location configuration */
    ngx_http_ezubo_common_merge_loc_conf,  /* merge location configuration */
};

ngx_module_t  ngx_http_ezubo_common_module = {
    NGX_MODULE_V1,
    &ngx_http_ezubo_common_module_ctx,      /* module context */
    ngx_http_ezubo_common_commands,         /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    ngx_http_ezubo_common_module_init,    /* init module */
    ngx_http_ezubo_common_process_init,   /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};

static ngx_int_t ngx_http_ezubo_common_module_init(ngx_cycle_t* cycle)
{
        return NGX_OK;
}

static void* ngx_http_ezubo_common_create_loc_conf(ngx_conf_t* cf)
{
    ngx_http_ezubo_common_loc_conf_t*  conf  = (ngx_http_ezubo_common_loc_conf_t*)ngx_pcalloc(cf->pool,sizeof(ngx_http_ezubo_common_loc_conf_t));

    if (conf == NULL) {
        return NULL;
    }

    conf->ezubo_common_enabled = NGX_CONF_UNSET;
    return conf;
}

static char* ngx_http_ezubo_common_merge_loc_conf(ngx_conf_t* cf, void* parent, void* child)
{
    ngx_http_ezubo_common_loc_conf_t* prev = (ngx_http_ezubo_common_loc_conf_t*)parent;
    ngx_http_ezubo_common_loc_conf_t* conf = (ngx_http_ezubo_common_loc_conf_t*)child;
    ngx_conf_merge_value(conf->ezubo_common_enabled, prev->ezubo_common_enabled,0);//默认关闭开关
    return NGX_CONF_OK;
}

static ngx_int_t ngx_http_ezubo_common_add_variables(ngx_conf_t* cf)
{
    ngx_http_variable_t* var = NULL;
    ngx_http_variable_t* v = NULL;

    for (v = ngx_http_ezubo_common_vars; v->name.len; v++) {
        var = ngx_http_add_variable(cf, &v->name, v->flags);

        if (var == NULL) {
            ngx_log_error(NGX_LOG_WARN, cf->log, 0, "add variable %s failed!", v->name.data);
            return NGX_ERROR;
        }

        var->get_handler = v->get_handler;
        var->data = v->data;
    }

    return NGX_OK;
}

static ngx_int_t ngx_http_get_variable_ezubo_common_module_ctx(ngx_http_request_t* r,
        ngx_http_variable_value_t* v, uintptr_t data)
{
    return NGX_OK;
}

static ngx_int_t ngx_http_get_variable_ezubo_common_ctx_param (ngx_http_request_t* r,
        ngx_http_variable_value_t* v, uintptr_t data)
{
    ngx_http_ezubo_common_ctx_t* ctx;
    if(r->internal){
        ctx = (ngx_http_ezubo_common_ctx_t*)(r->variables[g_module_ctx_index].data);
    }else{
        ctx = (ngx_http_ezubo_common_ctx_t*)ngx_http_get_module_ctx(r,ngx_http_ezubo_common_module);
    }

    if (ctx == NULL) {
        return NGX_ERROR;
    }

    ngx_str_t* value = (ngx_str_t*)((char*)ctx + data);
    v->data = value->data;
    v->len = value->len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0, "offset:%d, queryparam %V", (int)data, value);
    return NGX_OK;
}

static ngx_int_t ngx_http_ezubo_common_init(ngx_conf_t* cf)
{
    ngx_http_handler_pt*        h = NULL;
    ngx_http_core_main_conf_t*  cmcf = NULL;
    cmcf = (ngx_http_core_main_conf_t*)ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);
    h = (ngx_http_handler_pt*)ngx_array_push(&cmcf->phases[NGX_HTTP_REWRITE_PHASE].handlers);

    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_ezubo_common_handler;
    ngx_str_t ezubo_common_module_ctx;
    ngx_str_set(&ezubo_common_module_ctx, "ezubo_common_module_ctx");
    g_module_ctx_index = ngx_http_get_variable_index(cf, &ezubo_common_module_ctx);

    return NGX_OK;
}

static ngx_int_t ngx_http_ezubo_common_handler(ngx_http_request_t* r)
{
    ngx_http_ezubo_common_loc_conf_t* lrcf = (ngx_http_ezubo_common_loc_conf_t*)ngx_http_get_module_loc_conf(r,ngx_http_ezubo_common_module);
    if (lrcf->ezubo_common_enabled != 1) {
        ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "r=%d,r->main=%d,ezubo_common_enabled not enabled", r, r->main);
        return NGX_DECLINED;
    }

    ngx_http_ezubo_common_ctx_t* ctx;
    if(r->internal){
        ctx = (ngx_http_ezubo_common_ctx_t*)(r->variables[g_module_ctx_index].data);
    }else{
        ctx = (ngx_http_ezubo_common_ctx_t*)ngx_http_get_module_ctx(r,ngx_http_ezubo_common_module);
    }

    if (NULL == ctx) {
        ctx = (ngx_http_ezubo_common_ctx_t*)ngx_pcalloc(r->pool,
                sizeof(ngx_http_ezubo_common_ctx_t));

        if (ctx == NULL) {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        ngx_http_set_ctx(r, ctx, ngx_http_ezubo_common_module);
        r->variables[g_module_ctx_index].len = sizeof(ctx);
        r->variables[g_module_ctx_index].valid = 1;
        r->variables[g_module_ctx_index].no_cacheable = 1;
        r->variables[g_module_ctx_index].not_found = 0;
        r->variables[g_module_ctx_index].data = (u_char*)ctx;
    } else {
        ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                      "common_handler has done, r:%p,r->main:%p", r, r->main);
        return NGX_DECLINED;
    }
    get_logid(r,ctx);
    return NGX_DECLINED;
}

static ngx_int_t get_logid(ngx_http_request_t* r, ngx_http_ezubo_common_ctx_t* ctx)
{
    unsigned long logid = 0;
    int len = 0;
    char* p = NULL;

    if (ctx == NULL) {
        return NGX_ERROR;
    }

    ngx_str_t var_name;
    ngx_uint_t v_hash_key;
    ngx_http_variable_value_t* var = NULL;
    ngx_str_set(&var_name, "http_logid");
    v_hash_key = ngx_hash_key(var_name.data, var_name.len);
    var = ngx_http_get_variable(r, &var_name, v_hash_key);
    if (var != NULL && var->data != NULL && var->not_found != 1) {
        p = (char*)ngx_pcalloc(r->pool, MAX_QUERYID_LEN);
        len =  var->len < MAX_QUERYID_LEN ? var->len : MAX_QUERYID_LEN - 1;
        ngx_memcpy(p, var->data, len);
        *(p + len) = '\0';
        logid = strtoull(p, NULL, 10);
        len = snprintf(p, MAX_QUERYID_LEN - 1, "0x%lx", logid);
        ctx->logid.data = (u_char*)p;
        ctx->logid.len = len;
    }
    calculate_own_logid(r, ctx);
    return NGX_OK;
}

static ngx_int_t calculate_own_logid(ngx_http_request_t* r,ngx_http_ezubo_common_ctx_t* ctx)
{
    unsigned long logid = 0;
    char* p = NULL;
    int len = 0;
    logid = g_qid_high + (g_qid_low++);
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "%s:%d, r=%d, logid=%d",__FUNCTION__, __LINE__, r, logid);
    p = (char*)ngx_pcalloc(r->pool, MAX_QUERYID_LEN);

    if (p == NULL) {
        ngx_log_error(NGX_LOG_WARN, r->connection->log, 0, "%s:%d, r=%d, malloc error",
                      __FUNCTION__, __LINE__, r);
        return NGX_ERROR;
    }

    len = snprintf(p, MAX_QUERYID_LEN - 1, "0x%lx", logid);
    ctx->logid.data = (u_char*)p;

    if (len > 0) {
        ctx->logid.len = len;
    } else {
        ctx->logid.len = 0;
    }

    p = (char*)ngx_pcalloc(r->pool, MAX_QUERYID_LEN);

    if (p == NULL) {
        ngx_log_error(NGX_LOG_WARN, r->connection->log, 0, "%s:%d, r=%d, malloc error",
                      __FUNCTION__, __LINE__, r);
        return NGX_ERROR;
    }

    return NGX_OK;
}

static ngx_int_t ngx_http_ezubo_common_process_init(ngx_cycle_t* cycle)
{
    //初始化自身计算的logid的高32位
    int len = 0;
    unsigned int sign[2];
    char buffer[1024] = {0};
    time_t now = time(NULL);

    if (gethostname(buffer, sizeof(buffer) - sizeof(now)) != 0) {
        ngx_log_error(NGX_LOG_WARN, cycle->log, 0, "%s:%d, gethostname error",
                      __FUNCTION__, __LINE__);
        return NGX_ERROR;
    }

    len = ngx_strlen(buffer);
    memcpy(buffer + len, &now, sizeof(now));
    len += sizeof(now);
    len += snprintf(buffer + len, 1024 - len, "%d", ngx_pid);
    creat_sign_md64(buffer, len, &sign[0], &sign[1]);
    g_qid_high = (((unsigned long)(sign[0] + sign[1])) | 0x80000000) << 32;
    ngx_log_debug(NGX_LOG_DEBUG_CORE, cycle->log, 0,
                  "%s:%d, sizeof(unsigned long)=%d, sign:%d+%d=%d, g_qid_high=%d",
                  __FUNCTION__, __LINE__, sizeof(unsigned long), sign[0], sign[1], sign[0] + sign[1], g_qid_high);
    //初始化自身计算的logid的低32位
    g_qid_low = 0;
    return NGX_OK;
}

int creat_sign_md64(char* psrc,int slen,unsigned int* sign1,unsigned int*sign2)
{
    unsigned int md5res[4];
    assert(psrc[slen]==0);
    MD5((unsigned char*)psrc,(unsigned int)slen,(unsigned char*)md5res);
    *sign1=md5res[0]+md5res[1];
    *sign2=md5res[2]+md5res[3];
    return 1;
}
