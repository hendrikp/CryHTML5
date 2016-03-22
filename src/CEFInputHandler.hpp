/* CryHTML5 - for licensing and copyright see license.txt */

#include <Game.h>
#include <IHardwareMouse.h>

#pragma once

#include <cef_app.h>
#include <cef_client.h>

#include <CEFInputHandler.hpp>

/** @brief CryENGINE CEF input handler class */
class CEFCryInputHandler :
    public ISystemEventListener,
    public IInputEventListener,
    public IHardwareMouseEventListener,
    public IGameFrameworkListener
{
    private:
        /** input type */
        enum eCryIputType
        {
            Position = 0,
            Click,
            DoubleClick,
            Scroll,
            Key,
            TextChar,
            LostFocus,
            GotFocus
        };

        /** @brief structure to hold input information */
        struct SCryInputEvent
        {
            eCryIputType type; //!< the input type
            union _data
            {
                int x; // pos
                int amount; // scroll
                int pressed; // clicks, keys
            } d;

            union _data2
            {
                int y; // pos
                int code; // keys, mbuttons
            } d2;

            int modifiers; //!< key modefiers
            int key; //!< the key itself

            SCryInputEvent( eCryIputType t, int _d, int _d2 )
            {
                type = t;
                d.x = _d;
                d2.y = _d2;
                modifiers = 0;
                key = 0;
            };

            SCryInputEvent( eCryIputType t, int _d, int _d2, int m, int k )
            {
                type = t;
                d.x = _d;
                d2.y = _d2;
                modifiers = m;
                key = k;
            };

            SCryInputEvent( const SInputEvent& ev );
        };

        std::queue<SCryInputEvent*> m_qEvents; //!< all queued input events

        float m_scaleEmulation; //!< scale for analog pad emuolation
        float m_xEmulation; //!< emulation x pos
        float m_yEmulation; //!< emulation y pos
        float m_xPosition; //!< hardware x pos
        float m_yPosition; //!< hardware y pos
        bool m_leftMB; //!< true when left mouse button pressed
        bool m_middleMB; //!< true when middle mouse button pressed
        bool m_rightMB; //!< true when rigth mouse button pressed
        
        int m_nMode; //!< the current input mode
        bool m_bExclusive; //!< exlusive input listener?

    public:
        CEFCryInputHandler() :
            m_scaleEmulation( 500 ), // max 500 px per second when using controller mouse emulation
            m_xEmulation( 0.0 ),
            m_yEmulation( 0.0 ),
            m_xPosition( 0.0 ),
            m_yPosition( 0.0 ),
            m_leftMB( false ),
            m_middleMB( false ),
            m_rightMB( false ),
            m_bExclusive( true ),
            m_nMode( 0 )
        {
            gEnv->pGame->GetIGameFramework()->RegisterListener( this, HTML5Plugin::gPlugin->GetName(), FRAMEWORKLISTENERPRIORITY_DEFAULT );
            gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener( this );
            gEnv->pHardwareMouse->AddListener( this );
            gEnv->pSystem->GetIInput()->AddEventListener( this );
        }

        void SetInputMode( int nMode, bool bExclusive )
        {
            m_bExclusive = bExclusive;

            if ( nMode != 3 && m_nMode == 3 )
            {
                gEnv->pHardwareMouse->DecrementCounter();
            }

            else if ( nMode == 3 && m_nMode != 3 )
            {
                gEnv->pHardwareMouse->IncrementCounter();
            }

            m_nMode = nMode;
        }

        void UnregisterListeners()
        {
            SetInputMode( 0, false );

            gEnv->pGame->GetIGameFramework()->UnregisterListener( this );
            gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener( this );
            gEnv->pSystem->GetIInput()->RemoveEventListener( this );
            gEnv->pHardwareMouse->RemoveListener( this );
        }

        // IInputEventListener
        virtual bool OnInputEventUI( const SInputEvent& event )
        {
            return OnInputEvent( event );
        }

        virtual void OnHardwareMouseEvent( int iX, int iY, EHARDWAREMOUSEEVENT eHardwareMouseEvent, int wheelDelta = 0 )
        {
            if ( HTML5Plugin::gPlugin->cm5_active == 0.0f )
            {
                return;
            }

            gEnv->pHardwareMouse->GetHardwareMouseClientPosition( &m_xPosition, &m_yPosition );

            if ( m_nMode == 3 )
            {
                m_qEvents.push( new SCryInputEvent( Position, m_xPosition, m_yPosition ) );

                switch ( eHardwareMouseEvent )
                {
                    case HARDWAREMOUSEEVENT_LBUTTONDOWN:
                        m_qEvents.push( new SCryInputEvent( Click, eIS_Pressed, eKI_Mouse1 ) );
                        break;

                    case HARDWAREMOUSEEVENT_LBUTTONUP:
                        m_qEvents.push( new SCryInputEvent( Click, eIS_Released, eKI_Mouse1 ) );
                        break;

                    case HARDWAREMOUSEEVENT_LBUTTONDOUBLECLICK:
                        m_qEvents.push( new SCryInputEvent( DoubleClick, eIS_Pressed, eKI_Mouse1 ) );
                        break;

                    case HARDWAREMOUSEEVENT_RBUTTONDOWN:
                        m_qEvents.push( new SCryInputEvent( Click, eIS_Pressed, eKI_Mouse2 ) );
                        break;

                    case HARDWAREMOUSEEVENT_RBUTTONUP:
                        m_qEvents.push( new SCryInputEvent( Click, eIS_Released, eKI_Mouse2 ) );
                        break;

                    case HARDWAREMOUSEEVENT_RBUTTONDOUBLECLICK:
                        m_qEvents.push( new SCryInputEvent( DoubleClick, eIS_Pressed, eKI_Mouse2 ) );
                        break;

                    case HARDWAREMOUSEEVENT_MBUTTONDOWN:
                        m_qEvents.push( new SCryInputEvent( Click, eIS_Pressed, eKI_Mouse3 ) );
                        break;

                    case HARDWAREMOUSEEVENT_MBUTTONUP:
                        m_qEvents.push( new SCryInputEvent( Click, eIS_Released, eKI_Mouse3 ) );
                        break;

                    case HARDWAREMOUSEEVENT_MBUTTONDOUBLECLICK:
                        m_qEvents.push( new SCryInputEvent( DoubleClick, eIS_Pressed, eKI_Mouse3 ) );
                        break;

                    case HARDWAREMOUSEEVENT_WHEEL:
                        m_qEvents.push( new SCryInputEvent( Scroll, wheelDelta, 0 ) );
                        break;
                }
            }
        }

        /**
        * @brief Convert CryENGINE input event to generalized input event
        * @param inputEvent inputEvent
        * @param event event
        * @param bChar if text input
        * @return true when printable character
        */
        bool MapKeyEvent( const SInputEvent& inputEvent, SCryInputEvent& event, bool bChar )
        {
            event.key = gEnv->pInput->GetInputCharAscii( inputEvent );

            // Requires printable character
            if ( bChar )
            {
                // We only have ACP atm
                char temp[2];
                temp[0] = event.key;
                temp[1] = 0;

                // ACP to UCS-2
                event.key = PluginManager::UTF82UCS2( PluginManager::ACP2UTF8( temp ) )[0];

                // Only UI Events support real UCS-2
                if ( inputEvent.state == eIS_UI )
                {
                    event.key = inputEvent.inputChar;
                }
            }

            event.d2.code = event.key;

            event.modifiers = EVENTFLAG_NONE;
            event.modifiers |= ( ( inputEvent.modifiers & eMM_Shift ) != 0 ) ? EVENTFLAG_SHIFT_DOWN : 0;
            event.modifiers |= ( ( inputEvent.modifiers & eMM_Ctrl ) != 0 ) ? EVENTFLAG_CONTROL_DOWN : 0;
            event.modifiers |= ( ( inputEvent.modifiers & eMM_Alt ) != 0 ) ?  EVENTFLAG_ALT_DOWN : 0;
            event.modifiers |= ( ( inputEvent.modifiers & eMM_CapsLock ) != 0 ) ? EVENTFLAG_CAPS_LOCK_ON : 0;
            event.modifiers |= ( ( inputEvent.modifiers & eMM_NumLock ) != 0 ) ? EVENTFLAG_NUM_LOCK_ON : 0;

            event.modifiers |= ( ( inputEvent.modifiers & eMM_LShift ) || ( inputEvent.modifiers & eMM_LCtrl ) || ( inputEvent.modifiers & eMM_LAlt ) ) ? EVENTFLAG_IS_LEFT : 0;
            event.modifiers |= ( ( inputEvent.modifiers & eMM_RShift ) || ( inputEvent.modifiers & eMM_RCtrl ) || ( inputEvent.modifiers & eMM_RAlt ) ) ? EVENTFLAG_IS_RIGHT : 0;

            bool bPrintable = false;

            switch ( EKeyId( inputEvent.keyId ) )
            {
                case eKI_Backspace:
                    event.key = VK_BACK;
                    break;

                case eKI_Tab:
                    event.key = VK_TAB;
                    bPrintable = true;
                    break;

                case eKI_Enter:
                    event.key = VK_RETURN;
                    bPrintable = true;
                    break;

                case eKI_LShift:
                    event.key = VK_LSHIFT;
                    break;

                case eKI_RShift:
                    event.key = VK_RSHIFT;
                    break;

                case eKI_LCtrl:
                    event.key = VK_LCONTROL;
                    break;

                case eKI_RCtrl:
                    event.key = VK_RCONTROL;
                    break;

                case eKI_LAlt:
                    event.key = VK_LMENU;
                    break;

                case eKI_RAlt:
                    event.key = VK_RMENU;
                    break;

                case eKI_Escape:
                    event.key = VK_ESCAPE;
                    break;

                case eKI_Left:
                    event.key = VK_LEFT;
                    break;

                case eKI_Up:
                    event.key = VK_UP;
                    break;

                case eKI_Right:
                    event.key = VK_RIGHT;
                    break;

                case eKI_Down:
                    event.key = VK_DOWN;
                    break;

                case eKI_Insert:
                    event.key = VK_INSERT;
                    break;

                case eKI_Delete:
                    event.key = VK_DELETE;
                    break;

                default:
                    bPrintable = true;
            }

            if ( !bChar )
            {
                event.d2.code = event.key;
            }

            return bPrintable;
        }

        virtual bool OnInputEvent( const SInputEvent& ev )
        {
            if ( gEnv->pConsole->GetStatus() ) // disable UI inputs when console is open
            {
                return false;
            }

            if ( HTML5Plugin::gPlugin->cm5_active == 0.0f )
            {
                return false;
            }

            bool bRet = m_bExclusive;

            if ( m_nMode >= 2 && ( ev.deviceId == eDI_Mouse || ev.deviceId == eDI_XI ) )
            {
                switch ( ev.keyId )
                {
                    case eKI_MouseX:
                        m_xEmulation = 0;

                        if ( gEnv->pHardwareMouse )
                        {
                            gEnv->pHardwareMouse->GetHardwareMouseClientPosition( &m_xPosition, &m_yPosition );
                        }

                        m_xPosition += ev.value;

                        m_qEvents.push( new SCryInputEvent( Position, m_xPosition, m_yPosition ) );
                        break;

                    case eKI_XI_ThumbLX: // XBOX Controller

                        m_xEmulation = ev.value;
                        break;

                    case eKI_MouseY:
                        m_yEmulation = 0;

                        if ( gEnv->pHardwareMouse )
                        {
                            gEnv->pHardwareMouse->GetHardwareMouseClientPosition( &m_xPosition, &m_yPosition );
                        }

                        m_yPosition += ev.value;

                        m_qEvents.push( new SCryInputEvent( Position, m_xPosition, m_yPosition ) );
                        break;

                    case eKI_XI_ThumbLY: // XBOX Controller
                        m_yEmulation = -1 * ev.value; // invert for emulation
                        break;

                    case eKI_Mouse1:
                    case eKI_XI_X:
                    case eKI_XI_A:
                    case eKI_XI_ThumbL:
                        if ( ev.state == eIS_Released || ev.state == eIS_Pressed )
                        {
                            m_qEvents.push( new SCryInputEvent( Click, ev.state, eKI_Mouse1 ) );
                        }

                        break;

                    case eKI_Mouse2:
                    case eKI_XI_B:
                    case eKI_XI_ThumbR:
                        if ( ev.state == eIS_Released || ev.state == eIS_Pressed )
                        {
                            m_qEvents.push( new SCryInputEvent( Click, ev.state, eKI_Mouse2 ) );
                        }

                        break;

                    case eKI_XI_Y:
                    case eKI_Mouse3:
                        if ( ev.state == eIS_Released || ev.state == eIS_Pressed )
                        {
                            m_qEvents.push( new SCryInputEvent( Click, ev.state, eKI_Mouse3 ) );
                        }

                        break;

                    case eKI_XI_DPadUp:
                    case eKI_MouseWheelUp:
                        if ( ev.state == eIS_Pressed )
                        {
                            m_qEvents.push( new SCryInputEvent( Scroll, 50, 0 ) );
                        }

                        break;

                    case eKI_XI_DPadDown:
                    case eKI_MouseWheelDown:
                        if ( ev.state == eIS_Pressed )
                        {
                            m_qEvents.push( new SCryInputEvent( Scroll, -50, 0 ) );
                        }

                        break;
                }
            }

            else if ( m_nMode >= 1 && ev.deviceId == eDI_Keyboard )
            {
                if ( ev.state == eIS_Released || ev.state == eIS_Pressed )
                {
                    SCryInputEvent* eva = new SCryInputEvent( Key, ev.state, 0 );
                    MapKeyEvent( ev, *eva, false );
                    m_qEvents.push( eva );

                    if ( ev.state == eIS_Pressed )
                    {
                        eva = new SCryInputEvent( TextChar, ev.state, 0 );

                        if ( MapKeyEvent( ev, *eva, true ) )
                        {
                            m_qEvents.push( eva );
                        }

                        else
                        {
                            delete eva;
                        }
                    }
                }
            }

            return bRet;
        }

        /**
        * @brief Scales the mouse position to screen size
        * @param[in,out] mouse mouse
        */
        void ScaleMouse( CefMouseEvent& mouse )
        {
            float x = 0.f, y = 0.f;
            HTML5Plugin::gPlugin->ScaleCoordinates( mouse.x, mouse.y, x, y, true, true );
            mouse.x = x;
            mouse.y = y;
        }

        /**
        * @brief push all queued input events to CEF
        * @param frame the browser to push the events too
        */
        void GetInput( CefRefPtr<CefFrame> frame )
        {
            float frameTime = gEnv->pTimer->GetFrameTime( ITimer::ETimer::ETIMER_UI );

            if ( fabsf( m_xEmulation ) > 0.01f || fabsf( m_yEmulation ) > 0.01f )
            {
                float cx = m_xEmulation * m_scaleEmulation * frameTime;
                float cy = m_yEmulation * m_scaleEmulation * frameTime;

                if ( gEnv->pHardwareMouse )
                {
                    gEnv->pHardwareMouse->GetHardwareMouseClientPosition( &m_xPosition, &m_yPosition );
                }

                m_xPosition += cx;
                m_yPosition += cy;

                if ( gEnv->pHardwareMouse )
                {
                    gEnv->pHardwareMouse->SetHardwareMouseClientPosition( m_xPosition, m_yPosition );
                }

                m_qEvents.push( new SCryInputEvent( Position, m_xPosition, m_yPosition ) );
            }

            CefMouseEvent mouse;
            mouse.x = m_xPosition;
            mouse.y = m_yPosition;

            mouse.modifiers = EVENTFLAG_NONE;
            mouse.modifiers |= m_leftMB ? EVENTFLAG_LEFT_MOUSE_BUTTON : 0;
            mouse.modifiers |= m_middleMB ? EVENTFLAG_MIDDLE_MOUSE_BUTTON : 0;
            mouse.modifiers |= m_rightMB ? EVENTFLAG_RIGHT_MOUSE_BUTTON : 0;

            ScaleMouse( mouse );

            CefRefPtr<CefBrowserHost> bh = frame->GetBrowser()->GetHost();

            CefKeyEvent cefKey;

            // Inject events
            while ( !m_qEvents.empty() )
            {
                SCryInputEvent& item = *m_qEvents.front();

                switch ( item.type )
                {
                    case Position:
                        mouse.x = item.d.x;
                        mouse.y = item.d2.y;
                        ScaleMouse( mouse );
                        //HTML5Plugin::gPlugin->LogAlways( "Move: %d, %d", mouse.x, mouse.y );

                        bh->SendMouseMoveEvent( mouse, false );
                        break;

                    case Click:
                        switch ( item.d2.code )
                        {
                            case eKI_Mouse1:
                                m_leftMB = item.d.pressed == eIS_Pressed;

                                mouse.modifiers = EVENTFLAG_NONE;
                                mouse.modifiers |= m_leftMB ? EVENTFLAG_LEFT_MOUSE_BUTTON : 0;
                                mouse.modifiers |= m_middleMB ? EVENTFLAG_MIDDLE_MOUSE_BUTTON : 0;
                                mouse.modifiers |= m_rightMB ? EVENTFLAG_RIGHT_MOUSE_BUTTON : 0;

                                bh->SendMouseClickEvent( mouse, MBT_LEFT, !m_leftMB, 1 );
                                break;

                            case eKI_Mouse2:
                                m_middleMB = item.d.pressed == eIS_Pressed;

                                mouse.modifiers = EVENTFLAG_NONE;
                                mouse.modifiers |= m_leftMB ? EVENTFLAG_LEFT_MOUSE_BUTTON : 0;
                                mouse.modifiers |= m_middleMB ? EVENTFLAG_MIDDLE_MOUSE_BUTTON : 0;
                                mouse.modifiers |= m_rightMB ? EVENTFLAG_RIGHT_MOUSE_BUTTON : 0;

                                bh->SendMouseClickEvent( mouse, MBT_RIGHT, !m_middleMB, 1 );
                                break;

                            case eKI_Mouse3:
                                m_rightMB = item.d.pressed == eIS_Pressed;

                                mouse.modifiers = EVENTFLAG_NONE;
                                mouse.modifiers |= m_leftMB ? EVENTFLAG_LEFT_MOUSE_BUTTON : 0;
                                mouse.modifiers |= m_middleMB ? EVENTFLAG_MIDDLE_MOUSE_BUTTON : 0;
                                mouse.modifiers |= m_rightMB ? EVENTFLAG_RIGHT_MOUSE_BUTTON : 0;

                                bh->SendMouseClickEvent( mouse, MBT_MIDDLE, !m_rightMB, 1 );
                                break;
                        }

                        //HTML5Plugin::gPlugin->LogAlways( "Click: %d, %d", mouse.x, mouse.y );

                        break;

                    case DoubleClick:
                        switch ( item.d2.code )
                        {
                            case eKI_Mouse1:
                                bh->SendMouseClickEvent( mouse, MBT_LEFT, true, 2 );
                                break;

                            case eKI_Mouse2:
                                bh->SendMouseClickEvent( mouse, MBT_RIGHT, true, 2 );
                                break;

                            case eKI_Mouse3:
                                bh->SendMouseClickEvent( mouse, MBT_MIDDLE, true, 2 );
                                break;
                        }

                        break;

                    case Scroll:
                        bh->SendMouseWheelEvent( mouse, 0, item.d.amount );
                        break;

                    case Key:
                        cefKey.type = item.d.pressed == eIS_Pressed ? KEYEVENT_KEYDOWN : KEYEVENT_KEYUP;

                        cefKey.native_key_code = cefKey.windows_key_code = item.key;
                        cefKey.character = cefKey.unmodified_character = item.d2.code;

                        bh->SendKeyEvent( cefKey );
                        break;

                    case TextChar:
                        cefKey.type = KEYEVENT_CHAR;

                        cefKey.native_key_code = cefKey.windows_key_code = item.key;
                        cefKey.character = cefKey.unmodified_character = item.d2.code;

                        bh->SendKeyEvent( cefKey );
                        break;

                    case LostFocus:
                        bh->SetFocus( false );
                        bh->SendFocusEvent( false );

                        m_leftMB = false;
                        m_middleMB = false;
                        m_rightMB = false;
                        break;

                    case GotFocus:
                        bh->SetFocus( true );
                        bh->SendFocusEvent( true );
                        break;
                }

                m_qEvents.pop();
            }
        }

        // ISystemEventListener
        virtual void OnSystemEvent( ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam )
        {
            switch ( event )
            {
                // Lost Focus?
                case ESYSTEM_EVENT_ACTIVATE:
                    if ( !wparam )
                    {
                        m_qEvents.push( new SCryInputEvent( LostFocus, 0, 0 ) );
                    }

                    break;

                case ESYSTEM_EVENT_CHANGE_FOCUS: // wparam is not 0 is focused, 0 if not focused
                    m_qEvents.push( new SCryInputEvent( wparam ? GotFocus : LostFocus, 0, 0 ) );
                    break;
            }
        }

        virtual void OnPostUpdate( float fDeltaTime )
        {
            if ( HTML5Plugin::gPlugin->cm5_active == 0.0f )
            {
                return;
            }

            GetInput( HTML5Plugin::gPlugin->m_refCEFFrame );
        }

        virtual void OnSaveGame( ISaveGame* pSaveGame )
        { }

        virtual void OnLoadGame( ILoadGame* pLoadGame )
        { }

        virtual void OnLevelEnd( const char* nextLevel )
        { }

        virtual void OnActionEvent( const SActionEvent& event )
        { }

        virtual void GetCursorPos( float& fX, float& fY )
        {
            fX = m_xPosition;
            fY = m_yPosition;
        }
};
