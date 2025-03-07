#pragma once

namespace Offsets {
namespace BSRenderManager {
// inline constexpr REL::ID Singleton(static_cast<std::uint64_t>(411393));
inline constexpr REL::Offset Singleton(0x03181700); // VR 1_4_15 143181700
}

inline RE::ObjectRefHandle GetInvalidRefHandle()
{
  REL::Relocation<RE::ObjectRefHandle*> handle{ REL::Offset(
    0x01F8319C) }; // VR 1_4_15 141F8319C
  return *handle;
}

/**
 * This is called from CallNative::CallNativeSafe
 * no idea what it does, should be renamed.
 */
inline float Unknown(void* unk1, void* unk2, RE::TESObjectREFR* obj)
{
  using func_t = decltype(&Unknown);
  REL::Relocation<func_t> func{ REL::ID(55622) }; // VR
  return func(unk1, unk2, obj);
}

inline void PushActorAway(void* vm, StackID stackId, RE::Actor* self,
                          RE::Actor* targetActor, float magnitude)
{
}
}

/**
 * This class is copied from skse
 * --
 * for some reason in commonlib TESRace has TintAsset member
 * which looks to be exactly like TintMask
 * but TESNPC and PlayerCharacter both have TintMask member
 * which is a stub class with no definition
 * most likely needs fixing on their side
 */
class TintMask
{
public:
  TintMask()
  {
    alpha = 0.0;
    tintType = 0;
    texture = nullptr;
  };
  ~TintMask(){};

#ifdef PAPYRUS_CUSTOM_CLASS
  enum
  {
    kTypeID = 300
  };
#endif

  enum
  {
    kMaskType_Freckles = 0,
    kMaskType_Lips,
    kMaskType_Cheeks,
    kMaskType_Eyeliner,
    kMaskType_UpperEyeSocket,
    kMaskType_LowerEyeSocket,
    kMaskType_SkinTone,
    kMaskType_WarPaint,
    kMaskType_FrownLines,
    kMaskType_LowerCheeks,
    kMaskType_Nose,
    kMaskType_Chin,
    kMaskType_Neck,
    kMaskType_Forehead,
    kMaskType_Dirt
  };

  RE::TESTexture* texture;

  union
  {
    struct Color
    {
      uint8_t red, green, blue,
        alpha; // The alpha isn't actually used here so its usually zero
    } color;
    uint32_t abgr;
  };

  float alpha;
  uint32_t tintType;

  uint32_t ToARGB()
  {
    return ((((uint32_t)(alpha * 255) & 0xFF) << 24) |
            ((color.red & 0xFF) << 16) | ((color.green & 0xFF) << 8) |
            ((color.blue & 0xFF) << 0));
  }
};

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain4;
struct ID3D11RenderTargetView1;
struct ID3D11ShaderResourceView1;

class BSRenderManager
{
public:
  static BSRenderManager* GetSingleton()
  {
    REL::Relocation<BSRenderManager**> singleton{
      Offsets::BSRenderManager::Singleton
    };
    return *singleton;
  }

  uint64_t unk00[0x48 >> 3];               // 00
  ID3D11Device* forwarder;                 // 48 - Actually CID3D11Forwarder
  ID3D11DeviceContext* context;            // 50 - ID3D11DeviceContext4
  uint64_t unk58;                          // 58
  uint64_t unk60;                          // 60
  uint64_t unk68;                          // 68
  IDXGISwapChain4* swapChain;              // 70
  uint64_t unk78;                          // 78
  uint64_t unk80;                          // 80
  ID3D11RenderTargetView1* renderView;     // 88
  ID3D11ShaderResourceView1* resourceView; // 90
  uint64_t unk2788[(0x2788 - 0x90) >> 3];
  mutable RE::BSReadWriteLock _lock; // 2790
};

class MenuScreenData
{
public:
  static MenuScreenData* GetSingleton()
  {
    REL::Relocation<MenuScreenData**> singleton{ REL::Offset(
      0x02FEBC40) }; // VR 1_4_15 142FEBC40
    return *singleton;
  }

  std::uint32_t unk00;    // 00
  RE::NiPoint2 mousePos;  // 04
  float unk0C;            // 0C
  float unk10;            // 10
  float screenWidth;      // 14
  float screenHeight;     // 18
  float mouseSensitivity; // 1C
  float unk20;            // 20
  float unk24;            // 24
  float unk28;            // 28 - scaling related
  std::uint32_t unk2C;    // 2C
};
static_assert(sizeof(MenuScreenData) == 0x30);
