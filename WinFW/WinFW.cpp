#include "WinFW.hpp"
#include <string>
#include <cstring>
#include <windowsx.h>

#pragma warning(disable : 4250)

#define Interface_GetRefName(name)	const char* name::GetRefName() { return #name; }

Interface_GetRefName(WinFW::Ref)
Interface_GetRefName(WinFW::Copyable)
Interface_GetRefName(WinFW::Text::StringHolder)
Interface_GetRefName(WinFW::Text::WStringHolder)
Interface_GetRefName(WinFW::Exception::Exception)
Interface_GetRefName(WinFW::Exception::InvalidObjectException)
Interface_GetRefName(WinFW::WinClass)
Interface_GetRefName(WinFW::WinClassStyle)
Interface_GetRefName(WinFW::WinClassConfig)
Interface_GetRefName(WinFW::Window)
Interface_GetRefName(WinFW::WindowStyle)
Interface_GetRefName(WinFW::WindowExStyle)
Interface_GetRefName(WinFW::WindowConfig)
Interface_GetRefName(WinFW::Keyboard)
Interface_GetRefName(WinFW::Mouse)

HINSTANCE g_hInstance;

// IO
namespace WinFW {
	namespace IO {
		void MsgBox::notify(const char *title, const char *body) {
			MessageBoxA(nullptr, body, title, MB_OK | MB_ICONINFORMATION);
		}

		void MsgBox::notify(const wchar_t *title, const wchar_t *body) {
			MessageBoxW(nullptr, body, title, MB_OK | MB_ICONINFORMATION);
		}

		void MsgBox::error(const char *title, const char *body) {
			MessageBoxA(nullptr, body, title, MB_OK | MB_ICONERROR);
		}

		void MsgBox::error(const wchar_t *title, const wchar_t *body) {
			MessageBoxW(nullptr, body, title, MB_OK | MB_ICONERROR);
		}
	}
}

// EventLoop
namespace WinFW {
	struct EventLoop_Impl {
		static bool isRunning;
		static double tpc;
		static double tpl;
		static double tpf;
		static INT64 currTime;
		static INT64 prevLoop;
		static INT64 prevFrame;
		static MSG msg;
	};

	bool EventLoop_Impl::isRunning = false;
	double EventLoop_Impl::tpc = 0.0;
	double EventLoop_Impl::tpl = 0.0;
	double EventLoop_Impl::tpf = 0.0;
	INT64 EventLoop_Impl::currTime = 0;
	INT64 EventLoop_Impl::prevLoop = 0;
	INT64 EventLoop_Impl::prevFrame = 0;
	MSG EventLoop_Impl::msg = {};

	void EventLoop::init() {
		INT64 cps;
		EventLoop_Impl::isRunning = true;
		QueryPerformanceFrequency((LARGE_INTEGER*)&cps);
		EventLoop_Impl::tpc = 1.0 / cps;
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&EventLoop_Impl::prevLoop));
		EventLoop_Impl::prevFrame = EventLoop_Impl::prevLoop;
	}

	INT64 EventLoop::getCountPerSecond() {
		INT64 cps;
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&cps));
		return cps;
	}

	INT64 EventLoop::getCurrentCount() {
		INT64 curr;
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&curr));
		return curr;
	}

	INT64 EventLoop::getCountLastLoop() {
		return EventLoop_Impl::prevLoop;
	}

	INT64 EventLoop::getCountLastFrame() {
		return EventLoop_Impl::prevFrame;
	}

	bool EventLoop::fps(UINT fps) {
		EventLoop_Impl::tpf = (EventLoop_Impl::currTime - EventLoop_Impl::prevFrame) * EventLoop_Impl::tpc;
		if (1.0 / EventLoop_Impl::tpf <= fps) {
			EventLoop_Impl::prevFrame = EventLoop_Impl::currTime;
			return true;
		}
		return false;
	}

	bool EventLoop::isActive(HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) {
		while (PeekMessageW(&EventLoop_Impl::msg, nullptr, wMsgFilterMin, wMsgFilterMax, wRemoveMsg)) {
			TranslateMessage(&EventLoop_Impl::msg);
			DispatchMessageW(&EventLoop_Impl::msg);
		}

		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&EventLoop_Impl::currTime));
		EventLoop_Impl::tpl = (EventLoop_Impl::currTime - EventLoop_Impl::prevLoop) * EventLoop_Impl::tpc;
		EventLoop_Impl::prevLoop = EventLoop_Impl::currTime;
		return EventLoop_Impl::isRunning;
	}

	void EventLoop::destroy() {
		EventLoop_Impl::isRunning = false;
	}

	MSG EventLoop::getMSG() {
		return EventLoop_Impl::msg;
	}

	double EventLoop::getTimePerFrame() {
		return EventLoop_Impl::tpf;
	}

	double EventLoop::getTimePerLoop() {
		return EventLoop_Impl::tpl;
	}
}

// Mouse
namespace WinFW {
	void Mouse::useRawInputMouse(Window *window) {
		HWND hWnd = window == nullptr ? nullptr : window->get();
		RAWINPUTDEVICE rid{ 0x01, 0x02, 0, hWnd };
		RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE));
	}

	void Mouse::disableRawInputMouse() {
		RAWINPUTDEVICE rid{ 0x01, 0x02, RIDEV_REMOVE, nullptr };
		RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE));
	}
}

// Interface
namespace WinFW {
	void init(HINSTANCE hInstance) {
		g_hInstance = hInstance;
	}
	
	class Ref_Impl : public virtual Ref {
		unsigned long long m_refCount;

		bool setInterface(void **const ppRef) {
			if (ppRef != nullptr) {
				incRef();
				*ppRef = static_cast<Ref*>(this);
			}
			return true;
		}
	protected:
		virtual bool queryRefByCmpPtr(void **const ppRef, const char *id) {
			if (id == Ref::GetRefName()) return setInterface(ppRef);
			else return false;
		}

		virtual bool queryRefByCmpStr(void **const ppRef, const char *id) {
			if (std::strcmp(id, Ref::GetRefName()) == 0) return setInterface(ppRef);
			else return false;
		}
	public:
		virtual ~Ref_Impl() = default;

		Ref_Impl() : Ref_Impl(1ULL) {
		}

		Ref_Impl(unsigned long long refCount) : m_refCount(refCount) {
		}

		unsigned long long numRef() {
			return m_refCount;
		}

		unsigned long long incRef() {
			return InterlockedIncrement(&m_refCount);
		}

		unsigned long long decRef() {
			unsigned long long res = InterlockedDecrement(&m_refCount);
			if (res == 0) delete this;
			return res;
		}

		bool delRef() {
			delete this;
			return true;
		}

		bool queryRef(void **const ppRef, const char *id, bool b) {
			if (b) return queryRefByCmpPtr(ppRef, id);
			else return queryRefByCmpStr(ppRef, id);
		}

		const char* getRefName() {
			return Ref::GetRefName();
		}
	};

