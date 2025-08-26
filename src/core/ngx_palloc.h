typedef struct ngx_pool_s ngx_pool_t;
ngx_pool_t *ngx_create_pool(size_t size, ngx_log_t *log);
void        ngx_destroy_pool(ngx_pool_t *pool);
void       *ngx_palloc(ngx_pool_t *pool, size_t size);