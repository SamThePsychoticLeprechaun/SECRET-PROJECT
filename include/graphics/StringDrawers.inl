/**
 * @file StringDrawers.inl
 * @brief Provides implementations for string drawing.
 */

#pragma once

#if !defined(SP_Graphics_StringDrawers_h__)
#define SP_Graphics_StringDrawers_h__

namespace SecretProject {
    namespace graphics {
        /**
         * @brief The data needed to draw a glyph.
         */
        struct DrawableGlyph {
            Glyph*  glyph;
            f32     xPos;
            f32v2   scaling;
            colour4 tint;
            GLuint  texture;
        };

        /**
         * @brief The data needed to draw a line.
         */
        struct DrawableLine {
            f32 length;
            f32 height;
            std::vector<DrawableGlyph> drawables;
        };
        using DrawableLines = std::vector<DrawableLine>;



        /******************************************************\
         * No Wrap Draw                                       *
        \******************************************************/

        /**
         * @brief Draws a string with no wrapping.
         *
         * @param batcher The sprite batcher to draw the string to.
         * @param components The string components to draw.
         * @param rect The bounding rectangle the string must be kept within.
         * @param align The alignment for the text.
         * @param depth The depth at which to render the string.
         */
        inline void drawNoWrapString(SpriteBatcher* batcher, StringComponents components, f32v4 rect, TextAlign align, f32 depth) {
            // We will populate these data points for drawing later.
            DrawableLines lines;
            f32 totalHeight = 0.0f;

            // Place the first line.
            lines.emplace_back(DrawableLine{0.0f, 0.0f, std::vector<DrawableGlyph>()});

            // TODO(Matthew): Can we make guesses as to the amount of drawables to reserve for a line? For amount of lines?

            for (auto& component : components) {
                // Simplify property names.
                const char*  str    = component.first;
                FontInstance font   = component.second.fontInstance;
                StringSizing sizing = component.second.sizing;
                colour4      tint   = component.second.tint;

                char start = font.owner->getStart();
                char end   = font.owner->getEnd();

                // Process sizing into a simple scale factor.
                f32v2 scaling;
                f32   height;
                if (sizing.kind == StringSizingKind::SCALED) {
                    scaling    = sizing.scaling;
                    height = static_cast<f32>(font.height) * scaling.y;
                } else {
                    scaling.x  = sizing.scaleX;
                    scaling.y  = sizing.targetHeight / static_cast<f32>(font.height);
                    height = sizing.targetHeight;
                }

                // Gets set to true if we go out of the height of the rect.
                bool verticalOverflow = false;
                // Iterate over this component's string.
                for (size_t i = 0; str[i] != '\0'; ++i) {
                    char   character      = str[i];
                    size_t characterIndex = static_cast<size_t>(character) - static_cast<size_t>(start);

                    // If character is a new line character, add a new line and go to next character.
                    if (character == '\n') {
                        totalHeight += lines.back().height;
                        lines.emplace_back(DrawableLine{ 0.0f, height, std::vector<DrawableGlyph>() });

                        continue;
                    }

                    // If character is unsupported, skip.
                    if (character < start || character > end ||
                            !font.glyphs[characterIndex].supported) continue;

                    // If the line's height is less than the height of this font instance, and we're about to add a
                    // glyph from this font instance, then the line's height needs setting to the font instances's.
                    if (lines.back().height < height) {
                        // If we have overflowed the rectangle with the new line height, break out instead.
                        if (totalHeight + height > rect.w) {
                            verticalOverflow = true;
                            break;
                        }
                        lines.back().height = height;
                    }

                    // Determine character width after scaling.
                    f32 characterWidth = font.glyphs[characterIndex].size.x * scaling.x;

                    // Add character to line for drawing.
                    lines.back().drawables.emplace_back(DrawableGlyph{ &font.glyphs[characterIndex], lines.back().length, scaling, tint, font.texture });
                    lines.back().length += characterWidth;
                }

                // If we have overflown vertically, break out of outer loop.
                if (verticalOverflow) break;
            }
            // Update the total height for last line.
            totalHeight += lines.back().height;

            f32 currentY = 0.0f;
            for (auto& line : lines) {
                f32v2 offsets = calculateOffset(align, rect, totalHeight, line.length);

                for (auto& drawable : line.drawables) {
                    f32v2 size         = drawable.glyph->size * drawable.scaling;
                    f32v2 position     = f32v2(drawable.xPos, currentY) + offsets + f32v2(rect.x, rect.y) + f32v2(0.0f, line.height - size.y);
                    f32v4 uvDimensions = drawable.glyph->uvDimensions;

                    f32v2 oldSize = size;

                    clip(rect, position, size, uvDimensions);

                    // Reject any character that even slightly clips with the bounding rectangle.
                    if (oldSize == size) {
                        batcher->draw(drawable.texture, position, size, drawable.tint,
                                        { 255, 255, 255, 255 }, Gradient::NONE, depth, uvDimensions);
                    } else {
                        continue;
                    }
                }

                currentY += line.height;
            }
        }



