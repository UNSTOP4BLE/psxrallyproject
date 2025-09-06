#include "gpu.hpp"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "ps1/registers.h"

namespace GFX {
static void waitForGP0Ready(void);
static void waitForDMADone(void);
static void waitForVSync(void);
static void sendLinkedList(const void *data);
static void sendVRAMData(const void *data, RECT<int32_t> rect);
static void clearOT(uint32_t *table, int numentries);

void Renderer::init(GP1VideoMode mode) {
	// Set the origin of the displayed framebuffer. These "magic" values,
	// derived from the GPU's internal clocks, will center the picture on most
	// displays and upscalers.
	int _x = 0x760;
	int _y = (mode == GP1_MODE_PAL) ? 0xa3 : 0x88;

	// horizontal (256, 320, 368, 512, 640) and vertical (240-256, 480-512) resolutions to pick
	//todo make width and height actually effect it lol
	GP1HorizontalRes _xres = GP1_HRES_320;
	GP1VerticalRes   _yres = (SCREEN_HEIGHT > 256) ? GP1_VRES_512 : GP1_VRES_256;

	int _offx = (SCREEN_WIDTH  * gp1_clockMultiplierH(_xres)) / 2;
	int _offy = (SCREEN_HEIGHT / gp1_clockDividerV(_yres))    / 2;

	GPU_GP1 = gp1_resetGPU();
	GPU_GP1 = gp1_fbRangeH(_x - _offx, _x + _offx);
	GPU_GP1 = gp1_fbRangeV(_y - _offy, _y + _offy);
	GPU_GP1 = gp1_fbMode(
		_xres,
		_yres,
		mode,
		false,
		GP1_COLOR_16BPP
	);

	// Enable DMA for GPU and OTC
    DMA_DPCR |= DMA_DPCR_CH_ENABLE(DMA_GPU) | DMA_DPCR_CH_ENABLE(DMA_OTC);

    // Configure GPU to accept DMA for GP0 writes
    GPU_GP1 = gp1_dmaRequestMode(GP1_DREQ_GP0_WRITE);

    // Turn the display on (unblank)
    GPU_GP1 = gp1_dispBlank(false);
	usingsecondframe = false;
	framecounter = 0;
	setClearCol(64,64,64);
}

void Renderer::beginFrame(void) {
    auto _newchain = getCurrentChain();

    // determine where new framebuffer to draw to is in vram
    int _bufx = usingsecondframe ? SCREEN_WIDTH : 0;
    int _bufy = 0;

    // clear and prepare new chain
    clearOT(_newchain->orderingtable, ORDERING_TABLE_SIZE);
    _newchain->nextpacket = _newchain->data;

    // add gpu commands to clear buffer and set drawing origin to new chain
    // z is set to (ORDERING_TABLE_SIZE - 1) so they're executed before anything else
    auto _ptr = allocatePacket(ORDERING_TABLE_SIZE - 1, 7);
    _ptr[0]   = gp0_texpage(0, true, false);
    _ptr[1]   = gp0_xy(_bufx, _bufy);
    _ptr[2]   = gp0_fbOffset2(_bufx + SCREEN_WIDTH -  1, _bufy + SCREEN_HEIGHT - 2);
    _ptr[3]   = gp0_fbOrigin(_bufx, _bufy);
    _ptr[4]   = clearcol | gp0_vramFill();
    _ptr[5]   = gp0_xy(_bufx, _bufy);
    _ptr[6]   = gp0_xy(SCREEN_WIDTH, SCREEN_HEIGHT);
}

void Renderer::endFrame(void) {
    auto _oldchain = getCurrentChain();

    // switch active chain
    usingsecondframe = !usingsecondframe;

    // determine where new framebuffer to display is in vram
    int _bufx = usingsecondframe ? SCREEN_WIDTH : 0;
    int _bufy = 0;

    // display new framebuffer after vsync
    waitForGP0Ready();
    waitForVSync();
    GPU_GP1 = gp1_fbOffset(_bufx, _bufy);

    // terminate and start drawing current chain
    *(_oldchain->nextpacket) = gp0_endTag(0);
    sendLinkedList(&(_oldchain->orderingtable)[ORDERING_TABLE_SIZE - 1]);
}

static inline void addPacketData(uint32_t *ptr, int i, uint32_t p) {
	ptr[i] = p;       
}

void Renderer::drawRect(RECT<int32_t> rect, int z, uint32_t col) {
	auto _ptr      = allocatePacket(z, 3);
	_ptr[0]        = col | gp0_rectangle(false, false, false); 
	_ptr[1]        = gp0_xy(rect.x, rect.y);       
	_ptr[2]        = gp0_xy(rect.w, rect.h);  
}

void Renderer::drawTexRect(const TextureInfo &tex, XY<int32_t> pos, int z, int col) {
	auto _ptr = allocatePacket(z, 5);
	_ptr[0]        = gp0_texpage(tex.page, false, false);
	_ptr[1]        = col | gp0_rectangle(true, true, false);
	_ptr[2]        = gp0_xy(pos.x, pos.y);
	_ptr[3]        = gp0_uv(tex.u, tex.v, tex.clut);
	_ptr[4]        = gp0_xy(tex.w, tex.h);
}

void Renderer::drawTexQuad(const TextureInfo &tex, RECT<int32_t> pos, int z, uint32_t col) {
    auto *_ptr = allocatePacket(z, 10);
	_ptr[0]    = gp0_texpage(tex.page, false, false); // set texture page and CLUT
    _ptr[1]    = col | gp0_shadedQuad(false, true, false);
    _ptr[2]    = gp0_xy(pos.x, pos.y);
    _ptr[3]    = gp0_uv(tex.u, tex.v, tex.clut);
    _ptr[4]    = gp0_xy(pos.x+pos.w, pos.y);
    _ptr[5]    = gp0_uv(tex.u+tex.w, tex.v, tex.page);
    _ptr[6]    = gp0_xy(pos.x, pos.y+pos.h);
    _ptr[7]    = gp0_uv(tex.u, tex.v+tex.h, 0);
    _ptr[8]    = gp0_xy(pos.x+pos.w, pos.y+pos.h);
    _ptr[9]    = gp0_uv(tex.u+tex.w, tex.v+tex.h, 0);
}

//to clean up
void Renderer::drawModel(const ModelFile *model, GTEVector32 pos, GTEVector32 rot) {
	gte_setControlReg(GTE_TRX, pos.x);
	gte_setControlReg(GTE_TRY, pos.y);
	gte_setControlReg(GTE_TRZ, pos.z);
	gte_setRotationMatrix(
		ONE,   0,   0,
		  0, ONE,   0,
		  0,   0, ONE
	);

	GTE::rotateCurrentMatrix(rot.x, rot.y, rot.z);

	int _lasttexid = -1;

	for (int i = 0; i < static_cast<int>(model->header->numfaces); i++) {
		const Face *_face = &model->faces[i];

		bool _istriangle = false;
		if (_face->indices[3] < 0) //for triangles the 4th indice is negative
			_istriangle = true;

		// Apply perspective projection to the first 3 vertices. The GTE can
		// only process up to 3 vertices at a time, so we'll transform the
		// last one separately.
		gte_loadV0(&model->vertices[_face->indices[0]]);
		gte_loadV1(&model->vertices[_face->indices[1]]);
		gte_loadV2(&model->vertices[_face->indices[2]]);
		gte_command(GTE_CMD_RTPT | GTE_SF);

		// Determine the winding order of the vertices on screen. If they
		// are ordered clockwise then the face is visible, otherwise it can
		// be skipped as it is not facing the camera.
		gte_command(GTE_CMD_NCLIP);

		if (gte_getDataReg(GTE_MAC0) <= 0)
			continue;
		
		// Save the first transformed vertex (the GTE only keeps the X/Y
		// coordinates of the last 3 vertices processed and Z coordinates of
		// the last 4 vertices processed) and apply projection to the last
		// vertex.
		uint32_t _xy0 = gte_getDataReg(GTE_SXY0);
		if (!_istriangle) {
			gte_loadV0(&model->vertices[_face->indices[3]]);
			gte_command(GTE_CMD_RTPS | GTE_SF);
		}
		// Calculate the average Z coordinate of all vertices and use it to
		// determine the ordering table bucket index for this face.
		gte_command(GTE_CMD_AVSZ4 | GTE_SF);
		int _z = gte_getDataReg(GTE_OTZ);

		if ((_z < 0) || (_z >= ORDERING_TABLE_SIZE))
			continue;

		// Create a new quad and give its vertices the X/Y coordinates
		// calculated by the GTE.

		uint32_t *_ptr;
		//todo set texture only once
		if (_face->texid >= 0) { //textured
			auto _clut = model->textures[_face->texid].clut;
			auto _page = model->textures[_face->texid].page;
			
			if (_istriangle) {
				_ptr           = allocatePacket(_z, 7);
				_ptr[0]        = _face->color | gp0_shadedTriangle(false, true, false);
				_ptr[1]        = _xy0;
				_ptr[2]        = gp0_uv(_face->u[0], _face->v[0], _clut);
				gte_storeDataReg(GTE_SXY1, 4 * 4, _ptr);
				_ptr[4]        = gp0_uv(_face->u[1], _face->v[1], _page);
				gte_storeDataReg(GTE_SXY2, 6 * 4, _ptr);
				_ptr[6]        = gp0_uv(_face->u[2], _face->v[2], 0);
			}
			else { //quad
	    		_ptr       = allocatePacket(_z, 9);
				_ptr[0]    = _face->color | gp0_shadedQuad(false, true, false);
				_ptr[1]    = _xy0;
				_ptr[2]    = gp0_uv(_face->u[0], _face->v[0], _clut);
				gte_storeDataReg(GTE_SXY0, 4 * 4, _ptr);
				_ptr[4]    = gp0_uv(_face->u[1], _face->v[1], _page);
				gte_storeDataReg(GTE_SXY1, 6 * 4, _ptr);
				_ptr[6]    = gp0_uv(_face->u[2], _face->v[2], 0);
				gte_storeDataReg(GTE_SXY2, 8 * 4, _ptr);
				_ptr[8]    = gp0_uv(_face->u[3], _face->v[3], 0);
			}	

			if (_lasttexid != _face->texid) {
	    		_ptr       = allocatePacket(_z, 1);
				_ptr[0]    = gp0_texpage(_page, false, false); // set texture page and CLUT		
				_lasttexid = _face->texid;
			}
		}
		else  { //untextured
			if (_istriangle) {
				_ptr    = allocatePacket(_z,6);
				_ptr[0] = _face->color | gp0_shadedTriangle(true, false, false);
				_ptr[1] = _xy0;
				_ptr[2] = _face->color;
				gte_storeDataReg(GTE_SXY1, 3 * 4, _ptr);
				_ptr[4] = _face->color;
				gte_storeDataReg(GTE_SXY2, 5 * 4, _ptr);
			}
			else { //quad
	    		_ptr    = allocatePacket(_z, 5);
				_ptr[0] = _face->color | gp0_shadedQuad(false, false, false);
				_ptr[1] = _xy0;
				gte_storeDataReg(GTE_SXY0, 2 * 4, _ptr);
				gte_storeDataReg(GTE_SXY1, 3 * 4, _ptr);
				gte_storeDataReg(GTE_SXY2, 4 * 4, _ptr);
			}
		}
	}
}

uint32_t *Renderer::allocatePacket(int z, int numcommands) {
    auto _chain = getCurrentChain();
    auto _ptr   = _chain->nextpacket;

    // check z index is valid
    assert((z >= 0) && (z < ORDERING_TABLE_SIZE));

    // link new packet into ordering table at specified z index
    *_ptr = gp0_tag(numcommands, (void *) _chain->orderingtable[z]);
    _chain->orderingtable[z] = gp0_tag(0, _ptr);

    // bump up allocator and check we haven't run out of space
    _chain->nextpacket += numcommands + 1;
    assert(_chain->nextpacket < &(_chain->data)[CHAIN_BUFFER_SIZE]);

    return &_ptr[1];
}

void Renderer::printString(XY<int32_t> pos, int z, const char *str) {
	assert(fontmap);
	int _curx = pos.x, _cury = pos.y;

	uint32_t *_ptr;
	int _tabwidth = fontmap->header->tabwidth;

	for (; *str; str++) {
		char _c = *str;

		// Check if the character is "special"
		switch (_c) {
			case '\t':
				_curx += _tabwidth - 1;
				_curx -= _curx % _tabwidth;
				continue;

			case '\n':
				_curx  = pos.x;
				_cury += fontmap->header->lineheight;
				continue;

			case ' ':
				_curx += fontmap->header->spacewidth;
				continue;

			case '\x80' ... '\xff':
				_c = '\x7f';
				break;
		}

		const RECT<uint8_t> &_rect = fontmap->rects[_c - fontmap->header->firstchar];
		
		// Enable blending to make sure any semitransparent pixels in the font get rendered correctly.
		_ptr    = allocatePacket(z, 4);
		_ptr[0] = gp0_rectangle(true, true, true);
		_ptr[1] = gp0_xy(_curx, _cury);
		_ptr[2] = gp0_uv(fonttex.u + _rect.x, fonttex.v + _rect.y, fonttex.clut);
		_ptr[3] = gp0_xy(_rect.w, _rect.h);

		_curx += _rect.w;
	}
	
	_ptr    = allocatePacket(z, 1);
	_ptr[0] = gp0_texpage(fonttex.page, false, false);
}

void Renderer::printStringf(XY<int32_t> pos, int z, const char *fmt, ...) {
    char buf[256];  // Adjust size as needed
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    printString(pos, z, buf);
}

//indexed
void uploadTexture(TextureInfo &info, const void *image) {
    const TexHeader* _header = reinterpret_cast<const TexHeader*>(image);
	assert(_header->isValid());
	info = _header->texinfo;

	assert((info.w <= 256) && (info.h <= 256));

	int _ncolors = (info.bpp == 8) ? 256 : 16;
	int _widthdivider = (info.bpp == 8) ? 2 : 4;
	
	sendVRAMData(_header->texdata(), {_header->vrampos[0], _header->vrampos[1], info.w / _widthdivider, info.h});
	waitForDMADone();
	sendVRAMData(_header->clut(), {_header->clutpos[0], _header->clutpos[1], _ncolors, 1});
	waitForDMADone();
}

const ModelFile* loadModel(const uint8_t* data) {
    auto _model = new ModelFile(); 
    _model->header = reinterpret_cast<const ModelFileHeader*>(data);
	assert(_model->header->isValid());

    _model->vertices = _model->header->vertices();
    _model->faces = _model->header->faces();

	//textures
	auto _texptr = _model->header->textures();
	
	int _ntex = static_cast<int>(_model->header->numtex);

    if (_ntex > 0) {
		_model->textures = new TextureInfo[_ntex];

		for (int i = 0; i < static_cast<int>(_model->header->numtex); i++) {
			const TexHeader* tex_header = reinterpret_cast<const TexHeader*>(_texptr);
			uploadTexture(_model->textures[i], _texptr);
			
			size_t _clutbytes = tex_header->clutsize;
			size_t _texbytes = tex_header->texsize;

			_texptr += sizeof(TexHeader) + _clutbytes + _texbytes;
		}
	}
    return _model;
}

FontData *loadFontMap(const uint8_t *data) {
    auto _fntdata = new FontData();
    _fntdata->header = reinterpret_cast<const FontHeader*>(data);
	assert(_fntdata->header->isValid());
    _fntdata->rects = _fntdata->header->rects();

	return _fntdata;
}

static void waitForGP0Ready(void) {
	while (!(GPU_GP1 & GP1_STAT_CMD_READY))
		__asm__ volatile("");
}

static void waitForDMADone(void) {
	while (DMA_CHCR(DMA_GPU) & DMA_CHCR_ENABLE)
		__asm__ volatile("");
}

static void waitForVSync(void) {
	while (!(IRQ_STAT & (1 << IRQ_VSYNC)))
		__asm__ volatile("");

	IRQ_STAT = ~(1 << IRQ_VSYNC);
}

static void sendLinkedList(const void *data) {
	waitForDMADone();
	assert(!((uint32_t) data % 4));

	DMA_MADR(DMA_GPU) = (uint32_t) data;
	DMA_CHCR(DMA_GPU) = 0
		| DMA_CHCR_WRITE
		| DMA_CHCR_MODE_LIST
		| DMA_CHCR_ENABLE;
}

static void sendVRAMData(const void *data, RECT<int32_t> rect) {
	waitForDMADone();
	assert(!((uint32_t) data % 4));

	size_t _length = (rect.w * rect.h) / 2;
	size_t _chunksize, _numchunks;

	if (_length < DMA_MAX_CHUNK_SIZE) {
		_chunksize = _length;
		_numchunks = 1;
	} else {
		_chunksize = DMA_MAX_CHUNK_SIZE;
		_numchunks = _length / DMA_MAX_CHUNK_SIZE;

		assert(!(_length % DMA_MAX_CHUNK_SIZE));
	}

	waitForGP0Ready();
	GPU_GP0 = gp0_vramWrite();
	GPU_GP0 = gp0_xy(rect.x, rect.y);
	GPU_GP0 = gp0_xy(rect.w, rect.h);

	DMA_MADR(DMA_GPU) = (uint32_t) data;
	DMA_BCR (DMA_GPU) = _chunksize | (_numchunks << 16);
	DMA_CHCR(DMA_GPU) = 0
		| DMA_CHCR_WRITE
		| DMA_CHCR_MODE_SLICE
		| DMA_CHCR_ENABLE;
}

static void clearOT(uint32_t *table, int numentries) {
	DMA_MADR(DMA_OTC) = (uint32_t) &table[numentries - 1];
	DMA_BCR (DMA_OTC) = numentries;
	DMA_CHCR(DMA_OTC) = 0
		| DMA_CHCR_READ
		| DMA_CHCR_REVERSE
		| DMA_CHCR_MODE_BURST
		| DMA_CHCR_ENABLE
		| DMA_CHCR_TRIGGER;

	while (DMA_CHCR(DMA_OTC) & DMA_CHCR_ENABLE)
		__asm__ volatile("");
}
}