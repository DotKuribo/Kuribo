#include "Loader.hxx"

#include "core/patch.hxx"
#include "core/sync.hxx"

namespace kuribo {

struct KBHeader {
	u32 magic1;
	u16 magic2;
	u16 version;
	u32 bssSize;
	u32 codeSize;
};
enum class k
{
	Addr32 = 1,
	Addr16Lo = 4,
	Addr16Hi = 5,
	Addr16Ha = 6,
	Rel24 = 10,
	Write32 = 32,
	Write16 = 33,
	Write8 = 34,
	CondWritePointer = 35,
	CondWrite32 = 36,
	CondWrite16 = 37,
	CondWrite8 = 38,
	Branch = 64,
	BranchLink = 65
};

constexpr u32 resolveAddress(u32 address, u32 text)
{
	return ((address & 0x80000000) || (address & 0x90000000)) ? address : text + address;
}

bool handleElfRelocation(const u8*& input, u32 text)
{
	u32 cmdHeader = *((u32*)input);
	input += 4;

	k cmd = static_cast<k>(cmdHeader >> 24);
	KURIBO_LOG("kCommand: %u\n", (u32)cmd);
	union addr
	{
		u32 d32;
		u32* p32;
		u16* p16;
		u8* p8;
	};
	addr address;
	address.d32 = cmdHeader & 0xffffff;

	if (address.d32 == 0xfffffe)
	{
		// Absolute address
		address.d32 = *((u32*)input);
		input += 4;
	}
	else
	{
		// Relative address
		address.d32 += text;
	}
	const u32 resolved = resolveAddress(*(const u32*)input, text);

	auto handleRelocElf = [&](k cmd) -> bool
	{
		KURIBO_SCOPED_LOG("ELF RELOCATION");
		KURIBO_LOG("TYPE: %u\n", (u32)cmd);
		switch (cmd)
		{
		case k::Addr32:
			*address.p32 = resolved;
			break;
		case k::Addr16Lo:
			*address.p16 = resolved & 0xffff;
			break;
		case k::Addr16Hi:
			*address.p16 = resolved >> 16;
			break;
		case k::Addr16Ha: {
			*address.p16 = (resolved >> 16) + !!(resolved & 0x8000);
			break;
		}
		case k::Rel24:
		{
		rel24:
			*address.p32 &= 0xFC000003;
			// TODO -- isCode check?
			u32 r = resolved;
			if ((address.d32 & 0x90000000) && (resolved & 0x80000000))
			{
				KURIBO_LOG("MEM2 -> MEM1 ISSUE\n");
				u32* jump_helper = new u32[4];
				KURIBO_LOG("Jump helper at %p\n", jump_helper);
				// lis r11, HA
				// addi r11, r11, LO
				// mtctr r11
				// bctr
				jump_helper[0] = 0x3D600000 | (resolved >> 16);
				jump_helper[1] = 0x616B0000 | (resolved & 0xffff);
				jump_helper[2] = 0x7D6903A6;
				jump_helper[3] = 0x4E800420;
				r = (u32)jump_helper;
			}
			*address.p32 |= ((r- address.d32) & 0x3FFFFFC);
			break;
		}
		case k::Write32:
			*address.p32 = *(const u32*)input;
			break;
		case k::Write16:
			*address.p16 = (*(const u32*)input) & 0xffff;
			break;
		case k::Write8:
			*address.p8 = (*(const u32*)input) & 0xff;
			break;
		case k::Branch:
		case k::BranchLink:
			*address.p32 = (cmd == k::BranchLink) + 0x48000000;
			//	KURIBO_ASSERT(!"UNSUPPORTED");
			//	goto rel24;
			{
				*address.p32 &= 0xFC000003;
				// TODO -- isCode check?
				u32 r = resolved;
				if ((address.d32 & 0x80000000) && (resolved & 0x90000000))
				{
					KURIBO_LOG("MEM1 -> MEM2 ISSUE\n");
					u32* jump_helper = new (&mem::GetHeap(mem::GlobalHeapType::MEM1)) u32[4];
					KURIBO_LOG("Jump helper at %p\n", jump_helper);
					// lis r11, HA
					// addi r11, r11, LO
					// mtctr r11
					// bctr
					jump_helper[0] = 0x3D600000 | (resolved >> 16);
					jump_helper[1] = 0x616B0000 | (resolved & 0xffff);
					jump_helper[2] = 0x7D6903A6;
					jump_helper[3] = 0x4E800420;
					r = (u32)jump_helper;
				}
				*address.p32 |= ((r - address.d32) & 0x3FFFFFC);
			}
			break;
		default:
			KURIBO_LOG("Kamek relocation..\n");
			return false;
		}
		return true;
	};

	auto handleRelocKamek = [&](k cmd)
	{
		const u32 original = ((const u32*)input)[1];
		input += 4;

		bool ignored = false;

		if (cmd == k::CondWritePointer)
		{
			if (*address.p32 == original)
				*address.p32 = resolved;
			else
				ignored = true;
		}
		else
		{
			const u32 value = *(const u32*)input;

			if (cmd == k::CondWrite32)
			{
				if (*address.p32 == original)
					*address.p32 = value;
				else
					ignored = true;
			}
			else if (cmd == k::CondWrite16)
			{
				if (*address.p16 == (original & 0xffff))
					*address.p16 = value & 0xffff;
				else
					ignored = true;
			}
			else if (cmd == k::CondWrite8)
			{
				if (*address.p8 == (original & 0xff))
					*address.p8 = value & 0xff;
				else
					ignored = true;
			}
			else if ((u32)cmd == 0)
			{
				
			}
			else
			{
				input -= 4;
				return false;
			}
		}

		if (ignored)
			KURIBO_LOG("KAMEK Conditional write could not execute.\n");
		return true;
	};

	bool bad = false;

	if (!handleRelocElf(cmd) && !handleRelocKamek(cmd))
	{
		KURIBO_LOG("Unknown command: %d\n", (u32)cmd);
		bad = true;
	}

	input += 4;
#if KURIBO_PLATFORM_WII
	dcbst(address);
	asm("sync");
	icbi(address);
#endif

	return !bad;
}

KamekLoadResult loadKamekBinary(KamekLoadParam param)
{
	const KBHeader* header = reinterpret_cast<const KBHeader*>(param.binary.data());

	if (!header) return KamekLoadResult::MalformedRequest;

	if (header->magic1 != 'Kame' || header->magic2 != 'k\0')
		return KamekLoadResult::InvalidFileType;

	if (header->version != 1)
		return KamekLoadResult::InvalidVersion;

	KURIBO_LOG("bssSize=%u, codeSize=%u\n", header->bssSize, header->codeSize);

	u32 text = reinterpret_cast<u32>(mem::Alloc(header->codeSize + header->bssSize, param.heap ? *param.heap : mem::GetDefaultHeap()));

	if (!text)
		return KamekLoadResult::BadAlloc;

	if (param.textStartCb)
		*param.textStartCb = reinterpret_cast<void*>(text);

	const u8* input = reinterpret_cast<const u8*>(header + 1);
	const u8* inputEnd = reinterpret_cast<const u8*>(param.binary.data() + param.binary.size());
	KURIBO_LOG("Input: %p; InputEnd: %p\n", input, inputEnd);
	u8* output = reinterpret_cast<u8*>(text);

	for (u32 i = 0; i < header->codeSize; ++i)
		*(output++) = *(input++);
	for (u32 i = 0; i < header->bssSize; ++i)
		*(output++) = 0;

	while (input < inputEnd)
	{
		KURIBO_SCOPED_LOG("Handling relocation...");
		if (!handleElfRelocation(input, text))
			return KamekLoadResult::BadReloc;
	}
#if KURIBO_PLATFORM_WII
	__sync();
#endif
	return KamekLoadResult::OK;
}

}