#include "project/Json.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace incdaw::project {
namespace {

const Json& nullValue()
{
    static const Json instance;
    return instance;
}

void escapeInto(std::string& out, const std::string& text)
{
    out += '"';

    for (const char character : text) {
        switch (character) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            default:
                if (static_cast<unsigned char>(character) < 0x20) {
                    char buffer[8];
                    std::snprintf(buffer, sizeof(buffer), "\\u%04x",
                                  static_cast<unsigned>(static_cast<unsigned char>(character)));
                    out += buffer;
                } else {
                    out += character;   // UTF-8 passes through unchanged
                }
        }
    }

    out += '"';
}

/// Formats a double so that parsing it returns exactly the same value.
///
/// "%.17g" is the shortest precision guaranteed to round-trip an IEEE-754
/// double. Anything shorter silently loses the last bits of a tempo or a gain,
/// and the loss only shows up as a project that sounds subtly different after a
/// save/load cycle.
std::string formatDouble(double value)
{
    if (std::isnan(value) || std::isinf(value))
        return "0";   // JSON has no representation for these

    char buffer[40];
    std::snprintf(buffer, sizeof(buffer), "%.17g", value);
    return buffer;
}

// ── Parser ────────────────────────────────────────────────────────────────────

class Parser {
public:
    Parser(const std::string& text) : text_(text) {}

    bool parseValue(Json& out);

    [[nodiscard]] const std::string& error() const noexcept { return error_; }

    void skipWhitespace()
    {
        while (position_ < text_.size()) {
            const char character = text_[position_];
            if (character == ' ' || character == '\t' || character == '\n' || character == '\r')
                ++position_;
            else
                break;
        }
    }

    [[nodiscard]] bool atEnd()
    {
        skipWhitespace();
        return position_ >= text_.size();
    }

private:
    bool fail(const std::string& message)
    {
        if (error_.empty())
            error_ = message + " at offset " + std::to_string(position_);
        return false;
    }

    bool parseString(std::string& out);
    bool parseNumber(Json& out);
    bool parseObject(Json& out);
    bool parseArray(Json& out);
    bool parseLiteral(const char* literal, Json value, Json& out);

    const std::string& text_;
    std::size_t        position_ = 0;
    std::string        error_;

    // A malformed or hostile file must not blow the stack. Real projects nest
    // far shallower than this.
    static constexpr int maxDepth = 64;
    int                  depth_   = 0;
};

bool Parser::parseLiteral(const char* literal, Json value, Json& out)
{
    const std::size_t length = std::char_traits<char>::length(literal);

    if (text_.compare(position_, length, literal) != 0)
        return fail("unexpected token");

    position_ += length;
    out = std::move(value);
    return true;
}

bool Parser::parseString(std::string& out)
{
    if (position_ >= text_.size() || text_[position_] != '"')
        return fail("expected a string");

    ++position_;
    out.clear();

    while (position_ < text_.size()) {
        const char character = text_[position_++];

        if (character == '"')
            return true;

        if (character != '\\') {
            out += character;
            continue;
        }

        if (position_ >= text_.size())
            return fail("truncated escape sequence");

        switch (const char escape = text_[position_++]) {
            case '"':  out += '"';  break;
            case '\\': out += '\\'; break;
            case '/':  out += '/';  break;
            case 'n':  out += '\n'; break;
            case 'r':  out += '\r'; break;
            case 't':  out += '\t'; break;
            case 'b':  out += '\b'; break;
            case 'f':  out += '\f'; break;
            case 'u': {
                if (position_ + 4 > text_.size())
                    return fail("truncated \\u escape");

                const auto code = static_cast<unsigned>(
                    std::strtoul(text_.substr(position_, 4).c_str(), nullptr, 16));
                position_ += 4;

                // Encode as UTF-8. Surrogate pairs are not reassembled: INCDAW
                // only ever writes raw UTF-8, so they cannot occur in our own
                // files, and mangling one is better than rejecting the project.
                if (code < 0x80) {
                    out += static_cast<char>(code);
                } else if (code < 0x800) {
                    out += static_cast<char>(0xC0 | (code >> 6));
                    out += static_cast<char>(0x80 | (code & 0x3F));
                } else {
                    out += static_cast<char>(0xE0 | (code >> 12));
                    out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                    out += static_cast<char>(0x80 | (code & 0x3F));
                }
                break;
            }
            default:
                return fail(std::string{"unknown escape '"} + escape + "'");
        }
    }

    return fail("unterminated string");
}

