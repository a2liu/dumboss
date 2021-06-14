#pragma once

#define NARG(...)                                                              \
  NARG_INTERNAL_PRIVATE(0, ##__VA_ARGS__, 70, 69, 68, 67, 66, 65, 64, 63, 62,  \
                        61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49,    \
                        48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36,    \
                        35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23,    \
                        22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, \
                        8, 7, 6, 5, 4, 3, 2, 1, 0)
#define NARG_INTERNAL_PRIVATE(                                                 \
    _0, _1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_,   \
    _14_, _15_, _16_, _17_, _18_, _19_, _20_, _21_, _22_, _23_, _24_, _25_,    \
    _26_, _27_, _28_, _29_, _30_, _31_, _32_, _33_, _34_, _35_, _36, _37, _38, \
    _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, \
    _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, \
    _69, _70, count, ...)                                                      \
  count

#define PASTE(a, b) a##b
#define XPASTE(a, b) PASTE(a, b)

// function, separator, capture
#define _FOR_EACH0(func, ...)
#define _FOR_EACH1(func, a) func(a)
#define _FOR_EACH2(func, a, b) func(a), func(b)
#define _FOR_EACH3(func, a, b, c) func(a), func(b), func(c)
#define _FOR_EACH4(func, a, b, c, d) func(a), func(b), func(c), func(d)
#define _FOR_EACH5(func, a, b, c, d, e)                                        \
  func(a), func(b), func(c), func(d), func(e)
#define _FOR_EACH6(func, a, b, c, d, e, f)                                     \
  func(a), func(b), func(c), func(d), func(e), func(f)
#define _FOR_EACH7(func, a, b, c, d, e, f, g)                                  \
  func(a), func(b), func(c), func(d), func(e), func(f), func(g)
#define FOR_EACH(func, ...)                                                    \
  XPASTE(_FOR_EACH, NARG(__VA_ARGS__))                                         \
  (func, __VA_ARGS__)