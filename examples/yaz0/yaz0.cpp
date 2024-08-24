#include <kuribo_sdk.h>
#include <kuribo_sdk/kuribo_szs.h>

kx::DefineModule("YAZ0", "examples", "1.0");

extern "C" void OSReport(const char*, ...);

static unsigned char sStaticBuffer[0x200];
static u32 sCurOffset = 0;

static void* dummy_malloc(u32 size) {
  if (sCurOffset + size > sizeof(sStaticBuffer)) {
    OSReport("dummy_malloc: Out of memory!\n");
    return nullptr;
  }
  void* ptr = &sStaticBuffer[sCurOffset];
  sCurOffset += size;
  return ptr;
}

static void dummy_free(void* ptr) {}

void MyFunction() {
  static const unsigned char exampleCompressedData[] = {
      0x59, 0x41, 0x5A, 0x30, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
      0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x78, 0x9C,
      0x0B, 0xC9, 0xC8, 0x2C, 0x56, 0x00, 0xA2, 0x44, 0x85, 0x92};

  u32 compressionType = kxSzsCheckCompressed(exampleCompressedData,
                                             sizeof(exampleCompressedData));
  if (compressionType != KX_COMPRESSION_SZS) {
    OSReport("The provided data is not in SZS (YAZ0) format.\n");
    return;
  }

  u32 decompressedSize =
      kxSzsDecodedSize(exampleCompressedData, sizeof(exampleCompressedData));

  void* decompressedData = dummy_malloc(decompressedSize);
  if (decompressedData == nullptr) {
    OSReport("Failed to allocate memory (%u bytes) for decompressed data.\n",
             decompressedSize);
    return;
  }

  const char* error =
      kxSzsDecodeIntoV1(decompressedData, decompressedSize,
                        exampleCompressedData, sizeof(exampleCompressedData));
  if (error != nullptr) {
    OSReport("Decompression failed: %s\n", error);
    return;
  }

  OSReport("Decompression succeeded! Decompressed size: %u bytes.\n",
           decompressedSize);
  OSReport("First 16 bytes of decompressed data: ");
  for (u32 i = 0; i < 16 && i < decompressedSize; i++) {
    OSReport("%02X ", ((unsigned char*)decompressedData)[i]);
  }
  OSReport("\n");
}

kx::OnLoad(MyFunction);
