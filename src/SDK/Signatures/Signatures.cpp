#include "Signatures.h"
#include "../../Utils/Pattern/Pattern.h"
#include "../../Utils/Minidump/Minidump.h"
#include <format>

void CSignature::Find()
{
	m_Address = g_Pattern.Find(m_Module, m_Pattern, false);
	if (m_Address == 0)
	{
#if defined(FW_DEBUG_DIAGNOSTICS)
		const auto msg = std::format("Name: {}\nModule: {}\nPattern: {}\nAddress: {:#x} (+{:d})", m_Name, m_Module, m_Pattern, m_Address, m_Offset);
		OutputDebugStringA(("Pattern not found\n" + msg + "\n").c_str());
#endif
		FW_TRACE(std::format("[ERROR] Signature not found | name={} module={} offset={} reason=pattern-not-found pattern={}", m_Name, m_Module, m_Offset, m_Pattern).c_str());
		return;
	}

	m_Address += m_Offset;
	FW_TRACE(std::format("[SIG] name={} module={} offset={} address={}", m_Name, m_Module, m_Offset, Minidump::DescribeAddress(m_Address, true)).c_str());

	if (!Minidump::IsExecutableAddress(m_Address))
	{
		FW_TRACE(std::format("[WARN] Signature resolved to a non-executable address | name={} address={}", m_Name, Minidump::DescribeAddress(m_Address, true)).c_str());
	}
}
