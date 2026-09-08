/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

// The bgfx side of the presenter. This is the only translation unit that includes bgfx,
// which keeps the library's headers and build settings away from the rest of the engine.

#include "bgfxbackend.h"

#include "dbgprint.h"
#include "except.h"

#include <bx/allocator.h>
#include <bgfx/bgfx.h>
#include <bgfx/embedded_shader.h>

#include <vs_ocornut_imgui.bin.h>
#include <fs_ocornut_imgui.bin.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <malloc.h>


static const bgfx::EmbeddedShader _EmbeddedShaders[] = {
	BGFX_EMBEDDED_SHADER(vs_ocornut_imgui),
	BGFX_EMBEDDED_SHADER(fs_ocornut_imgui),
	BGFX_EMBEDDED_SHADER_END()
};


// The views that magnify a layer when the pixel art filter needs an intermediate
// target, and the one that draws onto the window. Views render in ascending order, so
// the magnify passes must carry the lower ids for the present pass to sample their output
// from this frame rather than the last one.
static const bgfx::ViewId VIEW_PRESCALE_FRAME = 0;
static const bgfx::ViewId VIEW_PRESCALE_SIDEBAR = 1;
static const bgfx::ViewId VIEW_PRESENT = 2;


// A picture the game uploads each present: the frame, or the sidebar when it is
// presented apart from the frame.
struct BackendLayer
{
	bgfx::TextureHandle Texture = BGFX_INVALID_HANDLE;
	int Width = 0;
	int Height = 0;
	bgfx::FrameBufferHandle Prescale = BGFX_INVALID_HANDLE;
	int PrescaleWidth = 0;
	int PrescaleHeight = 0;
	unsigned int * ConvertBuffer = NULL;
	bgfx::ViewId PrescaleView = 0;
};


static bool _Initialized = false;

static BackendLayer _Frame;
static BackendLayer _Sidebar;
static bgfx::ProgramHandle _Program = BGFX_INVALID_HANDLE;
static bgfx::UniformHandle _TextureSampler = BGFX_INVALID_HANDLE;
static bgfx::VertexLayout _VertexLayout;

static int _DrawableWidth = 0;
static int _DrawableHeight = 0;
static unsigned int _ResetFlags = BGFX_RESET_FLIP_AFTER_RENDER;

// True while the textures hold the game's own 565 layout. When the hardware cannot
// sample that format each upload is widened to 32 bits on the way in instead.
static bool _FrameIs565 = false;
static unsigned int _ConvertTable[65536];


struct BackendVertex
{
	float X;
	float Y;
	float U;
	float V;
	unsigned int Color;
};


// bgfx reports lost devices and shader failures through this rather than a return code,
// so the engine would otherwise present to a black window with no explanation.
class BackendCallback : public bgfx::CallbackI
{
	public:
		virtual ~BackendCallback(void) override {}

		virtual void fatal(const char * filepath, uint16_t line, bgfx::Fatal::Enum code, const char * str) override
		{
			// A debug check is the library's own assertion, not a renderer failure. The ones it
			// runs while shutting down compare reference counts on interfaces that an overlay
			// or the Direct3D debug layer is free to hold, so ending the process over one would
			// report somebody else's reference as a crash.
			if (code == bgfx::Fatal::DebugCheck) {
				DebugString("Renderer check failed at %s(%u): %s\n",
							filepath != NULL ? filepath : "", (unsigned)line, str != NULL ? str : "");
				return;
			}

			Fatal("Renderer error %d at %s(%u): %s", (int)code,
						filepath != NULL ? filepath : "", (unsigned)line, str != NULL ? str : "");
		}

		virtual void traceVargs(const char * filepath, uint16_t line, const char * format, va_list argList) override
		{
			char message[1024];
			vsnprintf(message, sizeof(message), format, argList);
			OutputDebugString(message);
		}

		virtual void profilerBegin(const char *, uint32_t, const char *, uint16_t) override {}
		virtual void profilerBeginLiteral(const char *, uint32_t, const char *, uint16_t) override {}
		virtual void profilerEnd(void) override {}
		virtual uint32_t cacheReadSize(uint64_t) override { return(0); }
		virtual bool cacheRead(uint64_t, void *, uint32_t) override { return(false); }
		virtual void cacheWrite(uint64_t, const void *, uint32_t) override {}
		virtual void screenShot(const char *, uint32_t, uint32_t, uint32_t, bgfx::TextureFormat::Enum, const void *, uint32_t, bool) override {}
		virtual void captureBegin(uint32_t, uint32_t, uint32_t, bgfx::TextureFormat::Enum, bool) override {}
		virtual void captureEnd(void) override {}
		virtual void captureFrame(const void *, uint32_t) override {}
};

