#pragma once
#include <optional>
#include "Panel.h"
#include "Color.h"
#include "UTF8.h"

namespace UI::LineWrap {

inline std::vector<std::string> Wrap(size_t lineCountMax, size_t lineLenMax, std::string_view str) {
    using Line = std::deque<std::string>;
    using Lines = std::deque<Line>;
    
    Lines linesInput;
    {
        std::istringstream stream((std::string)str);
        std::string line;
        while (std::getline(stream, line)) {
            if (line.empty()) continue;
            
            Line& lineInput = linesInput.emplace_back();
            std::istringstream linestream(line);
            std::string word;
            while (linestream >> word) lineInput.push_back(word);
        }
    }
    
    std::vector<std::string> lines;
    Line* lastLine = nullptr;
    while (lines.size()<lineCountMax && !linesInput.empty()) {
        Line& lineInput = linesInput.front();
        std::string& msgline = lines.emplace_back();
        
        while (!lineInput.empty()) {
            // Add the current word
            {
                const std::string& word = lineInput.front();
                const size_t wordLen = UTF8::Strlen(word);
                const std::string add = (msgline.empty() ? "" : " ") + word;
                const size_t addLen = UTF8::Strlen(add);
                const size_t rem = lineLenMax-UTF8::Strlen(msgline);
                // No more space -> next line
                if (!rem) break;
                // Check if the line would overflow with `add`
                if (addLen > rem) {
                    // The word (without potential space) would fit by itself on a line -> next line
                    if (wordLen <= lineLenMax) break;
                    // The word wouldn't fit by itself on a line -> split word
                    std::string head = add.substr(0, rem);
                    std::string tail = add.substr(rem, addLen-rem);
                    
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
        const std::string& word = lastLine->front();
        std::string& line = lines.back();
        line += (line.empty() ? "" : " ") + word;
        // Our logic guarantees that if the word would have fit, it would've been included in the last line.
        // So since the word isn't included, the length of the line (with the word included) must be larger
        // than `lineLenMax`. So verify that assumption.
        assert(UTF8::Strlen(line) > lineLenMax);
        line.erase(lineLenMax);
    }
    
    return lines;
}

} // namespace UI::LineWrap
