#pragma once
#include <optional>
#include "Panel.h"
#include "Color.h"
#include "Attr.h"
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
            std::istringstream stream(line);
            std::string word;
            while (stream >> word) lineInput.push_back(word);
        }
    }
    
    std::vector<std::string> lines;
    Line* lastLine = nullptr;
    while (lines.size()<lineCountMax && !linesInput.empty()) {
        Line& lineInput = linesInput.front();
        std::string& msgline = lines.emplace_back();
        
        while (!lineInput.empty()) {
            // Add the space between words
            if (!msgline.empty()) {
                const size_t rem = lineLenMax-UTF8::Strlen(msgline);
                // No more space -> next line
                if (!rem) break;
                // Add the space between words
                msgline += " ";
            }
            
            // Add the current word
            {
                const std::string& word = lineInput.front();
                const size_t wordLen = UTF8::Strlen(word);
                const size_t rem = lineLenMax-UTF8::Strlen(msgline);
                // No more space -> next line
                if (!rem) break;
                // Check if the line would overflow with `word`
                if (wordLen > rem) {
                    // The word would fit by itself on a line -> next line
                    if (wordLen <= lineLenMax) break;
                    // The word wouldn't fit by itself on a line -> split word
                    std::string head = word.substr(0, rem);
                    std::string tail = word.substr(rem, wordLen-rem);
                    
                    msgline += head;
                    lineInput.pop_front();
                    lineInput.push_front(tail);
                    continue;
                }
                
                msgline += word;
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
        
//            const char*const ellipses = "...";
//            // Find the first non-space character, at least strlen(ellipses) characters before the end
//            auto it =line.begin()+lineLenMax-strlen(ellipses);
//            for (; it!=line.begin() && std::isspace(*it); it--);
//            
//            line.erase(it, line.end());
//            
//            // Replace the line's final characters with an ellipses
//            line.replace(it, it+strlen(ellipses), ellipses);
    }
    
    return lines;
}

} // namespace UI::LineWrap
