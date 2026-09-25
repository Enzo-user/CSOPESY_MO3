#include "AsciiFont.h"

#include <istream>

bool AsciiFont::read(std::istream &in) {
    std::map<char, std::vector<std::string>> glyphs;
    std::string line;

    for (int code = 33; code <= 126; ++code) {
        std::vector<std::string> rows;
        for (int r = 0; r < ROWS && std::getline(in, line); ++r) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back(); // hidden Windows carriage return
            }
            rows.push_back(line);
        }
        if (rows.size() != static_cast<std::size_t>(ROWS)) {
            break; // the file ended early
        }

        std::string::size_type width = 0;
        for (std::size_t r = 0; r < rows.size(); ++r) {
            const std::string::size_type last = rows[r].find_last_not_of(' ');
            if (last != std::string::npos && last + 1 > width) {
                width = last + 1;
            }
        }
        for (std::size_t r = 0; r < rows.size(); ++r) {
            rows[r].resize(width, ' '); // crop (or pad) to the glyph width
            rows[r] += ' ';             // kerning
        }
        glyphs[static_cast<char>(code)] = rows;
    }

    if (glyphs.empty()) {
        return false;
    }
    glyphs[' '] = std::vector<std::string>(ROWS, "     ");
    glyphs_ = glyphs;
    return true;
}

std::vector<std::string> AsciiFont::render(const std::string &text) const {
    if (glyphs_.empty()) {
        return std::vector<std::string>(1, text);
    }
    std::vector<std::string> rows(ROWS);
    for (std::string::size_type i = 0; i < text.size(); ++i) {
        std::map<char, std::vector<std::string>>::const_iterator glyph = glyphs_.find(text[i]);
        if (glyph == glyphs_.end()) {
            glyph = glyphs_.find(' '); // unsupported character: draw a gap
        }
        for (int r = 0; r < ROWS; ++r) {
            rows[r] += glyph->second[r];
        }
    }
    return rows;
}
