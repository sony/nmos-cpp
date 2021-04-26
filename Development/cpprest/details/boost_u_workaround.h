// Workaround for conflict between the 'U' macro and some uses of U as e.g. a template parameter in Boost libraries
#pragma once
#ifdef __cplusplus
#include "cpprest/details/push_undef_u.h"
// See https://stackoverflow.com/questions/43245055/issue-with-boost-1-64-and-visual-studio-2017
#include <boost/move/detail/type_traits.hpp>
#include "cpprest/details/pop_u.h"
#endif