	class Copyable_Impl : public virtual Copyable, public virtual Ref_Impl {
		bool setInterface(void **const ppRef) {
			if (ppRef != nullptr) {
				incRef();
				*ppRef = static_cast<Copyable*>(this);
			}
			return true;
		}
	protected:
		bool queryRefByCmpPtr(void **const ppRef, const char *id) {
			if (id == Copyable::GetRefName()) return setInterface(ppRef);
			else return Ref_Impl::queryRefByCmpPtr(ppRef, id);
		}

		bool queryRefByCmpStr(void **const ppRef, const char *id) {
			if (std::strcmp(id, Copyable::GetRefName()) == 0) return setInterface(ppRef);
			else return Ref_Impl::queryRefByCmpStr(ppRef, id);
		}

		virtual bool copyByCmpPtr(void **const ppRef, const char *id) {
			return queryRefByCmpPtr(ppRef, id);
		}

		virtual bool copyByCmpStr(void **const ppRef, const char *id) {
			return queryRefByCmpStr(ppRef, id);
		}
	public:
		Copyable_Impl() : Copyable_Impl(1ULL) {
		}

		Copyable_Impl(unsigned long long refCount) : Ref_Impl(refCount) {
		}

		const char* getRefName() {
			return Copyable::GetRefName();
		}

		bool copy(void **const ppRef, const char *id, bool b) {
			if (b) return copyByCmpPtr(ppRef, id);
			else return copyByCmpStr(ppRef, id);
		}
	};

	namespace Text {
		class StringHolder_Impl : public virtual StringHolder, public virtual Copyable_Impl {
			char *m_str;
			size_t m_count;

			bool setInterface(void **const ppRef) {
				if (ppRef != nullptr) {
					incRef();
					*ppRef = static_cast<StringHolder*>(this);
				}
				return true;
			}

			bool copyInterface(void **const ppRef) {
				if (ppRef != nullptr) {
					try {
						size_t countBuff = m_count + 1;
						char *buff = new char[countBuff];

						try {
							std::memcpy(buff, m_str, countBuff * sizeof(char));
							*ppRef = static_cast<StringHolder*>(new StringHolder_Impl(buff, m_count));
						}
						catch (...) {
							delete[] buff;
							return false;
						}
					}
					catch (...) {
						return false;
					}
				}
				return true;
			}
		protected:
			bool queryRefByCmpPtr(void **const ppRef, const char *id) {
				if (id == StringHolder::GetRefName()) return setInterface(ppRef);
				else return Copyable_Impl::queryRefByCmpPtr(ppRef, id);
			}

			bool queryRefByCmpStr(void **const ppRef, const char *id) {
				if (std::strcmp(id, StringHolder::GetRefName()) == 0) return setInterface(ppRef);
				else return Copyable_Impl::queryRefByCmpStr(ppRef, id);
			}

			bool copyByCmpPtr(void **const ppRef, const char *id) {
				if (id == StringHolder::GetRefName()) return copyInterface(ppRef);
				else return Copyable_Impl::copyByCmpPtr(ppRef, id);
			}

			bool copyByCmpStr(void **const ppRef, const char *id) {
				if (std::strcmp(id, StringHolder::GetRefName()) == 0) return copyInterface(ppRef);
				else return Copyable_Impl::copyByCmpStr(ppRef, id);
			}
		public:
			~StringHolder_Impl() {
				if (m_str != nullptr) delete[] m_str;
			}

			StringHolder_Impl(char *str, size_t count) : m_str(str), m_count(count) {
			}

			const char* getRefName() {
				return StringHolder::GetRefName();
			}

			size_t getSize() {
				return m_count;
			}

			char* getString() {
				return m_str;
			}
		};

		class WStringHolder_Impl : public virtual WStringHolder, public virtual Copyable_Impl {
			wchar_t *m_str;
			size_t m_count;

			bool setInterface(void **const ppRef) {
				if (ppRef != nullptr) {
					incRef();
					*ppRef = static_cast<WStringHolder*>(this);
				}
				return true;
			}

			bool copyInterface(void **const ppRef) {
				if (ppRef != nullptr) {
					try {
						size_t countBuff = m_count + 1;
						wchar_t *buff = new wchar_t[countBuff];

						try {
							std::memcpy(buff, m_str, countBuff * sizeof(wchar_t));
							*ppRef = static_cast<WStringHolder*>(new WStringHolder_Impl(buff, m_count));
						}
						catch (...) {
							delete[] buff;
							return false;
						}
					}
					catch (...) {
						return false;
					}
				}
				return true;
			}
		protected:
			bool queryRefByCmpPtr(void **const ppRef, const char *id) {
				if (id == WStringHolder::GetRefName()) return setInterface(ppRef);
				else return Copyable_Impl::queryRefByCmpPtr(ppRef, id);
			}

			bool queryRefByCmpStr(void **const ppRef, const char *id) {
				if (std::strcmp(id, WStringHolder::GetRefName()) == 0) return setInterface(ppRef);
				else return Copyable_Impl::queryRefByCmpStr(ppRef, id);
			}

			bool copyByCmpPtr(void **const ppRef, const char *id) {
				if (id == WStringHolder::GetRefName()) return copyInterface(ppRef);
				else return Copyable_Impl::copyByCmpPtr(ppRef, id);
			}

			bool copyByCmpStr(void **const ppRef, const char *id) {
				if (std::strcmp(id, WStringHolder::GetRefName()) == 0) return copyInterface(ppRef);
				else return Copyable_Impl::copyByCmpStr(ppRef, id);
			}
		public:
			~WStringHolder_Impl() {
				if (m_str != nullptr) delete[] m_str;
			}

			WStringHolder_Impl(wchar_t *str, size_t count) : m_str(str), m_count(count) {
			}

			const char* getRefName() {
				return WStringHolder::GetRefName();
			}

			size_t getSize() {
				return m_count;
			}

			wchar_t* getWString() {
				return m_str;
			}
		};
	}

	namespace Exception {
		class Exception_Impl : public virtual Exception, public virtual Ref_Impl {
			Text::StringHolder *m_msg;

			bool setInterface(void **const ppRef) {
				if (ppRef != nullptr) {
					incRef();
					*ppRef = static_cast<Exception*>(this);
				}
				return true;
			}
		protected:
			bool queryRefByCmpPtr(void **const ppRef, const char *id) {
				if (id == Exception::GetRefName()) return setInterface(ppRef);
				else return Ref_Impl::queryRefByCmpPtr(ppRef, id);
			}

			bool queryRefByCmpStr(void **const ppRef, const char *id) {
				if (std::strcmp(id, Exception::GetRefName()) == 0) return setInterface(ppRef);
				else return Ref_Impl::queryRefByCmpStr(ppRef, id);
			}
		public:
			~Exception_Impl() {
				m_msg->decRef();
			}

			Exception_Impl(Text::StringHolder *msg) : m_msg(msg) {
			}

			const char* getRefName() const {
				return Exception::GetRefName();
			}

			const char* getMsg() {
				return m_msg->getString();
			}

			void showMsg() {
				IO::MsgBox::error("Exception", m_msg->getString());
			}
		};

		class InvalidObjectException_Impl : public virtual InvalidObjectException, public virtual Exception_Impl {
			bool setInterface(void **const ppRef) {
				if (ppRef != nullptr) {
					incRef();
					*ppRef = static_cast<InvalidObjectException*>(this);
				}
				return true;
			}
		protected:
			bool queryRefByCmpPtr(void **const ppRef, const char *id) {
				if (id == InvalidObjectException::GetRefName()) return setInterface(ppRef);
				else return Exception_Impl::queryRefByCmpPtr(ppRef, id);
			}

