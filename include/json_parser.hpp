#pragma once

// ===== JSON BEGIN ===========================================================
struct Json
{
    enum class Type { Null, Bool, Number, String, Array, Object };

    Type type = Type::Null;
    bool boolean = false;
    double number = 0.0;
    std::string string;
    std::vector<Json> array;
    std::vector<std::string> keys;
    std::vector<Json> values;

    const Json* get(const std::string& key) const
    {
        for (size_t i = 0; i < keys.size(); i++)
        {
            if (keys[i] == key)
                return &values[i];
        }
        return nullptr;
    }

    std::string getString(const std::string& key, const std::string& fallback = "") const
    {
        const Json* v = get(key);
        return (v && v->type == Type::String) ? v->string : fallback;
    }

    int getInt(const std::string& key, int fallback = 0) const
    {
        const Json* v = get(key);
        return (v && v->type == Type::Number) ? static_cast<int>(v->number) : fallback;
    }
};

class JsonParser
{
public:
    explicit JsonParser(const std::string& text) : src(text) {}

    bool parse(Json& out)
    {
        if (!parseValue(out))
            return false;
        skipWhitespace();
        return pos == src.size();
    }

private:
    const std::string& src;
    size_t pos = 0;

    void skipWhitespace()
    {
        while (pos < src.size() && std::isspace(static_cast<unsigned char>(src[pos])))
            pos++;
    }

    bool consume(char c)
    {
        skipWhitespace();
        if (pos < src.size() && src[pos] == c)
        {
            pos++;
            return true;
        }
        return false;
    }

    bool parseLiteral(const char* word)
    {
        const size_t len = std::strlen(word);
        if (src.compare(pos, len, word) != 0)
            return false;
        pos += len;
        return true;
    }

    static void appendUtf8(std::string& out, std::uint32_t cp)
    {
        if (cp < 0x80)
        {
            out += static_cast<char>(cp);
        }
        else if (cp < 0x800)
        {
            out += static_cast<char>(0xC0 | (cp >> 6));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
        else if (cp < 0x10000)
        {
            out += static_cast<char>(0xE0 | (cp >> 12));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
        else
        {
            out += static_cast<char>(0xF0 | (cp >> 18));
            out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }

    bool parseHex4(std::uint32_t& value)
    {
        if (pos + 4 > src.size())
            return false;

        value = 0;
        for (int i = 0; i < 4; i++)
        {
            const char h = src[pos++];
            value <<= 4;
            if (h >= '0' && h <= '9')      value |= static_cast<std::uint32_t>(h - '0');
            else if (h >= 'a' && h <= 'f') value |= static_cast<std::uint32_t>(h - 'a' + 10);
            else if (h >= 'A' && h <= 'F') value |= static_cast<std::uint32_t>(h - 'A' + 10);
            else return false;
        }
        return true;
    }

    bool parseString(std::string& out)
    {
        skipWhitespace();
        if (pos >= src.size() || src[pos] != '"')
            return false;
        pos++;
        out.clear();

        while (pos < src.size())
        {
            const char c = src[pos++];
            if (c == '"')
                return true;

            if (c != '\\')
            {
                out += c;
                continue;
            }

            if (pos >= src.size())
                return false;

            const char e = src[pos++];
            switch (e)
            {
            case '"':  out += '"';  break;
            case '\\': out += '\\'; break;
            case '/':  out += '/';  break;
            case 'b':  out += '\b'; break;
            case 'f':  out += '\f'; break;
            case 'n':  out += '\n'; break;
            case 'r':  out += '\r'; break;
            case 't':  out += '\t'; break;
            case 'u':
            {
                std::uint32_t cp = 0;
                if (!parseHex4(cp))
                    return false;

                if (cp >= 0xD800 && cp <= 0xDBFF && src.compare(pos, 2, "\\u") == 0)
                {
                    pos += 2;
                    std::uint32_t low = 0;
                    if (!parseHex4(low))
                        return false;
                    cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                }
                appendUtf8(out, cp);
                break;
            }
            default:
                return false;
            }
        }
        return false;
    }

    bool parseValue(Json& out)
    {
        skipWhitespace();
        if (pos >= src.size())
            return false;

        const char c = src[pos];

        if (c == '{')
        {
            pos++;
            out.type = Json::Type::Object;
            if (consume('}'))
                return true;

            while (true)
            {
                std::string key;
                if (!parseString(key))
                    return false;
                if (!consume(':'))
                    return false;

                Json value;
                if (!parseValue(value))
                    return false;

                out.keys.push_back(std::move(key));
                out.values.push_back(std::move(value));

                if (consume(','))
                    continue;
                return consume('}');
            }
        }

        if (c == '[')
        {
            pos++;
            out.type = Json::Type::Array;
            if (consume(']'))
                return true;

            while (true)
            {
                Json value;
                if (!parseValue(value))
                    return false;
                out.array.push_back(std::move(value));

                if (consume(','))
                    continue;
                return consume(']');
            }
        }

        if (c == '"')
        {
            out.type = Json::Type::String;
            return parseString(out.string);
        }

        if (c == 't')
        {
            out.type = Json::Type::Bool;
            out.boolean = true;
            return parseLiteral("true");
        }

        if (c == 'f')
        {
            out.type = Json::Type::Bool;
            out.boolean = false;
            return parseLiteral("false");
        }

        if (c == 'n')
        {
            out.type = Json::Type::Null;
            return parseLiteral("null");
        }

        char* end = nullptr;
        const char* start = src.c_str() + pos;
        const double number = std::strtod(start, &end);
        if (end == start)
            return false;

        out.type = Json::Type::Number;
        out.number = number;
        pos += static_cast<size_t>(end - start);
        return true;
    }
};
// ===== JSON END =============================================================

