//
//  KTX.h
//  ktx/src/ktx
//
//  Created by Zach Pomerantz on 2/08/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_ktx_KTX_h
#define hifi_ktx_KTX_h

#include <array>
#include <list>
#include <vector>
#include <cstdint>
#include <memory>

/* KTX Spec:

Byte[12] identifier
UInt32 endianness
UInt32 glType
UInt32 glTypeSize
UInt32 glFormat
Uint32 glInternalFormat
Uint32 glBaseInternalFormat
UInt32 pixelWidth
UInt32 pixelHeight
UInt32 pixelDepth
UInt32 numberOfArrayElements
UInt32 numberOfFaces
UInt32 numberOfMipmapLevels
UInt32 bytesOfKeyValueData
  
for each keyValuePair that fits in bytesOfKeyValueData
    UInt32   keyAndValueByteSize
    Byte     keyAndValue[keyAndValueByteSize]
    Byte     valuePadding[3 - ((keyAndValueByteSize + 3) % 4)]
end
  
for each mipmap_level in numberOfMipmapLevels*
    UInt32 imageSize; 
    for each array_element in numberOfArrayElements*
       for each face in numberOfFaces
           for each z_slice in pixelDepth*
               for each row or row_of_blocks in pixelHeight*
                   for each pixel or block_of_pixels in pixelWidth
                       Byte data[format-specific-number-of-bytes]**
                   end
               end
           end
           Byte cubePadding[0-3]
       end
    end
    Byte mipPadding[3 - ((imageSize + 3) % 4)]
end

* Replace with 1 if this field is 0.

** Uncompressed texture data matches a GL_UNPACK_ALIGNMENT of 4.
*/



namespace ktx {

    enum GLType : uint32_t {
        COMPRESSED_TYPE                 = 0,

        // GL 4.4 Table 8.2
        UNSIGNED_BYTE                   = 0x1401,
        BYTE                            = 0x1400,
        UNSIGNED_SHORT                  = 0x1403,
        SHORT                           = 0x1402,
        UNSIGNED_INT                    = 0x1405,
        INT                             = 0x1404,
        HALF_FLOAT                      = 0x140B,
        FLOAT                           = 0x1406,
        UNSIGNED_BYTE_3_3_2             = 0x8032,
        UNSIGNED_BYTE_2_3_3_REV         = 0x8362,
        UNSIGNED_SHORT_5_6_5            = 0x8363,
        UNSIGNED_SHORT_5_6_5_REV        = 0x8364,
        UNSIGNED_SHORT_4_4_4_4          = 0x8033,
        UNSIGNED_SHORT_4_4_4_4_REV      = 0x8365,
        UNSIGNED_SHORT_5_5_5_1          = 0x8034,
        UNSIGNED_SHORT_1_5_5_5_REV      = 0x8366,
        UNSIGNED_INT_8_8_8_8            = 0x8035,
        UNSIGNED_INT_8_8_8_8_REV        = 0x8367,
        UNSIGNED_INT_10_10_10_2         = 0x8036,
        UNSIGNED_INT_2_10_10_10_REV     = 0x8368,
        UNSIGNED_INT_24_8               = 0x84FA,
        UNSIGNED_INT_10F_11F_11F_REV    = 0x8C3B,
        UNSIGNED_INT_5_9_9_9_REV        = 0x8C3E,
        FLOAT_32_UNSIGNED_INT_24_8_REV  = 0x8DAD,

        NUM_GLTYPES = 25,
    };

    enum GLFormat : uint32_t {
        COMPRESSED_FORMAT               = 0,

        // GL 4.4 Table 8.3
        STENCIL_INDEX                   = 0x1901,
        DEPTH_COMPONENT                 = 0x1902,
        DEPTH_STENCIL                   = 0x84F9,

        RED                             = 0x1903,
        GREEN                           = 0x1904,
        BLUE                            = 0x1905,
        RG                              = 0x8227,
        RGB                             = 0x1907,
        RGBA                            = 0x1908,
        BGR                             = 0x80E0,
        BGRA                            = 0x80E1,

        RG_INTEGER                      = 0x8228,
        RED_INTEGER                     = 0x8D94,
        GREEN_INTEGER                   = 0x8D95,
        BLUE_INTEGER                    = 0x8D96,
        RGB_INTEGER                     = 0x8D98,
        RGBA_INTEGER                    = 0x8D99,
        BGR_INTEGER                     = 0x8D9A,
        BGRA_INTEGER                    = 0x8D9B,