			bool queryRefByCmpStr(void **const ppRef, const char *id) {
				if (std::strcmp(id, InvalidObjectException::GetRefName()) == 0) return setInterface(ppRef);
				else return Exception_Impl::queryRefByCmpStr(ppRef, id);
			}
		public:
			InvalidObjectException_Impl(Text::StringHolder *msg) : Exception_Impl(msg) {
			}

			const char* getRefName() const {
				return InvalidObjectException::GetRefName();
			}

			void showMsg() {
				IO::MsgBox::error("InvalidObject", getMsg());
			}
		};
	}

	class WinClassStyle_Impl : public virtual WinClassStyle, public virtual Copyable_Impl {
		UINT m_value = 0;

		bool setInterface(void **const ppRef) {
			if (ppRef != nullptr) {
				incRef();
				*ppRef = static_cast<WinClassStyle*>(this);
			}
			return true;
		}

		bool copyInterface(void **const ppRef) {
			if (ppRef != nullptr) {
				try {
					*ppRef = static_cast<WinClassStyle*>(new WinClassStyle_Impl(m_value));
				}
				catch (...) {
					return false;
				}
			}
			return true;
		}
	protected:
		bool queryRefByCmpPtr(void **const ppRef, const char *id) {
			if (id == WinClassStyle::GetRefName()) return setInterface(ppRef);
			else if (id == WinClassStyle_Impl::GetRefName()) {
				if (ppRef != nullptr) *ppRef = this;
				return true;
			}
			else return Copyable_Impl::queryRefByCmpPtr(ppRef, id);
		}

		bool queryRefByCmpStr(void **const ppRef, const char *id) {
			if (std::strcmp(id, WinClassStyle::GetRefName()) == 0) return setInterface(ppRef);
			else return Copyable_Impl::queryRefByCmpStr(ppRef, id);
		}

		bool copyByCmpPtr(void **const ppRef, const char *id) {
			if (id == WinClassStyle::GetRefName()) return copyInterface(ppRef);
			else return Copyable_Impl::copyByCmpPtr(ppRef, id);
		}

		bool copyByCmpStr(void **const ppRef, const char *id) {
			if (std::strcmp(id, WinClassStyle::GetRefName()) == 0) return copyInterface(ppRef);
			else return Copyable_Impl::copyByCmpStr(ppRef, id);
		}
	public:
		static const char* GetRefName() {
			return "WinFW::WinClassStyle_Impl";
		}

		WinClassStyle_Impl() : WinClassStyle_Impl(0) {
		}

		WinClassStyle_Impl(UINT value) : m_value(value) {
		}

		const char* getRefName() const {
			return WinClassStyle::GetRefName();
		}

		virtual UINT getValue() {
			return m_value;
		}

		WinClassStyle* clear() {
			m_value = 0;
			return this;
		}

		WinClassStyle* VRedraw() {
			m_value |= CS_VREDRAW;
			return this;
		}

		WinClassStyle* HRedraw() {
			m_value |= CS_HREDRAW;
			return this;
		}

		WinClassStyle* DBLCLKS() {
			m_value |= CS_DBLCLKS;
			return this;
		}

		WinClassStyle* OwnDC() {
			m_value |= CS_OWNDC;
			return this;
		}

		WinClassStyle* ClassDC() {
			m_value |= CS_CLASSDC;
			return this;
		}

		WinClassStyle* ParentDC() {
			m_value |= CS_PARENTDC;
			return this;
		}

		WinClassStyle* NoClose() {
			m_value |= CS_NOCLOSE;
			return this;
		}

		WinClassStyle* SaveBits() {
			m_value |= CS_SAVEBITS;
			return this;
		}

		WinClassStyle* ByteAlignClient() {
			m_value |= CS_BYTEALIGNCLIENT;
			return this;
		}

		WinClassStyle* ByteAlignWindow() {
			m_value |= CS_BYTEALIGNWINDOW;
			return this;
		}

		WinClassStyle* GlobalClass() {
			m_value |= CS_GLOBALCLASS;
			return this;
		}

		WinClassStyle* DropShadow() {
			m_value |= CS_DROPSHADOW;
			return this;
		}
	};

	class WinClassConfig_Impl : public virtual WinClassConfig, public virtual Copyable_Impl {
		UINT m_style;
		int	m_cbClsExtra;
		int	m_cbWndExtra;
		HICON m_hIcon;
		HCURSOR	m_hCursor;
		HBRUSH m_hbrBackground;
		HICON m_hIconSm;
		Text::WStringHolder *m_lpszMenuName;
		Text::WStringHolder *m_lpszClassName;
		WNDPROC m_lpfnWndProc;

		bool setInterface(void **const ppRef) {
			if (ppRef != nullptr) {
				incRef();
				*ppRef = static_cast<WinClassConfig*>(this);
			}
			return true;
		}

		bool copyInterface(void **const ppRef) {
			if (ppRef != nullptr) {
				try {
					m_lpszClassName->incRef();
					m_lpszMenuName->incRef();
					*ppRef = static_cast<WinClassConfig*>(new WinClassConfig_Impl(m_style, m_cbClsExtra, m_cbWndExtra, m_hIcon, m_hCursor,
						m_hbrBackground, m_hIconSm, m_lpszMenuName, m_lpszClassName, m_lpfnWndProc));
				}
				catch (...) {
					m_lpszClassName->decRef();
					m_lpszMenuName->decRef();
					return false;
				}
			}
			return true;
		}
	protected:
		bool queryRefByCmpPtr(void **const ppRef, const char *id) {
			if (id == WinClassConfig::GetRefName()) return setInterface(ppRef);
			else if (id == WinClassConfig_Impl::GetRefName()) {
				if (ppRef != nullptr) *ppRef = this;
				return true;
			}
			else return Copyable_Impl::queryRefByCmpPtr(ppRef, id);
		}

		bool queryRefByCmpStr(void **const ppRef, const char *id) {
			if (std::strcmp(id, WinClassConfig::GetRefName()) == 0) return setInterface(ppRef);
			else return Copyable_Impl::queryRefByCmpStr(ppRef, id);
		}

		bool copyByCmpPtr(void **const ppRef, const char *id) {
			if (id == WinClassConfig::GetRefName()) return copyInterface(ppRef);
			else return Copyable_Impl::copyByCmpPtr(ppRef, id);
		}

		bool copyByCmpStr(void **const ppRef, const char *id) {
			if (std::strcmp(id, WinClassConfig::GetRefName()) == 0) return copyInterface(ppRef);
			else return Copyable_Impl::copyByCmpStr(ppRef, id);
		}
	public:
		static const char* GetRefName() {
			return "WinFW::WinClassConfig_Impl";
		}

		~WinClassConfig_Impl() {
			m_lpszMenuName->decRef();
			m_lpszClassName->decRef();
		}

