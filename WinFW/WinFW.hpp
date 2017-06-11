#pragma once

#if !defined(_WIN32) && !defined(_WIN64)
#error This header only available on Windows Platform
#endif

#if !defined(UNICODE) && !defined( _UNICODE)
#error Please use Unicode Character Set
#endif

#pragma push_macro("WIN32_LEAN_AND_MEAN")
#pragma push_macro("DLL_DECLSPEC")

#include <type_traits>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#ifdef WINFW_DEV_MODE
#define DLL_DECLSPEC __declspec(dllexport)
#else
#define DLL_DECLSPEC __declspec(dllimport)
#endif

namespace WinFW {
	DLL_DECLSPEC void init(HINSTANCE);
	
	class Ref {
	public:
		DLL_DECLSPEC static const char* GetRefName();

		virtual unsigned long long numRef() = 0;
		virtual unsigned long long incRef() = 0;
		virtual unsigned long long decRef() = 0;
		virtual bool delRef() = 0;
		virtual bool queryRef(void**const, const char*, bool = true) = 0;
		virtual const char* getRefName() = 0;
	};

	class Copyable : public virtual Ref {
	public:
		DLL_DECLSPEC static const char* GetRefName();

		virtual bool copy(void**const, const char*, bool = true) = 0;
	};

	namespace Hidden {
		namespace IPtr {
			template<typename Interface>
			class Ref {
			protected:
				Interface *m_ptr;

				inline unsigned long long incRef() {
					if (isActive()) return m_ptr->incRef();
					return 0;
				}

				inline unsigned long long decRef() {
					unsigned long long result = 0;
					if (isActive()) {
						result = m_ptr->decRef();
						m_ptr = nullptr;
					}
					return result;
				}

				inline void set(Interface *ptr) {
					m_ptr = ptr;
				}
			public:
				inline Ref(Interface *ptr) : m_ptr(ptr) {
				}

				inline Interface* operator->() {
					return m_ptr;
				}

				inline operator Interface*&() {
					return m_ptr;
				}

				inline Interface** operator&() {
					return &m_ptr;
				}

				inline Interface* get() {
					return m_ptr;
				}

				inline bool isActive() {
					return m_ptr != nullptr;
				}

				template<typename TargetType>
				inline bool queryRef(TargetType **target, bool cmpByStr = false) {
					if (*target != nullptr) {
						(*target)->decRef();
						*target = nullptr;
					}
					return m_ptr->queryRef(reinterpret_cast<void**>(target), TargetType::GetRefName(), cmpByStr);
				}
			};

			template<typename Interface, bool>
			class Copyable : public Ref<Interface> {
			public:
				inline Copyable(Interface *ptr) : Ref<Interface>(ptr) {
				}
			};

			template<typename Interface>
			class Copyable<Interface, true> : public Ref<Interface> {
			public:
				inline Copyable(Interface *ptr) : Ref<Interface>(ptr) {
				}

				template<typename TargetType>
				inline bool copy(TargetType **target, bool cmpByStr = false) {
					if (*target != nullptr) {
						(*target)->decRef();
						*target = nullptr;
					}
					return m_ptr->copy(reinterpret_cast<void**>(target), TargetType::GetRefName(), cmpByStr);
				}
			};
		}
	}

	template<typename Interface>
	class IPtr : public Hidden::IPtr::Copyable<Interface, std::is_base_of<WinFW::Copyable, Interface>::value> {
		template<typename> friend class IPtr;
		using Base = Hidden::IPtr::Copyable<Interface, std::is_base_of<WinFW::Copyable, Interface>::value>;

		inline IPtr& copyPtr(Interface *ptr) {
			decRef();
			m_ptr = ptr;
			incRef();
			return *this;
		}

		inline IPtr& movePtr(Interface *&&ptr) {
			decRef();
			m_ptr = ptr;
			ptr = nullptr;
			return *this;
		}
	public:
		inline ~IPtr() { decRef(); }
		inline IPtr() : Base(nullptr) {}
		inline IPtr(decltype(nullptr)) : Base(nullptr) {}

		template<typename SomeInterface>
		inline IPtr(SomeInterface *const &rhs) : Base(static_cast<Interface*>(rhs)) { incRef(); }

		template<typename SomeInterface>
		inline IPtr(SomeInterface *&&rhs) : Base(static_cast<Interface*>(rhs)) { rhs = nullptr; }

		inline IPtr(const IPtr& rhs) : IPtr(static_cast<Interface*const&>(rhs.m_ptr)) {}

		inline IPtr(IPtr &&rhs) : IPtr(static_cast<Interface*&&>(rhs.m_ptr)) {}

