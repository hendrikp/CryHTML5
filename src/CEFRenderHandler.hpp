/* CryHTML5 - for licensing and copyright see license.txt */

#pragma once

#include <cef_app.h>
#include <cef_client.h>
#include <cef_process_util.h>
#include <cef_runnable.h>

#include <CPluginHTML5.h>
#include <FullscreenTriangleDrawer.h>

#include <d3d11.h>

//#define USE_MAPPED // define when using mapped memory (not performant)

//#include "../GameSDK/GameDll/MyFiles/Camera/MeasureTime.hpp"

#define TEXTURE_FLAGS FILTER_LINEAR | FT_DONT_STREAM | FT_NOMIPS // texture flags

/** @brief CryENGINE & Direct3D renderer handler */
class CEFCryRenderHandler : public CefRenderHandler, public D3DPlugin::ID3DEventListener
{
    public:
        int _windowHeight; //!< the frame height in pixels
        int _windowWidth; //!< the frame width in pixels

    private:
        const void* _buffer; //!< the CEF frame buffer (not synchronized to reduce overhead)

        ID3D11Texture2D* _texture; //!< the Direct3D 11 texture
        ITexture* _itexture;  //!< the CryENGINE texture
        ID3D11ShaderResourceView* _srv; //!< the Direct3D 11 texture resource

        HTML5Plugin::CFullscreenTriangleDrawer _triangledrawer; //!< the draw helper

        int _dx; //!< dirtyrect start x
        int _dy; //!< dirtyrect start y
        int _dy2; //!< dirtyrect end x
        int _dx2; //!< dirtyrect end y

    private:
        /**
        * @brief reset dirty rect
        */
        void resetdirty()
        {
            _dx = 10000;
            _dy = 10000;
            _dy2 = 0;
            _dx2 = 0;
        }

        /**
        * @brief increase dirty rect area
        */
        void dirtyarea( int x, int y, int w, int h )
        {
            int x2 = x + w;
            int y2 = y + h;

            _dx = min( _dx, x );
            _dy = min( _dy, y );
            _dx2 = max( _dx2, x2 );
            _dy2 = max( _dy2, y2 );
        }

    public:
        /** @brief see interface */
        virtual void ScaleCoordinates( float fX, float fY, float& foX, float& foY, bool bLimit = false, bool bCERenderer = true )
        {
            //HTML5Plugin::gPlugin->LogAlways( "1: X %f Y %f", fX, fY );

            int x, y, width, height;
            gEnv->pRenderer->GetViewport( &x, &y, &width, &height );

            // make relative for scaling
            if ( bCERenderer )
            {
                fX = fX / ( f32 )width;
                fY = fY / ( f32 )height;
            }

            //HTML5Plugin::gPlugin->LogAlways( "2: X %.2f %d Y %.2f %d XY %d %d", fX, width, fY, height );

            // overflow
            if ( bLimit )
            {
                fX = clamp_tpl( fX, 0.0f, 1.0f );
                fY = clamp_tpl( fY, 0.0f, 1.0f );
            }

            //HTML5Plugin::gPlugin->LogAlways( "3: X %f Y %f", fX, fY );

            // finally transform into CEF/texture coordinates
            foX = fX * _windowWidth;
            foY = fY * _windowHeight;

            //HTML5Plugin::gPlugin->LogAlways( "4: X %f Y %f", foX, foY );
        }

        // D3DPlugin::ID3DEventListener
        virtual void OnPostPresent() {};
        virtual void OnPreReset() {};
        virtual void OnPostReset() {};
        virtual void OnPostBeginScene() {};

        virtual void OnPrePresent()
        {
//#ifdef _DEBUG
//            profileFunc( "html5_dx_measurement", [&]()
//            {
//#endif

                if ( HTML5Plugin::gPlugin->cm5_active == 0.0f )
                {
                    return;
                }

                // Create resources when they dont exist yet
                if ( !_texture )
                {
                    CreateResources();
                }

                // When something to update exists
                if ( _texture && _buffer && _srv && _dx2 > 0 && _dy2 > 0 )
                {
                    UpdateResources();
                }

                // When something to draw exists
                if ( _srv )
                {
                    _triangledrawer.Draw( _srv );
                }

//#ifdef _DEBUG
//            },
//            HTML5Plugin::gPlugin->cm5_active == 2 );
//#endif
        };