		WinClassConfig_Impl(Text::WStringHolder *lpszClassName, WNDPROC lpfnWndProc) : WinClassConfig_Impl(CS_HREDRAW | CS_VREDRAW, 0, 0, nullptr, 
			LoadCursorW(NULL, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1), nullptr, Text::WStringHolder::New(nullptr),
			lpszClassName, lpfnWndProc) {
		}

		WinClassConfig_Impl(UINT style, int cbClsExtra,	int cbWndExtra, HICON hIcon, HCURSOR hCursor, HBRUSH hbrBackground, 
			HICON hIconSm, Text::WStringHolder *lpszMenuName, Text::WStringHolder *lpszClassName, WNDPROC lpfnWndProc) : m_style(style),
			m_cbClsExtra(cbClsExtra), m_cbWndExtra(cbWndExtra), m_hIcon(hIcon), m_hCursor(hCursor), m_hbrBackground(hbrBackground),
			m_hIconSm(hIconSm), m_lpszMenuName(lpszMenuName), m_lpszClassName(lpszClassName), m_lpfnWndProc(lpfnWndProc) {
		}

		const char* getRefName() const {
			return WinClassConfig::GetRefName();
		}

		WinClassConfig* setWndProc(WNDPROC lpfnWndProc) {
			m_lpfnWndProc = lpfnWndProc;
			return this;
		}

		WinClassConfig* setClassName(LPCWSTR lpszClassName) {
			m_lpszClassName->decRef();
			m_lpszClassName = Text::WStringHolder::New(lpszClassName);
			return this;
		}

		WinClassConfig* setStyle(WinClassStyle *&style) {
			WinClassStyle_Impl *buff;
			if (!style->queryRef(reinterpret_cast<void**>(&buff), WinClassStyle_Impl::GetRefName(), true)) throw Exception::InvalidObjectException::New("WinClassStyle : incompatible");
			m_style = buff->getValue();
			return this;
		}

		WinClassConfig* setStyle(WinClassStyle *&&style) {
			WinClassConfig *buff = setStyle(static_cast<WinClassStyle*&>(style));
			style->decRef();
			return buff;
		}

		WinClassConfig* setClsExtraBytes(int cbClsExtra) {
			m_cbClsExtra = cbClsExtra;
			return this;
		}

		WinClassConfig* setWndExtraBytes(int cbWndExtra) {
			m_cbWndExtra = cbWndExtra;
			return this;
		}

		WinClassConfig* setIcon(HICON hIcon) {
			m_hIcon = hIcon;
			return this;
		}

		WinClassConfig* setCursor(HCURSOR hCursor) {
			m_hCursor = hCursor;
			return this;
		}

		WinClassConfig* setMenuName(LPCWSTR lpszMenuName) {
			m_lpszMenuName->decRef();
			m_lpszMenuName = Text::WStringHolder::New(lpszMenuName);
			return this;
		}

		WinClassConfig* setBackgroundColor(HBRUSH hbrBackground) {
			m_hbrBackground = hbrBackground;
			return this;
		}

		WinClassConfig* setIconSm(HICON hIconSm) {
			m_hIconSm = hIconSm;
			return this;
		}

		virtual UINT getStyle() {
			return m_style;
		}

		virtual int getClsExtraBytes() {
			return m_cbClsExtra;
		}

		virtual int getWndExtraBytes() {
			return m_cbWndExtra;
		}

		virtual HICON getIcon() {
			return m_hIcon;
		}

		virtual HCURSOR getCursor() {
			return m_hCursor;
		}

		virtual HBRUSH getBackgroundColor() {
			return m_hbrBackground;
		}

		virtual HICON getIconSm() {
			return m_hIconSm;
		}

		virtual LPCWSTR getMenuName() {
			return m_lpszMenuName->getWString();
		}

		virtual WNDPROC getWndProc() {
			return m_lpfnWndProc;
		}

		virtual LPCWSTR getClassName() {
			return m_lpszClassName->getWString();
		}
	};

	class WinClass_Impl : public virtual WinClass, public virtual Ref_Impl {
		Text::WStringHolder *m_name;

		bool setInterface(void **const ppRef) {
			if (ppRef != nullptr) {
				incRef();
				*ppRef = static_cast<WinClass*>(this);
			}
			return true;
		}
	protected:
		bool queryRefByCmpPtr(void **const ppRef, const char *id) {
			if (id == WinClass::GetRefName()) return setInterface(ppRef);
			else return Ref_Impl::queryRefByCmpPtr(ppRef, id);
		}

		bool queryRefByCmpStr(void **const ppRef, const char *id) {
			if (std::strcmp(id, WinClass::GetRefName()) == 0) return setInterface(ppRef);
			else return Ref_Impl::queryRefByCmpStr(ppRef, id);
		}
	public:
		~WinClass_Impl() {
			UnregisterClassW(m_name->getWString(), g_hInstance);
			m_name->decRef();
		}

		WinClass_Impl(Text::WStringHolder *name) : m_name(name) {
		}

		const char* getRefName() const {
			return WinClassConfig::GetRefName();
		}

		LPCWSTR getName() const {
			return m_name->getWString();
		}
	};

	class WindowStyle_Impl : public virtual WindowStyle, public virtual Copyable_Impl {
		DWORD m_style = 0;

		bool setInterface(void **const ppRef) {
			if (ppRef != nullptr) {
				incRef();
				*ppRef = static_cast<WindowStyle*>(this);
			}
			return true;
		}

		bool copyInterface(void **const ppRef) {
			if (ppRef != nullptr) {
				try {
					*ppRef = static_cast<WindowStyle*>(new WindowStyle_Impl(m_style));
				}
				catch (...) {
					return false;
				}
			}
			return true;
		}
	protected:
		bool queryRefByCmpPtr(void **const ppRef, const char *id) {
			if (id == WindowStyle::GetRefName()) return setInterface(ppRef);
			else if (id == WindowStyle_Impl::GetRefName()) {
				if (ppRef != nullptr) *ppRef = this;
				return true;
			}
			else return Copyable_Impl::queryRefByCmpPtr(ppRef, id);
		}

		bool queryRefByCmpStr(void **const ppRef, const char *id) {
			if (std::strcmp(id, WindowStyle::GetRefName()) == 0) return setInterface(ppRef);
			else return Copyable_Impl::queryRefByCmpStr(ppRef, id);
		}

		bool copyByCmpPtr(void **const ppRef, const char *id) {
			if (id == WindowStyle::GetRefName()) return copyInterface(ppRef);
			else return Copyable_Impl::copyByCmpPtr(ppRef, id);
		}

		bool copyByCmpStr(void **const ppRef, const char *id) {
			if (std::strcmp(id, WindowStyle::GetRefName()) == 0) return copyInterface(ppRef);
			else return Copyable_Impl::copyByCmpStr(ppRef, id);
		}
	public:
		static const char* GetRefName() {
			return "WinFW::WindowStyle_Impl";
		}

		WindowStyle_Impl() : WindowStyle_Impl(0) {
		}

		WindowStyle_Impl(DWORD style) : m_style(style) {
		}

		const char* getRefName() const {
			return WindowStyle::GetRefName();
		}

		virtual DWORD getValue() {
			return m_style;
		}

		WindowStyle* clear() {
			m_style = 0;
			return this;
		}

