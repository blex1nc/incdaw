#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace incdaw::project {

/// A minimal JSON value with **ordered** object keys.
///
/// Written rather than vendored for one reason that matters more than the code
/// it costs: docs/PROJECT_FORMAT.md §7 requires that saving an unmodified
/// project twice produces byte-identical output. Most JSON libraries iterate
/// objects in hash order, which changes between runs and would make that
/// guarantee — and the determinism test that enforces it — impossible.
///
/// Insertion order is preserved exactly, numbers are formatted with a fixed
/// round-trippable representation, and there is no pretty-printing ambiguity.
class Json {
public:
    enum class Type { null, boolean, number, integer, string, array, object };

    Json() = default;
    Json(std::nullptr_t) noexcept : type_(Type::null) {}
    Json(bool value) noexcept : type_(Type::boolean), boolean_(value) {}
    Json(std::int64_t value) noexcept : type_(Type::integer), integer_(value) {}
    Json(int value) noexcept : type_(Type::integer), integer_(value) {}
    Json(double value) noexcept : type_(Type::number), number_(value) {}
    Json(std::string value) : type_(Type::string), string_(std::move(value)) {}
    Json(const char* value) : type_(Type::string), string_(value != nullptr ? value : "") {}

    [[nodiscard]] static Json array()  { Json value; value.type_ = Type::array;  return value; }
    [[nodiscard]] static Json object() { Json value; value.type_ = Type::object; return value; }

    [[nodiscard]] Type type() const noexcept { return type_; }

    [[nodiscard]] bool isNull()    const noexcept { return type_ == Type::null; }
    [[nodiscard]] bool isObject()  const noexcept { return type_ == Type::object; }
    [[nodiscard]] bool isArray()   const noexcept { return type_ == Type::array; }
    [[nodiscard]] bool isString()  const noexcept { return type_ == Type::string; }
    [[nodiscard]] bool isNumber()  const noexcept { return type_ == Type::number || type_ == Type::integer; }
    [[nodiscard]] bool isBoolean() const noexcept { return type_ == Type::boolean; }

    // ── Reading. Every accessor takes a fallback: a project file is untrusted
    //    input, and a missing or wrong-typed field must degrade, never throw.
    [[nodiscard]] bool         asBool(bool fallback = false) const noexcept;
    [[nodiscard]] std::int64_t asInt(std::int64_t fallback = 0) const noexcept;
    [[nodiscard]] double       asDouble(double fallback = 0.0) const noexcept;
    [[nodiscard]] std::string  asString(const std::string& fallback = {}) const;

    [[nodiscard]] const Json& operator[](const std::string& key) const noexcept;
    [[nodiscard]] const Json& operator[](std::size_t index) const noexcept;

    [[nodiscard]] bool        contains(const std::string& key) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

    [[nodiscard]] const std::vector<std::pair<std::string, Json>>& members() const noexcept { return members_; }
    [[nodiscard]] const std::vector<Json>& elements() const noexcept { return elements_; }

    // ── Building ────────────────────────────────────────────────────────────
    /// Appends a member. Existing keys are replaced in place, keeping their
    /// original position so that output stays stable across edits.
    void set(const std::string& key, Json value);
    void append(Json value);

    // ── Text ────────────────────────────────────────────────────────────────
    /// Deterministic: same value in, same bytes out, always.
    [[nodiscard]] std::string dump(int indent = 2) const;

    /// Parses `text`. Returns false and fills `error` on malformed input.
    /// Never throws and never partially applies a broken document.
    [[nodiscard]] static bool parse(const std::string& text, Json& out, std::string& error);

private:
    void dumpTo(std::string& out, int indent, int depth) const;

    Type         type_    = Type::null;
    bool         boolean_ = false;
    std::int64_t integer_ = 0;
    double       number_  = 0.0;
    std::string  string_;

    std::vector<Json>                             elements_;
    std::vector<std::pair<std::string, Json>>     members_;
};

} // namespace incdaw::project
