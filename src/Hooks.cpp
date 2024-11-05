#include "Hooks.h"
#include "UI.h"

namespace Hooks
{
	struct Init3d11Hook
	{
		static void thunk()
		{
			func();

			UI::GetSingleton()->Init();

		};
		static inline REL::Relocation<decltype(thunk)> func;


		static void Install()
		{
			REL::Relocation<uintptr_t> target1{ REL::VariantID(75595, 77226, 0xDC5530), REL::Relocate(0x9, 0x275) };
			stl::write_thunk_call<Init3d11Hook>(target1.address());
		}

	};

	struct Renderer_End
	{
		static void thunk(std::uint32_t a_timer)
		{
			func(a_timer);

			UI::GetSingleton()->Render();

		};
		static inline REL::Relocation<decltype(thunk)> func;


		static void Install()
		{
			REL::Relocation<uintptr_t> target1{ REL::VariantID(75461, 77246, 0xDBBDD0), REL::Relocate(0x9, 0x9, 0x15) };
			stl::write_thunk_call<Renderer_End>(target1.address());
		}

	};

	void InstallHooks()
	{
		Init3d11Hook::Install();
		Renderer_End::Install();

		SKSE::log::info("Installed Hooks!");
	}

}
