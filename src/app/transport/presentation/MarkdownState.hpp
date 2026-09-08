#pragma once

#include <string>

namespace transport
{
struct MarkdownState
{
    bool        codeBlock = false; // ```
    bool        codeSpan = false;  // `
    bool        bold = false;      // *
    bool        italic = false;    // _
    bool        underline = false; // __
    bool        strike = false;    // ~
    std::string language;          // язык блока кода, чтобы продолжить с подсветкой

    bool empty() const
    {
        return !codeBlock && !codeSpan && !bold && !italic && !underline && !strike;
    }
};
} // namespace transport