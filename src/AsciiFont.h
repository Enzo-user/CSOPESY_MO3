#pragma once

#include <iosfwd>
#include <map>
#include <string>
#include <vector>

// The ASCII font the marquee is drawn with: one block of ROWS lines per
// printable character, read from ascii_big.txt at startup.
class AsciiFont {
public:
    static constexpr const char *FILE_NAME = "ascii_big.txt";
    static constexpr int ROWS = 8; // the font is 8 rows tall

    // Reads the font: one block per printable character, starting at '!'
    // (ASCII 33) and continuing in ASCII order up to '~' (ASCII 126). Each
    // glyph is cropped to its real width plus one space of kerning.
    bool read(std::istream &in);

    bool loaded() const { return !glyphs_.empty(); }

    // Stitches the glyphs of text side by side. Without a font the text is
    // returned as a single plain row.
    std::vector<std::string> render(const std::string &text) const;

private:
    std::map<char, std::vector<std::string>> glyphs_;
};
