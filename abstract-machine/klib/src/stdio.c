#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>


/* 内部输出状态 */
struct output {
    char *buf;
    size_t size;      /* 缓冲区剩余可写字节数（含结尾 '\0'） */
    size_t written;   /* 已输出的字符总数（不包含结尾 '\0'） */
};

/* 输出一个字符 */
static void out_char(struct output *o, char c) {
    if (o->size > 1) {          /* 预留一个位置给 '\0' */
        *o->buf++ = c;
        o->size--;
    }
    o->written++;
}

/* 输出长度为 len 的字符串片段 */
static void out_str(struct output *o, const char *s, size_t len) {
    for (size_t i = 0; i < len; i++)
        out_char(o, s[i]);
}

/* 重复输出字符 c 共 count 次 */
static void out_pad(struct output *o, char c, size_t count) {
    for (size_t i = 0; i < count; i++)
        out_char(o, c);
}

/* 将无符号整数转换为字符串，返回长度（不含结尾 '\0'） */
static size_t u64_to_str(uint64_t val, unsigned base, int upper,
                         char *buf, size_t bufsz) {
    static const char digits_lower[] = "0123456789abcdef";
    static const char digits_upper[] = "0123456789ABCDEF";
    const char *digits = upper ? digits_upper : digits_lower;
    char tmp[32];
    size_t pos = 0;

    if (val == 0) {
        tmp[pos++] = '0';
    } else {
        while (val > 0) {
            tmp[pos++] = digits[val % base];
            val /= base;
        }
    }

    /* 反转填入 buf */
    size_t len = pos;
    if (len >= bufsz) len = bufsz - 1;  /* 保证空间 */
    for (size_t i = 0; i < len; i++)
        buf[i] = tmp[len - 1 - i];
    buf[len] = '\0';
    return len;
}

/* 标志位 */
#define FLAG_MINUS  (1 << 0)
#define FLAG_PLUS   (1 << 1)
#define FLAG_SPACE  (1 << 2)
#define FLAG_ZERO   (1 << 3)
#define FLAG_HASH   (1 << 4)


#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
char sbuf[3 * 1024 * 1024];

static inline int __isspace(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}
static inline int __isdigit(int c) {
    return c >= '0' && c <= '9';
}
static inline int __isxdigit(int c) {
    return __isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}


int printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vsprintf(sbuf, fmt, args);
    va_end(args);

    // 将格式化后的字符串逐个字符输出（假设环境中有 putch）
    for (char *p = sbuf; *p; p++) {
        putch(*p);
    }

    free(sbuf);
    return ret;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
    char *start = out;
    while (*fmt) {
        if (*fmt != '%') {
            *out++ = *fmt++;
            continue;
        }
        fmt++; // 跳过 %
        switch (*fmt) {
            case 'd': {
                int num = va_arg(ap, int);
                char buf[32];
                char *p = buf + 31;
                *p = '\0';
                int is_negative = num < 0;
                unsigned int unum = is_negative ? -num : num;
                do {
                    *--p = '0' + (unum % 10);
                    unum /= 10;
                } while (unum > 0);
                if (is_negative) *--p = '-';
                strcpy(out, p);
                out += strlen(p);
                break;
            }
            case 's': {
                char *s = va_arg(ap, char *);
                strcpy(out, s ? s : "(null)");
                out += strlen(out);
                break;
            }
            case 'c': {
                *out++ = (char)va_arg(ap, int);
                break;
            }
            case '%': {
                *out++ = '%';
                break;
            }
            case 'p': {
                void *ptr = va_arg(ap, void *);
                uintptr_t val = (uintptr_t)ptr;
                char buf[32];
                char *p = buf + 31;
                *p = '\0';

                // 转换为十六进制
                if (val == 0) {
                    *--p = '0';
                } else {
                    while (val > 0) {
                        unsigned int digit = val % 16;
                        *--p = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
                        val /= 16;
                    }
                }
                // 添加 0x 前缀
                *--p = 'x';
                *--p = '0';

                strcpy(out, p);
                out += strlen(p);
                break;
            }
            case 'x':{
                unsigned int num = va_arg(ap, unsigned int);
                char buf[32];
                char *p = buf + 31;
                *p = '\0';
                do {
                    unsigned int digit = num % 16;
                    *--p = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
                    num /= 16;
                } while (num > 0);
                strcpy(out, p);
                out += strlen(p);
                break;
            }
            default: {
                char msg[32];
                sprintf(msg, "Not implemented %c", *fmt);   // 如果可用
                panic(msg);
                // panic("Not implemented " + *fmt);
            }
        }
        fmt++;
    }
    *out = '\0';
    return (int)(out - start);
}

