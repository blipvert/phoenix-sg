/** 
 * @file llwindowmacosx-objc.mm
 * @brief Definition of functions shared between llwindowmacosx.cpp
 * and llwindowmacosx-objc.mm.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include <AppKit/AppKit.h>
#include <Accelerate/Accelerate.h>
#include <Quartz/Quartz.h>

/*
 * These functions are broken out into a separate file because the
 * objective-C typedef for 'BOOL' conflicts with the one in
 * llcommon/stdtypes.h.  This makes it impossible to use the standard
 * linden headers with any objective-C++ source.
 */

#include "llwindowmacosx-objc.h"
#include "lldir.h"

#if 0
// This version of the code is specific to OS X 10.5 and above due to the use
//  of CGDataProviderCopyData(). The 10.4 version of this routine is below.
//  When 10.4 is no longer supported, that version can be ripped out.
BOOL decodeImageQuartz(const UInt8* data, int len, LLImageRaw *raw_image, std::string ext)
{
	CFDataRef theData = CFDataCreate(kCFAllocatorDefault, data, len);
	CGImageSourceRef srcRef = CGImageSourceCreateWithData(theData, NULL);
	CGImageRef image_ref = CGImageSourceCreateImageAtIndex(srcRef, 0, NULL);

	size_t width = CGImageGetWidth(image_ref);
	size_t height = CGImageGetHeight(image_ref);
	size_t comps = CGImageGetBitsPerPixel(image_ref) / 8;
	size_t bytes_per_row = CGImageGetBytesPerRow(image_ref);
	CFDataRef result = CGDataProviderCopyData(CGImageGetDataProvider(image_ref));
	UInt8* bitmap = (UInt8*)CFDataGetBytePtr(result);

	CGImageAlphaInfo format = CGImageGetAlphaInfo(image_ref);
	if (comps == 4)
	{
		vImage_Buffer vb;
		vb.data = bitmap;
		vb.height = height;
		vb.width = width;
		vb.rowBytes = bytes_per_row;

		if (format == kCGImageAlphaPremultipliedFirst)
		{
			// Ele: Skip unpremultiplication for PSD, PNG and TGA files
			if (ext != std::string("psd") && ext != std::string("tga") && ext != std::string("png"))
				vImageUnpremultiplyData_ARGB8888(&vb, &vb, 0);
		}
		else if (format == kCGImageAlphaPremultipliedLast)
		{
			// Ele: Photoshop Native Transparency needs unmultiplication
			vImageUnpremultiplyData_RGBA8888(&vb, &vb, 0);
		}
	}

	raw_image->resize(width, height, comps);
	memcpy(raw_image->getData(), bitmap, height * bytes_per_row);
	raw_image->verticalFlip();
	CFRelease(theData);
	CGImageRelease(image_ref);
	CFRelease(result);
	return TRUE;
}
#else
// This version works on OS X 10.4. The code is adapted from Apple Technical
//  Q&A 1509, found at
//  http://developer.apple.com/library/mac/#qa/qa2007/qa1509.html .
// Yeah, all this code is ugly. Ugly but working beats clean and broken any
//  day. -- TS
BOOL decodeImageQuartz(const UInt8* data, int len, LLImageRaw *raw_image, std::string ext)
{
	CFDataRef theData = CFDataCreate(kCFAllocatorDefault, data, len);
	CGImageSourceRef srcRef = CGImageSourceCreateWithData(theData, NULL);
	CGImageRef image_ref = CGImageSourceCreateImageAtIndex(srcRef, 0, NULL);
	CGImageAlphaInfo format = CGImageGetAlphaInfo(image_ref);
	CGColorSpaceRef colorSpace = NULL;
	CMProfileRef sysprof = NULL;

	size_t width = CGImageGetWidth(image_ref);
	size_t height = CGImageGetHeight(image_ref);
	size_t comps = CGImageGetBitsPerPixel(image_ref) / 8;
	size_t bytes_per_row = width * 4; // bytes per output row, not input
	size_t byte_count = bytes_per_row * height;

	llinfos << "Image extension='" << ext << "' height=" << height <<
		" width=" << width << " components=" << comps <<
		" bytes per row=" << bytes_per_row << " total output size=" <<
		byte_count << " format=" << format << llendl;

	// Get the Systems Profile for the main display
	if (CMGetSystemProfile(&sysprof) == noErr)
	{
		// Create a colorspace with the systems profile
		colorSpace = CGColorSpaceCreateWithPlatformColorSpace(sysprof);

		// Close the profile
		CMCloseProfile(sysprof);
	}
	else
	{
		llwarns << "Unable to get system profile for the main display" << llendl;
		CFRelease(theData);
		CFRelease(srcRef);
		CGImageRelease(image_ref);
		return FALSE;
	}

	if (colorSpace == NULL)
	{
		llwarns << "Error allocating color space" << llendl;
		CFRelease(theData);
		CFRelease(srcRef);
		CGImageRelease(image_ref);
		return FALSE;
	}

	// Allocate memory for image data. This is the destination in memory
	// where any drawing to the bitmap context will be rendered.
	void *bitmapData = malloc(byte_count);
	if (bitmapData == NULL) 
	{
		llwarns << "Bitmap memory not allocated" << llendl;
		CGColorSpaceRelease( colorSpace );
		CFRelease(theData);
		CFRelease(srcRef);
		CGImageRelease(image_ref);
		return FALSE;
	}

	// Create the bitmap context. If the input has alpha, we want
	//  pre-multiplied ARGB, 8-bits per component. If not, we want no
	//  alpha here either. Regardless of what the source image format is 
	//  (CMYK, Grayscale, and so on) it will be converted over to the
	//  format specified here by CGBitmapContextCreate.
	CGContextRef cgctx = CGBitmapContextCreate (bitmapData,
        				width,
					height,
					8,      // bits per component
					bytes_per_row,
					colorSpace,
					(format==kCGImageAlphaNone ?
					kCGImageAlphaNoneSkipFirst :
					kCGImageAlphaPremultipliedFirst) |
						kCGBitmapByteOrder32Host);


	if (cgctx == NULL) 
	{ 
		// error creating context
		llwarns << "Context not created!" << llendl;
		free (bitmapData);
		CGColorSpaceRelease( colorSpace );
		CFRelease(theData);
		CFRelease(srcRef);
		CGImageRelease(image_ref);
		return FALSE;
	}
	CGRect rect = {{0,0},{width,height}};
	CGContextDrawImage(cgctx, rect, image_ref);

	// We're finished with the color space, so release it no matter what.
	CGColorSpaceRelease( colorSpace );

	UInt8* bitmap = (UInt8*)CGBitmapContextGetData(cgctx);
	if (bitmap == NULL) 
	{ 
		// error getting the bitmap data
		CGContextRelease(cgctx);
		CFRelease(theData);
		CFRelease(srcRef);
		CGImageRelease(image_ref);
		return FALSE;
	}

	// The image has R and B reversed. Don't ask me how. Byte order
	//  differences are a pain to deal with. -- TS
	for (int i=0; i<height; i++)
	{
		for (int j=0; j<bytes_per_row; j+=4)
		{
			unsigned char tmp[4];

			tmp[0] = bitmap[j+(i*bytes_per_row)+2];
			tmp[1] = bitmap[j+(i*bytes_per_row)+1];
			tmp[2] = bitmap[j+(i*bytes_per_row)];
			tmp[3] = bitmap[j+(i*bytes_per_row)+3];

			memcpy(&bitmap[j+(i*bytes_per_row)], &tmp, comps);
		}
	}

	// If the original image had an alpha channel, unpremultiply it. We
	//  created it as ARGB8888, so we use that unpremultiply regardless
	//  of what the original image was. The exception is Photoshop:
	//  even PSDs with native transparency don't need unpremultipication,
	//  for reasons I'm not entirely sure of.
	if (comps == 4)
	{
		vImage_Buffer vb;
		vb.data = bitmap;
		vb.height = height;
		vb.width = width;
		vb.rowBytes = bytes_per_row;
		if (((ext != std::string("psd")) && (ext != std::string("tga")) && 
			(ext != std::string("png")) &&
			(format != kCGImageAlphaNoneSkipLast)))
		{
			llinfos << "Unpremultiplying ARGB8888" << llendl;
			vImageUnpremultiplyData_ARGB8888(&vb, &vb, 0);
		}
	}

	raw_image->resize(width, height, 4);
	memcpy(raw_image->getData(), bitmap, byte_count);
	raw_image->verticalFlip();
	CGContextRelease(cgctx);
	free(bitmap);
	CFRelease(theData);
	CGImageRelease(image_ref);
	return TRUE;
}
#endif