        /******************************************************\
         * Quick Wrap Draw                                    *
        \******************************************************/

        /**
         * @brief Draws a string with quick wrapping.
         *
         * @param batcher The sprite batcher to draw the string to.
         * @param components The string components to draw.
         * @param rect The bounding rectangle the string must be kept within.
         * @param align The alignment for the text.
         * @param depth The depth at which to render the string.
         */
        inline void drawQuickWrapString(SpriteBatcher* batcher, StringComponents components, f32v4 rect, TextAlign align, f32 depth) {
            // We will populate these data points for drawing later.
            DrawableLines lines;
            f32 totalHeight = 0.0f;

            // Place the first line.
            lines.emplace_back(DrawableLine{0.0f, 0.0f, std::vector<DrawableGlyph>()});

            // TODO(Matthew): Can we make guesses as to the amount of drawables to reserve for a line? For amount of lines?

            for (auto& component : components) {
                // Simplify property names.
                const char*  str    = component.first;
                FontInstance font   = component.second.fontInstance;
                StringSizing sizing = component.second.sizing;
                colour4      tint   = component.second.tint;

                char start = font.owner->getStart();
                char end   = font.owner->getEnd();

                // Process sizing into a simple scale factor.
                f32v2 scaling;
                f32   height;
                if (sizing.kind == StringSizingKind::SCALED) {
                    scaling    = sizing.scaling;
                    height = static_cast<f32>(font.height) * scaling.y;
                } else {
                    scaling.x  = sizing.scaleX;
                    scaling.y  = sizing.targetHeight / static_cast<f32>(font.height);
                    height = sizing.targetHeight;
                }

                // auto hyphenate = [&]() {
                //     char   hyphen = '-';
                //     size_t index  = static_cast<size_t>(hyphen) - static_cast<size_t>(start);

                //     f32 characterWidth = font.glyphs[index].size.x * scaling.x;

                //     lines.back().drawables.emplace_back(DrawableGlyph{ &font.glyphs[index], lines.back().length, scaling, tint, font.texture });
                //     lines.back().length += characterWidth;
                // };

                // Gets set to true if we go out of the height of the rect.
                bool verticalOverflow = false;
                // Iterate over this component's string.
                for (size_t i = 0; str[i] != '\0'; ++i) {
                    char   character      = str[i];
                    size_t characterIndex = static_cast<size_t>(character) - static_cast<size_t>(start);

                    // If character is a new line character, add a new line and go to next character.
                    if (character == '\n') {
                        totalHeight += lines.back().height;
                        lines.emplace_back(DrawableLine{ 0.0f, height, std::vector<DrawableGlyph>() });

                        continue;
                    }

                    // If character is unsupported, skip.
                    if (character < start || character > end ||
                            !font.glyphs[characterIndex].supported) continue;

                    // Determine character width after scaling.
                    f32 characterWidth = font.glyphs[characterIndex].size.x * scaling.x;

                    // Given we are about to add a character, make sure it fits on the line, if not, make
                    // a new line and if the about-to-be-added character isn't a whitespace revisit it.
                    if (lines.back().length + characterWidth > rect.z) {
                        totalHeight += lines.back().height;
                        lines.emplace_back(DrawableLine{ 0.0f, height, std::vector<DrawableGlyph>() });

                        // Make sure to revisit this character if not whitespace.
                        if (character != ' ') {
                            // hyphenate();
                            --i;
                        }
                        continue;
                    }

                    // If the line's height is less than the height of this font instance, and we're about to add a
                    // glyph from this font instance, then the line's height needs setting to the font instances's.
                    if (lines.back().height < height) {
                        // If we have overflowed the rectangle with the new line height, break out instead.
                        if (totalHeight + height > rect.w) {
                            verticalOverflow = true;
                            break;
                        }
                        lines.back().height = height;
                    }

                    // Add character to line for drawing.
                    lines.back().drawables.emplace_back(DrawableGlyph{ &font.glyphs[characterIndex], lines.back().length, scaling, tint, font.texture });
                    lines.back().length += characterWidth;
                }

                // If we have overflown vertically, break out of outer loop.
                if (verticalOverflow) break;
            }
            // Update the total height for last line.
            totalHeight += lines.back().height;

            f32 currentY = 0.0f;
            for (auto& line : lines) {
                f32v2 offsets = calculateOffset(align, rect, totalHeight, line.length);

                for (auto& drawable : line.drawables) {
                    f32v2 size         = drawable.glyph->size * drawable.scaling;
                    f32v2 position     = f32v2(drawable.xPos, currentY) + offsets + f32v2(rect.x, rect.y) + f32v2(0.0f, line.height - size.y);
                    f32v4 uvDimensions = drawable.glyph->uvDimensions;

                    f32v2 oldSize = size;

                    clip(rect, position, size, uvDimensions);

                    // Reject any character that even slightly clips with the bounding rectangle.
                    if (oldSize == size) {
                        batcher->draw(drawable.texture, position, size, drawable.tint,
                                        { 255, 255, 255, 255 }, Gradient::NONE, depth, uvDimensions);
                    } else {
                        continue;
                    }
                }

                currentY += line.height;
            }
        }