		WindowStyle* Caption() {
			m_style |= WS_CAPTION;
			return this;
		}

		WindowStyle* Child() {
			m_style |= WS_CHILD;
			return this;
		}

		WindowStyle* ChildWindow() {
			m_style |= WS_CHILDWINDOW;
			return this;
		}

		WindowStyle* ClipChildren() {
			m_style |= WS_CLIPCHILDREN;
			return this;
		}

		WindowStyle* ClipSiblings() {
			m_style |= WS_CLIPSIBLINGS;
			return this;
		}

		WindowStyle* Disabled() {
			m_style |= WS_DISABLED;
			return this;
		}

		WindowStyle* DLGFrame() {
			m_style |= WS_DLGFRAME;
			return this;
		}

		WindowStyle* Group() {
			m_style |= WS_GROUP;
			return this;
		}

		WindowStyle* HScroll() {
			m_style |= WS_HSCROLL;
			return this;
		}

		WindowStyle* Iconic() {
			m_style |= WS_ICONIC;
			return this;
		}

		WindowStyle* Maximize() {
			m_style |= WS_MAXIMIZE;
			return this;
		}

		WindowStyle* MaximizeBox() {
			m_style |= WS_MAXIMIZEBOX;
			return this;
		}

		WindowStyle* Minimize() {
			m_style |= WS_MINIMIZE;
			return this;
		}

		WindowStyle* MinimizeBox() {
			m_style |= WS_MINIMIZEBOX;
			return this;
		}

		WindowStyle* Overlapped() {
			m_style |= WS_OVERLAPPED;
			return this;
		}

		WindowStyle* OverlappedWindow() {
			m_style |= WS_OVERLAPPED;
			return this;
		}

		WindowStyle* PopUp() {
			m_style |= WS_POPUP;
			return this;
		}

		WindowStyle* PopUpWindow() {
			m_style |= WS_POPUPWINDOW;
			return this;
		}

		WindowStyle* SizeBox() {
			m_style |= WS_SIZEBOX;
			return this;
		}

		WindowStyle* SysMenu() {
			m_style |= WS_SYSMENU;
			return this;
		}

		WindowStyle* TabStop() {
			m_style |= WS_TABSTOP;
			return this;
		}

		WindowStyle* ThickFrame() {
			m_style |= WS_THICKFRAME;
			return this;
		}

		WindowStyle* Tiled() {
			m_style |= WS_TILED;
			return this;
		}

		WindowStyle* TiledWindow() {
			m_style |= WS_TILEDWINDOW;
			return this;
		}

		WindowStyle* Visible() {
			m_style |= WS_VISIBLE;
			return this;
		}

		WindowStyle* VScroll() {
			m_style |= WS_VSCROLL;
			return this;
		}
	};

	class WindowExStyle_Impl : public virtual WindowExStyle, public virtual Copyable_Impl {
		DWORD m_exStyle = 0;

		bool setInterface(void **const ppRef) {
			if (ppRef != nullptr) {
				incRef();
				*ppRef = static_cast<WindowExStyle*>(this);
			}
			return true;
		}

		bool copyInterface(void **const ppRef) {
			if (ppRef != nullptr) {
				try {
					*ppRef = static_cast<WindowExStyle*>(new WindowExStyle_Impl(m_exStyle));
				}
				catch (...) {
					return false;
				}
			}
			return true;
		}
	protected:
		bool queryRefByCmpPtr(void **const ppRef, const char *id) {
			if (id == WindowExStyle::GetRefName()) return setInterface(ppRef);
			else if (id == WindowStyle_Impl::GetRefName()) {
				if (ppRef != nullptr) *ppRef = this;
				return true;
			}
			else return Copyable_Impl::queryRefByCmpPtr(ppRef, id);
		}

		bool queryRefByCmpStr(void **const ppRef, const char *id) {
			if (std::strcmp(id, WindowExStyle::GetRefName()) == 0) return setInterface(ppRef);
			else return Copyable_Impl::queryRefByCmpStr(ppRef, id);
		}

		bool copyByCmpPtr(void **const ppRef, const char *id) {
			if (id == WindowExStyle::GetRefName()) return copyInterface(ppRef);
			else return Copyable_Impl::copyByCmpPtr(ppRef, id);
		}

		bool copyByCmpStr(void **const ppRef, const char *id) {
			if (std::strcmp(id, WindowExStyle::GetRefName()) == 0) return copyInterface(ppRef);
			else return Copyable_Impl::copyByCmpStr(ppRef, id);
		}
	public:
		static const char* GetRefName() {
			return "WinFW::WindowExStyle_Impl";
		}

		WindowExStyle_Impl() : WindowExStyle_Impl(0) {
		}

		WindowExStyle_Impl(DWORD exStyle) : m_exStyle(exStyle) {
		}

		const char* getRefName() const {
			return WindowExStyle::GetRefName();
		}

		virtual DWORD getValue() {
			return m_exStyle;
		}

		WindowExStyle* clear() {
			m_exStyle = 0;
			return this;
		}

		WindowExStyle* AcceptFiles() {
			m_exStyle |= WS_EX_ACCEPTFILES;
			return this;
		}

		WindowExStyle* AppWindow() {
			m_exStyle |= WS_EX_APPWINDOW;
			return this;
		}

		WindowExStyle* ClientEdge() {
			m_exStyle |= WS_EX_CLIENTEDGE;
			return this;
		}

		WindowExStyle* Composited() {
			m_exStyle |= WS_EX_COMPOSITED;
			return this;
		}

		WindowExStyle* ContextHelp() {
			m_exStyle |= WS_EX_CONTEXTHELP;
			return this;
		}

		WindowExStyle* ControlParent() {
			m_exStyle |= WS_EX_CONTROLPARENT;
			return this;
		}

		WindowExStyle* DLGModalFrame() {
			m_exStyle |= WS_EX_DLGMODALFRAME;
			return this;
		}

		WindowExStyle* Layered() {
			m_exStyle |= WS_EX_LAYERED;
			return this;
		}

		WindowExStyle* LayoutRTL() {
			m_exStyle |= WS_EX_LAYOUTRTL;
			return this;
		}

		WindowExStyle* Left() {
			m_exStyle |= WS_EX_LEFT;
			return this;
		}

		WindowExStyle* LeftScrollBar() {
			m_exStyle |= WS_EX_LEFTSCROLLBAR;
			return this;
		}

		WindowExStyle* LTRReadding() {
			m_exStyle |= WS_EX_LTRREADING;
			return this;
		}

		WindowExStyle* MDIChild() {
			m_exStyle |= WS_EX_MDICHILD;
			return this;
		}

		WindowExStyle* NoActivate() {
			m_exStyle |= WS_EX_NOACTIVATE;
			return this;
		}

		WindowExStyle* NoInheriteLayout() {
			m_exStyle |= WS_EX_NOINHERITLAYOUT;
			return this;
		}

		WindowExStyle* NoParentNotify() {
			m_exStyle |= WS_EX_NOPARENTNOTIFY;
			return this;
		}

		WindowExStyle* NoRedirectBitmap() {
			m_exStyle |= WS_EX_NOREDIRECTIONBITMAP;
			return this;
		}