static BackendCallback _Callback;


// bgfx contains cache-line-aligned render records but requests their backing arrays with
// the allocator's default alignment. The Win32 CRT only guarantees eight-byte alignment,
// which is insufficient when clang-cl copies those records with aligned SSE instructions.
class BackendAllocator : public bx::AllocatorI
{
	public:
		virtual ~BackendAllocator(void) override {}

		virtual void * realloc(void * ptr, size_t size, size_t alignment, const char *, uint32_t) override
		{
			if (size == 0) {
				_aligned_free(ptr);
				return(NULL);
			}

			const size_t cachelinealignment = BX_CACHE_LINE_SIZE;
			alignment = std::max(alignment, cachelinealignment);
			return(_aligned_realloc(ptr, size, alignment));
		}
};

static BackendAllocator _Allocator;


/// <summary>
/// Builds the table that widens a 565 pixel to the 32 bit color the fallback path uploads.
/// </summary>
static void Build_Convert_Table(void)
{
	for (int pixel = 0; pixel < 65536; pixel++) {
		unsigned int red = (unsigned int)(((pixel >> 11) & 0x1F) * 255 / 31);
		unsigned int green = (unsigned int)(((pixel >> 5) & 0x3F) * 255 / 63);
		unsigned int blue = (unsigned int)((pixel & 0x1F) * 255 / 31);

		_ConvertTable[pixel] = 0xFF000000 | (red << 16) | (green << 8) | blue;
	}
}


/// <summary>
/// Submits one textured rectangle covering the given destination.
/// </summary>
static void Submit_Quad(bgfx::ViewId view, bgfx::TextureHandle texture, float x, float y, float width, float height, unsigned int samplerflags, bool flipv = false)
{
	bgfx::TransientVertexBuffer buffer;

	if (bgfx::getAvailTransientVertexBuffer(6, _VertexLayout) < 6) {
		return;
	}

	bgfx::allocTransientVertexBuffer(&buffer, 6, _VertexLayout);

	BackendVertex * vertex = (BackendVertex *)buffer.data;
	const unsigned int white = 0xFFFFFFFF;

	const float vtop = flipv ? 1.0f : 0.0f;
	const float vbottom = flipv ? 0.0f : 1.0f;

	vertex[0] = { x, y, 0.0f, vtop, white };
	vertex[1] = { x + width, y, 1.0f, vtop, white };
	vertex[2] = { x + width, y + height, 1.0f, vbottom, white };
	vertex[3] = { x, y, 0.0f, vtop, white };
	vertex[4] = { x + width, y + height, 1.0f, vbottom, white };
	vertex[5] = { x, y + height, 0.0f, vbottom, white };

	bgfx::setVertexBuffer(0, &buffer);
	bgfx::setTexture(0, _TextureSampler, texture, samplerflags);
	bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
	bgfx::submit(view, _Program);
}


/// <summary>
/// Builds an orthographic projection over a target measured in pixels, with the origin in
/// its top left corner.
/// </summary>
static void Build_Ortho_Projection(float * result, int width, int height)
{
	const float depthnear = 0.0f;
	const float depthfar = 1000.0f;
	const bool homogeneous = bgfx::getCaps()->homogeneousDepth;

	memset(result, 0, sizeof(float) * 16);

	result[0] = 2.0f / (float)width;
	result[5] = -2.0f / (float)height;
	result[10] = homogeneous ? 2.0f / (depthfar - depthnear) : 1.0f / (depthfar - depthnear);
	result[12] = -1.0f;
	result[13] = 1.0f;
	result[14] = homogeneous ? -(depthfar + depthnear) / (depthfar - depthnear) : -depthnear / (depthfar - depthnear);
	result[15] = 1.0f;
}


/// <summary>
/// Sets a view to draw into a target of the given size using pixel coordinates.
/// </summary>
static void Set_View_Transform(bgfx::ViewId view, int width, int height)
{
	float projection[16];
	bgfx::setViewRect(view, 0, 0, (uint16_t)width, (uint16_t)height);
	Build_Ortho_Projection(projection, width, height);
	bgfx::setViewTransform(view, NULL, projection);
}


/// <summary>
/// Discards the intermediate target the pixel art filter magnifies a layer through.
/// </summary>
static void Destroy_Prescale_Target(BackendLayer & layer)
{
	if (bgfx::isValid(layer.Prescale)) {
		bgfx::destroy(layer.Prescale);
		layer.Prescale = BGFX_INVALID_HANDLE;
	}
	layer.PrescaleWidth = 0;
	layer.PrescaleHeight = 0;
}