bool Parser::parseNumber(Json& out)
{
    const std::size_t start = position_;

    if (position_ < text_.size() && (text_[position_] == '-' || text_[position_] == '+'))
        ++position_;

    bool isFloatingPoint = false;

    while (position_ < text_.size()) {
        const char character = text_[position_];

        if (character >= '0' && character <= '9') {
            ++position_;
        } else if (character == '.' || character == 'e' || character == 'E') {
            isFloatingPoint = true;
            ++position_;
        } else if ((character == '-' || character == '+')
                   && (text_[position_ - 1] == 'e' || text_[position_ - 1] == 'E')) {
            ++position_;
        } else {
            break;
        }
    }

    if (position_ == start)
        return fail("expected a number");

    const std::string token = text_.substr(start, position_ - start);

    // Integers are kept as integers: writing a 64-bit id back out through a
    // double would quietly round it once it exceeded 2^53.
    out = isFloatingPoint ? Json{std::strtod(token.c_str(), nullptr)}
                          : Json{static_cast<std::int64_t>(std::strtoll(token.c_str(), nullptr, 10))};
    return true;
}

bool Parser::parseArray(Json& out)
{
    ++position_;   // '['
    out = Json::array();

    skipWhitespace();
    if (position_ < text_.size() && text_[position_] == ']') {
        ++position_;
        return true;
    }

    while (true) {
        Json element;
        if (!parseValue(element))
            return false;

        out.append(std::move(element));
        skipWhitespace();

        if (position_ >= text_.size())
            return fail("unterminated array");

        if (text_[position_] == ',') {
            ++position_;
            continue;
        }

        if (text_[position_] == ']') {
            ++position_;
            return true;
        }

        return fail("expected ',' or ']'");
    }
}

bool Parser::parseObject(Json& out)
{
    ++position_;   // '{'
    out = Json::object();

    skipWhitespace();
    if (position_ < text_.size() && text_[position_] == '}') {
        ++position_;
        return true;
    }

    while (true) {
        skipWhitespace();

        std::string key;
        if (!parseString(key))
            return false;

        skipWhitespace();
        if (position_ >= text_.size() || text_[position_] != ':')
            return fail("expected ':'");
        ++position_;

        Json value;
        if (!parseValue(value))
            return false;

        out.set(key, std::move(value));
        skipWhitespace();

        if (position_ >= text_.size())
            return fail("unterminated object");

        if (text_[position_] == ',') {
            ++position_;
            continue;
        }

        if (text_[position_] == '}') {
            ++position_;
            return true;
        }

        return fail("expected ',' or '}'");
    }
}

bool Parser::parseValue(Json& out)
{
    if (++depth_ > maxDepth) {
        --depth_;
        return fail("nesting too deep");
    }

    struct DepthGuard {
        int& depth;
        ~DepthGuard() { --depth; }
    } guard{depth_};

    skipWhitespace();

    if (position_ >= text_.size())
        return fail("unexpected end of input");

    switch (const char character = text_[position_]) {
        case '{': return parseObject(out);
        case '[': return parseArray(out);
        case '"': {
            std::string value;
            if (!parseString(value))
                return false;
            out = Json{std::move(value)};
            return true;
        }
        case 't': return parseLiteral("true", Json{true}, out);
        case 'f': return parseLiteral("false", Json{false}, out);
        case 'n': return parseLiteral("null", Json{nullptr}, out);
        default:
            if (character == '-' || character == '+' || (character >= '0' && character <= '9'))
                return parseNumber(out);
            return fail(std::string{"unexpected character '"} + character + "'");
    }
}

} // namespace