		template<typename SomeInterface>
		inline IPtr(const IPtr<SomeInterface> &rhs) : IPtr(static_cast<Interface*const&>(rhs.m_ptr)) {}

		template<typename SomeInterface>
		inline IPtr(IPtr<SomeInterface> &&rhs) : IPtr(static_cast<Interface*&&>(rhs.m_ptr)) {}

		inline IPtr& operator=(decltype(__nullptr)) {
			decRef();
			return *this;
		}

		template<typename SomeInterface>
		inline IPtr& operator=(SomeInterface *&rhs) {
			return copyPtr(static_cast<Interface*>(rhs));
		}

		template<typename SomeInterface>
		inline IPtr& operator=(SomeInterface *&&rhs) {
			return movePtr(static_cast<Interface*&&>(rhs));
		}

		inline IPtr& operator=(const IPtr& rhs) {
			return copyPtr(static_cast<Interface*>(rhs.m_ptr));
		}

		inline IPtr& operator=(IPtr &&rhs) {
			return movePtr(static_cast<Interface*&&>(rhs.m_ptr));
		}

		template<typename SomeInterface>
		inline IPtr& operator=(const IPtr<SomeInterface> &rhs) {
			return copyPtr(static_cast<Interface*>(rhs.m_ptr));
		}

		template<typename SomeInterface>
		inline IPtr& operator=(IPtr<SomeInterface> &&rhs) {
			return movePtr(static_cast<Interface*&&>(rhs.m_ptr));
		}

		template<typename SomeInterface>
		inline bool operator==(SomeInterface *ptr) {
			return m_ptr == ptr;
		}

		template<typename SomeInterface>
		inline bool operator!=(SomeInterface *ptr) {
			return m_ptr == ptr;
		}

		inline bool operator==(const IPtr& rhs) {
			return m_ptr == rhs.m_ptr;
		}

		template<typename SomeInterface>
		inline bool operator==(const IPtr<SomeInterface> &rhs) {
			return m_ptr == rhs.m_ptr;
		}

		inline bool operator!=(const IPtr& rhs) {
			return m_ptr != rhs.m_ptr;
		}

		template<typename SomeInterface>
		inline bool operator!=(const IPtr<SomeInterface> &rhs) {
			return m_ptr != rhs.m_ptr;
		}
	};

	namespace Text {
		class StringHolder : public virtual Copyable {
		public:
			DLL_DECLSPEC static const char* GetRefName();
			DLL_DECLSPEC static StringHolder* New(const char*);
			DLL_DECLSPEC static StringHolder* New(const char*, size_t);

			virtual size_t getSize() = 0;
			virtual char* getString() = 0;
		};

		class WStringHolder : public virtual Copyable {
		public:
			DLL_DECLSPEC static const char* GetRefName();
			DLL_DECLSPEC static WStringHolder* New(const wchar_t*);
			DLL_DECLSPEC static WStringHolder* New(const wchar_t*, size_t);

			virtual size_t getSize() = 0;
			virtual wchar_t* getWString() = 0;
		};
	}

	namespace IO {
		class DLL_DECLSPEC MsgBox {
		public:
			static void notify(const char*, const char*);
			static void notify(const wchar_t*, const wchar_t*);
			static void error(const char*, const char*);
			static void error(const wchar_t*, const wchar_t*);
		};
	}

	namespace Exception {
		class Exception : public virtual Ref {
		public:
			DLL_DECLSPEC static const char* GetRefName();
			DLL_DECLSPEC static Exception* New(const char*);

			virtual const char* getMsg() = 0;
			virtual void showMsg() = 0;
		};

		class InvalidObjectException : public virtual Exception {
		public:
			DLL_DECLSPEC static const char* GetRefName();
			DLL_DECLSPEC static InvalidObjectException* New(const char*);
		};
	}
	
	class WinClassStyle : public virtual Copyable {
	public:
		DLL_DECLSPEC static const char* GetRefName();
		DLL_DECLSPEC static WinClassStyle* New();

		virtual WinClassStyle* clear() = 0;
		virtual WinClassStyle* VRedraw() = 0;
		virtual WinClassStyle* HRedraw() = 0;
		virtual WinClassStyle* DBLCLKS() = 0;
		virtual WinClassStyle* OwnDC() = 0;
		virtual WinClassStyle* ClassDC() = 0;
		virtual WinClassStyle* ParentDC() = 0;
		virtual WinClassStyle* NoClose() = 0;
		virtual WinClassStyle* SaveBits() = 0;
		virtual WinClassStyle* ByteAlignClient() = 0;
		virtual WinClassStyle* ByteAlignWindow() = 0;
		virtual WinClassStyle* GlobalClass() = 0;
		virtual WinClassStyle* DropShadow() = 0;
	};

