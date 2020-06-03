#pragma once

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/tuple/to_seq.hpp>
#include <boost/preprocessor/tuple/pop_front.hpp>
#include <boost/preprocessor/tuple/push_front.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/control/expr_if.hpp>
#include <boost/preprocessor/expand.hpp>
#include <boost/preprocessor/seq/size.hpp>

#include "Macro/boost_pp_for_each.h_dupped.hpp"

#define EVAL(...) EVAL1024(__VA_ARGS__)
#define EVAL1024(...) EVAL512(EVAL512(__VA_ARGS__))
#define EVAL512(...) EVAL256(EVAL256(__VA_ARGS__))
#define EVAL256(...) EVAL128(EVAL128(__VA_ARGS__))
#define EVAL128(...) EVAL64(EVAL64(__VA_ARGS__))
#define EVAL64(...) EVAL32(EVAL32(__VA_ARGS__))
#define EVAL32(...) EVAL16(EVAL16(__VA_ARGS__))
#define EVAL16(...) EVAL8(EVAL8(__VA_ARGS__))
#define EVAL8(...) EVAL4(EVAL4(__VA_ARGS__))
#define EVAL4(...) EVAL2(EVAL2(__VA_ARGS__))
#define EVAL2(...) EVAL1(EVAL1(__VA_ARGS__))
#define EVAL1(...) __VA_ARGS__

#define EMPTY()
#define DEFER(m) m EMPTY()

#define PP_ARG_COUNT_I(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, \
a13, a14, a15, ...) a15
#define PP_ARG_COUNT(...) PP_ARG_COUNT_I(foo, ## __VA_ARGS__, 13, 12, 11, \
10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define class_impl(...) EVAL(__rc(__VA_ARGS__))
#define __impl_for_each(r, data, x) data::x

#define _BOOST_PP_SEQ_FOR_EACH() BOOST_PP_SEQ_FOR_EACH

#define __rc(name, sub, first, ...) DEFER(_BOOST_PP_SEQ_FOR_EACH)()(__sub_for_each, name, sub) first BOOST_PP_SEQ_FOR_EACH(__impl_for_each, name, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))
#define __sub_for_each(r, data, x) __rc BOOST_PP_TUPLE_PUSH_FRONT(BOOST_PP_TUPLE_POP_FRONT(x), data::BOOST_PP_TUPLE_ELEM(0, x))

#define cl(...) ((__VA_ARGS__))

#define sc(name, ...) (name, __VA_ARGS__)
#define lc(name, ...) (name, (), __VA_ARGS__)
#define lc_single(name, ...) (lc(name, __VA_ARGS__))