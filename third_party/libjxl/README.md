# Bundled JPEG XL decoder

Decode-only subset of [libjxl](https://github.com/libjxl/libjxl) **v0.12.0**, plus the pieces it links:

- Google Highway (`third_party/highway`)
- Brotli decoder (`third_party/brotli`, common + dec)
- skcms (`third_party/skcms`, baseline transform only)

`jxl_dec.vcxproj` builds these into a static library. The viewer links that library and does not need a JPEG XL DLL. `sources.txt` is the translation-unit list used by the project.

Color conversion uses libjxl's skcms backend (`JPEGXL_ENABLE_SKCMS`). JPEG-recompressed `.jxl` files decode to pixels; bit-exact JPEG reconstruction is not exposed.
