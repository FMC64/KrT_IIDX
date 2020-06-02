# /* Copyright (C) 2001
#  * Housemarque Oy
#  * http://www.housemarque.com
#  *
#  * Distributed under the Boost Software License, Version 1.0. (See
#  * accompanying file LICENSE_1_0.txt or copy at
#  * http://www.boost.org/LICENSE_1_0.txt)
#  */
#
# /* Revised by Paul Mensonides (2002) */
#
# /* See http://www.boost.org for most recent version. */
#
#pragma once
#
# include <boost/preprocessor/config/config.hpp>
# include <boost/preprocessor/control/expr_iif.hpp>
# include <boost/preprocessor/logical/bool.hpp>
#
# /* BOOST_PP_EXPR_IF238 */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_EXPR_IF238(cond, expr) BOOST_PP_EXPR_IIF238(BOOST_PP_BOOL238(cond), expr)
# else
#    define BOOST_PP_EXPR_IF238(cond, expr) BOOST_PP_EXPR_IF238_I(cond, expr)
#    define BOOST_PP_EXPR_IF238_I(cond, expr) BOOST_PP_EXPR_IIF238(BOOST_PP_BOOL238(cond), expr)
# endif