int sprintf(char *out, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vsprintf(out, fmt, args);
    va_end(args);
    return ret;
  // panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
    struct output o;
    o.buf = out;
    o.size = n;
    o.written = 0;

    if (n == 0)
        o.buf = NULL;   /* 防止任何写入 */

    while (*fmt) {
        if (*fmt != '%') {
            out_char(&o, *fmt++);
            continue;
        }

        /* 解析 % 开始 */
        fmt++;  /* 跳过 '%' */
        if (*fmt == '\0') break;

        int flags = 0;
        int width = 0;
        int prec = -1;          /* -1 表示未指定精度 */
        int length_mod = 0;     /* 0: 无, 1: l, 2: ll, 3: z */

        /* 标志 */
        while (1) {
            if (*fmt == '-') { flags |= FLAG_MINUS; fmt++; }
            else if (*fmt == '+') { flags |= FLAG_PLUS; fmt++; }
            else if (*fmt == ' ') { flags |= FLAG_SPACE; fmt++; }
            else if (*fmt == '0') { flags |= FLAG_ZERO; fmt++; }
            else if (*fmt == '#') { flags |= FLAG_HASH; fmt++; }
            else break;
        }

        /* 宽度 */
        if (*fmt == '*') {
            width = va_arg(ap, int);
            if (width < 0) {
                flags |= FLAG_MINUS;
                width = -width;
            }
            fmt++;
        } else {
            while (*fmt >= '0' && *fmt <= '9') {
                width = width * 10 + (*fmt - '0');
                fmt++;
            }
        }

        /* 精度 */
        if (*fmt == '.') {
            fmt++;
            if (*fmt == '*') {
                prec = va_arg(ap, int);
                if (prec < 0) prec = -1;
                fmt++;
            } else {
                prec = 0;
                while (*fmt >= '0' && *fmt <= '9') {
                    prec = prec * 10 + (*fmt - '0');
                    fmt++;
                }
            }
        }

        /* 长度修饰符 */
        if (*fmt == 'l') {
            fmt++;
            if (*fmt == 'l') {
                length_mod = 2;
                fmt++;
            } else {
                length_mod = 1;
            }
        } else if (*fmt == 'z') {
            length_mod = 3;
            fmt++;
        }

        /* 转换说明符 */
        char spec = *fmt++;
        if (spec == '\0') break;

        /* 数值临时缓冲区 */
        char num_buf[64];
        const char *str = NULL;
        size_t str_len = 0;
        int is_neg = 0;
        char sign_char = 0;
        uint64_t uval = 0;
        int base = 10;
        int upper = 0;

        switch (spec) {
        case 'd':
        case 'i': {
            /* 有符号整数 */
            int64_t sval = 0;
            if (length_mod == 2)       sval = va_arg(ap, long long);
            else if (length_mod == 1)  sval = va_arg(ap, long);
            else if (length_mod == 3)  sval = va_arg(ap, size_t);
            else                       sval = va_arg(ap, int);

            if (sval < 0) {
                is_neg = 1;
                uval = (uint64_t)(-sval);
            } else {
                uval = (uint64_t)sval;
            }
            base = 10;
            break;
        }
        case 'u':
            if (length_mod == 2)       uval = va_arg(ap, unsigned long long);
            else if (length_mod == 1)  uval = va_arg(ap, unsigned long);
            else if (length_mod == 3)  uval = va_arg(ap, size_t);
            else                       uval = va_arg(ap, unsigned int);
            base = 10;
            break;
        case 'x':
            upper = 0;
            if (length_mod == 2)       uval = va_arg(ap, unsigned long long);
            else if (length_mod == 1)  uval = va_arg(ap, unsigned long);
            else if (length_mod == 3)  uval = va_arg(ap, size_t);
            else                       uval = va_arg(ap, unsigned int);
            base = 16;
            break;
        case 'X':
            upper = 1;
            if (length_mod == 2)       uval = va_arg(ap, unsigned long long);
            else if (length_mod == 1)  uval = va_arg(ap, unsigned long);
            else if (length_mod == 3)  uval = va_arg(ap, size_t);
            else                       uval = va_arg(ap, unsigned int);
            base = 16;
            break;
        case 'p': {
            void *ptr = va_arg(ap, void *);
            if (ptr == NULL) {
                str = "(nil)";
                str_len = 5;
            } else {
                uval = (uintptr_t)ptr;
                base = 16;
                upper = 0;
                /* 指针通常带 0x 前缀，通过 FLAG_HASH 或直接处理 */
                flags |= FLAG_HASH;   /* 强制前缀 */
            }
            break;
        }
        case 's':
            str = va_arg(ap, const char *);
            if (str == NULL) str = "(null)";
            /* 计算字符串长度，考虑精度截断 */
            {
                size_t max_len = (prec >= 0) ? (size_t)prec : (size_t)-1;
                str_len = 0;
                while (str[str_len] && max_len--) str_len++;
            }
            break;
        case 'c': {
            char c = (char)va_arg(ap, int);
            num_buf[0] = c;
            str = num_buf;
            str_len = 1;
            break;
        }
        case '%':
            num_buf[0] = '%';
            str = num_buf;
            str_len = 1;
            break;
        default:
            /* 未知说明符，直接输出原字符序列 */
            num_buf[0] = '%';
            num_buf[1] = spec;
            str = num_buf;
            str_len = 2;
            break;
        }

        /* 如果前面没有设置 str，则从 uval 生成数字字符串 */
        if (str == NULL) {
            u64_to_str(uval, base, upper, num_buf, sizeof(num_buf));
            str = num_buf;
            str_len = 0;
            while (str[str_len]) str_len++;

            /* 处理精度（整数最小数字位数） */
            size_t num_digits = str_len;
            if (prec >= 0 && (size_t)prec > num_digits)
                num_digits = (size_t)prec;
            if (prec == 0 && uval == 0)
                num_digits = 0;   /* 精度为0且值为0时输出空 */

            /* 符号处理 */
            if (is_neg)
                sign_char = '-';
            else if (flags & FLAG_PLUS)
                sign_char = '+';
            else if (flags & FLAG_SPACE)
                sign_char = ' ';

            /* 前缀（仅对八进制和十六进制有效） */
            const char *prefix = "";
            size_t prefix_len = 0;
            if ((flags & FLAG_HASH) && base == 16 && uval != 0) {
                prefix = upper ? "0X" : "0x";
                prefix_len = 2;
            } else if ((flags & FLAG_HASH) && base == 8) {
                /* 八进制前缀，但通常我们不实现 %o，这里留作扩展 */
                if (num_digits > 0 && (uval != 0 || prec == 0)) {
                    prefix = "0";
                    prefix_len = 1;
                }
            }

            /* 计算实际内容总长度 */
            size_t content_len = 0;
            if (num_digits == 0 && sign_char == 0) {
                content_len = prefix_len;
            } else {
                content_len = prefix_len + num_digits + (sign_char ? 1 : 0);
            }

            /* 填充字段 */
            size_t pad = (width > 0 && (size_t)width > content_len)
                         ? (size_t)width - content_len : 0;

            if (!(flags & FLAG_MINUS) && !(flags & FLAG_ZERO))
                out_pad(&o, ' ', pad);

            if (sign_char) out_char(&o, sign_char);
            out_str(&o, prefix, prefix_len);

            if (!(flags & FLAG_MINUS) && (flags & FLAG_ZERO) && prec < 0)
                out_pad(&o, '0', pad);

            /* 前导零以满足精度 */
            if (num_digits > str_len)
                out_pad(&o, '0', num_digits - str_len);

            if (num_digits > 0)
                out_str(&o, str, str_len);

            if (flags & FLAG_MINUS)
                out_pad(&o, ' ', pad);

            continue;   /* 数字处理已完毕，跳过后续通用字符串输出 */
        }

        /* 字符串 / 字符 / 百分号 / 指针 (nil) 的统一输出 */
        {
            size_t content_len = str_len;
            size_t pad = (width > 0 && (size_t)width > content_len)
                         ? (size_t)width - content_len : 0;

            if (!(flags & FLAG_MINUS))
                out_pad(&o, ' ', pad);
            out_str(&o, str, str_len);
            if (flags & FLAG_MINUS)
                out_pad(&o, ' ', pad);
        }
    }

    /* 写入结尾 '\0' */
    if (o.size > 0)
        *o.buf = '\0';
    else if (n > 0 && out != NULL)
        out[n - 1] = '\0';    /* 确保截断的字符串以 '\0' 结尾 */

    return (int)o.written;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = vsnprintf(out, n, fmt, ap);
    va_end(ap);
    return ret;
}


