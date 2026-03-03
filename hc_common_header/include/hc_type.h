/**
 * @file    hc_type.h
 * @brief   HC 团队基础类型定义
 */

#ifndef HC_TYPE_H
#define HC_TYPE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t  HC_U8;
typedef uint16_t HC_U16;
typedef uint32_t HC_U32;
typedef uint64_t HC_U64;

typedef int8_t  HC_S8;
typedef int16_t HC_S16;
typedef int32_t HC_S32;
typedef int64_t HC_S64;

/* 这些别名保留在基础层，默认不建议在新的 HAL 公共接口中直接暴露。 */
typedef float  HC_FLOAT;
typedef double HC_DOUBLE;
typedef char   HC_CHAR;

/* HC_VOID 是当前仓库约定的函数签名风格。 */
#define HC_VOID void

typedef enum {
    HC_FALSE = 0,
    HC_TRUE  = 1,
} HC_Bool_e;

typedef enum {
    HC_DISABLE = 0,
    HC_ENABLE  = 1,
} HC_Enable_e;

#define HC_NULL     (0L)
#define HC_NULL_PTR ((HC_VOID *)0)
#define HC_NULL_FN  (0)            /* 函数指针 null 判定专用，保持为标准空指针常量 */

/* HC_LOCAL 统一标记模块内私有符号。 */
#define HC_LOCAL static

/* HC_WEAK 统一封装弱符号，避免业务代码直接写编译器专用属性。 */
#if defined(__GNUC__) || defined(__clang__)
#define HC_WEAK __attribute__((weak))
#elif defined(__ICCARM__)
#define HC_WEAK __weak
#elif defined(__ARMCC_VERSION)
#define HC_WEAK __attribute__((weak))
#else
#define HC_WEAK
#endif

/* 用于显式消除“未使用变量”告警。 */
#define HC_UNUSED(x) ((void)(x))

/* 工程返回值统一使用 hc_error.h 中的 HC_Error_e / HC_ERR_xxx。 */

#ifdef __cplusplus
}
#endif

#endif /* HC_TYPE_H */
