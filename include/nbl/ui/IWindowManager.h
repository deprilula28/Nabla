#ifndef _NBL_UI_I_WINDOWMANAGER_
#define _NBL_UI_I_WINDOWMANAGER_

#include <nbl/core/IReferenceCounted.h>
#include "IWindow.h"

namespace nbl::ui
{

struct SDisplayInfo
{
	int32_t x;
	int32_t y;
	uint32_t resX;
	uint32_t resY;
	std::string name; // this one is really more of a placeholder right now
};

class NBL_API2 IWindowManager : public core::IReferenceCounted
{
	public:
		virtual core::smart_refctd_ptr<IWindow> createWindow(IWindow::SCreationParams&& creationParams) = 0;
		virtual SDisplayInfo getPrimaryDisplayInfo() const = 0;

	private:
		virtual void destroyWindow(IWindow* wnd) = 0;

	protected:
		virtual ~IWindowManager() = default;
};

}
#endif