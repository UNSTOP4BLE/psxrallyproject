#pragma once 

#include "common.hpp"
#ifdef PLATFORM_PSX

#else
#include <glad/glad.h>
#include <SDL2/SDL.h>
#endif

namespace ENGINE {

	class Renderer {
	public:
		virtual void beginFrame(void) {}
		virtual void endFrame(void) {}

		virtual void drawTri(const ENGINE::COMMON::TRI32 &tri, uint32_t z, uint32_t col) {}
//			virtual void drawTexTri(const TextureInfo &tex, const ENGINE::COMMON::TRI32 &tri, uint32_t z, uint32_t col) {}
		virtual void drawRect(const ENGINE::COMMON::RECT32 &rect, uint32_t z, uint32_t col) {}
	//	virtual void drawTexRect(const TextureInfo &tex, const ENGINE::COMMON::XY32 &pos, uint32_t z, uint32_T col) {}
	//  virtual void drawTexQuad(const TextureInfo &tex, const ENGINE::COMMON::RECT32 &pos, uint32_t z, uint32_t col) {}
	//	virtual void drawModel(const Model *model, FIXED::Vector12 pos, FIXED::Vector12 rot) {}
		
		virtual void setClearCol(uint8_t r, uint8_t g, uint8_t b) {}
		uint32_t getFPS(void) {return fps;}

		static Renderer &instance();

	protected:
		uint32_t scrw, scrh;
		uint32_t refreshrate, fps; //todo populate on gl render
		uint32_t vsynccounter, framecounter; //todo populate on gl render
		Renderer() {}
	};

	extern TEMPLATES::ServiceLocator<Renderer> g_rendererInstance;

	//psx
#ifdef PLATFORM_PSX
	namespace PSX {
			
		struct DMAChain {
			uint32_t data[ENGINE::COMMON::CHAIN_BUFFER_SIZE];
			uint32_t orderingtable[ENGINE::COMMON::ORDERING_TABLE_SIZE];
			uint32_t *nextpacket;
		};

		class PSXRenderer : public Renderer {
		public:
			PSXRenderer(void);
			
			void beginFrame(void);
			void endFrame(void);

			void drawTri(const ENGINE::COMMON::TRI32 &tri, uint32_t z, uint32_t col);
			//void drawTexTri(const TextureInfo &tex, const ENGINE::COMMON::TRI32 &tri, uint32_t z, uint32_t col);
			void drawRect(const ENGINE::COMMON::RECT32 &rect, uint32_t z, uint32_t col);
	//	 void drawTexRect(const TextureInfo &tex, const ENGINE::COMMON::XY32 &pos, uint32_t z, uint32_T col);
	//   void drawTexQuad(const TextureInfo &tex, const ENGINE::COMMON::RECT32 &pos, uint32_t z, uint32_t col);
			//void drawModel(const Model *model, FIXED::Vector12 pos, FIXED::Vector12 rot);

			void setClearCol(uint8_t r, uint8_t g, uint8_t b) {
				clearcol = (b << 16) | (g << 8) | r; 
			}
		
			void handleVSyncInterrupt(void); //for irqs
		private:
			uint32_t clearcol;
			bool usingsecondframe;
			DMAChain dmachains[2];

			void waitForVSync(void);
			uint32_t *allocatePacket(uint32_t z, size_t numcommands);

			DMAChain *getCurrentChain(void) {
				return &dmachains[usingsecondframe];
			}

		};
	} //namespace PSX
#else
	namespace GENERIC {
			
		struct GLShader {
		public:
			void init(const char *path, GLenum type);
			uint32_t getId(void) {return id;}
			void free(void) {glDeleteShader(id);}
		private:
			uint32_t id;
		};

		class GLRenderer : public Renderer {
		public:
			GLRenderer(void);
			
			void beginFrame(void);
			void endFrame(void);

			void drawTri(const ENGINE::COMMON::TRI32 &tri, uint32_t z, uint32_t col);
			//void drawTexTri(const TextureInfo &tex, const ENGINE::COMMON::TRI32 &tri, uint32_t z, uint32_t col);
			void drawRect(const ENGINE::COMMON::RECT32 &rect, uint32_t z, uint32_t col);
	//	 void drawTexRect(const TextureInfo &tex, const ENGINE::COMMON::XY32 &pos, uint32_t z, uint32_T col);
	//   void drawTexQuad(const TextureInfo &tex, const ENGINE::COMMON::RECT32 &pos, uint32_t z, uint32_t col);
			//void drawModel(const Model *model, FIXED::Vector12 pos, FIXED::Vector12 rot);
			void setClearCol(uint8_t r, uint8_t g, uint8_t b) {
				glClearColor(r/255.0f, g/255.0f, b/255.0f, 1.0f);
			}
	
		private:
			SDL_Window* window;
			GLShader vertshader;
			GLShader fragshader;
			uint32_t shaderprog;
		};

	} //namespace GENERIC
#endif
} //namespace ENGINE