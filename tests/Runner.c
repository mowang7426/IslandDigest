typedef struct napi_env__ *napi_env;
typedef struct napi_value__ *napi_value;
extern int napi_throw_error(napi_env environment, const char *code, const char *message);
extern int IDRunRegression(void);

__attribute__((visibility("default")))
napi_value napi_register_module_v1(napi_env environment, napi_value exports) {
    if (IDRunRegression() != 0) {
        napi_throw_error(environment, "ID_REGRESSION_FAILED", "IslandDigest native regression failed");
        return 0;
    }
    return exports;
}