// 为字符串输入设计的内部版本
int vsscanf(const char *str, const char *fmt, va_list ap) {
    // 保存原有标准输入状态，简单实现：直接使用 str 作为“输入流”
    // 用指针模拟 getch / ungetch
    const char *s = str;
    #define STRGETCH()   (*s ? (int)(unsigned char)*s++ : -1)
    #define STRUNGETCH() (s--)

    int count = 0;
    const char *p = fmt;
    int c;

    while (*p) {
        if (*p == '%') {
            p++;
            if (*p == '%') {
                c = STRGETCH();
                if (c != '%') {
                    if (c != -1) STRUNGETCH();
                    return count;
                }
                p++;
                continue;
            }
            if (*p != 'c') {
                while ((c = STRGETCH()) != -1 && __isspace(c));
                if (c != -1) STRUNGETCH();
            }
            switch (*p) {
            case 'd': {
                int num = 0, sign = 1;
                c = STRGETCH();
                if (c == '-') { sign = -1; c = STRGETCH(); }
                else if (c == '+') c = STRGETCH();
                if (!__isdigit(c)) {
                    if (c != -1) STRUNGETCH();
                    return count;
                }
                do {
                    num = num * 10 + (c - '0');
                    c = STRGETCH();
                } while (__isdigit(c));
                if (c != -1) STRUNGETCH();
                *va_arg(ap, int*) = sign * num;
                count++;
                break;
            }
            case 'x': {
                unsigned int num = 0;
                c = STRGETCH();
                if (c == '0') {
                    c = STRGETCH();
                    if (c == 'x' || c == 'X') c = STRGETCH();
                    else { STRUNGETCH(); c = '0'; STRUNGETCH(); }
                }
                if (!__isxdigit(c)) {
                    if (c != -1) STRUNGETCH();
                    return count;
                }
                do {
                    if (c >= '0' && c <= '9') num = num * 16 + (c - '0');
                    else if (c >= 'a' && c <= 'f') num = num * 16 + (c - 'a' + 10);
                    else if (c >= 'A' && c <= 'F') num = num * 16 + (c - 'A' + 10);
                    c = STRGETCH();
                } while (__isxdigit(c));
                if (c != -1) STRUNGETCH();
                *va_arg(ap, unsigned int*) = num;
                count++;
                break;
            }
            case 's': {
                char *str_ptr = va_arg(ap, char*);
                while ((c = STRGETCH()) != -1 && !__isspace(c)) {
                    *str_ptr++ = c;
                }
                *str_ptr = '\0';
                if (c != -1) STRUNGETCH();
                count++;
                break;
            }
            case 'c': {
                char *ch_ptr = va_arg(ap, char*);
                c = STRGETCH();
                if (c == -1) return count;
                *ch_ptr = (char)c;
                count++;
                break;
            }
            case 'p': {
                void **ptr = va_arg(ap, void**);
                __uint64_t val = 0;
                c = STRGETCH();
                if (c == '0') {
                    c = STRGETCH();
                    if (c == 'x' || c == 'X') c = STRGETCH();
                    else { STRUNGETCH(); c = '0'; STRUNGETCH(); }
                }
                if (!__isxdigit(c)) {
                    if (c != -1) STRUNGETCH();
                    return count;
                }
                do {
                    if (c >= '0' && c <= '9') val = val * 16 + (c - '0');
                    else if (c >= 'a' && c <= 'f') val = val * 16 + (c - 'a' + 10);
                    else if (c >= 'A' && c <= 'F') val = val * 16 + (c - 'A' + 10);
                    c = STRGETCH();
                } while (__isxdigit(c));
                if (c != -1) STRUNGETCH();
                *ptr = (void*)val;
                count++;
                break;
            }
            default:
                panic("vsscanf: unsupported format specifier");
            }
            p++;
        } else if (__isspace(*p)) {
            while ((c = STRGETCH()) != -1 && __isspace(c));
            if (c != -1) STRUNGETCH();
            p++;
        } else {
            c = STRGETCH();
            if (c != *p) {
                if (c != -1) STRUNGETCH();
                return count;
            }
            p++;
        }
    }
    #undef STRGETCH
    #undef STRUNGETCH
    return count;
}

int sscanf(const char *str, const char *fmt, ...) {
    // 基于字符串的输入，复用 vsscanf
    va_list ap;
    va_start(ap, fmt);
    int ret = vsscanf(str, fmt, ap);
    va_end(ap);
    return ret;
}

#endif