		WindowExStyle* OverlappedWindow() {
			m_exStyle |= WS_EX_OVERLAPPEDWINDOW;
			return this;
		}

		WindowExStyle* PaletteWindow() {
			m_exStyle |= WS_EX_PALETTEWINDOW;
			return this;
		}

		WindowExStyle* Right() {
			m_exStyle |= WS_EX_RIGHT;
			return this;
		}

		WindowExStyle* RightScrollBar() {
			m_exStyle |= WS_EX_RIGHTSCROLLBAR;
			return this;
		}

		WindowExStyle* RTLReadding() {
			m_exStyle |= WS_EX_RTLREADING;
			return this;
		}

		WindowExStyle* StaticEdge() {
			m_exStyle |= WS_EX_STATICEDGE;
			return this;
		}

		WindowExStyle* ToolWindow() {
			m_exStyle |= WS_EX_TOOLWINDOW;
			return this;
		}

		WindowExStyle* TopMost() {
			m_exStyle |= WS_EX_TOPMOST;
			return this;
		}

		WindowExStyle* Transparent() {
			m_exStyle |= WS_EX_TRANSPARENT;
			return this;
		}

		WindowExStyle* WindowEdge() {
			m_exStyle |= WS_EX_WINDOWEDGE;
			return this;
		}
	};

	class WindowConfig_Impl : public virtual WindowConfig, public virtual Copyable_Impl {
		DWORD m_dwExStyle;
		Text::WStringHolder *m_lpWindowName;
		DWORD m_dwStyle;
		int m_x;
		int m_y;
		HWND m_hWndParent;
		HMENU m_hMenu;
		LPVOID m_lpParam;
		int m_width;
		int m_height;
		WinClass *m_winClass;

		bool setInterface(void **const ppRef) {
			if (ppRef != nullptr) {
				incRef();
				*ppRef = static_cast<WindowConfig*>(this);
			}
			return true;
		}

		bool copyInterface(void **const ppRef) {
			if (ppRef != nullptr) {
				try {
					m_winClass->incRef();
					m_lpWindowName->incRef();
					*ppRef = static_cast<WindowConfig*>(new WindowConfig_Impl(m_dwExStyle, m_lpWindowName, m_dwStyle, m_x, m_y, m_hWndParent,
						m_hMenu, m_lpParam, m_width, m_height, m_winClass));
				}
				catch (...) {
					m_lpWindowName->decRef();
					m_winClass->decRef();
					return false;
				}
			}
			return true;
		}
	protected:
		bool queryRefByCmpPtr(void **const ppRef, const char *id) {
			if (id == WindowConfig::GetRefName()) return setInterface(ppRef);
			else if (id == WindowConfig_Impl::GetRefName()) {
				if (ppRef != nullptr) *ppRef = this;
				return true;
			}
			else return Copyable_Impl::queryRefByCmpPtr(ppRef, id);
		}

		bool queryRefByCmpStr(void **const ppRef, const char *id) {
			if (std::strcmp(id, WindowConfig::GetRefName()) == 0) return setInterface(ppRef);
			else return Copyable_Impl::queryRefByCmpStr(ppRef, id);
		}

		bool copyByCmpPtr(void **const ppRef, const char *id) {
			if (id == WindowConfig::GetRefName()) return copyInterface(ppRef);
			else return Copyable_Impl::copyByCmpPtr(ppRef, id);
		}

		bool copyByCmpStr(void **const ppRef, const char *id) {
			if (std::strcmp(id, WindowConfig::GetRefName()) == 0) return copyInterface(ppRef);
			else return Copyable_Impl::copyByCmpStr(ppRef, id);
		}
	public:
		static const char* GetRefName() {
			return "WinFW::WindowConfig_Impl";
		}

		~WindowConfig_Impl() {
			m_winClass->decRef();
			m_lpWindowName->decRef();
		}

		WindowConfig_Impl(WinClass *winClass, int width, int height) : WindowConfig_Impl(NULL, Text::WStringHolder::New(nullptr),
			WS_SYSMENU | WS_MINIMIZEBOX | WS_CAPTION, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, nullptr, width, height, winClass) {
		}

		WindowConfig_Impl(DWORD dwExStyle, Text::WStringHolder *lpWindowName, DWORD dwStyle, int x, int y, HWND hWndParent, HMENU hMenu,
			LPVOID lpParam, int width, int height, WinClass *winClass) : m_dwExStyle(dwExStyle), m_lpWindowName(lpWindowName), m_dwStyle(dwStyle), m_x(x), m_y(y),
			m_hWndParent(hWndParent), m_hMenu(hMenu), m_lpParam(lpParam), m_width(width), m_height(height), m_winClass(winClass) {
		}

		const char* getRefName() const {
			return WindowConfig::GetRefName();
		}

		WindowConfig* setX(int x) {
			m_x = x;
			return this;
		} 

		WindowConfig* setY(int y) {
			m_y = y;
			return this;
		}

		WindowConfig* setWidth(int width) {
			m_width = width;
			return this;
		}

		WindowConfig* setHeight(int height) {
			m_height = height;
			return this;
		}

		WindowConfig* setWinClass(WinClass *&winClass) {
			m_winClass = winClass;
			return this;
		}

		WindowConfig* setWinClass(WinClass *&&winClass) {
			WindowConfig *buff = setWinClass(static_cast<WinClass*&>(winClass));
			winClass->decRef();
			return buff;
		}

		WindowConfig* setStyle(WindowStyle *&style) {
			WindowStyle_Impl *buff;
			if (!style->queryRef(reinterpret_cast<void**>(&buff), WindowStyle_Impl::GetRefName(), true)) throw Exception::InvalidObjectException::New("WindowStyle : incompatible");
			m_dwStyle = buff->getValue();
			return this;
		}

		WindowConfig* setStyle(WindowStyle *&&style) {
			WindowConfig *buff = setStyle(static_cast<WindowStyle*&>(style));
			style->decRef();
			return buff;
		}

		WindowConfig* setExStyle(WindowExStyle *&exStyle) {
			WindowExStyle_Impl *buff;
			if (!exStyle->queryRef(reinterpret_cast<void**>(&buff), WindowExStyle_Impl::GetRefName(), true)) throw Exception::InvalidObjectException::New("WindowExStyle : incompatible");
			m_dwExStyle = buff->getValue();
			return this;
		}

		WindowConfig* setExStyle(WindowExStyle *&&exStyle) {
			WindowConfig *buff = setExStyle(static_cast<WindowExStyle*&>(exStyle));
			exStyle->decRef();
			return buff;
		}

		WindowConfig* setParent(HWND hWndParent) {
			m_hWndParent = hWndParent;
			return this;
		}

		WindowConfig* setMenu(HMENU hMenu) {
			m_hMenu = hMenu;
			return this;
		}

		WindowConfig* setLpParam(LPVOID lpParam) {
			m_lpParam = lpParam;
			return this;
		}

		WindowConfig* setTitle(LPCWSTR title) {
			m_lpWindowName->decRef();
			m_lpWindowName = Text::WStringHolder::New(title);
			return this;
		}

		virtual int getX() {
			return m_x;
		}

		virtual int getY() {
			return m_y;
		}