        /******************************************************\
         * Greedy Wrap Draw                                   *
        \******************************************************/

        /**
         * @brief Draws a string with greedy wrapping.
         *
         * @param batcher The sprite batcher to draw the string to.
         * @param components The string components to draw.
         * @param rect The bounding rectangle the string must be kept within.
         * @param align The alignment for the text.
         * @param depth The depth at which to render the string.
         */
        inline void drawGreedyWrapString(SpriteBatcher* batcher, StringComponents components, f32v4 rect, TextAlign align, f32 depth) {
            // We will populate these data points for drawing later.
            DrawableLines lines;
            f32 totalHeight = 0.0f;

            // Place the first line.
            lines.emplace_back(DrawableLine{0.0f, 0.0f, std::vector<DrawableGlyph>()});

            // TODO(Matthew): Can we make guesses as to the amount of drawables to reserve for a line? For amount of lines?

            for (auto& component : components) {
                // Simplify property names.
                const char*  str    = component.first;
                FontInstance font   = component.second.fontInstance;
                StringSizing sizing = component.second.sizing;
                colour4      tint   = component.second.tint;

                char start = font.owner->getStart();
                char end   = font.owner->getEnd();

                // Process sizing into a simple scale factor.
                f32v2 scaling;
                f32   height;
                if (sizing.kind == StringSizingKind::SCALED) {
                    scaling    = sizing.scaling;
                    height = static_cast<f32>(font.height) * scaling.y;
                } else {
                    scaling.x  = sizing.scaleX;
                    scaling.y  = sizing.targetHeight / static_cast<f32>(font.height);
                    height = sizing.targetHeight;
                }

                // We have to do per-word lookahead before adding any characters to the lines, these data points
                // enable us to do this.
                ui32 beginIndex   = 0;
                ui32 currentIndex = 0;
                f32  wordLength   = 0.0f;

                // Useful functor to flush current word to line.
                auto flushWordToLine = [&]() {
                    while (beginIndex != currentIndex) {
                        // Get glyph index of character at string index.
                        char   character      = str[beginIndex];
                        size_t characterIndex = static_cast<size_t>(character) - static_cast<size_t>(start);

                        // Add character to line for drawing.
                        lines.back().drawables.emplace_back(DrawableGlyph{ &font.glyphs[characterIndex], lines.back().length, scaling, tint, font.texture });

                        // Determine character width after scaling & add to line length.
                        f32 characterWidth = font.glyphs[characterIndex].size.x * scaling.x;
                        lines.back().length += characterWidth;

                        ++beginIndex;
                    }

                    // Reset word length.
                    wordLength = 0.0f;
                }; 

                // Gets set to true if we go out of the height of the rect.
                bool verticalOverflow = false;
                // Iterate over this component's string.
                for (; str[currentIndex] != '\0'; ++currentIndex) {
                    char   character      = str[currentIndex];
                    size_t characterIndex = static_cast<size_t>(character) - static_cast<size_t>(start);

                    // If character is a new line character, add a new line and go to next character.
                    if (character == '\n') {
                        // But first, we need to flush the most recent word if it exists.
                        flushWordToLine();

                        totalHeight += lines.back().height;
                        lines.emplace_back(DrawableLine{ 0.0f, height, std::vector<DrawableGlyph>() });

                        continue;
                    }

                    // If character is unsupported, skip.
                    if (character < start || character > end ||
                            !font.glyphs[characterIndex].supported) continue;

                    // Determine character width after scaling.
                    f32 characterWidth = font.glyphs[characterIndex].size.x * scaling.x;

                    // For characters on which we may break a line, flush the word so far
                    // prematurely so that the breakable character can be handled correctly.
                    if (character == ' ' || character == '-') {
                        flushWordToLine();
                    }

                    // Given we are about to add a character, make sure it fits on the line, if not, make
                    // a new line and if the about-to-be-added character isn't a whitespace revisit it.
                    if (lines.back().length + wordLength + characterWidth > rect.z) {
                        totalHeight += lines.back().height;
                        lines.emplace_back(DrawableLine{ 0.0f, height, std::vector<DrawableGlyph>() });

                        // Skip whitespace at start of new line.
                        if (str[beginIndex] == ' ') {
                            ++beginIndex;
                        }
                        continue;
                    }

                    // If the line's height is less than the height of this font instance, and we're about to add a
                    // glyph from this font instance, then the line's height needs setting to the font instances's.
                    if (lines.back().height < height) {
                        // If we have overflowed the rectangle with the new line height, break out instead.
                        if (totalHeight + height > rect.w) {
                            verticalOverflow = true;
                            break;
                        }
                        lines.back().height = height;
                    }

                    wordLength += characterWidth;
                }

                // If we have overflown vertically, break out of outer loop.
                if (verticalOverflow) break;

                // Flush any remaining word to line.
                flushWordToLine();
            }
            // Update the total height for last line.
            totalHeight += lines.back().height;

            f32 currentY = 0.0f;
            for (auto& line : lines) {
                f32v2 offsets = calculateOffset(align, rect, totalHeight, line.length);

                for (auto& drawable : line.drawables) {
                    f32v2 size         = drawable.glyph->size * drawable.scaling;
                    f32v2 position     = f32v2(drawable.xPos, currentY) + offsets + f32v2(rect.x, rect.y) + f32v2(0.0f, line.height - size.y);
                    f32v4 uvDimensions = drawable.glyph->uvDimensions;

                    f32v2 oldSize = size;

                    clip(rect, position, size, uvDimensions);

                    // Reject any character that even slightly clips with the bounding rectangle.
                    if (oldSize == size) {
                        batcher->draw(drawable.texture, position, size, drawable.tint,
                                        { 255, 255, 255, 255 }, Gradient::NONE, depth, uvDimensions);
                    } else {
                        continue;
                    }
                }

                currentY += line.height;
            }
        }



