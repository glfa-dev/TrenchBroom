/*
 Copyright (C) 2010-2012 Kristian Duske
 
 This file is part of TrenchBroom.
 
 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with TrenchBroom.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "MapParser.h"
#include <assert.h>
#include <cmath>
#include "Controller/ProgressIndicator.h"

namespace TrenchBroom {
    namespace IO {
        char MapTokenizer::nextChar() {
            if (m_state == TS_EOF)
                return 0;
            
            if (m_index == m_chars.size()) {
                m_state = TS_EOF;
                return 0;
            }
            
            char c = m_chars[m_index++];
            if (c == '\n') {
                m_line++;
                m_column = 0;
            } else {
                m_column++;
            }
            
            return c;
        }
        
        char MapTokenizer::peekChar() {
            if (m_state == TS_EOF || m_index == m_chars.size())
                return 0;
            
            char c = m_chars[m_index];
            return c;
        }
        
        MapToken* MapTokenizer::token(ETokenType type, char* data, int index, int line, int column) {
            m_token.type = type;
            if (data != NULL) m_token.data = string(data, index);
            else m_token.data.clear();
            m_token.line = line;
            m_token.column = column;
            m_token.charsRead = m_index;
            return &m_token;
        }
        
        MapTokenizer::MapTokenizer(istream& stream) : m_state(TS_DEF), m_line(1), m_column(1) {
            istreambuf_iterator<char> begin(stream), end;
            m_chars = vector<char>(begin, end);
            m_index = 0;
        }
        
        MapToken* MapTokenizer::next() {
            char c;
            
            while ((c = nextChar()) != 0) {
                switch (m_state) {
                    case TS_DEF:
                        switch (c) {
                            case '/': {
                                char d = peekChar();
                                if (d == '/') {
                                    m_state = TS_COM;
                                    m_bufferIndex = 0;
                                    m_startLine = m_line;
                                    m_startColumn = m_column;
                                    nextChar();
                                    break;
                                }
                            }
                            case '\r':
                            case '\n':
                            case '\t':
                            case ' ':
                                break; // ignore whitespace in boundaries
                            case '{':
                                return token(TT_CB_O, NULL, 0, m_line, m_column);
                            case '}':
                                return token(TT_CB_C, NULL, 0, m_line, m_column);
                            case '(':
                                return token(TT_B_O, NULL, 0, m_line, m_column);
                            case ')':
                                return token(TT_B_C, NULL, 0, m_line, m_column);
                            case '[':
                                return token(TT_SB_O, NULL, 0, m_line, m_column);
                            case ']':
                                return token(TT_CB_C, NULL, 0, m_line, m_column);
                            case '"':
                                m_state = TS_Q_STR;
                                m_bufferIndex = 0;
                                m_startLine = m_line;
                                m_startColumn = m_column;
                                break;
                            case '-':
                            case '0':
                            case '1':
                            case '2':
                            case '3':
                            case '4':
                            case '5':
                            case '6':
                            case '7':
                            case '8':
                            case '9':
                                m_state = TS_DEC;
                                m_bufferIndex = 0;
                                m_buffer[m_bufferIndex++] = c;
                                m_startLine = m_line;
                                m_startColumn = m_column;
                                break;
                            default:
                                m_state = TS_STR;
                                m_bufferIndex = 0;
                                m_buffer[m_bufferIndex++] = c;
                                m_startLine = m_line;
                                m_startColumn = m_column;
                                break;
                        }
                        break;
                    case TS_Q_STR:
                        switch (c) {
                            case '"': {
                                MapToken* tok = token(TT_STR, m_buffer, m_bufferIndex, m_startLine, m_startColumn);
                                m_bufferIndex = 0;
                                m_state = TS_DEF;
                                return tok;
                            }
                            default:
                                m_buffer[m_bufferIndex++] = c;
                                break;
                        }
                        break;
                    case TS_STR: {
                        bool comment = false;
                        switch (c) {
                            case '/': {
                                if (peekChar() == '/') comment = true, nextChar();
                            }
                            case '\r':
                            case '\n':
                            case '\t':
                            case ' ': {
                                MapToken* tok = token(TT_STR, m_buffer, m_bufferIndex, m_startLine, m_startColumn);
                                m_bufferIndex = 0;
                                m_state = comment ? TS_COM : TS_DEF;
                                return tok;
                            }
                            default:
                                m_buffer[m_bufferIndex++] = c;
                                break;
                        }
                        break;
                    }
                    case TS_DEC:
                        if (c == '.') m_state = TS_FRAC;
                    case TS_FRAC: {
                        bool comment = false;
                        switch (c) {
                            case '/':
                                if (peekChar() == '/') comment = true, nextChar();
                            case '\r':
                            case '\n':
                            case '\t':
                            case ' ': {
                                MapToken* tok;
                                if (m_state == TS_DEC) tok = token(TT_DEC, m_buffer, m_bufferIndex, m_startLine, m_startColumn);
                                else tok = token(TT_FRAC, m_buffer, m_bufferIndex, m_startLine, m_startColumn);
                                m_bufferIndex = 0;
                                m_state = comment ? TS_COM : TS_DEF;
                                return tok;
                            }
                            default:
                                if ((c < '0' || c > '9') && (c != '.'))
                                    m_state = TS_STR;
                                m_buffer[m_bufferIndex++] = c;
                                break;
                        }
                        break;
                    }
                    case TS_COM:
                        switch (c) {
                            case '\r':
                            case '\n': {
                                MapToken* tok = token(TT_COM, m_buffer, m_bufferIndex, m_startLine, m_startColumn);
                                m_bufferIndex = 0;
                                m_state = TS_DEF;
                                return tok;
                            }
                            default:
                                m_buffer[m_bufferIndex++] = c;
                                break;
                        }
                        break;
                    default:
                        break;
                }
            }
            
            return NULL;
        }
        
        int MapTokenizer::size() {
            return (int)m_chars.size();
        }

        void MapParser::expect(int expectedType, const MapToken* actualToken) const {
            assert(actualToken != NULL);
            assert((actualToken->type & expectedType) != 0);
        }
        
        MapToken* MapParser::nextToken() {
            MapToken* token = NULL;
            if (!m_tokenStack.empty()) {
                token = m_tokenStack.back();
                m_tokenStack.pop_back();
            } else {
                token = m_tokenizer->next();
                while (token != NULL && token->type == TT_COM)
                    token = m_tokenizer->next();
            }
            
            return token;
        }
        
        void MapParser::pushToken(MapToken* token) {
            m_tokenStack.push_back(token);
        }
        
        MapParser::MapParser(istream& stream, const BBox& worldBounds, Assets::TextureManager& textureManager) : m_worldBounds(worldBounds), m_textureManager(textureManager) {
            streamoff cur = stream.tellg();
            stream.seekg(0, ios::end);
            m_size = (int)stream.tellg();
            m_size -= cur;
            stream.seekg(cur, ios::beg);
            m_tokenizer = new MapTokenizer(stream);
        }
        
        MapParser::~MapParser() {
            delete m_tokenizer;
        }
        
        Map* MapParser::parseMap(const string& entityDefinitionFilePath, Controller::ProgressIndicator* indicator) {
            m_format = MF_UNDEFINED;
            Map* map = new Map(m_worldBounds, entityDefinitionFilePath);
            map->setPostNotifications(false);
            Entity* entity = NULL;
            
            if (indicator != NULL) indicator->reset(m_tokenizer->size());
            while ((entity = parseEntity(indicator)) != NULL) map->addEntity(entity);
            if (indicator != NULL) indicator->update(m_tokenizer->size());
            return map;
        }
        
        Entity* MapParser::parseEntity(Controller::ProgressIndicator* indicator) {
            MapToken* token = nextToken();
            if (token == NULL) return NULL;
            
            expect(TT_CB_O | TT_CB_C, token);
            if (token->type == TT_CB_C) return NULL;
            
            Entity* entity = new Entity();
            entity->setFilePosition(token->line);
            
            while ((token = nextToken()) != NULL) {
                switch (token->type) {
                    case TT_STR: {
                        string key = token->data;
                        token = nextToken();
                        expect(TT_STR, token);
                        string value = token->data;
                        entity->setProperty(key, value);
                        break;
                    }
                    case TT_CB_O: {
                        pushToken(token);
                        Brush* brush = NULL;
                        while ((brush = parseBrush(indicator)) != NULL)
                            entity->addBrush(brush);
                    }
                    case TT_CB_C: {
                        if (indicator != NULL) indicator->update(token->charsRead);
                        return entity;
                    }
                    default:
                        fprintf(stdout, "Warning: Unexpected token type %i at line %i\n", token->type, token->line);
                        return NULL;
                }
            }
            
            return entity;
        }
        
        Brush* MapParser::parseBrush(Controller::ProgressIndicator* indicator) {
            MapToken* token;

            expect(TT_CB_O | TT_CB_C, token = nextToken());
            if (token->type == TT_CB_C) return NULL;
            
            Brush* brush = new Brush(m_worldBounds);
            brush->setFilePosition(token->line);
            
            while ((token = nextToken()) != NULL) {
                switch (token->type) {
                    case TT_B_O: {
                        pushToken(token);
                        Face* face = parseFace();
                        if (face != NULL) brush->addFace(face);
                        break;
                    }
                    case TT_CB_C:
                        if (indicator != NULL) indicator->update(token->charsRead);
                        return brush;
                    default:
                        fprintf(stdout, "Warning: Unexpected token type %i at line %i\n", token->type, token->line);
                        return NULL;
                }
            }
            return NULL;
        }
        
        Face* MapParser::parseFace() {
            Vec3f p1, p2, p3;
            float xOffset, yOffset, rotation, xScale, yScale;
            MapToken* token;
            
            expect(TT_B_O, token = nextToken());
            expect(TT_DEC | TT_FRAC, token = nextToken());
            p1.x = atoi(token->data.c_str());
            expect(TT_DEC | TT_FRAC, token = nextToken());
            p1.y = atoi(token->data.c_str());
            expect(TT_DEC | TT_FRAC, token = nextToken());
            p1.z = atoi(token->data.c_str());
            expect(TT_B_C, token = nextToken());
            expect(TT_B_O, token = nextToken());
            expect(TT_DEC | TT_FRAC, token = nextToken());
            p2.x = atoi(token->data.c_str());
            expect(TT_DEC | TT_FRAC, token = nextToken());
            p2.y = atoi(token->data.c_str());
            expect(TT_DEC | TT_FRAC, token = nextToken());
            p2.z = atoi(token->data.c_str());
            expect(TT_B_C, token = nextToken());
            expect(TT_B_O, token = nextToken());
            expect(TT_DEC | TT_FRAC, token = nextToken());
            p3.x = atoi(token->data.c_str());
            expect(TT_DEC | TT_FRAC, token = nextToken());
            p3.y = atoi(token->data.c_str());
            expect(TT_DEC | TT_FRAC, token = nextToken());
            p3.z = atoi(token->data.c_str());
            expect(TT_B_C, token = nextToken());
            
            expect(TT_STR, token = nextToken());
            Assets::Texture* texture = m_textureManager.texture(token->data);
            
            token = nextToken();
            if (m_format == MF_UNDEFINED) {
                expect(TT_DEC | TT_FRAC | TT_SB_O, token);
                m_format = token->type == TT_DEC ? MF_STANDARD : MF_VALVE;
                if (m_format == MF_VALVE) fprintf(stdout, "Warning: Loading unsupported map Valve 220 map format");
            }
            
            if (m_format == MF_STANDARD) {
                expect(TT_DEC | TT_FRAC, token);
                bool frac = token->type == TT_FRAC;
                xOffset = atof(token->data.c_str());
                expect(TT_DEC | TT_FRAC, token = nextToken());
                yOffset = atof(token->data.c_str());
                if (frac || token->type == TT_FRAC) fprintf(stdout, "Warning: Rounding fractional texture offset in line %i", token->line);
            } else { // Valve 220 format
                expect(TT_SB_O, token);
                expect(TT_DEC | TT_FRAC, token = nextToken()); // X texture axis x
                expect(TT_DEC | TT_FRAC, token = nextToken()); // X texture axis y
                expect(TT_DEC | TT_FRAC, token = nextToken()); // X texture axis z
                expect(TT_DEC | TT_FRAC, token = nextToken()); // X texture axis offset
                xOffset = atof(token->data.c_str());
                expect(TT_SB_C, token = nextToken());
                expect(TT_SB_O, token = nextToken());
                expect(TT_DEC | TT_FRAC, token = nextToken()); // Y texture axis x
                expect(TT_DEC | TT_FRAC, token = nextToken()); // Y texture axis y
                expect(TT_DEC | TT_FRAC, token = nextToken()); // Y texture axis z
                expect(TT_DEC | TT_FRAC, token = nextToken()); // Y texture axis offset
                yOffset = atof(token->data.c_str());
                expect(TT_SB_C, token = nextToken());
            }
            
            expect(TT_DEC | TT_FRAC, token = nextToken());
            rotation = atof(token->data.c_str());
            expect(TT_DEC | TT_FRAC, token = nextToken());
            xScale = atof(token->data.c_str());
            expect(TT_DEC | TT_FRAC, token = nextToken());
            yScale = atof(token->data.c_str());
            
            if (((p3 - p1) % (p2 - p1)).equals(Null3f)) {
                fprintf(stdout, "Warning: Skipping invalid face in line %i", token->line);
                return NULL;
            }
            
            Face* face = new Face(m_worldBounds, p1, p2, p3);
            face->setTexture(texture);
            face->setXOffset((int)roundf(xOffset));
            face->setYOffset((int)roundf(yOffset));
            face->setRotation(rotation);
            face->setXScale(xScale);
            face->setYScale(yScale);
            face->setFilePosition(token->line);
            return face;
        }
    }
}