		virtual int getWidth() {
			return m_width;
		}

		virtual int getHeight() {
			return m_height;
		}

		virtual WinClass* getWinClass() {
			return m_winClass;
		}

		virtual DWORD getStyle() {
			return m_dwStyle;
		}

		virtual DWORD getExStyle() {
			return m_dwExStyle;
		}

		virtual HWND getParent() {
			return m_hWndParent;
		}

		virtual HMENU getMenu() {
			return m_hMenu;
		}

		virtual LPVOID getLpParam() {
			return m_lpParam;
		}

		virtual LPCWSTR getTitle() {
			return m_lpWindowName->getWString();
		}
	};

	class Window_Impl : public virtual Window, public virtual Ref_Impl {
		HWND m_hWnd;
		WinClass *m_winClass;

		bool setInterface(void **const ppRef) {
			if (ppRef != nullptr) {
				incRef();
				*ppRef = static_cast<Window*>(this);
			}
			return true;
		}
	protected:
		bool queryRefByCmpPtr(void **const ppRef, const char *id) {
			if (id == Window::GetRefName()) return setInterface(ppRef);
			else return Ref_Impl::queryRefByCmpPtr(ppRef, id);
		}

		bool queryRefByCmpStr(void **const ppRef, const char *id) {
			if (std::strcmp(id, Window::GetRefName()) == 0) return setInterface(ppRef);
			else return Ref_Impl::queryRefByCmpStr(ppRef, id);
		}
	public:
		~Window_Impl() {
			m_winClass->decRef();
		}

		Window_Impl(HWND hWnd, WinClass *winClass) : m_hWnd(hWnd), m_winClass(winClass) {
		}

		const char* getRefName() const {
			return Window::GetRefName();
		}

		HWND get() {
			return m_hWnd;
		}

		BOOL setTitle(LPCWSTR title) {
			return SetWindowTextW(m_hWnd, title);
		}

		BOOL hide() {
			return ShowWindow(m_hWnd, SW_HIDE);
		}

		BOOL show() {
			return ShowWindow(m_hWnd, SW_SHOW);
		}

		BOOL minimize() {
			return ShowWindow(m_hWnd, SW_MINIMIZE);
		}

		BOOL update() {
			return UpdateWindow(m_hWnd);
		}

		BOOL setPos(int x, int y) {
			return SetWindowPos(m_hWnd, NULL, x, y, 0, 0, SWP_NOSIZE);
		}

		BOOL setSize(int width, int height) {
			return SetWindowPos(m_hWnd, NULL, 0, 0, width, height, SWP_NOMOVE);
		}

		BOOL setClientSize(int width, int height) {
			RECT r{ 0, 0, width, height };
			AdjustWindowRectEx(&r, GetWindowLongW(m_hWnd, GWL_STYLE), GetMenu(m_hWnd) == nullptr ? 0 : 1, GetWindowLongW(m_hWnd, GWL_EXSTYLE));
			width = r.right - r.left;
			height = r.bottom - r.top;
			return SetWindowPos(m_hWnd, nullptr, 0, 0, width, height, SWP_NOMOVE);
		}

		BOOL querySize(LPRECT r) const {
			return GetWindowRect(m_hWnd, r);
		}

		BOOL queryClientSize(LPRECT r) const {
			return GetClientRect(m_hWnd, r);
		}
	};

	class Keyboard_Impl : public virtual Keyboard, public virtual Ref_Impl {
		BYTE m_states[256];
		bool m_lastPress[256];

		bool setInterface(void **const ppRef) {
			if (ppRef != nullptr) {
				incRef();
				*ppRef = static_cast<Keyboard*>(this);
			}
			return true;
		}
	protected:
		bool queryRefByCmpPtr(void **const ppRef, const char *id) {
			if (id == Keyboard::GetRefName()) return setInterface(ppRef);
			else return Ref_Impl::queryRefByCmpPtr(ppRef, id);
		}

		bool queryRefByCmpStr(void **const ppRef, const char *id) {
			if (std::strcmp(id, Keyboard::GetRefName()) == 0) return setInterface(ppRef);
			else return Ref_Impl::queryRefByCmpStr(ppRef, id);
		}
	public:
		Keyboard_Impl() : m_states{ 0 }, m_lastPress{ false } {
		}

		const char* getRefName() const {
			return Keyboard::GetRefName();
		}

		BOOL update() {
			return GetKeyboardState(m_states);
		}

		KeyAction getKeyAction(BYTE vKey) {
			if (m_states[vKey] & (1 << 7)) {
				if (!m_lastPress[vKey]) {
					m_lastPress[vKey] = true;
					return KeyAction::Press;
				}
			}
			else {
				if (m_lastPress[vKey]) {
					m_lastPress[vKey] = false;
					return KeyAction::Release;
				}
			}
			return KeyAction::NoAction;
		}

		bool isPress(BYTE vKey) {
			return m_states[vKey] == (1 << 7);
		}
	};

	class Mouse_Impl : public virtual Mouse, public virtual Ref_Impl {
		POINT m_pos;
		POINT m_mov;
		BYTE m_pData[40];
		UINT m_pcbSize;

		bool setInterface(void **const ppRef) {
			if (ppRef != nullptr) {
				incRef();
				*ppRef = static_cast<Mouse*>(this);
			}
			return true;
		}
	protected:
		bool queryRefByCmpPtr(void **const ppRef, const char *id) {
			if (id == Mouse::GetRefName()) return setInterface(ppRef);
			else return Ref_Impl::queryRefByCmpPtr(ppRef, id);
		}

		bool queryRefByCmpStr(void **const ppRef, const char *id) {
			if (std::strcmp(id, Mouse::GetRefName()) == 0) return setInterface(ppRef);
			else return Ref_Impl::queryRefByCmpStr(ppRef, id);
		}
	public:
		Mouse_Impl() : m_pos{ 0 }, m_mov{ 0 }, m_pData{ 0 }, m_pcbSize(sizeof(m_pData) / sizeof(BYTE)) {
		}

		const char* getRefName() const {
			return Mouse::GetRefName();
		}

		BOOL updatePos() {
			return GetCursorPos(&m_pos);
		}

		POINT getPos() {
			return m_pos;
		}

		int getPosX() {
			return m_pos.x;
		}

		int getPosY() {
			return m_pos.y;
		}

		void updateRawMouseMove(LPARAM lParam) {
			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, m_pData, &m_pcbSize, sizeof(RAWINPUTHEADER));
			RAWINPUT* rawInput = (RAWINPUT*)m_pData;

			if (rawInput->header.dwType == RIM_TYPEMOUSE) {
				m_mov.x = rawInput->data.mouse.lLastX;
				m_mov.y = rawInput->data.mouse.lLastY;
			}
		}

		POINT passMove() {
			POINT buff = m_mov;
			m_mov.x = 0;
			m_mov.y = 0;
			return buff;
		}
	};
}

// Interface : New
namespace WinFW {
	namespace Text {
		StringHolder* StringHolder::New(const char *str) {
			try {
				if (str == nullptr) {
					return new StringHolder_Impl(nullptr, 0);
				}
				else {
					size_t count = std::strlen(str);
					size_t countBuff = count + 1;
					char *buff = new char[countBuff];
					std::memcpy(buff, str, countBuff * sizeof(char));
					return new StringHolder_Impl(buff, count);
				}
			}
			catch (...) {
				return nullptr;
			}
		}
		
