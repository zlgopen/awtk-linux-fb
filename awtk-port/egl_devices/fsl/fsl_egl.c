/****************************************************************************
* Copyright (c) 2012 Freescale Semiconductor, Inc.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without 
* modification, are permitted provided that the following conditions are met:
*    
*    * Redistributions of source code must retain the above copyright notice,
*		this list of conditions and the following disclaimer.
*    
*    * Redistributions in binary form must reproduce the above copyright notice, 
*		this list of conditions and the following disclaimer in the documentation 
*		and/or other materials provided with the distribution.
*
* 	 * Neither the name of the Freescale Semiconductor, Inc. nor the names of 
*		its contributors may be used to endorse or promote products derived from 
*		this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Labels parameters     

*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <EGL/egl.h>

#ifdef EGL_USE_X11
#include <X11/X.h>
#include <X11/Xlib.h>
#endif

EGLNativeDisplayType fsl_getNativeDisplay()
{
	EGLNativeDisplayType eglNativeDisplayType = NULL;
#if (defined EGL_USE_X11)
	eglNativeDisplayType = XOpenDisplay(NULL);
	assert(eglNativeDisplayType != NULL);
#elif (defined EGL_API_FB)
	eglNativeDisplayType = fbGetDisplayByIndex(0); //Pass the argument as required to show the framebuffer	
#else	
	display = EGL_DEFAULT_DISPLAY;
#endif	
	return eglNativeDisplayType;
}

EGLNativeWindowType fsl_createwindow(EGLDisplay egldisplay, EGLNativeDisplayType eglNativeDisplayType, const char* filename)
{
	EGLNativeWindowType native_window = (EGLNativeWindowType)0;

#if (defined EGL_USE_X11)
	Window window, rootwindow;
	int screen = DefaultScreen(eglNativeDisplayType);		
	rootwindow = RootWindow(eglNativeDisplayType,screen); 
	window = XCreateSimpleWindow(eglNativeDisplayType, rootwindow, 0, 0, 400, 533, 0, 0, WhitePixel (eglNativeDisplayType, screen));
	XMapWindow(eglNativeDisplayType, window);
	native_window = window;	
#else
	const char *vendor = eglQueryString(egldisplay, EGL_VENDOR);
	printf("vendor:%s \r\n", vendor);
	if (strstr(vendor, "Imagination Technologies"))
		native_window = (EGLNativeWindowType)0;
	else if (strstr(vendor, "AMD"))
		native_window = (EGLNativeWindowType)  open(filename, O_RDWR);
	else if (strstr(vendor, "Vivante")) //NEEDS FIX - functs don't exist on other platforms
	{
#if (defined EGL_API_FB)
		native_window = fbCreateWindow(eglNativeDisplayType, 0, 0, 0, 0);
#endif		
	}
	else
	{
		printf("Unknown vendor [%s]\n", vendor);
		return 0;
	}	
#endif
	return native_window;

}


void fsl_destroywindow(EGLNativeWindowType eglNativeWindowType, EGLNativeDisplayType eglNativeDisplayType)
{
	(void) eglNativeWindowType;
#if (defined EGL_USE_X11)
	//close x display		
	XCloseDisplay(eglNativeDisplayType);
#endif
}