	class WinClassConfig : public virtual Copyable {
	public:
		DLL_DECLSPEC static const char* GetRefName();
		DLL_DECLSPEC static WinClassConfig* New(LPCWSTR, WNDPROC);

		virtual WinClassConfig* setWndProc(WNDPROC) = 0;
		virtual WinClassConfig* setClassName(LPCWSTR) = 0;
		virtual WinClassConfig* setStyle(WinClassStyle*&) = 0;
		virtual WinClassConfig* setStyle(WinClassStyle*&&) = 0;
		virtual WinClassConfig* setClsExtraBytes(int) = 0;
		virtual WinClassConfig* setWndExtraBytes(int) = 0;
		virtual WinClassConfig* setIcon(HICON) = 0;
		virtual WinClassConfig* setCursor(HCURSOR) = 0;
		virtual WinClassConfig* setBackgroundColor(HBRUSH) = 0;
		virtual WinClassConfig* setIconSm(HICON) = 0;
		virtual WinClassConfig* setMenuName(LPCWSTR) = 0;
	};

	class WinClass : public virtual Ref {
	public:
		DLL_DECLSPEC static const char* GetRefName();
		DLL_DECLSPEC static WinClass* New(WinClassConfig*&);
		DLL_DECLSPEC static WinClass* New(WinClassConfig*&&);

		virtual LPCWSTR getName() const = 0;
	};

	class WindowStyle : public virtual Copyable {
	public:
		DLL_DECLSPEC static const char* GetRefName();
		DLL_DECLSPEC static WindowStyle* New();

		virtual WindowStyle* clear() = 0;
		virtual WindowStyle* Caption() = 0;
		virtual WindowStyle* Child() = 0;
		virtual WindowStyle* ChildWindow() = 0;
		virtual WindowStyle* ClipChildren() = 0;
		virtual WindowStyle* ClipSiblings() = 0;
		virtual WindowStyle* Disabled() = 0;
		virtual WindowStyle* DLGFrame() = 0;
		virtual WindowStyle* Group() = 0;
		virtual WindowStyle* HScroll() = 0;
		virtual WindowStyle* Iconic() = 0;
		virtual WindowStyle* Maximize() = 0;
		virtual WindowStyle* MaximizeBox() = 0;
		virtual WindowStyle* Minimize() = 0;
		virtual WindowStyle* MinimizeBox() = 0;
		virtual WindowStyle* Overlapped() = 0;
		virtual WindowStyle* OverlappedWindow() = 0;
		virtual WindowStyle* PopUp() = 0;
		virtual WindowStyle* PopUpWindow() = 0;
		virtual WindowStyle* SizeBox() = 0;
		virtual WindowStyle* SysMenu() = 0;
		virtual WindowStyle* TabStop() = 0;
		virtual WindowStyle* ThickFrame() = 0;
		virtual WindowStyle* Tiled() = 0;
		virtual WindowStyle* TiledWindow() = 0;
		virtual WindowStyle* Visible() = 0;
		virtual WindowStyle* VScroll() = 0;
	};

	class WindowExStyle : public virtual Copyable {
	public:
		DLL_DECLSPEC static const char* GetRefName();
		DLL_DECLSPEC static WindowExStyle* New();

		virtual WindowExStyle* clear() = 0;
		virtual WindowExStyle* AcceptFiles() = 0;
		virtual WindowExStyle* AppWindow() = 0;
		virtual WindowExStyle* ClientEdge() = 0;
		virtual WindowExStyle* Composited() = 0;
		virtual WindowExStyle* ContextHelp() = 0;
		virtual WindowExStyle* ControlParent() = 0;
		virtual WindowExStyle* DLGModalFrame() = 0;
		virtual WindowExStyle* Layered() = 0;
		virtual WindowExStyle* LayoutRTL() = 0;
		virtual WindowExStyle* Left() = 0;
		virtual WindowExStyle* LeftScrollBar() = 0;
		virtual WindowExStyle* LTRReadding() = 0;
		virtual WindowExStyle* MDIChild() = 0;
		virtual WindowExStyle* NoActivate() = 0;
		virtual WindowExStyle* NoInheriteLayout() = 0;
		virtual WindowExStyle* NoParentNotify() = 0;
		virtual WindowExStyle* NoRedirectBitmap() = 0;
		virtual WindowExStyle* OverlappedWindow() = 0;
		virtual WindowExStyle* PaletteWindow() = 0;
		virtual WindowExStyle* Right() = 0;
		virtual WindowExStyle* RightScrollBar() = 0;
		virtual WindowExStyle* RTLReadding() = 0;
		virtual WindowExStyle* StaticEdge() = 0;
		virtual WindowExStyle* ToolWindow() = 0;
		virtual WindowExStyle* TopMost() = 0;
		virtual WindowExStyle* Transparent() = 0;
		virtual WindowExStyle* WindowEdge() = 0;
	};

