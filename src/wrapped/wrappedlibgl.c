#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <dlfcn.h>

#include "wrappedlibs.h"

#include "debug.h"
#include "wrapper.h"
#include "bridge.h"
#include "librarian/library_private.h"
#include "x64emu.h"
#include "emu/x64emu_private.h"
#include "box64context.h"
#include "librarian.h"
#include "callback.h"
#include "gltools.h"

const char* libglName = "libGL.so.1";
#define LIBNAME libgl
static library_t* my_lib = NULL;

// FIXME: old wrapped* type of file, cannot use generated/wrappedlibgltypes.h

EXPORT void* my_glXGetProcAddress(x64emu_t* emu, void* name) 
{
    khint_t k;
    const char* rname = (const char*)name;
    return getGLProcAddress(emu, my_lib->w.priv, rname);
}
EXPORT void* my_glXGetProcAddressARB(x64emu_t* emu, void* name) __attribute__((alias("my_glXGetProcAddress")));

typedef int  (*iFi_t)(int);
typedef void (*vFpp_t)(void*, void*);
typedef void*(*pFp_t)(void*);
typedef void (*debugProc_t)(int32_t, int32_t, uint32_t, int32_t, int32_t, void*, void*);

typedef struct gl_wrappers_s {
    glprocaddress_t      procaddress;
    kh_symbolmap_t      *glwrappers;    // the map of wrapper for glProcs (for GLX or SDL1/2)
    kh_symbolmap_t      *glmymap;       // link to the mysymbolmap of libGL
} gl_wrappers_t;

KHASH_MAP_INIT_INT64(gl_wrappers, gl_wrappers_t*)

static kh_gl_wrappers_t *gl_wrappers = NULL;

#define SUPER() \
GO(0)   \
GO(1)   \
GO(2)   \
GO(3)   \
GO(4)

