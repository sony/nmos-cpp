#ifndef MDNS_CORE_H
#define MDNS_CORE_H

#include <vector>
#include <boost/algorithm/string/predicate.hpp>

namespace mdns
{
    const unsigned int default_timeout_seconds = 5;

    // DNS TXT records "are used to hold descriptive text. The semantics of the text depends on the domain where it is found."
    // See https://tools.ietf.org/html/rfc1035#section-3.3.14
    typedef std::vector<std::string> txt_records;

    // "DNS-SD uses DNS TXT records to store arbitrary key/value pairs [...] in the form "key=value" (without the quotation marks)."
    // See https://tools.ietf.org/html/rfc6763#section-6.3
    typedef std::pair<std::string, std::string> structured_txt_record;
    typedef std::vector<structured_txt_record> structured_txt_records;

    structured_txt_records parse_txt_records(const txt_records& records);
    txt_records make_txt_records(const structured_txt_records& records);

    // Note that RFC 1464 was first to propose that "to store new types of information, the TXT record uses a structured format
    // in its TXT-DATA field. The format consists of the attribute name followed by the value of the attribute. The name and value
    // are separated by an equals sign (=)."
    // See https://tools.ietf.org/html/rfc1464#section-2
    // However, the syntax and semantics are a little different between the two...
    // E.g.
    // "During lookups, TXT records that do not contain an unquoted "=" are ignored. TXT records that seem to contain a
    // null attribute name, that is, the TXT-DATA starts with the character "=", are also ignored." (RFC 1464)
    // vs.
    // "If there is no '=' in a DNS-SD TXT record string, then it is a boolean attribute, simply identified as being
    // present, with no value." (RFC 6763)

    template <typename Value, typename Parser>
    inline Value parse_txt_record(const structured_txt_records& records, const std::string& key, Parser parse, const Value& default_value = {})
    {
        auto found = std::find_if(records.begin(), records.end(), [&](const structured_txt_record& record)
        {
            // "Spaces in the key are significant, whether leading, trailing, or in the middle -- so don't include
            // any spaces unless you really intend that. Case is ignored when interpreting a key, so "papersize=A4",
            // "PAPERSIZE=A4", and "Papersize=A4" are all identical." (RFC 6763)
            // vs.
            // "Leading and trailing whitespace (spaces and tabs) in the attribute name are ignored unless they are
            // quoted (with a "`"). For example, "abc" matches " abc<tab>" but does not match "` abc"." (RFC 1464)

            // Follow simpler RFC 6763 specification...
            return boost::algorithm::iequals(key, record.first);
        });
        return records.end() != found ? parse(found->second) : default_value;
    }
}

#endif
