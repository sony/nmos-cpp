#include "cpprest/json_visit.h"

#include <nlohmann/json.hpp> // for nlohmann/detail/conversions/to_chars.hpp

namespace web
{
    namespace json
    {
        namespace details
        {
            template <>
            size_t num_put<char>::put(char* buffer, size_t capacity, double v, std::streamsize precision)
            {
                if (std::numeric_limits<double>::max_digits10 > precision)
                {
                    return std::snprintf(buffer, capacity, "%.*g", (int)precision, v);
                }
                else
                {
                    // max_digits10 is usually used to guarantee roundtrip, value -> string -> value,
                    // but this has the downside that some numbers are printed with more digits than
                    // necessary, for example, 59.94 is output as 59.939999999999998
                    // the Grisu2 algorithm implemented by nlohmann::detail::to_chars does better!
                    const auto last = nlohmann::detail::to_chars(buffer, buffer + capacity - 1, v);
                    *last = '\0';
                    return last - buffer;
                }
            }

#ifdef _WIN32
            template <>
            size_t num_put<wchar_t>::put(wchar_t* wbuffer, size_t capacity, double v, std::streamsize precision)
            {
                const auto cap = (std::min)(capacity, capacity_double);
                std::vector<char> buffer(cap);
                const auto len = num_put<char>::put(&buffer[0], cap, v, precision);
                std::copy_n(&buffer[0], (std::min)(len + 1, cap), wbuffer);
                return len;
            }
#endif
        }

        namespace experimental
        {
            // sample stylesheet for the HTML generated by html_visitor
            const char* html_stylesheet = R"-stylesheet-(
.json
{
  font-family: monospace
}
.json .number, .json .boolean, .json .null
{
  color: blue
}
.json .string
{
  color: brown
}
.json .object
{
  color: grey
}
.json .array
{
  color: grey
}
.json ul, .json ol
{
  list-style: none;
  border-left: 0.5px dotted rgba(0, 128, 0, 0.25);
}
.json ul:empty, .json ol:empty
{
  display: none
}
.json .name
{
  color: purple
}
/* folding support */
.json .elided
{
  display: none
}
.json .toggle::before
{
  position: relative;
  left: -0.25em;
  color: green;
  user-select: none;
  content: '\25bc'; /* '\25be' is smaller */
}
.json .open.toggle::before
{
  content: '\25ba'; /* '\25b8' is smaller */
}
/* remove toggle for browsers that don't support user-select */
_:-ms-lang(x), .json .toggle::before
{
  content: ''
}
_:-ms-lang(x), .json .open.toggle::before
{
  content: ''
}
/* optional gutter toggles */
.json
{
  position: relative
}
.json.gutter
{
  padding-left: 1.5em
}
.json.gutter .toggle::before
{
  position: absolute;
  left: 0;
}
)-stylesheet-";

            // sample script for the HTML generated by html_visitor
            const char* html_script = R"-script-(
window.onload = function() {
  var json = document.getElementsByClassName('json');
  for (var i = 0; i < json.length; ++i) {
    json[i].onclick = function(e) {
      if (!e.target.classList.contains('toggle')) return;
      var normal = e.target.parentElement.querySelector(':scope > .normal');
      var elided = e.target.parentElement.querySelector(':scope > .elided');

      // get state
      var open = getComputedStyle(normal).display == 'inline';

      // toggle state
      open ? e.target.classList.add('open') : e.target.classList.remove('open');
      normal.style.display = open ? 'none' : 'inline';
      elided.style.display = open ? 'inline' : 'none';
    }
  }
}
)-script-";
        }
    }
}