        /** @brief update the CryENGINE texture based on the last CEF frame */
        void UpdateResources()
        {
            ID3D11Device* pDevice = static_cast<ID3D11Device*>( HTML5Plugin::gD3DSystem->GetDevice() );

            ID3D11Texture2D* pTexture = static_cast<ID3D11Texture2D*>( _texture );

            ID3D11DeviceContext* pContext = NULL;
            pDevice->GetImmediateContext( &pContext );

            const int bytesPerRow = _windowWidth * 4;

#ifdef USE_MAPPED
            D3D11_MAPPED_SUBRESOURCE mapped;
            HRESULT hr = pContext->Map( pTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped ); // D3D11_MAP_WRITE doesn't work so we will not update all so don't use USE_MAPPED

            if ( FAILED( hr ) )
            {
                HTML5Plugin::gPlugin->LogAlways( "Fail Map hr=%d", hr );
            }

            else
            {
                int h = _dy2 - dy;
                int w = _dx2 - dx;

                const int _pitchDst = mapped.RowPitch + _dx * 4;
                int offsetDst = mapped.RowPitch * _dy;
                char* pDest = static_cast<char*>( mapped.pData );
                const char* pSrc = ( const char* )_buffer;

                const size_t size = _windowWidth * _windowHeight * 4;

                int offsetSrc = bytesPerRow * _dy;

                const int _bytesRow = w * 4;
                const int _pitchSrc = bytesPerRow + _dx * 4;

                if ( _windowWidth * 4 == mapped.RowPitch && h == _windowHeight && w == _windowWidth )
                {
                    ::memcpy( pDest, pSrc, size );
                }

                else
                {
                    pSrc += _dy * bytesPerRow +  _dx * 4;
                    pDest += _dy * mapped.RowPitch + _dx * 4;

                    for ( int row = _dy; row < _dy2; ++row )
                    {
                        ::memcpy( pDest, pSrc, _bytesRow );
                        pSrc += _pitchSrc;
                        pDest += _pitchDst;
                    }
                }

                pContext->Unmap( pTexture, 0 );

                resetdirty();
            }

#else
            D3D11_BOX box = {0}; // http://msdn.microsoft.com/en-us/library/windows/desktop/ff476486%28v=vs.85%29.aspx
            box.front = 0;
            box.back = 1;

            box.left = _dx;
            box.right = _dx2;
            box.top = _dy;
            box.bottom = _dy2;

            //HTML5Plugin::gPlugin->LogAlways( " Update x(%d) y(%d), r(%d) b(%d)", box.left, box.top, box.right, box.bottom );

            resetdirty();

            const char* pSrc = ( const char* )_buffer;
            pSrc += box.top * bytesPerRow + box.left * 4;

            pContext->UpdateSubresource( pTexture, 0, &box, ( void* )pSrc, bytesPerRow, 0 );
#endif
        }