        /******************************************************\
         * Minimum Raggedness Wrap Draw                       *
        \******************************************************/

        /**
         * @brief Data needed for each word in a text.
         */
        struct Word {
            ui64 start   : 31;
            ui64 end     : 31;
            ui64 newline :  1;
            ui64 hyphen  :  1;
            f32  length;
        };

        /**
         * @brief Draws a string with greedy wrapping.
         *
         * @param batcher The sprite batcher to draw the string to.
         * @param components The string components to draw.
         * @param rect The bounding rectangle the string must be kept within.
         * @param align The alignment for the text.
         * @param depth The depth at which to render the string.
         */
        // inline void drawMinRagWrapString(SpriteBatcher* batcher, StringComponents components, f32v4 rect, TextAlign align, f32 depth) {
        //     std::vector<std::vector<Word>> words;
        //     words.resize(components.size());

        //     // Split the string of each component into their costituent words in preparation for calculating the appropriate
        //     // construction of lines to minimise the raggedness.
        //     for (size_t i = 0; i < words.size(); ++i) {
        //         auto& component = components[i];

        //         // Simplify some component property names.
        //         const char*  str    = component.first;
        //         FontInstance font   = component.second.fontInstance;
        //         StringSizing sizing = component.second.sizing;

        //         char start = font.owner->getStart();
        //         char end   = font.owner->getEnd();

