/*
 Copyright (C) 2010-2013 Kristian Duske
 
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
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __TrenchBroom__MapView__
#define __TrenchBroom__MapView__

#include "Color.h"
#include "Controller/Command.h"
#include "GL/GL.h"
#include "Renderer/Camera.h"
#include "Renderer/Compass.h"
#include "Renderer/MapRenderer.h"
#include "Renderer/RenderResources.h"
#include "Renderer/Vbo.h"
#include "View/InputState.h"
#include "View/MovementRestriction.h"
#include "View/ViewTypes.h"

#include <vector>
#include <wx/event.h>
#include <wx/glcanvas.h>

namespace TrenchBroom {
    class Logger;
    
    namespace Renderer {
        class RenderContext;
    }
    
    namespace View {
        class BaseTool;
        class CameraTool;
        class ClipTool;
        class CreateBrushTool;
        class ResizeBrushesTool;
        class SelectionTool;
        
        class MapView : public wxGLCanvas {
        private:
            Logger* m_logger;
            bool m_initialized;
            wxGLContext* m_glContext;
            
            Renderer::Vbo m_auxVbo;
            Color m_focusColor;
            
            View::MapDocumentPtr m_document;
            ControllerFacade& m_controller;
            Renderer::Camera m_camera;
            Renderer::RenderResources m_renderResources;
            Renderer::MapRenderer m_renderer;
            Renderer::Compass m_compass;
            
            InputState m_inputState;
            MovementRestriction m_movementRestriction;
            wxPoint m_clickPos;
            CameraTool* m_cameraTool;
            ClipTool* m_clipTool;
            CreateBrushTool* m_createBrushTool;
            ResizeBrushesTool* m_resizeBrushesTool;
            SelectionTool* m_selectionTool;
            BaseTool* m_toolChain;
            BaseTool* m_dragReceiver;
            BaseTool* m_modalReceiver;
            
            bool m_ignoreNextClick;
        public:
            MapView(wxWindow* parent, Logger* logger, View::MapDocumentPtr document, ControllerFacade& controller);
            ~MapView();
            
            Renderer::RenderResources& renderResources();
            
            void toggleClipTool();
            bool clipToolActive() const;
            bool canToggleClipSide() const;
            void toggleClipSide();
            bool canPerformClip() const;
            void performClip();
            void toggleMovementRestriction();
            
            void OnKey(wxKeyEvent& event);
            void OnMouseButton(wxMouseEvent& event);
            void OnMouseMotion(wxMouseEvent& event);
            void OnMouseWheel(wxMouseEvent& event);
            void OnMouseCaptureLost(wxMouseCaptureLostEvent& event);
            void OnSetFocus(wxFocusEvent& event);
            void OnKillFocus(wxFocusEvent& event);
            void OnActivateFrame(wxActivateEvent& event);
            
            void OnPaint(wxPaintEvent& event);
            void OnSize(wxSizeEvent& event);

            void commandDo(Controller::Command::Ptr command);
            void commandDone(Controller::Command::Ptr command);
            void commandDoFailed(Controller::Command::Ptr command);
            void commandUndo(Controller::Command::Ptr command);
            void commandUndone(Controller::Command::Ptr command);
            void commandUndoFailed(Controller::Command::Ptr command);
        private:
            void updatePickResults(const int x, const int y);
            
            void createTools();
            void deleteTools();
            void toggleTool(BaseTool* tool);
            void cancelCurrentDrag();
            ModifierKeyState modifierKeys();
            bool updateModifierKeys();
            bool clearModifierKeys();
            MouseButtonState mouseButton(wxMouseEvent& event);
            void bindEvents();
            
            void setupGL(Renderer::RenderContext& context);
            void setRenderOptions(Renderer::RenderContext& context);
            void clearBackground(Renderer::RenderContext& context);
            void renderCoordinateSystem(Renderer::RenderContext& context);
            void renderMap(Renderer::RenderContext& context);
            void renderTools(Renderer::RenderContext& context);
            void renderCompass(Renderer::RenderContext& context);
            void renderFocusRect(Renderer::RenderContext& context);
            
            void initializeGL();
            
            static const Renderer::RenderResources::GLAttribs& attribs();
            static int depthBits();
            static bool multisample();
        };
    }
}

#endif /* defined(__TrenchBroom__MapView__) */