        NUM_GLFORMATS = 20,
    };

    enum GLInternalFormat_Uncompressed : uint32_t {
        // GL 4.4 Table 8.12
        R8                              = 0x8229,
        R8_SNORM                        = 0x8F94,

        R16                             = 0x822A,
        R16_SNORM                       = 0x8F98,

        RG8                             = 0x822B,
        RG8_SNORM                       = 0x8F95,

        RG16                            = 0x822C,
        RG16_SNORM                      = 0x8F99,

        R3_G3_B2                        = 0x2A10,
        RGB4                            = 0x804F,
        RGB5                            = 0x8050,
        RGB565                          = 0x8D62,

        RGB8                            = 0x8051,
        RGB8_SNORM                      = 0x8F96,
        RGB10                           = 0x8052,
        RGB12                           = 0x8053,

        RGB16                           = 0x8054,
        RGB16_SNORM                     = 0x8F9A,

        RGBA2                           = 0x8055,
        RGBA4                           = 0x8056,
        RGB5_A1                         = 0x8057,
        RGBA8                           = 0x8058,
        RGBA8_SNORM                     = 0x8F97,

        RGB10_A2                        = 0x8059,
        RGB10_A2UI                      = 0x906F,

        RGBA12                          = 0x805A,
        RGBA16                          = 0x805B,
        RGBA16_SNORM                    = 0x8F9B,

        SRGB8                           = 0x8C41,
        SRGB8_ALPHA8                    = 0x8C43,

        R16F                            = 0x822D,
        RG16F                           = 0x822F,
        RGB16F                          = 0x881B,
        RGBA16F                         = 0x881A,

        R32F      = 0x822E,
        RG32F      = 0x8230,
        RGB32F      = 0x8815,
        RGBA32F      = 0x8814,

        R11F_G11F_B10F      = 0x8C3A,
        RGB9_E5      = 0x8C3D,


        R8I      = 0x8231,
        R8UI      = 0x8232,
        R16I      = 0x8233,
        R16UI      = 0x8234,
        R32I      = 0x8235,
        R32UI      = 0x8236,
        RG8I      = 0x8237,
        RG8UI      = 0x8238,
        RG16I      = 0x8239,
        RG16UI      = 0x823A,
        RG32I      = 0x823B,
        RG32UI      = 0x823C,

        RGB8I      = 0x8D8F,
        RGB8UI      = 0x8D7D,
        RGB16I      = 0x8D89,
        RGB16UI      = 0x8D77,

        RGB32I      = 0x8D83,
        RGB32UI      = 0x8D71,
        RGBA8I      = 0x8D8E,
        RGBA8UI      = 0x8D7C,
        RGBA16I      = 0x8D88,
        RGBA16UI      = 0x8D76,
        RGBA32I      = 0x8D82,

        RGBA32UI      = 0x8D70,

        // GL 4.4 Table 8.13
        DEPTH_COMPONENT16 = 0x81A5,
        DEPTH_COMPONENT24 = 0x81A6,
        DEPTH_COMPONENT32 = 0x81A7,

        DEPTH_COMPONENT32F = 0x8CAC,
        DEPTH24_STENCIL8 = 0x88F0,
        DEPTH32F_STENCIL8 = 0x8CAD,

        STENCIL_INDEX1 = 0x8D46,
        STENCIL_INDEX4 = 0x8D47,
        STENCIL_INDEX8 = 0x8D48,
        STENCIL_INDEX16 = 0x8D49,

        NUM_UNCOMPRESSED_GLINTERNALFORMATS = 74,
    };

    enum GLInternalFormat_Compressed : uint32_t {
        // GL 4.4 Table 8.14
        COMPRESSED_RED = 0x8225,
        COMPRESSED_RG = 0x8226,
        COMPRESSED_RGB = 0x84ED,
        COMPRESSED_RGBA = 0x84EE,

        COMPRESSED_SRGB = 0x8C48,
        COMPRESSED_SRGB_ALPHA = 0x8C49,

        COMPRESSED_RED_RGTC1 = 0x8DBB,
        COMPRESSED_SIGNED_RED_RGTC1 = 0x8DBC,
        COMPRESSED_RG_RGTC2 = 0x8DBD,
        COMPRESSED_SIGNED_RG_RGTC2 = 0x8DBE,

        COMPRESSED_RGBA_BPTC_UNORM = 0x8E8C,
        COMPRESSED_SRGB_ALPHA_BPTC_UNORM = 0x8E8D,
        COMPRESSED_RGB_BPTC_SIGNED_FLOAT = 0x8E8E,
        COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT = 0x8E8F,

