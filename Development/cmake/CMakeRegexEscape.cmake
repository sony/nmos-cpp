# see https://cmake.org/cmake/help/latest/command/string.html#regex-specification

# match any special character
string(CONCAT MATCH_MATCH
    "("
    "\\^|"
    "\\$|"
    "\\.|"
    "\\\\|"
    "\\[|"
    "\\]|"
    "\\*|"
    "\\+|"
    "\\?|"
    "\\||"
    "\\(|"
    "\\)"
    ")"
    )

# replace it with a single backslash and the matched character
string(CONCAT MATCH_REPLACE
    "\\\\"
    "\\1"
    )

# match a single backslash
string(CONCAT REPLACE_MATCH
    "\\\\"
    )

# replace it with two backslashes
string(CONCAT REPLACE_REPLACE
    "\\\\"
    "\\\\"
    )
