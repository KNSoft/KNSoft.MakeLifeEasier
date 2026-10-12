# KNSoft.MakeLifeEasier

A fast, low-level and convenient C/C++ library to "Make Life Easier" when suffering from Windows NT development.

`NT_Win32PathToNtPath` supports long DOS paths through a single RTL-based implementation, without requiring the Win32 long-path policy. The contract, reverse-engineering evidence, and differential tests are described in [Source/Tests/NT.Path.md](Source/Tests/NT.Path.md).

Win32 file helpers accept DOS paths and convert them before calling the native file helpers. For root-relative operations, use the native helpers with a counted `UNICODE_STRING` and `RootDirectory`; the supplied names follow NT rules and are passed without normalization or copying.
