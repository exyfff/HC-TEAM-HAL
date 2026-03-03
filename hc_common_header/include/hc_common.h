/**
 * @file    hc_common.h
 * @brief   HC 团队公共头文件统一入口
 */

#ifndef HC_COMMON_H
#define HC_COMMON_H

/*
 * 通用标准头文件：
 * 1. string.h  : 常用字符串与内存操作
 * 2. stdlib.h  : 常用转换、分配与工具函数
 * 3. stdarg.h  : 可变参数封装支持
 * 4. limits.h  : 基础类型范围边界
 */
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>

#include "hc_type.h"
#include "hc_error.h"
#include "hc_macro.h"

#endif /* HC_COMMON_H */