        COMPRESSED_RGB8_ETC2 = 0x9274,
        COMPRESSED_SRGB8_ETC2 = 0x9275,
        COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 = 0x9276,
        COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 = 0x9277,
        COMPRESSED_RGBA8_ETC2_EAC = 0x9278,
        COMPRESSED_SRGB8_ALPHA8_ETC2_EAC = 0x9279,

        COMPRESSED_R11_EAC = 0x9270,
        COMPRESSED_SIGNED_R11_EAC = 0x9271,
        COMPRESSED_RG11_EAC = 0x9272,
        COMPRESSED_SIGNED_RG11_EAC = 0x9273,

         NUM_COMPRESSED_GLINTERNALFORMATS = 24,
    };
 
    enum GLBaseInternalFormat : uint32_t {
        // GL 4.4 Table 8.11
        BIF_DEPTH_COMPONENT = 0x1902,
        BIF_DEPTH_STENCIL = 0x84F9,
        BIF_RED = 0x1903,
        BIF_RG = 0x8227,
        BIF_RGB = 0x1907,
        BIF_RGBA = 0x1908,
        BIF_STENCIL_INDEX = 0x1901,

        NUM_GLBASEINTERNALFORMATS = 7,
    };

    enum CubeMapFace {
        POS_X = 0,
        NEG_X = 1,
        POS_Y = 2,
        NEG_Y = 3,
        POS_Z = 4,
        NEG_Z = 5,
        NUM_CUBEMAPFACES = 6,
    };

    using Byte = uint8_t;

    // Chunk of data
    struct Storage {
        size_t _size {0};
        Byte* _bytes {nullptr};

        Byte* data() {
            return _bytes;
        }
        const Byte* data() const {
            return _bytes;
        }
        size_t size() const { return _size; }

        ~Storage() { if (_bytes) { delete _bytes; } }

        Storage() {}
        Storage(size_t size) :
                _size(size)
        {
            if (_size) { _bytes = new Byte[_size]; }
        }

        Storage(size_t size, Byte* bytes) :
                _size(size)
        {
            if (_size && _bytes) { _bytes = bytes; }
        }
    };

    // Header
    struct Header {
        static const size_t IDENTIFIER_LENGTH = 12;
        using Identifier = std::array<uint8_t, IDENTIFIER_LENGTH>;
        static const Identifier IDENTIFIER;

        static const uint32_t ENDIAN_TEST = 0x04030201;
        static const uint32_t REVERSE_ENDIAN_TEST = 0x01020304;

        Header();

        Byte identifier[IDENTIFIER_LENGTH];
        uint32_t endianness { ENDIAN_TEST };
        uint32_t glType;
        uint32_t glTypeSize;
        uint32_t glFormat;
        uint32_t glInternalFormat;
        uint32_t glBaseInternalFormat;
        uint32_t pixelWidth;
        uint32_t pixelHeight;
        uint32_t pixelDepth;
        uint32_t numberOfArrayElements;
        uint32_t numberOfFaces;
        uint32_t numberOfMipmapLevels;
        uint32_t bytesOfKeyValueData;

        uint32_t evalMaxDimension() const;
        uint32_t evalMaxLevel() const;
        uint32_t evalPixelWidth(uint32_t level) const;
        uint32_t evalPixelHeight(uint32_t level) const;
        uint32_t evalPixelDepth(uint32_t level) const;

        size_t evalPixelSize() const;
        size_t evalRowSize(uint32_t level) const;
        size_t evalFaceSize(uint32_t level) const;
        size_t evalImageSize(uint32_t level) const;

    };

    // Key Values
    using KeyValue = std::pair<std::string, std::string>;
    using KeyValues = std::list<KeyValue>;
     

    struct Mip {
        uint32_t imageSize;
        const Byte* _bytes;
    };
    using Mips = std::vector<Mip>;

    class KTX {
        void resetStorage(Storage* src);

    public:

        KTX();

        bool read(const Storage* src); 
        bool read(Storage* src);

        std::unique_ptr<Storage> _storage;

        const Header* getHeader() const;
        const Byte* getKeyValueData() const;

        KeyValues _keyValues;

        Mips _mips;

        static bool checkStorageHeader(const Storage& storage);
        static KTX* create(const Storage* src);
    };

}

#endif // hifi_ktx_KTX_h