#ifndef _MENCI_ENSURE_H
#define _MENCI_ENSURE_H

#include <cassert>
#include <iostream>

#define ENSURE(cond)               (_ensure((cond), __FILE__, __LINE__, #cond))
#define ENSURE_ZERO(expr)          (_ensure((expr) == 0, __FILE__, __LINE__, #expr))
#define ENSURE_ZERO_ERRNO(expr)    (((expr) == 0) ? true : (_ensure_errno(errno, __FILE__, __LINE__, #expr), false))
#define ENSURE_NONZERO(expr)       (_ensure((expr) != 0, __FILE__, __LINE__, #expr))
#define ENSURE_NONZERO_ERRNO(expr) (((expr) != 0) ? true : (_ensure_errno(errno, __FILE__, __LINE__, #expr), false))
#define ENSURE_ERRNO(expr)         ((errno = 0), (expr), (_ensure_errno(errno, __FILE__, __LINE__, #expr)))
#define ERROR(message)             (_error(message, __FILE__, __LINE__))

void _ensure(bool success, const char *file, int line, const char *expr);
void _ensure_errno(int err, const char *file, int line, const char *expr);
void _error(const char *message, const char *file, int line);

#endif // _MENCI_ENSURE_H
