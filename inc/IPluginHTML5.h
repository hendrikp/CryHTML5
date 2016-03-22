/* CryHTML5 - for licensing and copyright see license.txt */

#include <IPluginBase.h>

#pragma once

/**
* @brief HTML5 Plugin Namespace
*/
namespace HTML5Plugin
{
    /**
    * @brief HTML5 Plugin concrete interface
    */
    struct IPluginHTML5
    {
        /**
        * @brief Get Plugin base interface
        */
        virtual PluginManager::IPluginBase* GetBase() = 0;

        /**
        * @brief set the url of the frame that should be rendered
        * @param sURL the url of the website
        * @return true if successful
        */
        virtual bool SetURL( const wchar_t* sURL ) = 0;

        /**
        * @brief execute java script code in the current browser frame
        * @param sJS the java script code
        * @return true if successful
        */
        virtual bool ExecuteJS( const wchar_t* sJS ) = 0;

        /**
        * @brief project a world position onto the screen
        * @param cam the screens camera (onto which the coordinates should be projected)
        * @param vWorld the world position
        * @param[out] vScreen the screen position (the z-coordinate is the distance from the screen)
        * @param vOffset offset in world space
        * @return true if successful
        */
        virtual bool WorldPosToScreenPos( CCamera cam, Vec3 vWorld, Vec3& vScreen, Vec3 vOffset = Vec3( ZERO ) ) = 0;

        /**
        * @brief scale the coordinates in screen space
        * @param fX the horizontal position in renderer/relative scale
        * @param fY the vertical position in renderer/relative scale
        * @param[out] foX the horizontal position in screen space (in pixels)
        * @param[out] foY the vertical position in screen space (in pixels)
        * @param bLimit limit the output coordinates to the screen area
        * @param bCERenderer use renderer or relative scale
        */
        virtual void ScaleCoordinates( float fX, float fY, float& foX, float& foY, bool bLimit = false, bool bCERenderer = true ) = 0;

        /**
        * @brief set input mode
        * @param nMode 0 No Input
        *              1 Only Keyboard Input
        *              2 Additional Mouse + Controller Input
        *              3 Activate also Hardware Mouse Cursor
        * @param bExclusive Exclusive Receiver
        */
        virtual void SetInputMode( int nMode = 0, bool bExclusive = false ) = 0;

        /**
        * @brief set the plugin active (rendering and input handling)
        * @param bActivate set true to activate
        */
        virtual void SetActive( bool bActive ) = 0;

        /**
        * @brief check if cursor is on HTML5 surface or CryEngine renderer surface (Alpha-test implementation)
        * @return true when cursor position is opaque
        */
        virtual bool IsCursorOnSurface() = 0;

        /**
        * @brief check if position is opaque on HTML5 surface
        * @param fX the horizontal position in screen space (in pixels)
        * @param fY the vertical position in screen space (in pixels)
        * @return true when position is opaque
        */
        virtual bool IsOpaque( float fX, float fY ) = 0;
    };
};