/// <summary>
/// Makes sure the pixel art filter has an intermediate target of the requested size for
/// a layer.
/// </summary>
/// <returns>bool; Is a target of that size ready to render into?</returns>
static bool Ensure_Prescale_Target(BackendLayer & layer, int width, int height)
{
	if (bgfx::isValid(layer.Prescale) && layer.PrescaleWidth == width && layer.PrescaleHeight == height) {
		return(true);
	}

	Destroy_Prescale_Target(layer);

	const bgfx::Caps * caps = bgfx::getCaps();
	if (width <= 0 || height <= 0 || width > caps->limits.maxTextureSize || height > caps->limits.maxTextureSize) {
		return(false);
	}

	layer.Prescale = bgfx::createFrameBuffer((uint16_t)width, (uint16_t)height, bgfx::TextureFormat::BGRA8, BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
	if (!bgfx::isValid(layer.Prescale)) {
		return(false);
	}

	layer.PrescaleWidth = width;
	layer.PrescaleHeight = height;
	return(true);
}


/// <summary>
/// Releases a layer's texture and everything that went with it.
/// </summary>
static void Destroy_Layer(BackendLayer & layer)
{
	Destroy_Prescale_Target(layer);

	if (bgfx::isValid(layer.Texture)) {
		bgfx::destroy(layer.Texture);
		layer.Texture = BGFX_INVALID_HANDLE;
	}

	delete [] layer.ConvertBuffer;
	layer.ConvertBuffer = NULL;
	layer.Width = 0;
	layer.Height = 0;
}


/// <summary>
/// Points a layer at a texture of the given size, replacing any earlier one. A zero size
/// releases the layer.
/// </summary>
/// <returns>bool; Is the layer ready to receive pictures of that size?</returns>
static bool Set_Layer_Size(BackendLayer & layer, int width, int height)
{
	if (!_Initialized) {
		return(false);
	}

	if (width <= 0 || height <= 0) {
		Destroy_Layer(layer);
		return(true);
	}

	if (bgfx::isValid(layer.Texture) && layer.Width == width && layer.Height == height) {
		return(true);
	}

	Destroy_Layer(layer);

	// bgfx names packed formats from their low bits up, so its B5G6R5 is the layout the
	// game already draws in. Emulated support would convert every upload on the way
	// through, which is what the fallback below does more cheaply.
	const bgfx::Caps * caps = bgfx::getCaps();
	_FrameIs565 = (caps->formats[bgfx::TextureFormat::B5G6R5] & BGFX_CAPS_FORMAT_TEXTURE_2D) != 0;

	layer.Texture = bgfx::createTexture2D((uint16_t)width, (uint16_t)height, false, 1, _FrameIs565 ? bgfx::TextureFormat::B5G6R5 : bgfx::TextureFormat::BGRA8);
	if (!bgfx::isValid(layer.Texture)) {
		return(false);
	}

	if (!_FrameIs565) {
		if (_ConvertTable[0xFFFF] == 0) {
			Build_Convert_Table();
		}
		layer.ConvertBuffer = new unsigned int[width * height];
	}

	layer.Width = width;
	layer.Height = height;
	return(true);
}


/// <summary>
/// Uploads a layer's pixels and draws it onto the present view.
/// </summary>
static void Present_Layer(BackendLayer & layer, BackendQuad const & quad, BackendScaleMode mode)
{
	if (!bgfx::isValid(layer.Texture) || quad.Pixels == NULL) {
		return;
	}

	if (_FrameIs565) {
		// The rows may be a window onto a wider surface, so only the bytes the rows cover
		// are copied: the last row ends at the layer's width rather than at the pitch.
		uint32_t size = (uint32_t)((layer.Height - 1) * quad.Pitch + layer.Width * 2);
		bgfx::updateTexture2D(layer.Texture, 0, 0, 0, 0, (uint16_t)layer.Width, (uint16_t)layer.Height, bgfx::copy(quad.Pixels, size), (uint16_t)quad.Pitch);
	} else if (layer.ConvertBuffer != NULL) {
		for (int y = 0; y < layer.Height; y++) {
			unsigned short const * source = (unsigned short const *)((char const *)quad.Pixels + y * quad.Pitch);
			unsigned int * dest = layer.ConvertBuffer + y * layer.Width;
			for (int x = 0; x < layer.Width; x++) {
				dest[x] = _ConvertTable[source[x]];
			}
		}
		bgfx::updateTexture2D(layer.Texture, 0, 0, 0, 0, (uint16_t)layer.Width, (uint16_t)layer.Height, bgfx::copy(layer.ConvertBuffer, (uint32_t)(layer.Width * layer.Height * 4)), (uint16_t)(layer.Width * 4));
	}

	bgfx::TextureHandle source = layer.Texture;
	unsigned int samplerflags = BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
	bool from_prescale = false;

	if (mode == BACKEND_SCALE_NEAREST) {
		samplerflags |= BGFX_SAMPLER_POINT;
	}

	// The pixel art filter keeps whole pixels whole. An exact multiple needs nothing but
	// point sampling; anything else is magnified to the next whole multiple with point
	// sampling and then shrunk to the window smoothly, which keeps edges sharp without
	// the uneven pixel sizes that point sampling alone would give.
	if (mode == BACKEND_SCALE_PIXELART && quad.DestWidth > layer.Width && quad.DestHeight > layer.Height) {
		if ((quad.DestWidth % layer.Width) == 0 && (quad.DestHeight % layer.Height) == 0) {
			samplerflags |= BGFX_SAMPLER_POINT;
		} else {
			int scale = (quad.DestWidth + layer.Width - 1) / layer.Width;
			int scaley = (quad.DestHeight + layer.Height - 1) / layer.Height;
			if (scaley > scale) {
				scale = scaley;
			}

			if (Ensure_Prescale_Target(layer, layer.Width * scale, layer.Height * scale)) {
				bgfx::setViewFrameBuffer(layer.PrescaleView, layer.Prescale);
				bgfx::setViewClear(layer.PrescaleView, BGFX_CLEAR_COLOR, 0x000000FF);
				Set_View_Transform(layer.PrescaleView, layer.PrescaleWidth, layer.PrescaleHeight);
				Submit_Quad(layer.PrescaleView, layer.Texture, 0.0f, 0.0f, (float)layer.PrescaleWidth, (float)layer.PrescaleHeight, BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_POINT);
				source = bgfx::getTexture(layer.Prescale);
				from_prescale = true;
			}
		}
	}

	bool flipv = from_prescale && bgfx::getCaps()->originBottomLeft;
	Submit_Quad(VIEW_PRESENT, source, (float)quad.DestX, (float)quad.DestY, (float)quad.DestWidth, (float)quad.DestHeight, samplerflags, flipv);
}


/// <summary>
/// Starts the renderer on an existing window.
/// </summary>
/// <param name="window">The window the frame is presented into.</param>
/// <param name="drawablewidth">The drawable area's width in physical pixels.</param>
/// <param name="drawableheight">The drawable area's height in physical pixels.</param>
/// <param name="renderer">Which graphics API to ask for, or auto to let bgfx decide.</param>
/// <param name="vsync">Should presents wait for the display's refresh?</param>
/// <returns>bool; Did the renderer start?</returns>
bool Backend_Init(NativeWindow const & window, int drawablewidth, int drawableheight, BackendRenderer renderer, bool vsync)
{
	if (_Initialized) {
		return(true);
	}

	// Presents happen at whatever depth the engine has reached, including from inside a
	// dialog's paint handler, so the renderer has to run on this thread. Calling
	// renderFrame before init is what selects that.
	bgfx::renderFrame();

	_DrawableWidth = drawablewidth;
	_DrawableHeight = drawableheight;
	_ResetFlags = BGFX_RESET_FLIP_AFTER_RENDER | (vsync ? BGFX_RESET_VSYNC : BGFX_RESET_NONE);

	bgfx::Init init;
	init.platformData.ndt = window.Display;
	init.platformData.nwh = window.Handle;
	init.platformData.type = window.Type == NATIVE_WINDOW_WAYLAND
		? bgfx::NativeWindowHandleType::Wayland
		: bgfx::NativeWindowHandleType::Default;
	init.resolution.width = (uint32_t)drawablewidth;
	init.resolution.height = (uint32_t)drawableheight;
	init.resolution.reset = _ResetFlags;
	init.callback = &_Callback;
	init.allocator = &_Allocator;

	switch (renderer) {
		case BACKEND_RENDERER_D3D11:
			init.type = bgfx::RendererType::Direct3D11;
			break;

		case BACKEND_RENDERER_D3D12:
			init.type = bgfx::RendererType::Direct3D12;
			break;

		case BACKEND_RENDERER_VULKAN:
			init.type = bgfx::RendererType::Vulkan;
			break;

		case BACKEND_RENDERER_OPENGL:
			init.type = bgfx::RendererType::OpenGL;
			break;

		default:
			init.type = bgfx::RendererType::Count;
			break;
	}

	if (!bgfx::init(init)) {
		return(false);
	}

	_VertexLayout.begin()
		.add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
		.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
		.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
		.end();

	bgfx::RendererType::Enum type = bgfx::getRendererType();
	bgfx::ShaderHandle vertexshader = bgfx::createEmbeddedShader(_EmbeddedShaders, type, "vs_ocornut_imgui");
	bgfx::ShaderHandle fragmentshader = bgfx::createEmbeddedShader(_EmbeddedShaders, type, "fs_ocornut_imgui");

	if (!bgfx::isValid(vertexshader) || !bgfx::isValid(fragmentshader)) {
		bgfx::shutdown();
		return(false);
	}

	_Program = bgfx::createProgram(vertexshader, fragmentshader, true);
	_TextureSampler = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);

	if (!bgfx::isValid(_Program) || !bgfx::isValid(_TextureSampler)) {
		bgfx::shutdown();
		return(false);
	}

	_Frame.PrescaleView = VIEW_PRESCALE_FRAME;
	_Sidebar.PrescaleView = VIEW_PRESCALE_SIDEBAR;

	_Initialized = true;
	return(true);
}