BOOL decodeImageQuartz(std::string filename, LLImageRaw *raw_image)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSURL *url = [[NSURL alloc] initFileURLWithPath:[NSString stringWithCString:filename.c_str()]];
	NSData *data = [NSData dataWithContentsOfURL:url];
	std::string ext = gDirUtilp->getExtension(filename);
	BOOL result = decodeImageQuartz((UInt8*)[data bytes], [data length], raw_image, ext);
	[pool release];
	return result;
}

void setupCocoa()
{
	static bool inited = false;
	
	if(!inited)
	{
		NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
		
		// This is a bit of voodoo taken from the Apple sample code "CarbonCocoa_PictureCursor":
		//   http://developer.apple.com/samplecode/CarbonCocoa_PictureCursor/index.html
		
		//	Needed for Carbon based applications which call into Cocoa
		NSApplicationLoad();

		//	Must first call [[[NSWindow alloc] init] release] to get the NSWindow machinery set up so that NSCursor can use a window to cache the cursor image
		[[[NSWindow alloc] init] release];

		[pool release];
	}
}

CursorRef createImageCursor(const char *fullpath, int hotspotX, int hotspotY)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	// extra retain on the NSCursor since we want it to live for the lifetime of the app.
	NSCursor *cursor =
		[[[NSCursor alloc] 
				initWithImage:
					[[[NSImage alloc] initWithContentsOfFile:
						[NSString stringWithFormat:@"%s", fullpath]
					]autorelease] 
				hotSpot:NSMakePoint(hotspotX, hotspotY)
		]retain];	
		
	[pool release];
	
	return (CursorRef)cursor;
}

// This is currently unused, since we want all our cursors to persist for the life of the app, but I've included it for completeness.
OSErr releaseImageCursor(CursorRef ref)
{
	if( ref != NULL )
	{
		NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
		NSCursor *cursor = (NSCursor*)ref;
		[cursor release];
		[pool release];
	}
	else
	{
		return paramErr;
	}
	
	return noErr;
}

OSErr setImageCursor(CursorRef ref)
{
	if( ref != NULL )
	{
		NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
		NSCursor *cursor = (NSCursor*)ref;
		[cursor set];
		[pool release];
	}
	else
	{
		return paramErr;
	}
	
	return noErr;
}

