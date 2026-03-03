/**
 * @file    hc_error.h
 * @brief   HC 团队通用错误码定义
 */

#ifndef HC_ERROR_H
#define HC_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HC_ERR_NONE = 0,
    HC_ERR_UNKNOWN = -1,
    HC_ERR_PARAM = -2,
    HC_ERR_NULL_PTR = -3,
    HC_ERR_NOT_SUPPORT = -4,
    HC_ERR_NOT_PERM = -5,
    HC_ERR_NO_MEM = -6,
    HC_ERR_NO_BUF = -7,
    HC_ERR_BUF_EMPTY = -8,
    HC_ERR_BUF_FULL = -9,
    HC_ERR_NOT_READY = -10,
    HC_ERR_BUSY = -11,
    HC_ERR_TIMEOUT = -12,
    HC_ERR_NOT_INIT = -13,
    HC_ERR_ALREADY_INIT = -14,
    HC_ERR_NOT_ENABLE = -15,
    HC_ERR_EXIST = -16,
    HC_ERR_NOT_EXIST = -17,
    HC_ERR_STATE = -18,
    HC_ERR_NACK = -19,
} HC_Error_e;

/* HAL 层统一复用公共错误码，不在各模块头里重复定义。 */
#ifndef HC_HAL_OK
#define HC_HAL_OK HC_ERR_NONE
#endif

#ifndef HC_HAL_ERR_INVALID
#define HC_HAL_ERR_INVALID HC_ERR_PARAM
#endif

#ifndef HC_HAL_ERR_NOT_INIT
#define HC_HAL_ERR_NOT_INIT HC_ERR_NOT_INIT
#endif

#ifndef HC_HAL_ERR_NOT_PERM
#define HC_HAL_ERR_NOT_PERM HC_ERR_NOT_PERM
#endif

#ifndef HC_HAL_ERR_NULL_PTR
#define HC_HAL_ERR_NULL_PTR HC_ERR_NULL_PTR
#endif

#ifndef HC_HAL_ERR_TIMEOUT
#define HC_HAL_ERR_TIMEOUT HC_ERR_TIMEOUT
#endif

#ifndef HC_HAL_ERR_ALREADY_INIT
#define HC_HAL_ERR_ALREADY_INIT HC_ERR_ALREADY_INIT
#endif

#ifndef HC_HAL_ERR_NOT_READY
#define HC_HAL_ERR_NOT_READY HC_ERR_NOT_READY
#endif

#ifndef HC_HAL_ERR_NACK
#define HC_HAL_ERR_NACK HC_ERR_NACK
#endif

/*
 * 入参校验编译开关：开发阶段使能 (=1)，发布阶段可置 0 以消除冗余检查开销。
 * 用法：HC_HAL_ASSERT_PARAM(expr, err_code);
 *   - HC_HAL_PARAM_CHECK=1 时展开为 if (!(expr)) return (err_code);
 *   - HC_HAL_PARAM_CHECK=0 时展开为空语句，零运行时成本。
 */
#ifndef HC_HAL_PARAM_CHECK
#define HC_HAL_PARAM_CHECK 1
#endif

#if HC_HAL_PARAM_CHECK
#define HC_HAL_ASSERT_PARAM(expr, err)                                                                                 \
    do {                                                                                                               \
        if (!(expr)) {                                                                                                 \
            return (err);                                                                                              \
        }                                                                                                              \
    } while (0)
#else
#define HC_HAL_ASSERT_PARAM(expr, err) ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* HC_ERROR_H */