/// <summary>
/// Shuts the renderer down and releases everything it created.
/// </summary>
void Backend_Shutdown(void)
{
	if (!_Initialized) {
		return;
	}

	Destroy_Layer(_Frame);
	Destroy_Layer(_Sidebar);

	if (bgfx::isValid(_TextureSampler)) {
		bgfx::destroy(_TextureSampler);
		_TextureSampler = BGFX_INVALID_HANDLE;
	}
	if (bgfx::isValid(_Program)) {
		bgfx::destroy(_Program);
		_Program = BGFX_INVALID_HANDLE;
	}

	bgfx::shutdown();

	_Initialized = false;
}


/// <summary>
/// Points the renderer at a frame of the given size, replacing any earlier one.
/// </summary>
/// <returns>bool; Is a texture of that size ready to receive frames?</returns>
bool Backend_Set_Frame_Size(int width, int height)
{
	if (width <= 0 || height <= 0) {
		return(false);
	}
	return(Set_Layer_Size(_Frame, width, height));
}


/// <summary>
/// Gives the sidebar a texture of its own at the given size, or drops it at zero.
/// </summary>
/// <returns>bool; Is the sidebar layer as asked?</returns>
bool Backend_Set_Sidebar_Size(int width, int height)
{
	return(Set_Layer_Size(_Sidebar, width, height));
}


