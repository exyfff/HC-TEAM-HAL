/**
 * @file    hc_macro.h
 * @brief   HC 团队通用工具宏
 */

#ifndef HC_MACRO_H
#define HC_MACRO_H

#include <string.h>
#include "hc_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HC_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/* 本头自行承担 memset/memcpy 依赖，不依赖 hc_type.h 的传递包含。 */

/*
 * 为了保持可移植性，HC_MAX/HC_MIN 仍然使用简单宏。
 * 调用时不要传入带副作用的表达式，例如 i++、函数调用返回自增对象等。
 */
#ifndef HC_MAX
#define HC_MAX(a, b) (((a) < (b)) ? (b) : (a))
#endif

#ifndef HC_MIN
#define HC_MIN(a, b) (((a) > (b)) ? (b) : (a))
#endif

/*
 * HC_SWAP 统一改为“临时变量交换”。
 * 这样可以避免旧版算术交换在有符号整型上的溢出未定义行为。
 */
#ifndef HC_SWAP
#if defined(__GNUC__) || defined(__clang__)
#define HC_SWAP(a, b)                                                                                                  \
    do {                                                                                                               \
        __typeof__(a) _hc_tmp = (a);                                                                                   \
        (a) = (b);                                                                                                     \
        (b) = _hc_tmp;                                                                                                 \
    } while (0)
#else
#define HC_SWAP(a, b)                                                                                                  \
    do {                                                                                                               \
        char _hc_size_check[(sizeof(a) == sizeof(b)) ? 1 : -1];                                                        \
        HC_U8 _hc_tmp[sizeof(a)];                                                                                      \
        HC_UNUSED(_hc_size_check);                                                                                     \
        memcpy(_hc_tmp, &(a), sizeof(a));                                                                              \
        memcpy(&(a), &(b), sizeof(a));                                                                                 \
        memcpy(&(b), _hc_tmp, sizeof(a));                                                                              \
    } while (0)
#endif
#endif

/* 不可传入带副作用的表达式（如 i++、func()），参数最多展开两次。 */
#define HC_CLAMP(x, lo, hi) (((x) < (lo)) ? (lo) : (((x) > (hi)) ? (hi) : (x)))

#ifndef HC_ALIGN_UP
#define HC_ALIGN_UP(x, a) (((a) != 0) ? ((((x) + ((a) - 1)) / (a)) * (a)) : (x))
#endif

#ifndef HC_ALIGN_DOWN
#define HC_ALIGN_DOWN(x, a) (((a) != 0) ? (((x) / (a)) * (a)) : (x))
#endif

/* 位号越界时返回 0，避免移位未定义行为。 */
#define HC_BIT(n)   (((n) < 32u) ? ((HC_U32)1u << (n)) : 0u)
#define HC_BIT64(n) (((n) < 64u) ? ((HC_U64)1u << (n)) : 0u)

HC_LOCAL inline HC_VOID HC_BitSet(HC_U32 *pData, HC_U8 bit)
{
    /* 同时保护空指针和无效位号，保持工具函数调用成本低。 */
    if ((pData == HC_NULL_PTR) || (bit >= 32u)) {
        return;
    }

    *pData |= (HC_U32)1u << bit;
}

HC_LOCAL inline HC_VOID HC_BitClear(HC_U32 *pData, HC_U8 bit)
{
    if ((pData == HC_NULL_PTR) || (bit >= 32u)) {
        return;
    }

    *pData &= ~((HC_U32)1u << bit);
}

HC_LOCAL inline HC_Bool_e HC_BitIsSet(const HC_U32 *pData, HC_U8 bit)
{
    if ((pData == HC_NULL_PTR) || (bit >= 32u)) {
        return HC_FALSE;
    }

    return ((*pData & ((HC_U32)1u << bit)) != 0u) ? HC_TRUE : HC_FALSE;
}

#define HC_BZERO(x) memset(&(x), 0, sizeof(x))

#define HC_MAKE_VERSION(major, minor, patch, build)                                                                    \
    ((HC_U32)((((major) & 0xFFu) << 24) | (((minor) & 0xFFu) << 16) | (((patch) & 0xFFu) << 8) | (((build) & 0xFFu))))

#define HC_DIV_SAFE(a) (((a) == 0) ? 1 : (a))

#ifdef __cplusplus
}
#endif

#endif /* HC_MACRO_H */
