switch (_fmt)
{
    case EF_R4G4_UNORM_PACK8: [[fallthrough]];
    case EF_R8_UNORM: [[fallthrough]];
    case EF_R8_SNORM: [[fallthrough]];
    case EF_R8_USCALED: [[fallthrough]];
    case EF_R8_SSCALED: [[fallthrough]];
    case EF_R8_UINT: [[fallthrough]];
    case EF_R8_SINT: [[fallthrough]];
    case EF_R8_SRGB:
        return EFC_8_BIT;
    case EF_R4G4B4A4_UNORM_PACK16: [[fallthrough]];
    case EF_B4G4R4A4_UNORM_PACK16: [[fallthrough]];
    case EF_R5G6B5_UNORM_PACK16: [[fallthrough]];
    case EF_B5G6R5_UNORM_PACK16: [[fallthrough]];
    case EF_R5G5B5A1_UNORM_PACK16: [[fallthrough]];
    case EF_B5G5R5A1_UNORM_PACK16: [[fallthrough]];
    case EF_A1R5G5B5_UNORM_PACK16: [[fallthrough]];
    case EF_R8G8_UNORM: [[fallthrough]];
    case EF_R8G8_SNORM: [[fallthrough]];
    case EF_R8G8_USCALED: [[fallthrough]];
    case EF_R8G8_SSCALED: [[fallthrough]];
    case EF_R8G8_UINT: [[fallthrough]];
    case EF_R8G8_SINT: [[fallthrough]];
    case EF_R8G8_SRGB: [[fallthrough]];
    case EF_R16_UNORM: [[fallthrough]];
    case EF_R16_SNORM: [[fallthrough]];
    case EF_R16_USCALED: [[fallthrough]];
    case EF_R16_SSCALED: [[fallthrough]];
    case EF_R16_UINT: [[fallthrough]];
    case EF_R16_SINT: [[fallthrough]];
    case EF_R16_SFLOAT:
        return EFC_16_BIT;
    case EF_R8G8B8_UNORM: [[fallthrough]];
    case EF_R8G8B8_SNORM: [[fallthrough]];
    case EF_R8G8B8_USCALED: [[fallthrough]];
    case EF_R8G8B8_SSCALED: [[fallthrough]];
    case EF_R8G8B8_UINT: [[fallthrough]];
    case EF_R8G8B8_SINT: [[fallthrough]];
    case EF_R8G8B8_SRGB: [[fallthrough]];
    case EF_B8G8R8_UNORM: [[fallthrough]];
    case EF_B8G8R8_SNORM: [[fallthrough]];
    case EF_B8G8R8_USCALED: [[fallthrough]];
    case EF_B8G8R8_SSCALED: [[fallthrough]];
    case EF_B8G8R8_UINT: [[fallthrough]];
    case EF_B8G8R8_SINT: [[fallthrough]];
    case EF_B8G8R8_SRGB:
        return EFC_24_BIT;
    case EF_R8G8B8A8_UNORM: [[fallthrough]];
    case EF_R8G8B8A8_SNORM: [[fallthrough]];
    case EF_R8G8B8A8_USCALED: [[fallthrough]];
    case EF_R8G8B8A8_SSCALED: [[fallthrough]];
    case EF_R8G8B8A8_UINT: [[fallthrough]];
    case EF_R8G8B8A8_SINT: [[fallthrough]];
    case EF_R8G8B8A8_SRGB: [[fallthrough]];
    case EF_B8G8R8A8_UNORM: [[fallthrough]];
    case EF_B8G8R8A8_SNORM: [[fallthrough]];
    case EF_B8G8R8A8_USCALED: [[fallthrough]];
    case EF_B8G8R8A8_SSCALED: [[fallthrough]];
    case EF_B8G8R8A8_UINT: [[fallthrough]];
    case EF_B8G8R8A8_SINT: [[fallthrough]];
    case EF_B8G8R8A8_SRGB: [[fallthrough]];
    case EF_A8B8G8R8_UNORM_PACK32: [[fallthrough]];
    case EF_A8B8G8R8_SNORM_PACK32: [[fallthrough]];
    case EF_A8B8G8R8_USCALED_PACK32: [[fallthrough]];
    case EF_A8B8G8R8_SSCALED_PACK32: [[fallthrough]];
    case EF_A8B8G8R8_UINT_PACK32: [[fallthrough]];
    case EF_A8B8G8R8_SINT_PACK32: [[fallthrough]];
    case EF_A8B8G8R8_SRGB_PACK32: [[fallthrough]];
    case EF_A2R10G10B10_UNORM_PACK32: [[fallthrough]];
    case EF_A2R10G10B10_SNORM_PACK32: [[fallthrough]];
    case EF_A2R10G10B10_USCALED_PACK32: [[fallthrough]];
    case EF_A2R10G10B10_SSCALED_PACK32: [[fallthrough]];
    case EF_A2R10G10B10_UINT_PACK32: [[fallthrough]];
    case EF_A2R10G10B10_SINT_PACK32: [[fallthrough]];
    case EF_A2B10G10R10_UNORM_PACK32: [[fallthrough]];
    case EF_A2B10G10R10_SNORM_PACK32: [[fallthrough]];
    case EF_A2B10G10R10_USCALED_PACK32: [[fallthrough]];
    case EF_A2B10G10R10_SSCALED_PACK32: [[fallthrough]];
    case EF_A2B10G10R10_UINT_PACK32: [[fallthrough]];
    case EF_A2B10G10R10_SINT_PACK32: [[fallthrough]];
    case EF_R16G16_UNORM: [[fallthrough]];
    case EF_R16G16_SNORM: [[fallthrough]];
    case EF_R16G16_USCALED: [[fallthrough]];
    case EF_R16G16_SSCALED: [[fallthrough]];
    case EF_R16G16_UINT: [[fallthrough]];
    case EF_R16G16_SINT: [[fallthrough]];
    case EF_R16G16_SFLOAT: [[fallthrough]];
    case EF_R32_UINT: [[fallthrough]];
    case EF_R32_SINT: [[fallthrough]];
    case EF_R32_SFLOAT: [[fallthrough]];
    case EF_B10G11R11_UFLOAT_PACK32: [[fallthrough]];
    case EF_E5B9G9R9_UFLOAT_PACK32:
        return EFC_32_BIT;
    case EF_R16G16B16_UNORM: [[fallthrough]];
    case EF_R16G16B16_SNORM: [[fallthrough]];
    case EF_R16G16B16_USCALED: [[fallthrough]];
    case EF_R16G16B16_SSCALED: [[fallthrough]];
    case EF_R16G16B16_UINT: [[fallthrough]];
    case EF_R16G16B16_SINT: [[fallthrough]];
    case EF_R16G16B16_SFLOAT:
        return EFC_48_BIT;
    case EF_R16G16B16A16_UNORM: [[fallthrough]];
    case EF_R16G16B16A16_SNORM: [[fallthrough]];
    case EF_R16G16B16A16_USCALED: [[fallthrough]];
    case EF_R16G16B16A16_SSCALED: [[fallthrough]];
    case EF_R16G16B16A16_UINT: [[fallthrough]];
    case EF_R16G16B16A16_SINT: [[fallthrough]];
    case EF_R16G16B16A16_SFLOAT: [[fallthrough]];
    case EF_R32G32_UINT: [[fallthrough]];
    case EF_R32G32_SINT: [[fallthrough]];
    case EF_R32G32_SFLOAT: [[fallthrough]];
    case EF_R64_UINT: [[fallthrough]];
    case EF_R64_SINT: [[fallthrough]];
    case EF_R64_SFLOAT:
        return EFC_64_BIT;
    case EF_R32G32B32_UINT: [[fallthrough]];
    case EF_R32G32B32_SINT: [[fallthrough]];
    case EF_R32G32B32_SFLOAT:
        return EFC_96_BIT;
    case EF_R32G32B32A32_UINT: [[fallthrough]];
    case EF_R32G32B32A32_SINT: [[fallthrough]];
    case EF_R32G32B32A32_SFLOAT: [[fallthrough]];
    case EF_R64G64_UINT: [[fallthrough]];
    case EF_R64G64_SINT: [[fallthrough]];
    case EF_R64G64_SFLOAT:
        return EFC_128_BIT;
    case EF_R64G64B64_UINT: [[fallthrough]];
    case EF_R64G64B64_SINT: [[fallthrough]];
    case EF_R64G64B64_SFLOAT:
        return EFC_192_BIT;
    case EF_R64G64B64A64_UINT: [[fallthrough]];
    case EF_R64G64B64A64_SINT: [[fallthrough]];
    case EF_R64G64B64A64_SFLOAT:
        return EFC_256_BIT;
    default:
        _NBL_DEBUG_BREAK_IF(true);
        return static_cast<E_FORMAT_CLASS>(EFC_256_BIT+1u);
}