/// <summary>
/// Tells the renderer the drawable area changed size.
/// </summary>
void Backend_On_Resize(int drawablewidth, int drawableheight)
{
	if (!_Initialized || drawablewidth <= 0 || drawableheight <= 0) {
		return;
	}

	if (_DrawableWidth == drawablewidth && _DrawableHeight == drawableheight) {
		return;
	}

	_DrawableWidth = drawablewidth;
	_DrawableHeight = drawableheight;
	bgfx::reset((uint32_t)drawablewidth, (uint32_t)drawableheight, _ResetFlags);
}


/// <summary>
/// Uploads the frame, and the sidebar when one is given, and puts them on the screen.
/// </summary>
/// <param name="frame">The frame's pixels and where it lands in the window.</param>
/// <param name="sidebar">The sidebar's pixels and place, or NULL while it is part of the
/// frame.</param>
/// <param name="mode">How the pictures are filtered when drawn larger than they are.</param>
void Backend_Present(BackendQuad const & frame, BackendQuad const * sidebar, BackendScaleMode mode)
{
	if (!_Initialized || frame.Pixels == NULL || !bgfx::isValid(_Frame.Texture)) {
		return;
	}

	// A minimized window has no client area to present into.
	if (_DrawableWidth <= 0 || _DrawableHeight <= 0) {
		return;
	}

	// Clearing the whole window is what paints the bars beside a frame that does not
	// share the window's shape.
	bgfx::setViewFrameBuffer(VIEW_PRESENT, BGFX_INVALID_HANDLE);
	bgfx::setViewClear(VIEW_PRESENT, BGFX_CLEAR_COLOR, 0x000000FF);
	Set_View_Transform(VIEW_PRESENT, _DrawableWidth, _DrawableHeight);

	Present_Layer(_Frame, frame, mode);
	if (sidebar != NULL) {
		Present_Layer(_Sidebar, *sidebar, mode);
	}

	bgfx::frame();
}


/// <summary>
/// Names the graphics API the renderer settled on.
/// </summary>
char const * Backend_Renderer_Name(void)
{
	if (!_Initialized) {
		return("none");
	}
	return(bgfx::getRendererName(bgfx::getRendererType()));
}
