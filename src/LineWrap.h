#pragma once
#include <optional>
#include "Panel.h"
#include "Color.h"
#include "Attr.h"
#include "UTF8.h"
#include "lib/tinyutf8/tinyutf8.h"

namespace UI::LineWrap {

inline std::vector<tiny_utf8::string> Wrap(size_t lineCountMax, size_t lineLenMax, const tiny_utf8::string& str) {
    using Line = std::deque<tiny_utf8::string>;
    using Lines = std::deque<Line>;
    
    Lines linesInput;
    {
        std::istringstream stream(str.c_str());
        std::string line;
        while (std::getline(stream, line)) {
            if (line.empty()) continue;
            
            Line& lineInput = linesInput.emplace_back();
            std::istringstream stream(line);
            tiny_utf8::string word;
            while (stream >> word) lineInput.push_back(word);
        }
    }
    
    std::vector<tiny_utf8::string> lines;
    Line* lastLine = nullptr;
    while (lines.size()<lineCountMax && !linesInput.empty()) {
        Line& lineInput = linesInput.front();
        tiny_utf8::string& msgline = lines.emplace_back();
        
        while (!lineInput.empty()) {
            // Add the current word
            {
                const tiny_utf8::string& word = lineInput.front();
                const size_t wordLen = word.length();
                const tiny_utf8::string add = (msgline.empty() ? "" : " ") + word;
                const size_t addLen = add.length();
                const size_t rem = lineLenMax-msgline.length();
                // No more space -> next line
                if (!rem) break;
                // Check if the line would overflow with `add`
                if (addLen > rem) {
                    // The word (without potential space) would fit by itself on a line -> next line
                    if (wordLen <= lineLenMax) break;
                    // The word wouldn't fit by itself on a line -> split word
                    tiny_utf8::string head = add.substr(0, rem);
                    tiny_utf8::string tail = add.substr(rem, addLen-rem);
                    
                    msgline += head;
                    lineInput.pop_front();
                    lineInput.push_front(tail);
                    continue;
                }
                
                msgline += add;
                lineInput.pop_front();
            }
        }
        
        if (lineInput.empty()) {
            linesInput.pop_front();
            lastLine = nullptr;
        } else {
            lastLine = &linesInput.front();
        }
    }
    
    // Add as many letters from the remaining word as will fit on the last line
    if (lastLine && !lastLine->empty()) {
        const tiny_utf8::string& word = lastLine->front();
        tiny_utf8::string& line = lines.back();
        line += (line.empty() ? "" : " ") + word;
        // Our logic guarantees that if the word would have fit, it would've been included in the last line.
        // So since the word isn't included, the length of the line (with the word included) must be larger
        // than `lineLenMax`. So verify that assumption.
        assert(line.length() > lineLenMax);
        line.erase(lineLenMax);
    }
    
    return lines;
}

} // namespace UI::LineWrap