        //         // Process sizing into a simple scale factor.
        //         f32v2 scaling;
        //         f32   height;
        //         if (sizing.kind == StringSizingKind::SCALED) {
        //             scaling = sizing.scaling;
        //             height  = static_cast<f32>(font.height) * scaling.y;
        //         } else {
        //             scaling.x = sizing.scaleX;
        //             scaling.y = sizing.targetHeight / static_cast<f32>(font.height);
        //             height    = sizing.targetHeight;
        //         }

        //         size_t beginIndex   = 0;
        //         size_t currentIndex = 0;

        //         // Estabilsh the Word object we will use to building each
        //         // word in the component string.
        //         Word latest = {
        //             0,
        //             0,
        //             0,
        //             0,
        //             0.0f
        //         };

        //         // Simple function to reset the latest Word object.
        //         auto resetLatest = [&]() {
        //             latest = {
        //                 currentIndex + 1,
        //                 currentIndex + 1,
        //                 0,
        //                 0,
        //                 0.0f
        //             };
        //         }

        //         for (; str[currentIndex] != '\0'; ++currentIndex) {
        //             // Determine character at current point in string.
        //             char   character      = str[currentIndex];
        //             size_t characterIndex = static_cast<size_t>(character) - static_cast<size_t>(start);

        //             // If character is unsupported, skip.
        //             if (character < start || character > end ||
        //                     !font.glyphs[characterIndex].supported) continue;

        //             //// For any of the breakable characters, add the current word to the list of words
        //             //// and prepare to accept a get the next word.
        //             // If we are going to a new line, mark the next word as such.
        //             if (character == '\n') {
        //                 words[i].push_back(latest);

        //                 resetLatest();

        //                 latest.newline = 1;

        //                 continue;
        //             // If we are dealing with a hyphen, mark the next word as being hyphenated.
        //             } else if (character == '-') {
        //                 words[i].push_back(latest);

        //                 resetLatest();

        //                 latest.hyphen = 1;

        //                 size_t index   = static_cast<size_t>('-') - static_cast<size_t>(start);
        //                 latest.length += font.glyphs[index].size.x * scaling.x;

        //                 continue;
        //             // If we are just dealing with a new word, don't do anything extra.
        //             } else if (character == ' ') {
        //                 words[i].push_back(latest);

        //                 resetLatest();

        //                 continue;
        //             }

        //             // Increment end index of word.
        //             ++latest.end;
        //             // Determine character width after scaling and increment word length by it.
        //             latest.length += font.glyphs[characterIndex].size.x * scaling.x;
        //         }

        //         //
        //     }

        //     std::vector<std::vector<f32>> displacements;
        //     // TODO(Matthew): Can we make guesses as to the amount of drawables to reserve for a line? For amount of lines?


        // }



        //     // We will populate these data points for drawing later.
        //     DrawableLines lines;
        //     f32 totalHeight = 0.0f;

        //     // Place the first line.
        //     lines.emplace_back(DrawableLine{0.0f, 0.0f, std::vector<DrawableGlyph>()});

        //     for (auto& component : components) {
        //         // Simplify property names.
        //         const char*  str    = component.first;
        //         FontInstance font   = component.second.fontInstance;
        //         StringSizing sizing = component.second.sizing;
        //         colour4      tint   = component.second.tint;

        //         char start = font.owner->getStart();
        //         char end   = font.owner->getEnd();

        //         // Process sizing into a simple scale factor.
        //         f32v2 scaling;
        //         f32   height;
        //         if (sizing.kind == StringSizingKind::SCALED) {
        //             scaling    = sizing.scaling;
        //             height = static_cast<f32>(font.height) * scaling.y;
        //         } else {
        //             scaling.x  = sizing.scaleX;
        //             scaling.y  = sizing.targetHeight / static_cast<f32>(font.height);
        //             height = sizing.targetHeight;
        //         }

        //         // We have to do per-word lookahead before adding any characters to the lines, these data points
        //         // enable us to do this.
        //         ui32 beginIndex   = 0;
        //         ui32 currentIndex = 0;
        //         f32  wordLength   = 0.0f;

