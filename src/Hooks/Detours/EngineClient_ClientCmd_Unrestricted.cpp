#include "../Hooks.h"


#include <algorithm>
#include <boost/algorithm/string/split.hpp> // Include for boost::split
#include <cctype>
#include "../../Features/Menu/MaterialEditor/MaterialEditor.h"
#include "../../Features/Commands/Commands.h"
#include "../../Features/Chams/DMEChams.h"

namespace
{
	bool ShouldTraceCommand(const std::string& command)
	{
		static constexpr const char* prefixes[] = {
			"autoteam",
			"jointeam",
			"join_class",
			"changeteam",
			"changeclass",
			"closedwelcomemenu",
			"disconnect",
			"retry",
			"map ",
			"changelevel",
			"connect "
		};

		for (const auto prefix : prefixes)
		{
			if (command.rfind(prefix, 0) == 0)
			{
				return true;
			}
		}

		return false;
	}

	std::string ToLower(std::string value)
	{
		std::transform(value.begin(), value.end(), value.begin(),
			[](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return value;
	}

	bool IsNativeTeamCommand(const std::string& commandName)
	{
		static constexpr const char* commands[] = {
			"autoteam",
			"jointeam",
			"join_class",
			"changeteam",
			"changeclass",
			"closedwelcomemenu"
		};

		for (const auto command : commands)
		{
			if (commandName == command)
			{
				return true;
			}
		}

		return false;
	}
}

class split_q
{
public:
	split_q() : in_q(false) {}

	bool operator()(char ch) const
	{
		if (ch == '\"')
		{
			in_q = !in_q;
		}
		return !in_q && ch == ' ';
	}

private:
	mutable bool in_q;

};

MAKE_HOOK(EngineClient_ClientCmd_Unrestricted, Utils::GetVFuncPtr(I::EngineClient, 106), void, __fastcall,
		  void* ecx, const char* szCmdString)
{
	if (!szCmdString)
	{
		return Hook.Original<FN>()(ecx, szCmdString);
	}

	std::string cmdString(szCmdString);
	if (ShouldTraceCommand(cmdString))
	{
		FW_TRACE(("[CMD] ClientCmd_Unrestricted: " + cmdString).c_str());
	}

	std::deque<std::string> cmdArgs;

	// Yes I will use boost for this
	boost::split(cmdArgs, cmdString, split_q());

	if (!cmdArgs.empty())
	{
		const std::string cmdName = cmdArgs.front();
		cmdArgs.pop_front();
		const std::string cmdNameLower = ToLower(cmdName);

		if (IsNativeTeamCommand(cmdNameLower))
		{
			return Hook.Original<FN>()(ecx, cmdString.c_str());
		}

		if (F::Commands.Run(cmdNameLower, cmdArgs))
		{
			return;
		}

		if (cmdNameLower == "disconnect")
		{
			F::DMEChams.DeleteMaterials();	//	schizoid
		}
	}

	Hook.Original<FN>()(ecx, cmdString.c_str());
}
