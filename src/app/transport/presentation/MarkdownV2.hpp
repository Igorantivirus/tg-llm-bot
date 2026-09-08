#pragma once

#include <string>
#include <string_view>

#include "MarkdownState.hpp"

namespace transport
{

class MarkdownV2
{
public:
    static std::string convert(const std::string_view text)
    {
        MarkdownState state;
        return convert(text, state);
    }

    static std::string convert(const std::string_view text, MarkdownState &state)
    {
        std::string res;
        res.reserve(text.size() + text.size() / 4);
        reopen(res, state);
        std::size_t i = 0;
        while (i < text.size())
        {
            const char c = text[i];

            if (c == '`' && startsWith(text, i, "```"))
            {
                i += 3;
                if (!state.codeBlock)
                {
                    state.codeBlock = true;
                    state.language.clear();
                    // Язык после ``` идёт до конца строки и не экранируется.
                    while (i < text.size() && text[i] != '\n')
                        state.language += text[i++];
                    res += "```" + state.language;
                }
                else
                {
                    state.codeBlock = false;
                    state.language.clear();
                    res += "```";
                }
                continue;
            }
            if (state.codeBlock)
            {
                appendCodeChar(res, c);
                ++i;
                continue;
            }
            if (c == '`')
            {
                state.codeSpan = !state.codeSpan;
                res += '`';
                ++i;
                continue;
            }
            if (state.codeSpan)
            {
                appendCodeChar(res, c);
                ++i;
                continue;
            }
            if (c == '\\' && i + 1 < text.size())
            {
                appendEscaped(res, text[i + 1]);
                i += 2;
                continue;
            }
            if (startsWith(text, i, "**"))
            {
                state.bold = !state.bold;
                res += '*';
                i += 2;
                continue;
            }
            if (startsWith(text, i, "~~"))
            {
                state.strike = !state.strike;
                res += '~';
                i += 2;
                continue;
            }
            if (startsWith(text, i, "__"))
            {
                state.underline = !state.underline;
                res += "__";
                i += 2;
                continue;
            }
            if (c == '*')
            {
                state.bold = !state.bold;
                res += '*';
                ++i;
                continue;
            }
            if (c == '_')
            {
                state.italic = !state.italic;
                res += '_';
                ++i;
                continue;
            }
            appendEscaped(res, c);
            ++i;
        }

        close(res, state);
        return res;
    }

    static std::string escape(const std::string_view text)
    {
        std::string res;
        res.reserve(text.size() + text.size() / 4);
        for (const char c : text)
            appendEscaped(res, c);
        return res;
    }

private:
    static constexpr std::string_view reserved_ = "_*[]()~`>#+-=|{}.!";

    static bool startsWith(const std::string_view text, const std::size_t pos, const std::string_view what)
    {
        return text.compare(pos, what.size(), what) == 0;
    }

    static void reopen(std::string &res, const MarkdownState &state)
    {
        if (state.bold)
            res += '*';
        if (state.italic)
            res += '_';
        if (state.underline)
            res += "__";
        if (state.strike)
            res += '~';
        if (state.codeBlock)
            res += "```" + state.language + '\n';
        else if (state.codeSpan)
            res += '`';
    }

    static void close(std::string &res, const MarkdownState &state)
    {
        if (state.codeBlock)
            res += "\n```";
        else if (state.codeSpan)
            res += '`';
        if (state.strike)
            res += '~';
        if (state.underline)
            res += "__";
        if (state.italic)
            res += '_';
        if (state.bold)
            res += '*';
    }

    static void appendEscaped(std::string &res, const char c)
    {
        if (reserved_.find(c) != std::string_view::npos)
            res += '\\';
        res += c;
    }

    // Внутри pre/code телеграмм требует экранировать только '`' и '\'.
    static void appendCodeChar(std::string &res, const char c)
    {
        if (c == '`' || c == '\\')
            res += '\\';
        res += c;
    }
};

} // namespace transport