		StringHolder* StringHolder::New(const char *str, size_t count) {
			try {
				if (str == nullptr) return new StringHolder_Impl(nullptr, 0);
				else {
					size_t countBuff = count + 1;
					char *buff = new char[countBuff];
					std::memcpy(buff, str, countBuff * sizeof(char));
					return new StringHolder_Impl(buff, count);
				}
			}
			catch (...) {
				return nullptr;
			}
		}
		
		WStringHolder* WStringHolder::New(const wchar_t *str) {
			try {
				if (str == nullptr) return new WStringHolder_Impl(nullptr, 0);
				else {
					size_t count = std::wcslen(str);
					size_t countBuff = count + 1;
					wchar_t *buff = new wchar_t[countBuff];
					std::memcpy(buff, str, countBuff * sizeof(wchar_t));
					return new WStringHolder_Impl(buff, count);
				}
			}
			catch (...) {
				return nullptr;
			}
		}
		
		WStringHolder* WStringHolder::New(const wchar_t *str, size_t count) {
			try {
				if (str == nullptr) {
					return new WStringHolder_Impl(nullptr, 0);
				}
				else {
					size_t countBuff = count + 1;
					wchar_t *buff = new wchar_t[countBuff];
					std::memcpy(buff, str, countBuff * sizeof(wchar_t));
					return new WStringHolder_Impl(buff, count);
				}
			}
			catch (...) {
				return nullptr;
			}
		}
	}

	namespace Exception {
		Exception* Exception::New(const char *str) {
			try {
				return new Exception_Impl(Text::StringHolder::New(str));
			}
			catch (...) {
				return nullptr;
			}
		}

		InvalidObjectException* InvalidObjectException::New(const char *str) {
			try {
				return new InvalidObjectException_Impl(Text::StringHolder::New(str));
			}
			catch (...) {
				return nullptr;
			}
		}
	}

	WinClassStyle* WinClassStyle::New() {
		try {
			return new WinClassStyle_Impl();
		}
		catch (...) {
			return nullptr;
		}
	}

	WinClassConfig* WinClassConfig::New(LPCWSTR lpszClassName, WNDPROC lpfnWndProc) {
		try {
			Text::WStringHolder *str = Text::WStringHolder::New(lpszClassName);
			try {
				return new WinClassConfig_Impl(str, lpfnWndProc);
			}
			catch (...) {
				str->decRef();
				return nullptr;
			}
		}
		catch (...) {
			return nullptr;
		}
	}

	WinClass* WinClass::New(WinClassConfig *&winclassConfig) {
		try {
			WinClassConfig_Impl *buff;
			if(!winclassConfig->queryRef(reinterpret_cast<void**>(&buff), WinClassConfig_Impl::GetRefName(), true)) throw Exception::InvalidObjectException::New("WinClassConfig : incompatible");

			WNDCLASSEXW wcex;
			wcex.cbSize = sizeof(WNDCLASSEXW);
			wcex.hInstance = g_hInstance;
			wcex.lpszClassName = buff->getClassName();
			wcex.lpfnWndProc = buff->getWndProc();
			wcex.cbClsExtra = buff->getClsExtraBytes();
			wcex.cbWndExtra = buff->getWndExtraBytes();
			wcex.hbrBackground = buff->getBackgroundColor();
			wcex.hCursor = buff->getCursor();
			wcex.hIcon = buff->getIcon();
			wcex.hIconSm = buff->getIconSm();
			wcex.lpszMenuName = buff->getMenuName();
			wcex.style = buff->getStyle();
			RegisterClassExW(&wcex);

			Text::WStringHolder *str = Text::WStringHolder::New(wcex.lpszClassName);

			try {
				return new WinClass_Impl(str);
			}
			catch (...) {
				str->decRef();
				return nullptr;
			}
		}
		catch (...) {
			return nullptr;
		}
	}

	WinClass* WinClass::New(WinClassConfig *&&winclassConfig) {
		try {
			WinClass *buff = New(static_cast<WinClassConfig*&>(winclassConfig));
			winclassConfig->decRef();
			return buff;
		}
		catch (...) {
			return nullptr;
		}
	}

	WindowStyle* WindowStyle::New() {
		try {
			return new WindowStyle_Impl();
		}
		catch (...) {
			return nullptr;
		}
	}

	WindowExStyle* WindowExStyle::New() {
		try {
			return new WindowExStyle_Impl();
		}
		catch (...) {
			return nullptr;
		}
	}

	WindowConfig* WindowConfig::New(WinClass *&winClass, int width, int height) {
		try {
			try {
				winClass->incRef();
			}
			catch (...) {
				return false;
			}
			return new WindowConfig_Impl(winClass, width, height);
		}
		catch (...) {
			winClass->decRef();
			return nullptr;
		}
	}

	WindowConfig* WindowConfig::New(WinClass *&&winClass, int width, int height) {
		try {
			WindowConfig *buff = New(static_cast<WinClass*&>(winClass), width, height);
			winClass->decRef();
			return buff;
		}
		catch (...) {
			return nullptr;
		}
	}

	Window* Window::New(WindowConfig *&windowConfig, bool clientSize) {
		WinClass *winClass = nullptr;
		try {
			WindowConfig_Impl *buff;
			if (!windowConfig->queryRef(reinterpret_cast<void**>(&buff), WindowConfig_Impl::GetRefName(), true)) throw Exception::InvalidObjectException::New("WindowConfig : incompatible");

			winClass = buff->getWinClass();

			try {
				winClass->incRef();
			}
			catch (...) {
				return nullptr;
			}

			LONG width, height;
			if (clientSize) {
				RECT r{ 0, 0, buff->getWidth(), buff->getHeight() };
				AdjustWindowRectEx(&r, buff->getStyle(), buff->getMenu() == nullptr ? FALSE : TRUE, buff->getExStyle());
				width = r.right - r.left;
				height = r.bottom - r.top;
			}
			else {
				width = buff->getWidth();
				height = buff->getHeight();
			}

			return new Window_Impl(CreateWindowExW(
				buff->getExStyle(), winClass->getName(), buff->getTitle(), buff->getStyle(),
				buff->getX(), buff->getY(), width, height,
				buff->getParent(), buff->getMenu(), g_hInstance, buff->getLpParam()),
				winClass);
		}
		catch (...) {
			winClass->decRef();
			return nullptr;
		}
	}

	Window* Window::New(WindowConfig *&&windowConfig, bool clientSize) {
		try {
			Window *buff = New(static_cast<WindowConfig*&>(windowConfig), clientSize);
			windowConfig->decRef();
			return buff;
		}
		catch (...) {
			return nullptr;
		}
	}

	Keyboard* Keyboard::New() {
		try {
			return new Keyboard_Impl();
		}
		catch (...) {
			return nullptr;
		}
	}

	Mouse* Mouse::New() {
		try {
			return new Mouse_Impl();
		}
		catch (...) {
			return nullptr;
		}
	}
}