/* CryHTML5 - for licensing and copyright see license.txt */

#pragma once

struct IDirect3DTexture9;
struct IDirect3DVertexDeclaration9;
struct IDirect3DVertexBuffer9;
struct IDirect3DVertexShader9;
struct IDirect3DPixelShader9;
struct IDirect3DStateBlock9;
struct ID3D11ShaderResourceView;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11BlendState;

namespace HTML5Plugin
{

    class CFullscreenTriangleDrawer
    {
        public:
            CFullscreenTriangleDrawer();
            ~CFullscreenTriangleDrawer();

            void Draw( void* pTexture );

        private:
            void CreateDX9Resources();
            void CreateDX11Resources();

            void DrawDX9( IDirect3DTexture9* pTexture );
            void DrawDX11( ID3D11ShaderResourceView* pTexture );

        private:
            // DX9
            IDirect3DVertexDeclaration9* m_pVertexDeclaration;
            IDirect3DVertexBuffer9* m_pVertexBuffer;
            IDirect3DVertexShader9* m_pVertexShader9;
            IDirect3DPixelShader9* m_pPixelShader9;
            IDirect3DStateBlock9* m_pStateBlock;

            // DX11
            ID3D11VertexShader* m_pVertexShader11;
            ID3D11PixelShader* m_pPixelShader11;
            ID3D11BlendState* m_pBlendState11;
    };

}