// debug_callback ...
#define GO(A)   \
static uintptr_t my_debug_callback_fct_##A = 0;                                                                         \
static void my_debug_callback_##A(int32_t a, int32_t b, uint32_t c, int32_t d, int32_t e, const char* f, const void* g) \
{                                                                                                                       \
    RunFunction(my_context, my_debug_callback_fct_##A, 7, a, b, c, d, e, f, g);                                         \
}
SUPER()
#undef GO
static void* find_debug_callback_Fct(void* fct)
{
    if(!fct) return fct;
    if(GetNativeFnc((uintptr_t)fct))  return GetNativeFnc((uintptr_t)fct);
    #define GO(A) if(my_debug_callback_fct_##A == (uintptr_t)fct) return my_debug_callback_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_debug_callback_fct_##A == 0) {my_debug_callback_fct_##A = (uintptr_t)fct; return my_debug_callback_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for libGL debug_callback callback\n");
    return NULL;
}
// program_callback ...
#define GO(A)                                                       \
static uintptr_t my_program_callback_fct_##A = 0;                   \
static void my_program_callback_##A(int32_t a, void* b)             \
{                                                                   \
    RunFunction(my_context, my_program_callback_fct_##A, 2, a, b);  \
}
SUPER()
#undef GO
static void* find_program_callback_Fct(void* fct)
{
    if(!fct) return fct;
    if(GetNativeFnc((uintptr_t)fct))  return GetNativeFnc((uintptr_t)fct);
    #define GO(A) if(my_program_callback_fct_##A == (uintptr_t)fct) return my_program_callback_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_program_callback_fct_##A == 0) {my_program_callback_fct_##A = (uintptr_t)fct; return my_program_callback_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for libGL program_callback callback\n");
    return NULL;
}

// glDebugMessageCallback ...
#define GO(A)                                                                       \
static vFpp_t my_glDebugMessageCallback_fct_##A = NULL;                             \
static void my_glDebugMessageCallback_##A(x64emu_t* emu, void* prod, void* param)   \
{                                                                                   \
    if(!my_glDebugMessageCallback_fct_##A)                                          \
        return;                                                                     \
    my_glDebugMessageCallback_fct_##A(find_debug_callback_Fct(prod), param);        \
}
SUPER()
#undef GO
static void* find_glDebugMessageCallback_Fct(void* fct)
{
    if(!fct) return fct;
    #define GO(A) if(my_glDebugMessageCallback_fct_##A == (vFpp_t)fct) return my_glDebugMessageCallback_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_glDebugMessageCallback_fct_##A == 0) {my_glDebugMessageCallback_fct_##A = (vFpp_t)fct; return my_glDebugMessageCallback_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for libGL glDebugMessageCallback callback\n");
    return NULL;
}
// glDebugMessageCallbackARB ...
#define GO(A)                                                                       \
static vFpp_t my_glDebugMessageCallbackARB_fct_##A = NULL;                             \
static void my_glDebugMessageCallbackARB_##A(x64emu_t* emu, void* prod, void* param)   \
{                                                                                   \
    if(!my_glDebugMessageCallbackARB_fct_##A)                                          \
        return;                                                                     \
    my_glDebugMessageCallbackARB_fct_##A(find_debug_callback_Fct(prod), param);        \
}
SUPER()
#undef GO
static void* find_glDebugMessageCallbackARB_Fct(void* fct)
{
    if(!fct) return fct;
    #define GO(A) if(my_glDebugMessageCallbackARB_fct_##A == (vFpp_t)fct) return my_glDebugMessageCallbackARB_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_glDebugMessageCallbackARB_fct_##A == 0) {my_glDebugMessageCallbackARB_fct_##A = (vFpp_t)fct; return my_glDebugMessageCallbackARB_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for libGL glDebugMessageCallbackARB callback\n");
    return NULL;
}
// glDebugMessageCallbackAMD ...
#define GO(A)                                                                       \
static vFpp_t my_glDebugMessageCallbackAMD_fct_##A = NULL;                             \
static void my_glDebugMessageCallbackAMD_##A(x64emu_t* emu, void* prod, void* param)   \
{                                                                                   \
    if(!my_glDebugMessageCallbackAMD_fct_##A)                                          \
        return;                                                                     \
    my_glDebugMessageCallbackAMD_fct_##A(find_debug_callback_Fct(prod), param);        \
}
SUPER()
#undef GO
static void* find_glDebugMessageCallbackAMD_Fct(void* fct)
{
    if(!fct) return fct;
    #define GO(A) if(my_glDebugMessageCallbackAMD_fct_##A == (vFpp_t)fct) return my_glDebugMessageCallbackAMD_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_glDebugMessageCallbackAMD_fct_##A == 0) {my_glDebugMessageCallbackAMD_fct_##A = (vFpp_t)fct; return my_glDebugMessageCallbackAMD_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for libGL glDebugMessageCallbackAMD callback\n");
    return NULL;
}
// glDebugMessageCallbackKHR ...
#define GO(A)                                                                       \
static vFpp_t my_glDebugMessageCallbackKHR_fct_##A = NULL;                             \
static void my_glDebugMessageCallbackKHR_##A(x64emu_t* emu, void* prod, void* param)   \
{                                                                                   \
    if(!my_glDebugMessageCallbackKHR_fct_##A)                                          \
        return;                                                                     \
    my_glDebugMessageCallbackKHR_fct_##A(find_debug_callback_Fct(prod), param);        \
}
SUPER()
#undef GO
static void* find_glDebugMessageCallbackKHR_Fct(void* fct)
{
    if(!fct) return fct;
    #define GO(A) if(my_glDebugMessageCallbackKHR_fct_##A == (vFpp_t)fct) return my_glDebugMessageCallbackKHR_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_glDebugMessageCallbackKHR_fct_##A == 0) {my_glDebugMessageCallbackKHR_fct_##A = (vFpp_t)fct; return my_glDebugMessageCallbackKHR_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for libGL glDebugMessageCallbackKHR callback\n");
    return NULL;
}
// glXSwapIntervalMESA ...
#define GO(A)                                           \
static iFi_t my_glXSwapIntervalMESA_fct_##A = NULL;     \
static int my_glXSwapIntervalMESA_##A(int interval)     \
{                                                       \
    if(!my_glXSwapIntervalMESA_fct_##A)                 \
        return 0;                                       \
    return my_glXSwapIntervalMESA_fct_##A(interval);    \
}
SUPER()
#undef GO
static void* find_glXSwapIntervalMESA_Fct(void* fct)
{
    if(!fct) return fct;
    #define GO(A) if(my_glXSwapIntervalMESA_fct_##A == (iFi_t)fct) return my_glXSwapIntervalMESA_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_glXSwapIntervalMESA_fct_##A == 0) {my_glXSwapIntervalMESA_fct_##A = (iFi_t)fct; return my_glXSwapIntervalMESA_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for libGL glXSwapIntervalMESA callback\n");
    return NULL;
}
// glProgramCallbackMESA ...
#define GO(A)                                                               \
static vFpp_t my_glProgramCallbackMESA_fct_##A = NULL;                      \
static void my_glProgramCallbackMESA_##A(x64emu_t* emu, void* f, void* data)\
{                                                                           \
    if(!my_glProgramCallbackMESA_fct_##A)                                   \
        return;                                                             \
    my_glProgramCallbackMESA_fct_##A(find_program_callback_Fct(f), data);   \
}
SUPER()
#undef GO
static void* find_glProgramCallbackMESA_Fct(void* fct)
{
    if(!fct) return fct;
    #define GO(A) if(my_glProgramCallbackMESA_fct_##A == (vFpp_t)fct) return my_glProgramCallbackMESA_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_glProgramCallbackMESA_fct_##A == 0) {my_glProgramCallbackMESA_fct_##A = (vFpp_t)fct; return my_glProgramCallbackMESA_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for libGL glProgramCallbackMESA callback\n");
    return NULL;
}
void* my_GetVkProcAddr(x64emu_t* emu, void* name, void*(*getaddr)(void*));  // defined in wrappedvulkan.c
// glGetVkProcAddrNV ...
#define GO(A)                                                           \
static pFp_t my_glGetVkProcAddrNV_fct_##A = NULL;                       \
static void* my_glGetVkProcAddrNV_##A(x64emu_t* emu, void* name)        \
{                                                                       \
    if(!my_glGetVkProcAddrNV_fct_##A)                                   \
        return NULL;                                                    \
    return my_GetVkProcAddr(emu, name, my_glGetVkProcAddrNV_fct_##A);   \
}
SUPER()
#undef GO
static void* find_glGetVkProcAddrNV_Fct(void* fct)
{
    if(!fct) return fct;
    #define GO(A) if(my_glGetVkProcAddrNV_fct_##A == (pFp_t)fct) return my_glGetVkProcAddrNV_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_glGetVkProcAddrNV_fct_##A == 0) {my_glGetVkProcAddrNV_fct_##A = (pFp_t)fct; return my_glGetVkProcAddrNV_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for libGL glGetVkProcAddrNV callback\n");
    return NULL;
}
#undef SUPER

#define PRE_INIT if(box64_libGL) {lib->w.lib = dlopen(box64_libGL, RTLD_LAZY | RTLD_GLOBAL); lib->path = strdup(box64_libGL);} else
#define CUSTOM_INIT \
    my_lib = lib;   \
    lib->w.priv = dlsym(lib->w.lib, "glXGetProcAddress"); \

#include "wrappedlib_init.h"

#define SUPER()                         \
 GO(vFpp_t, glDebugMessageCallback)     \
 GO(vFpp_t, glDebugMessageCallbackARB)  \
 GO(vFpp_t, glDebugMessageCallbackAMD)  \
 GO(vFpp_t, glDebugMessageCallbackKHR)  \
 GO(iFi_t, glXSwapIntervalMESA)         \
 GO(vFpp_t, glProgramCallbackMESA)      \
 GO(pFp_t, glGetVkProcAddrNV)           \


gl_wrappers_t* getGLProcWrapper(box64context_t* context, glprocaddress_t procaddress)
{
    int cnt, ret;
    khint_t k;
    if(!gl_wrappers) {
        gl_wrappers = kh_init(gl_wrappers);
    }
    k = kh_put(gl_wrappers, gl_wrappers, (uintptr_t)procaddress, &ret);
    if(!ret)
        return kh_value(gl_wrappers, k);
    gl_wrappers_t* wrappers = kh_value(gl_wrappers, k) = (gl_wrappers_t*)calloc(1, sizeof(gl_wrappers_t));

    wrappers->procaddress = procaddress;
    wrappers->glwrappers = kh_init(symbolmap);
    // populates maps...
    cnt = sizeof(libglsymbolmap)/sizeof(map_onesymbol_t);
    for (int i=0; i<cnt; ++i) {
        k = kh_put(symbolmap, wrappers->glwrappers, libglsymbolmap[i].name, &ret);
        kh_value(wrappers->glwrappers, k) = libglsymbolmap[i].w;
    }
    // and the my_ symbols map
    cnt = sizeof(MAPNAME(mysymbolmap))/sizeof(map_onesymbol_t);
    for (int i=0; i<cnt; ++i) {
        k = kh_put(symbolmap, wrappers->glwrappers, libglmysymbolmap[i].name, &ret);
        kh_value(wrappers->glwrappers, k) = libglmysymbolmap[i].w;
    }
    // my_* map
    wrappers->glmymap = kh_init(symbolmap);
    cnt = sizeof(MAPNAME(mysymbolmap))/sizeof(map_onesymbol_t);
    for (int i=0; i<cnt; ++i) {
        k = kh_put(symbolmap, wrappers->glmymap, libglmysymbolmap[i].name, &ret);
        kh_value(wrappers->glmymap, k) = libglmysymbolmap[i].w;
    }
    return wrappers;
}
void freeGLProcWrapper(box64context_t* context)
{
    if(!context)
        return;
    if(!gl_wrappers)
        return;
    gl_wrappers_t* wrappers;
    kh_foreach_value(gl_wrappers, wrappers,
        if(wrappers->glwrappers)
            kh_destroy(symbolmap, wrappers->glwrappers);
        if(wrappers->glmymap)
            kh_destroy(symbolmap, wrappers->glmymap);
        wrappers->glwrappers = NULL;
        wrappers->glmymap = NULL;
    );
    kh_destroy(gl_wrappers, gl_wrappers);
    gl_wrappers = NULL;
}

void* getGLProcAddress(x64emu_t* emu, glprocaddress_t procaddr, const char* rname)
{
    khint_t k;
    printf_dlsym(LOG_DEBUG, "Calling getGLProcAddress[%p](\"%s\") => ", procaddr, rname);
    gl_wrappers_t* wrappers = getGLProcWrapper(emu->context, procaddr);
    // check if glxprocaddress is filled, and search for lib and fill it if needed
    // get proc adress using actual glXGetProcAddress
    k = kh_get(symbolmap, wrappers->glmymap, rname);
    int is_my = (k==kh_end(wrappers->glmymap))?0:1;
    void* symbol;
    if(is_my) {
        // try again, by using custom "my_" now...
        #define GO(A, B) else if(!strcmp(rname, #B)) symbol = find_##B##_Fct(procaddr(rname));
        if(0) {}
        SUPER()
        else {
            if(strcmp(rname, "glXGetProcAddress") && strcmp(rname, "glXGetProcAddressARB")) {
                printf_log(LOG_NONE, "Warning, %s defined as GOM, but find_%s_Fct not defined\n", rname, rname);
            }
            char tmp[200];
            strcpy(tmp, "my_");
            strcat(tmp, rname);
            symbol = dlsym(emu->context->box64lib, tmp);
        }
        #undef GO
        #undef SUPER
    } else 
        symbol = procaddr(rname);
    if(!symbol) {
        printf_dlsym(LOG_DEBUG, "%p\n", NULL);
        return NULL;    // easy
    }
    // check if alread bridged
    uintptr_t ret = CheckBridged(emu->context->system, symbol);
    if(ret) {
        printf_dlsym(LOG_DEBUG, "%p\n", (void*)ret);
        return (void*)ret; // already bridged
    }
    // get wrapper    
    k = kh_get(symbolmap, wrappers->glwrappers, rname);
    if(k==kh_end(wrappers->glwrappers) && strstr(rname, "ARB")==NULL) {
        // try again, adding ARB at the end if not present
        char tmp[200];
        strcpy(tmp, rname);
        strcat(tmp, "ARB");
        k = kh_get(symbolmap, wrappers->glwrappers, tmp);
    }
    if(k==kh_end(wrappers->glwrappers) && strstr(rname, "EXT")==NULL) {
        // try again, adding EXT at the end if not present
        char tmp[200];
        strcpy(tmp, rname);
        strcat(tmp, "EXT");
        k = kh_get(symbolmap, wrappers->glwrappers, tmp);
    }
    if(k==kh_end(wrappers->glwrappers)) {
        printf_dlsym(LOG_DEBUG, "%p\n", NULL);
        printf_dlsym(LOG_INFO, "Warning, no wrapper for %s\n", rname);
        return NULL;
    }
    const char* constname = kh_key(wrappers->glwrappers, k);
    AddOffsetSymbol(emu->context->maplib, symbol, rname);
    ret = AddBridge(emu->context->system, kh_value(wrappers->glwrappers, k), symbol, 0, constname);
    printf_dlsym(LOG_DEBUG, "%p\n", (void*)ret);
    return (void*)ret;
}