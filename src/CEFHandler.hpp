/* CryHTML5 - for licensing and copyright see license.txt */

#pragma once

#include <cef_app.h>
#include <cef_client.h>

#include <CPluginHTML5.h>
#include <CEFRenderHandler.hpp>

#include <CEFInputHandler.hpp>

/** @brief handle loading of web pages */
class CEFCryLoadHandler : public CefLoadHandler
{
    public:
        virtual void OnLoadingStateChange( CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward )
        {
            std::string url = browser->GetMainFrame()->GetURL().ToString().c_str();
            HTML5Plugin::gPlugin->LogAlways( "LoadingStateChange: %s", SAFESTR( url.c_str() ) );
        }

        virtual void OnLoadStart( CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame )
        {
            // A single CefBrowser instance can handle multiple requests for a single URL if there are frames (i.e. <FRAME>, <IFRAME>).
            //if ( frame->IsMain() )

            std::string url = browser->GetMainFrame()->GetURL().ToString().c_str();
            HTML5Plugin::gPlugin->LogAlways( "LoadStart: %s", SAFESTR( url.c_str() ) );
        }

        virtual void OnLoadEnd( CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode )
        {

            std::string url = browser->GetMainFrame()->GetURL().ToString().c_str();
            HTML5Plugin::gPlugin->LogAlways( "LoadEnd: %s, %d", SAFESTR( url.c_str() ), httpStatusCode );
        }

        virtual void OnLoadError( CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString& errorText, const CefString& failedUrl )
        {
            std::string url = browser->GetMainFrame()->GetURL().ToString();
            std::string furl = failedUrl.ToString();
            std::string err = errorText.ToString();
            HTML5Plugin::gPlugin->LogError( "LoadError: %s, %s, %d, %s", SAFESTR( url.c_str() ), SAFESTR( furl.c_str() ), int( errorCode ), SAFESTR( err.c_str() ) );
        }

        IMPLEMENT_REFCOUNTING( CEFCryLoadHandler );
};

/** @brief central CEF handler class per browser */
class CEFCryHandler : public CefClient, public CefLifeSpanHandler, public CefContextMenuHandler, public CefDialogHandler, public CefJSDialogHandler
{
    private:
        CefRefPtr<CEFCryLoadHandler> _loadHandler; //!< the load handler

    public:
        CefRefPtr<CEFCryRenderHandler> _renderHandler; //!< the renderer handler
        CEFCryInputHandler m_input; //!< the input handler

    public:
        CEFCryHandler( int windowWidth, int windowHeight )
        {
            _renderHandler = new CEFCryRenderHandler( windowWidth, windowHeight );
            _loadHandler = new CEFCryLoadHandler();
        };

        ~CEFCryHandler() { };

#pragma warning(disable: 4927)

        // CefClient methods.
        virtual CefRefPtr<CefLoadHandler> GetLoadHandler()
        {
            return _loadHandler;
        }

        virtual CefRefPtr<CefRenderHandler> GetRenderHandler()
        {
            return _renderHandler;
        }

        // CefContextMenuHandler
        virtual CefRefPtr<CefContextMenuHandler> GetContextMenuHandler()
        {
            return this;
        }

        virtual void OnBeforeContextMenu( CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model )
        {
            model->Clear(); // Never show context menus
        }

        // CefDialog Handler
        virtual CefRefPtr<CefDialogHandler> GetDialogHandler()
        {
            return this;
        }

        virtual bool OnFileDialog( CefRefPtr<CefBrowser> browser, FileDialogMode mode, const CefString& title, const CefString& default_file_name, const std::vector<CefString>& accept_types, CefRefPtr<CefFileDialogCallback> callback )
        {
            callback->Cancel();
            return true;
        }

        // CefJSDialogHandler
        virtual CefRefPtr<CefJSDialogHandler> GetJSDialogHandler()
        {
            return this;
        }

        virtual bool OnJSDialog( CefRefPtr<CefBrowser> browser, const CefString& origin_url, const CefString& accept_lang, JSDialogType dialog_type, const CefString& message_text, const CefString& default_prompt_text, CefRefPtr<CefJSDialogCallback> callback, bool& suppress_message )
        {
            string text = message_text.ToString().c_str();
            HTML5Plugin::gPlugin->LogAlways( "JSDialog: %s", text.c_str() );

            // suppress javascript messages
            suppress_message = true;
            return false;
        }

        // CefLifeSpanHandler
        virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler()
        {
            return this;
        }

        virtual void OnAfterCreated( CefRefPtr<CefBrowser> browser )
        {
            if ( HTML5Plugin::gPlugin->m_sCEFDebugURL.empty() )
            {
                HTML5Plugin::gPlugin->m_refCEFFrame = browser->GetMainFrame(); // remember frame

                HTML5Plugin::gPlugin->m_sCEFDebugURL = browser->GetHost()->GetDevToolsURL( false ).ToString().c_str(); // browser->GetHost()->GetDevToolsURL( false ).ToString();
                HTML5Plugin::gPlugin->LogAlways( "Devtools URL: %s", HTML5Plugin::gPlugin->m_sCEFDebugURL.c_str() );

                HTML5Plugin::gPlugin->ShowDevTools();
            }
        };

        virtual void OnBeforeClose( CefRefPtr<CefBrowser> browser )
        {
            if ( HTML5Plugin::gPlugin->m_refCEFFrame.get() == nullptr || HTML5Plugin::gPlugin->m_refCEFFrame->GetBrowser()->GetIdentifier() == browser->GetIdentifier() )
            {
                HTML5Plugin::gPlugin->m_sCEFDebugURL = "";
                HTML5Plugin::gPlugin->m_refCEFFrame = nullptr;
            }
        };

        virtual bool OnBeforePopup( CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& target_url, const CefString& target_frame_name, const CefPopupFeatures& popupFeatures, CefWindowInfo& windowInfo, CefRefPtr<CefClient>& client, CefBrowserSettings& settings, bool* no_javascript_access )
        {
            return true; // Never allow Popups
        }

        IMPLEMENT_REFCOUNTING( CEFCryHandler );

};