// ── Json ──────────────────────────────────────────────────────────────────────

bool Json::asBool(bool fallback) const noexcept
{
    return type_ == Type::boolean ? boolean_ : fallback;
}

std::int64_t Json::asInt(std::int64_t fallback) const noexcept
{
    if (type_ == Type::integer) return integer_;
    if (type_ == Type::number)  return static_cast<std::int64_t>(number_);
    return fallback;
}

double Json::asDouble(double fallback) const noexcept
{
    if (type_ == Type::number)  return number_;
    if (type_ == Type::integer) return static_cast<double>(integer_);
    return fallback;
}

std::string Json::asString(const std::string& fallback) const
{
    return type_ == Type::string ? string_ : fallback;
}

const Json& Json::operator[](const std::string& key) const noexcept
{
    for (const auto& member : members_)
        if (member.first == key)
            return member.second;

    return nullValue();
}

const Json& Json::operator[](std::size_t index) const noexcept
{
    return index < elements_.size() ? elements_[index] : nullValue();
}

bool Json::contains(const std::string& key) const noexcept
{
    for (const auto& member : members_)
        if (member.first == key)
            return true;

    return false;
}

std::size_t Json::size() const noexcept
{
    if (type_ == Type::array)  return elements_.size();
    if (type_ == Type::object) return members_.size();
    return 0;
}

void Json::set(const std::string& key, Json value)
{
    if (type_ != Type::object)
        type_ = Type::object;

    for (auto& member : members_) {
        if (member.first == key) {
            // Replaced in place: keeping the original position is what makes
            // an edited project's diff show only what actually changed.
            member.second = std::move(value);
            return;
        }
    }

    members_.emplace_back(key, std::move(value));
}

void Json::append(Json value)
{
    if (type_ != Type::array)
        type_ = Type::array;

    elements_.push_back(std::move(value));
}

void Json::dumpTo(std::string& out, int indent, int depth) const
{
    const bool        pretty = indent > 0;
    const std::string newline = pretty ? "\n" : "";
    const std::string padding = pretty ? std::string(static_cast<std::size_t>(indent * (depth + 1)), ' ') : "";
    const std::string closing = pretty ? std::string(static_cast<std::size_t>(indent * depth), ' ') : "";

    switch (type_) {
        case Type::null:    out += "null"; break;
        case Type::boolean: out += boolean_ ? "true" : "false"; break;
        case Type::integer: out += std::to_string(integer_); break;
        case Type::number:  out += formatDouble(number_); break;
        case Type::string:  escapeInto(out, string_); break;

        case Type::array:
            if (elements_.empty()) { out += "[]"; break; }
            out += '[';
            out += newline;
            for (std::size_t index = 0; index < elements_.size(); ++index) {
                out += padding;
                elements_[index].dumpTo(out, indent, depth + 1);
                if (index + 1 < elements_.size()) out += ',';
                out += newline;
            }
            out += closing;
            out += ']';
            break;

        case Type::object:
            if (members_.empty()) { out += "{}"; break; }
            out += '{';
            out += newline;
            for (std::size_t index = 0; index < members_.size(); ++index) {
                out += padding;
                escapeInto(out, members_[index].first);
                out += pretty ? ": " : ":";
                members_[index].second.dumpTo(out, indent, depth + 1);
                if (index + 1 < members_.size()) out += ',';
                out += newline;
            }
            out += closing;
            out += '}';
            break;
    }
}

std::string Json::dump(int indent) const
{
    std::string out;
    dumpTo(out, indent, 0);

    if (indent > 0)
        out += '\n';   // files end with a newline

    return out;
}

bool Json::parse(const std::string& text, Json& out, std::string& error)
{
    error.clear();

    Parser parser{text};
    Json   parsed;

    if (!parser.parseValue(parsed)) {
        error = parser.error();
        return false;
    }

    if (!parser.atEnd()) {
        error = "trailing content after the top-level value";
        return false;
    }

    // Only assigned on success, so a failed parse never partially overwrites
    // the caller's value.
    out = std::move(parsed);
    return true;
}

} // namespace incdaw::project
