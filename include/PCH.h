#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "REX/REX/Singleton.h"

using namespace std::literals;

#include <SimpleIni.h>
#include <glaze/glaze.hpp>

#include "imgui_impl_win32.h"
#include "imgui_internal.h"
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include "Plugin.h"

namespace stl
{
	using namespace SKSE::stl;

	template <class T>
	void write_thunk_call(std::uintptr_t a_src)
	{
		auto& trampoline = SKSE::GetTrampoline();
		T::func = trampoline.write_call<5>(a_src, T::thunk);
	}
}