	class WindowConfig : public virtual Copyable {
	public:
		DLL_DECLSPEC static const char* GetRefName();
		DLL_DECLSPEC static WindowConfig* New(WinClass*&, int, int);
		DLL_DECLSPEC static WindowConfig* New(WinClass*&&, int, int);

		virtual WindowConfig* setX(int) = 0;
		virtual WindowConfig* setY(int) = 0;
		virtual WindowConfig* setWidth(int) = 0;
		virtual WindowConfig* setHeight(int) = 0;
		virtual WindowConfig* setWinClass(WinClass*&) = 0;
		virtual WindowConfig* setWinClass(WinClass*&&) = 0;
		virtual WindowConfig* setStyle(WindowStyle*&) = 0;
		virtual WindowConfig* setStyle(WindowStyle*&&) = 0;
		virtual WindowConfig* setExStyle(WindowExStyle*&) = 0;
		virtual WindowConfig* setExStyle(WindowExStyle*&&) = 0;
		virtual WindowConfig* setParent(HWND) = 0;
		virtual WindowConfig* setMenu(HMENU) = 0;
		virtual WindowConfig* setLpParam(LPVOID) = 0;
		virtual WindowConfig* setTitle(LPCWSTR) = 0;
	};

	class Window : public virtual Ref {
	public:
		DLL_DECLSPEC static const char* GetRefName();
		DLL_DECLSPEC static Window* New(WindowConfig*&, bool = true);
		DLL_DECLSPEC static Window* New(WindowConfig*&&, bool = true);

		virtual HWND get() = 0;
		virtual BOOL setTitle(LPCWSTR) = 0;
		virtual BOOL hide() = 0;
		virtual BOOL show() = 0;
		virtual BOOL minimize() = 0;
		virtual BOOL update() = 0;
		virtual BOOL setPos(int, int) = 0;
		virtual BOOL setSize(int, int) = 0;
		virtual BOOL setClientSize(int, int) = 0;
		virtual BOOL querySize(RECT*) const = 0;
		virtual BOOL queryClientSize(RECT*) const = 0;
	};

	class DLL_DECLSPEC EventLoop {
	public:
		static void init();
		static void destroy();
		static bool fps(UINT);
		static MSG getMSG();
		static INT64 getCurrentCount();
		static INT64 getCountPerSecond();
		static INT64 getCountLastLoop();
		static INT64 getCountLastFrame();
		static double getTimePerFrame();
		static double getTimePerLoop();
		static bool isActive(HWND = nullptr, UINT = 0, UINT = 0, UINT = PM_REMOVE);
	};

	enum class KeyAction {
		NoAction,
		Press,
		Release
	};

	class Keyboard : public virtual Ref {
	public:
		DLL_DECLSPEC static const char* GetRefName();
		DLL_DECLSPEC static Keyboard* New();

		virtual BOOL update() = 0;
		virtual KeyAction getKeyAction(BYTE) = 0;
		virtual bool isPress(BYTE) = 0;
	};

	class Mouse : public virtual Ref {
	public:
		DLL_DECLSPEC static const char* GetRefName();
		DLL_DECLSPEC static Mouse* New();

		DLL_DECLSPEC static void useRawInputMouse(Window* = nullptr);
		DLL_DECLSPEC static void disableRawInputMouse();

		virtual BOOL updatePos() = 0;
		virtual POINT getPos() = 0;
		virtual int getPosX() = 0;
		virtual int getPosY() = 0;
		virtual void updateRawMouseMove(LPARAM) = 0;
		virtual POINT passMove() = 0;
	};
}

#ifdef USE_MAIN
int main(HINSTANCE, char*, int);

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	UNREFERENCED_PARAMETER(hPrevInstance);
	WinFW::init(hInstance);
	try {
		return main(hInstance, lpCmdLine, nCmdShow);
	}
	catch (WinFW::Exception::Exception *e) {
		e->showMsg();
		e->decRef();
		return -1;
	}
	catch (...) {
		WinFW::IO::MsgBox::error("Exception", "Unknown Exception");
	}
}
#endif

#pragma pop_macro("DLL_DECLSPEC") 
#pragma pop_macro("WIN32_LEAN_AND_MEAN")