/* CryHTML5 - for licensing and copyright see license.txt */

#include <platform.h>
#include <ICryPak.h>
#include <PMUtils.hpp>

#include <cef_scheme.h>
#include <include/wrapper/cef_stream_resource_handler.h>

/** @brief Implementation of the resource handler for client requests. */
class CEFCryPakResourceHandler : public CefResourceHandler
{
    public:
        FILE* m_fHandle; //!< file handle inside CryPak
        string m_sPath; //!< file path
        string m_sExtension; //!< file extension
        string m_sMime; //!< mime type
        size_t m_nSize; //!< file size
        size_t m_nOffset; //!< current position inside of file

        CEFCryPakResourceHandler()
        {
            m_fHandle = NULL;
            m_sExtension = "html";
            m_nSize = 0;
            m_nOffset = 0;
        }

        /**
        * @brief Process Request
        * @param request request
        * @param callback callback
        */
        virtual bool ProcessRequest( CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback ) OVERRIDE
        {
            // Evaluate request to determine proper handling
            bool handled = false;

            // CryPak uses UTF-8
            string sPath = PluginManager::UCS22UTF8( request->GetURL().ToWString().c_str() );

            // Get path
            size_t nOffset = sPath.find_first_of( '/' ) + 2;
            m_sPath = sPath.Mid( nOffset ).Trim();

            // Get extension
            nOffset = m_sPath.find_last_of( '.' ) + 1;
            m_sExtension = m_sPath.Mid( nOffset, 3 ).Trim().MakeLower();

            // Get Mime (TODO: Later use CefGetMimeType -> requires update) // https://code.google.com/p/chromiumembedded/source/detail?r=1577
            m_sMime = "text/html"; // default

            if ( m_sExtension == "png" )
            {
                m_sMime = "image/png";
            }

            else if ( m_sExtension == "jpg" || m_sExtension == "jpe" )
            {
                m_sMime = "image/jpeg";
            }

            else if ( m_sExtension == "bmp" )
            {
                m_sMime = "image/bmp";
            }

            else if ( m_sExtension == "js" )
            {
                m_sMime = "application/javascript";
            }

            // Open File
            if ( ( m_fHandle = gEnv->pCryPak->FOpen( m_sPath, "rb" ) ) == NULL )
            {
                HTML5Plugin::gPlugin->LogWarning( "ProcessReques(%s) Unable to find specified path in pak", m_sPath.c_str() );
            }

            else {
                m_nSize = gEnv->pCryPak->FGetSize( m_fHandle );

                HTML5Plugin::gPlugin->LogAlways( "ProcessReques(%s) Success Ext(%s) Mime(%s) Size(%ld)", m_sPath.c_str(), m_sExtension.c_str(), m_sMime.c_str(), m_nSize );

                // Indicate the headers are available.
                callback->Continue();
                return true; // Return true to handle the request.
            }

            return false;
        }

        /**
        * @brief get standard response header
        * @param response response
        * @param response_length response_length
        * @param redirectUrl redirectUrl
        */
        virtual void GetResponseHeaders( CefRefPtr<CefResponse> response, int64& response_length, CefString& redirectUrl ) OVERRIDE
        {
            if(m_fHandle)
            {
                // Populate the response headers.
                response->SetStatus( 200 ); // OK
                response->SetMimeType( m_sMime.c_str() );

                // Specify the resulting response length.
                response_length = m_nSize;
            } else {
                response->SetStatus( 404 ); // not found
                response->SetMimeType( m_sMime.c_str() );

                // Specify the resulting response length.
                response_length = 0;
            }

        }

        virtual void Cancel() OVERRIDE
        {
            if ( m_fHandle )
            {
                gEnv->pCryPak->FClose( m_fHandle );
                m_fHandle = NULL;
            }
        }

        /**
        * @brief read data from disk
        * @param data_out data_out
        * @param bytes_to_read bytes_to_read
        * @param bytes_read bytes_read
        * @param callback callback
        * @return data left?
        */
        virtual bool ReadResponse( void* data_out, int bytes_to_read, int& bytes_read, CefRefPtr<CefCallback> callback ) OVERRIDE
        {
            // Read up to |bytes_to_read| data into |data_out| and set |bytes_read|.
            // If data isn't immediately available set bytes_read=0 and execute
            // |callback| asynchronously.
            // Return true to continue the request or false to complete the request.
            bool has_data = false;
            bytes_read = 0;

            if ( m_nOffset < m_nSize && data_out && m_fHandle )
            {
                // Copy the next block of data into the buffer.
                int transfer_size = min( bytes_to_read, static_cast<int>( m_nSize - m_nOffset ) );

                // Read from pack
                transfer_size = gEnv->pCryPak->FReadRaw( data_out, 1, transfer_size, m_fHandle );

                // Save offset
                m_nOffset += transfer_size;

                // Success
                bytes_read = transfer_size;
                has_data = true; // m_nOffset < m_nSize ?
            }

            // Close File Handle
            if ( !has_data )
            {
                Cancel();
            }

            return has_data;
        }

    private:
        IMPLEMENT_REFCOUNTING( CEFCryPakResourceHandler );
};

/** @brief Implementation of the factory for creating client request handlers. */
class CEFCryPakHandlerFactory : public CefSchemeHandlerFactory
{
    public:
        virtual CefRefPtr<CefResourceHandler> Create( CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& scheme_name, CefRefPtr<CefRequest> request ) OVERRIDE
        {
            // Return a new resource handler instance to handle the request.
            return new CEFCryPakResourceHandler();

            /*
            // Create a stream reader for |html_content|.
            CefRefPtr<CefStreamReader> stream =
                CefStreamReader::CreateForData(
                static_cast<void*>(const_cast<char*>(html_content.c_str())),
                html_content.size());

            // Constructor for HTTP status code 200 and no custom response headers.
            // There’s also a version of the constructor for custom status code and response headers.
            return new CefStreamResourceHandler("text/html", stream);
            */
        }

        IMPLEMENT_REFCOUNTING( CEFCryPakHandlerFactory );
};
