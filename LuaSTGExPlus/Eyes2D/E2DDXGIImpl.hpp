#pragma once
#include "E2DGlobal.hpp"
#include "dxgi.h"
//注：可能会爆一堆warning C4005，不要慌，M$ NM$L就对了

namespace Eyes2D {
	class EYESDLLAPI DXGIImpl {
	private:
		IDXGIFactory* m_DXGI;
	public:
		DXGIImpl();
		~DXGIImpl();
	public:
		bool SimpleGetResolutions(int*& width, int*& height, int& counts);
	};

	inline DXGIImpl& GetDXGI() {
		static DXGIImpl gs_DXGIInstance;
		return gs_DXGIInstance;
	}
}
