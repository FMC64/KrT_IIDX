# /* **************************************************************************
#  *                                                                          *
#  *     (C) Copyright Paul Mensonides 2002.
#  *     Distributed under the Boost Software License, Version 1.0. (See
#  *     accompanying file LICENSE_1_0.txt or copy at
#  *     http://www.boost.org/LICENSE_1_0.txt)
#  *                                                                          *
#  ************************************************************************** */
#
# /* See http://www.boost.org for most recent version. */
#
#pragma once
#
# include <boost/preprocessor/arithmetic/dec.hpp>
# include <boost/preprocessor/config/config.hpp>
# include <boost/preprocessor/control/if.hpp>
# include <boost/preprocessor/control/iif.hpp>
# include <boost/preprocessor/repetition/for.hpp>
# include <boost/preprocessor/seq/seq.hpp>
# include <boost/preprocessor/seq/size.hpp>
# include <boost/preprocessor/seq/detail/is_empty.hpp>
# include <boost/preprocessor/tuple/elem.hpp>
# include <boost/preprocessor/tuple/rem.hpp>
#
# /* BOOST_PP_SEQ_FOR_EACH159 */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_SEQ_FOR_EACH159(macro, data, seq) BOOST_PP_SEQ_FOR_EACH159_DETAIL_CHECK(macro, data, seq)
# else
#    define BOOST_PP_SEQ_FOR_EACH159(macro, data, seq) BOOST_PP_SEQ_FOR_EACH159_D(macro, data, seq)
#    define BOOST_PP_SEQ_FOR_EACH159_D(macro, data, seq) BOOST_PP_SEQ_FOR_EACH159_DETAIL_CHECK(macro, data, seq)
# endif
#
#    define BOOST_PP_SEQ_FOR_EACH159_DETAIL_CHECK_EXEC(macro, data, seq) BOOST_PP_FOR((macro, data, seq, BOOST_PP_SEQ_SIZE(seq)), BOOST_PP_SEQ_FOR_EACH159_P, BOOST_PP_SEQ_FOR_EACH159_O, BOOST_PP_SEQ_FOR_EACH159_M)
#    define BOOST_PP_SEQ_FOR_EACH159_DETAIL_CHECK_EMPTY(macro, data, seq)
#
#    define BOOST_PP_SEQ_FOR_EACH159_DETAIL_CHECK(macro, data, seq) \
        BOOST_PP_IIF \
            ( \
            BOOST_PP_SEQ_DETAIL_IS_NOT_EMPTY(seq), \
            BOOST_PP_SEQ_FOR_EACH159_DETAIL_CHECK_EXEC, \
            BOOST_PP_SEQ_FOR_EACH159_DETAIL_CHECK_EMPTY \
            ) \
        (macro, data, seq) \
/**/
#
# define BOOST_PP_SEQ_FOR_EACH159_P(r, x) BOOST_PP_TUPLE_ELEM(4, 3, x)
#
# if BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_STRICT()
#    define BOOST_PP_SEQ_FOR_EACH159_O(r, x) BOOST_PP_SEQ_FOR_EACH159_O_I x
# else
#    define BOOST_PP_SEQ_FOR_EACH159_O(r, x) BOOST_PP_SEQ_FOR_EACH159_O_I(BOOST_PP_TUPLE_ELEM(4, 0, x), BOOST_PP_TUPLE_ELEM(4, 1, x), BOOST_PP_TUPLE_ELEM(4, 2, x), BOOST_PP_TUPLE_ELEM(4, 3, x))
# endif
#
# define BOOST_PP_SEQ_FOR_EACH159_O_I(macro, data, seq, sz) \
    BOOST_PP_SEQ_FOR_EACH159_O_I_DEC(macro, data, seq, BOOST_PP_DEC(sz)) \
/**/
# define BOOST_PP_SEQ_FOR_EACH159_O_I_DEC(macro, data, seq, sz) \
    ( \
    macro, \
    data, \
    BOOST_PP_IF \
        ( \
        sz, \
        BOOST_PP_SEQ_FOR_EACH159_O_I_TAIL, \
        BOOST_PP_SEQ_FOR_EACH159_O_I_NIL \
        ) \
    (seq), \
    sz \
    ) \
/**/
# define BOOST_PP_SEQ_FOR_EACH159_O_I_TAIL(seq) BOOST_PP_SEQ_TAIL(seq)
# define BOOST_PP_SEQ_FOR_EACH159_O_I_NIL(seq) BOOST_PP_NIL
#
# if BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_STRICT()
#    define BOOST_PP_SEQ_FOR_EACH159_M(r, x) BOOST_PP_SEQ_FOR_EACH159_M_IM(r, BOOST_PP_TUPLE_REM_4 x)
#    define BOOST_PP_SEQ_FOR_EACH159_M_IM(r, im) BOOST_PP_SEQ_FOR_EACH159_M_I(r, im)
# else
#    define BOOST_PP_SEQ_FOR_EACH159_M(r, x) BOOST_PP_SEQ_FOR_EACH159_M_I(r, BOOST_PP_TUPLE_ELEM(4, 0, x), BOOST_PP_TUPLE_ELEM(4, 1, x), BOOST_PP_TUPLE_ELEM(4, 2, x), BOOST_PP_TUPLE_ELEM(4, 3, x))
# endif
#
# define BOOST_PP_SEQ_FOR_EACH159_M_I(r, macro, data, seq, sz) macro(r, data, BOOST_PP_SEQ_HEAD(seq))
#
# /* BOOST_PP_SEQ_FOR_EACH159_R */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_SEQ_FOR_EACH159_R(r, macro, data, seq) BOOST_PP_SEQ_FOR_EACH159_DETAIL_CHECK_R(r, macro, data, seq)
# else
#    define BOOST_PP_SEQ_FOR_EACH159_R(r, macro, data, seq) BOOST_PP_SEQ_FOR_EACH159_R_I(r, macro, data, seq)
#    define BOOST_PP_SEQ_FOR_EACH159_R_I(r, macro, data, seq) BOOST_PP_SEQ_FOR_EACH159_DETAIL_CHECK_R(r, macro, data, seq)
# endif
#
#    define BOOST_PP_SEQ_FOR_EACH159_DETAIL_CHECK_EXEC_R(r, macro, data, seq) BOOST_PP_FOR_ ## r((macro, data, seq, BOOST_PP_SEQ_SIZE(seq)), BOOST_PP_SEQ_FOR_EACH159_P, BOOST_PP_SEQ_FOR_EACH159_O, BOOST_PP_SEQ_FOR_EACH159_M)
#    define BOOST_PP_SEQ_FOR_EACH159_DETAIL_CHECK_EMPTY_R(r, macro, data, seq)
#
#    define BOOST_PP_SEQ_FOR_EACH159_DETAIL_CHECK_R(r, macro, data, seq) \
        BOOST_PP_IIF \
            ( \
            BOOST_PP_SEQ_DETAIL_IS_NOT_EMPTY(seq), \
            BOOST_PP_SEQ_FOR_EACH159_DETAIL_CHECK_EXEC_R, \
            BOOST_PP_SEQ_FOR_EACH159_DETAIL_CHECK_EMPTY_R \
            ) \
        (r, macro, data, seq) \
/**/