        //         // Useful functor to flush current word to line.
        //         auto flushWordToLine = [&]() {
        //             while (beginIndex != currentIndex) {
        //                 // Get glyph index of character at string index.
        //                 char   character      = str[beginIndex];
        //                 size_t characterIndex = static_cast<size_t>(character) - static_cast<size_t>(start);

        //                 // Add character to line for drawing.
        //                 lines.back().drawables.emplace_back(DrawableGlyph{ &font.glyphs[characterIndex], lines.back().length, scaling, tint, font.texture });

        //                 // Determine character width after scaling & add to line length.
        //                 f32 characterWidth = font.glyphs[characterIndex].size.x * scaling.x;
        //                 lines.back().length += characterWidth;

        //                 ++beginIndex;
        //             }

        //             // Reset word length.
        //             wordLength = 0.0f;
        //         };

        //         // Gets set to true if we go out of the height of the rect.
        //         bool verticalOverflow = false;
        //         // Iterate over this component's string.
        //         for (; str[currentIndex] != '\0'; ++currentIndex) {
        //             char   character      = str[currentIndex];
        //             size_t characterIndex = static_cast<size_t>(character) - static_cast<size_t>(start);

        //             // If character is a new line character, add a new line and go to next character.
        //             if (character == '\n') {
        //                 // But first, we need to flush the most recent word if it exists.
        //                 flushWordToLine();

        //                 totalHeight += lines.back().height;
        //                 lines.emplace_back(DrawableLine{ 0.0f, height, std::vector<DrawableGlyph>() });

        //                 continue;
        //             }

        //             // If character is unsupported, skip.
        //             if (character < start || character > end ||
        //                     !font.glyphs[characterIndex].supported) continue;

        //             // Determine character width after scaling.
        //             f32 characterWidth = font.glyphs[characterIndex].size.x * scaling.x;

        //             // For characters on which we may break a line, flush the word so far
        //             // prematurely so that the breakable character can be handled correctly.
        //             if (character == ' ' || character == '-') {
        //                 flushWordToLine();
        //             }

        //             // Given we are about to add a character, make sure it fits on the line, if not, make
        //             // a new line and if the about-to-be-added character isn't a whitespace revisit it.
        //             if (lines.back().length + wordLength + characterWidth > rect.z) {
        //                 totalHeight += lines.back().height;
        //                 lines.emplace_back(DrawableLine{ 0.0f, height, std::vector<DrawableGlyph>() });

        //                 // Skip whitespace at start of new line.
        //                 if (str[beginIndex] == ' ') {
        //                     ++beginIndex;
        //                 }
        //                 continue;
        //             }

        //             // If the line's height is less than the height of this font instance, and we're about to add a
        //             // glyph from this font instance, then the line's height needs setting to the font instances's.
        //             if (lines.back().height < height) {
        //                 // If we have overflowed the rectangle with the new line height, break out instead.
        //                 if (totalHeight + height > rect.w) {
        //                     verticalOverflow = true;
        //                     break;
        //                 }
        //                 lines.back().height = height;
        //             }

        //             wordLength += characterWidth;
        //         }

        //         // If we have overflown vertically, break out of outer loop.
        //         if (verticalOverflow) break;

        //         // Flush any remaining word to line.
        //         flushWordToLine();
        //     }
        //     // Update the total height for last line.
        //     totalHeight += lines.back().height;

        //     f32 currentY = 0.0f;
        //     for (auto& line : lines) {
        //         f32v2 offsets = calculateOffset(align, rect, totalHeight, line.length);

        //         for (auto& drawable : line.drawables) {
        //             f32v2 size         = drawable.glyph->size * drawable.scaling;
        //             f32v2 position     = f32v2(drawable.xPos, currentY) + offsets + f32v2(rect.x, rect.y) + f32v2(0.0f, line.height - size.y);
        //             f32v4 uvDimensions = drawable.glyph->uvDimensions;

        //             f32v2 oldSize = size;

        //             clip(rect, position, size, uvDimensions);

        //             // Reject any character that even slightly clips with the bounding rectangle.
        //             if (oldSize == size) {
        //                 batcher->draw(drawable.texture, position, size, drawable.tint,
        //                                 { 255, 255, 255, 255 }, Gradient::NONE, depth, uvDimensions);
        //             } else {
        //                 continue;
        //             }
        //         }

        //         currentY += line.height;
        //     }
        // }
    }
}

#endif // !SP_Graphics_StringDrawers_h__