        /** @brief Creates the CryENGINE and Direct3D resource */
        void CreateResources( )
        {
            ID3D11Device* pDevice = static_cast<ID3D11Device*>( HTML5Plugin::gD3DSystem->GetDevice() );

#ifdef USE_MAPPED
            _itexture = HTML5Plugin::gD3DSystem->CreateTexture( ( void** )&_texture, _windowWidth, _windowHeight, 1,  eTF_X8R8G8B8, FT_USAGE_DYNAMIC | TEXTURE_FLAGS );
#else
            _itexture = HTML5Plugin::gD3DSystem->CreateTexture( ( void** )&_texture, _windowWidth, _windowHeight, 1,  eTF_X8R8G8B8, TEXTURE_FLAGS ); //FT_USAGE_RENDERTARGET?
#endif

            HTML5Plugin::gPlugin->LogAlways( "CreateTexture: %p, %p", _texture, _itexture );

            if ( _texture )
            {
                D3D11_TEXTURE2D_DESC texdesc = {0};
                _texture->GetDesc( &texdesc );

                D3D11_SHADER_RESOURCE_VIEW_DESC srvdesc;
                ZeroMemory( &srvdesc, sizeof( D3D11_SHADER_RESOURCE_VIEW_DESC ) );

                srvdesc.Format = texdesc.Format;
                srvdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                srvdesc.Texture2D.MipLevels = texdesc.MipLevels;
                srvdesc.Texture2D.MostDetailedMip = texdesc.MipLevels - 1;

                HRESULT hr = pDevice->CreateShaderResourceView( _texture, &srvdesc, &_srv );

                if ( SUCCEEDED( hr ) )
                {
                    HTML5Plugin::gPlugin->LogAlways( "CreateShaderResourceView: %p", _srv );
                }

                else
                {
                    HTML5Plugin::gPlugin->LogAlways( "Fail CreateShaderResourceView" );
                }
            }
        }

    public:
        CEFCryRenderHandler( int windowWidth, int windowHeight )
        {
            _windowWidth = windowWidth;
            _windowHeight = windowHeight;

            resetdirty();
            _buffer = nullptr;
            _texture = nullptr;
            _itexture = nullptr;
            _srv = nullptr;
        }

        /** @brief get pixel color at position */
        virtual ColorB GetPixel( int x, int y )
        {
            // check if on surface
            if ( x < 0 || y < 0 || x >= _windowWidth || y >= _windowHeight )
            {
                return ColorB( 0, 0, 0, 0 );
            }

            // get pixel position in buffer
            size_t nPos = _windowWidth * y + x;
            uint8* pPos = &( ( uint8* )_buffer )[nPos];

            // CEF uses RGBA order
            return ColorB( pPos[0], pPos[1], pPos[2], pPos[3] );
        }

        virtual bool GetRootScreenRect( CefRefPtr<CefBrowser> browser, CefRect& rect )
        {
            return GetViewRect( browser, rect );
        }

        virtual bool GetScreenPoint( CefRefPtr<CefBrowser> browser, int viewX, int viewY, int& screenX, int& screenY )
        {
            screenX = viewX;
            screenY = viewY;
            return true;
        }

        virtual bool GetViewRect( CefRefPtr<CefBrowser> browser, CefRect& rect )
        {
            rect.x = 0;
            rect.y = 0;
            rect.width = _windowWidth;
            rect.height = _windowHeight;
            return true;
        }

        virtual bool GetScreenInfo( CefRefPtr<CefBrowser> browser, CefScreenInfo& screen_info )
        {
            return false;
        }

        virtual void OnPopupSize( CefRefPtr<CefBrowser> browser, const CefRect& rect )
        {
        }

        virtual void OnPaint( CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList& dirtyRects, const void* buffer, int width, int height )
        {
            std::string url = browser->GetMainFrame()->GetURL().ToString();

            // HTML5Plugin::gPlugin->LogAlways( "OnPaint: %s, type(%d), %d, %dm %0x016p", SAFESTR( url.c_str() ), int( type ), width, height, buffer );

            for ( auto iter = dirtyRects.begin(); iter != dirtyRects.end(); ++iter )
            {
                dirtyarea( iter->x, iter->y, iter->width, iter->height );
                //HTML5Plugin::gPlugin->LogAlways( " Dirty x(%d) y(%d), w(%d) h(%d)", iter->x, iter->y, iter->width, iter->height );
            }

            _buffer = buffer;
        }

        virtual void OnCursorChange( CefRefPtr<CefBrowser> browser, CefCursorHandle cursor )
        {
        }

        virtual void OnScrollOffsetChanged( CefRefPtr<CefBrowser> browser )
        {
        }

        virtual void OnPopupShow( CefRefPtr<CefBrowser> browser, bool show )
        {
        }

        IMPLEMENT_REFCOUNTING( CEFCryRenderHandler );
};