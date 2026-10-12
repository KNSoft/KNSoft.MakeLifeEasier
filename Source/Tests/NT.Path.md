# DOS-to-NT long-path conversion

`NT_Win32PathToNtPath` implements DOS-to-NT path conversion in MLE. It returns an
allocated, terminated `UNICODE_STRING`; release it with `NT_FreeNtPath`.
DOS-relative and drive-relative inputs follow RTL current-directory semantics.
The converter accepts only the DOS path and output string.

`NT_InitWin32PathObject` calls this converter and initializes case-insensitive
`OBJECT_ATTRIBUTES` with `RootDirectory` set to NULL. Its returned `NtPath` has
the same ownership and conversion rules as the converter.

The public file helpers keep DOS path conversion separate from native name
handling:

- `IO_CreateWin32File`, `IO_OpenWin32File`, `IO_DeleteWin32File` and
  `IO_GetWin32FileAttributes` accept DOS paths without a root-handle parameter.
  They convert the path, call the corresponding native helper with a NULL root,
  and release the temporary NT name.
- `IO_CreateFile`, `IO_OpenFile`, `IO_DeleteFile` and `IO_GetFileAttributes` accept
  a counted `UNICODE_STRING` and optional `RootDirectory`. They pass that name
  to NT without DOS normalization, separator replacement or name allocation.
  The string can reference caller-owned memory for the duration of the call.

For a filesystem root, native names such as `NUL`, `CON` and `COM1` can identify
ordinary child files. `C:file` can identify a named stream of the child file `C`.
An empty name can identify the root itself when the operation's options permit
it. NT and the filesystem determine these results; MLE does not reinterpret
the names as DOS paths or resolve dot components.

When migrating callers, remove the NULL root argument from the Win32 helpers and
from `NT_InitWin32PathObject`. Callers that used a non-NULL root must use the native
helpers with a `UNICODE_STRING`, and use native backslash separators explicitly.
The earlier initializer's root branch, name copy and slash conversion are removed.

## Conversion implementation

The public function contains a single RTL-based implementation for all supported
systems. It does not resolve or call
`RtlDosLongPathNameToNtPathName_U_WithStatus` in production. There is no build-number
gate, no Win32 long-path policy change, and no hard import of the RS3 export.

It first calls the traditional RTL converter. Only
`STATUS_NAME_TOO_LONG` enters the additional long-path handling. This preserves
short-path errors, device-name handling, and raw `\\?\` / `\??\` conversion.
The remaining normalization uses `RtlGetFullPathName_UstrEx` with no dynamic-string
argument: it queries the required capacity and, when necessary, retries with a
buffer of that exact size. Each further `STATUS_BUFFER_TOO_SMALL` result requests
another exact-size allocation and retry, with no fixed retry-count limit, matching
the native converter's capacity-growth rule. A new temporary buffer is allocated
before the old one is released; allocation or parsing failure releases the current
temporary buffer. Errors before the first successful temporary allocation return
directly. Namespace replacement uses `\??\` or `\??\UNC\` for normalized UNC paths.
The implementation checks the final byte length
including the terminating WCHAR against `UNICODE_STRING_MAX_BYTES` before casting
lengths to USHORT. Temporary buffers and the returned string use the RTL process
heap.

The path definitions come from KNSoft.NDK 1.2.76-beta. The converter classifies
its normalized counted string with
`_Inline_RtlDetermineDosPathNameType_Ustr`. DOS/UNC namespace replacement uses
`_Inline_RtlDosPathPrefix` and `_Inline_RtlUncPathPrefix`, without duplicate MLE
constants. These replacements leave the conversion, capacity-growth,
error-output and cleanup contracts unchanged.

## Reverse-engineering evidence

Analysis used the supplied, read-only IDA MCP instance at port 13337, including
both decompilation and disassembly. The analyzed x64 image is
`C:\Windows\System32\ntdll.dll`, version `10.0.26100.9278`, image base
`0x180000000`, SHA-256
`a74f7482085eab125ccc09152ab7e0b5994bcb13e1a7b29880bdbb24179ecb8b`.

| Routine | Address | Relevant behavior |
| --- | --- | --- |
| `RtlDosLongPathNameToNtPathName_U_WithStatus` | `0x1800DA2F0` | Nine-instruction wrapper; passes flag 4 to the common conversion code. |
| `RtlDosPathNameToNtPathName_U_WithStatus` | `0x1800DCBC0` | Uses the same converter without flag 4; validates the input Unicode length. |
| `RtlpDosPathNameToRelativeNtPathName_U` | `0x1800DA320` | Rejects more than 0x7FFE WCHARs before conversion. |
| `RtlpDosPathNameToRelativeNtPathName` | `0x18003D650` | Flag 4 allows growing the initial 520-byte normalization buffer; assembles namespace prefixes and enforces the 0xFFFE-byte allocation limit. |
| `RtlGetFullPathName_Ustr` | `0x18003F3F0` | Internal DOS parser used by both conversion and the exported UstrEx function; this routine itself is not exported. |
| `RtlGetFullPathName_UstrEx` | `0x180008240` | Without a dynamic-string argument, reports the parser's required capacity through `BytesRequired`. |
| `RtlpReferenceCurrentDirectory` | `0x180006BD0` | Private current-directory reference and volume-validation logic involved in optional relative-name outputs. |

The native normalization loop at `0x18003D714` retries whenever the parser reports
a capacity larger than the current buffer. Allocation at `0x18003DC58` uses that
exact capacity, and `0x18003DC6D` returns to the parser without a retry counter.
The MLE implementation follows that growth rule while releasing replaced
temporary buffers and cleaning up a retained buffer when a new allocation fails.

A boundary test exposed a real difference in UstrEx's automatic allocation mode:
it adds two bytes to the parser's reported capacity and rejects an allocation above
0xFFFE. The native long-path converter can allocate the exact capacity instead.
Using the explicit capacity-query mode fixes three failing local-device-path cases
near the maximum Unicode-string size, including trailing spaces and dots.

The implementation uses public RTL exports and does not bind the private functions
listed above. It exposes only the NT string needed by MLE/FWI, without the native
optional `FilePart` or `RTL_RELATIVE_NAME_U` outputs. It does not claim complete
equivalence to every parameter and internal side effect of that RTL API.

Raw IDA evidence is retained locally under
`E:\KNSoft\KNSoft.NDK\Source\OutDir\PathCompat`; that directory is ignored by Git.
The IDA database was not changed.

## Validation

`Source/Tests/NT.c` loads the system long-path export with `GetProcAddress` solely
as a test oracle, then compares it to `NT_Win32PathToNtPath`. Production API thunks
are not used by this test. Every comparison checks NTSTATUS, Length, MaximumLength,
UTF-16 bytes including the final NUL, and output-field preservation on failure.
The same path set now makes 21,428 comparisons, one for each input, when the native
export is available. The test covers:

- DOS absolute, rooted, relative and drive-relative paths, UNC, local devices,
  extended DOS / UNC, and DOS device names.
- Forward slashes, repeated separators, dot segments, trailing spaces / dots,
  Chinese text and paired / unpaired UTF-16 surrogates.
- MAX_PATH boundaries, 32,766-WCHAR boundaries, namespace-prefix overflow, and long
  input that collapses to a short normalized path.
- 20,000 deterministic generated inputs, plus fixed and boundary cases.

On this Windows 11 host, `LongPathsEnabled` remained 0. The traditional converter
returned `STATUS_NAME_TOO_LONG` (0xC0000106) for the test's ordinary 600-character
DOS path, ensuring that the RTL-based implementation exercised its allocation path.

Before removal of the native dispatcher, validation built the entire MLE solution
in x64, x86, ARM64 and ARM64EC
Debug / Release. For each x64/x86 configuration, `Tests.exe -Run` passed all 10
automatic entries (386 assertions), including 42,856 path comparisons with zero
mismatches. ARM64 and ARM64EC were compiled, not executed on this x64 host. Those
test and build logs are under `Source/OutDir/PathCompat`.

Also before removal of the dispatcher, the native retry rule was matched and the
entire solution was rebuilt in x64/x86
Debug; both automatic suites again passed 386 assertions and 42,856 path comparisons
with zero mismatches. A separate harness includes the actual `Object.c` with
controlled RTL call stubs. Its 27 x64 and 26 x86 cases cover consecutive required
capacities of 1024, 2048 and 4096 bytes before success, later invalid names and
other errors, intermediate/final allocation failure, exact capacity boundaries,
cleanup counts, and namespace prefixes. The three-growth success case makes four
full-path calls and releases each of its three temporary buffers exactly once.
Intermediate allocation failure retains the current buffer for cleanup. Final
allocation failure preserves the native behavior: Buffer becomes NULL while
Length and MaximumLength remain unchanged. Those logs and the harness are
under `Source/OutDir/NTObjectReview`.

After merging the implementation into `NT_Win32PathToNtPath` and removing the
production native dispatcher and thunk, the entire solution was rebuilt in x64
and x86 Debug. Each `Tests.exe -Run` suite passed all 10 automatic entries with
382 assertions, zero failures and zero skips. The focused path entry passed eight
assertions and 21,428 native comparisons with zero mismatches on each architecture.
The count is half the earlier 42,856 because each input now tests the public
implementation once; the path set is unchanged.

The controlled-call harness was updated to call the public implementation in the
actual `Object.c`. It passed all 27 x64 and 26 x86 cases with zero failures and
contract violations; both harness compilations reported no warnings. Consecutive
capacity growth and the cleanup of replaced or retained buffers remain covered.
Those logs are `Build-unified-x64.log`, `Build-unified-x86.log`,
`Tests-unified-x64.log`, `Tests-unified-x86.log`, `Faults-x64.log` and `Faults-x86.log`
under `Source/OutDir/NTObjectReview`.

The last validation before separating the Win32 and native file interfaces used
KNSoft.NDK 1.2.76-beta. Both x64/x86 Debug solution builds passed with zero errors
and 28 C4200 warnings per architecture from the NDK Win32/WinUser.h flexible-array
resource structures. Each automatic suite passed 11 entries and 569 assertions,
with zero failures or skips; the long-path entry made 21,428 native comparisons
per architecture with zero mismatches. Those logs are `Build-native-root-*.log`
and `Tests-native-root-*.log` under `Source/OutDir/NTObjectReview`.

These are historical results. The former initializer's root-allocation harness
and the root tests that called Win32 helpers do not validate the separated
interfaces. The converter's controlled-call harness was last rerun with NDK
1.2.76-beta: all 27 x64 and 26 x86 cases passed with zero failures or contract
violations. Its logs are `Faults-*.log` under `Source/OutDir/NTObjectReview`.

After separating the Win32 and native interfaces, x64/x86 Debug solution builds
passed with zero errors and the same 28 dependency C4200 warnings per architecture.
Each automatic suite passed all 11 entries and 521 assertions, with no failures
or skips. `NT_NativeRootDirectory` passed 139 assertions on each architecture.
It uses borrowed native names for root-relative file operations, covering literal
DOS device names, named streams, the empty root name, dot components, raw forward
slashes, absolute NT names paired with a root, and current-directory independence.
The initializer checks now verify DOS conversion with a NULL object root.

A real file with a 250-WCHAR leaf and an ordinary DOS path longer than MAX_PATH
is created, written, opened, read, queried and deleted through the four Win32
entry points. Querying the same borrowed leaf below the native root confirms
the target. Native root cleanup releases all created files, streams and directories.
The long-path differential entry still makes 21,428 comparisons per architecture,
with zero mismatches. C++ header probes passed for x64 and x86, checking the new
Win32 and initializer signatures and the retained native root signature.

The latest logs are `Build-path-api-*.log`, `Tests-path-api-*.log` and
`PathApiHeader-*.log` under `Source/OutDir/NTObjectReview`. The removed initializer
root-allocation harness is retired; the converter implementation was not changed
in this interface redesign.

Final pre-commit validation built the complete solution in x64, x86, ARM64 and
ARM64EC Debug/Release, with zero errors in all eight configurations. The four
x64/x86 automatic suites each passed 521 assertions and 21,428 native path
comparisons, with no failures, skips or mismatches. ARM64/ARM64EC were compiled,
not executed on this x64 host. Fresh configurations report the same 28 NDK C4200
warnings; incremental x64/x86 Debug builds report none. Those logs are
`Build-commit-review-*.log` and `Tests-commit-review-*.log` under
`Source/OutDir/NTObjectReview`.

For the earlier SIZE_T arithmetic fix, explicitly enabled MSVC Core Check analysis
reported no diagnostics in `Object.c` before or after that fix; both runs reported
the same two C26451 warnings in SDK `winioctl.h`. The original IDE warning was not
reproduced. An isolated control confirms that widening only the result variable to
SIZE_T leaves C26451 warnings, while widening the first operand before arithmetic
avoids them.

Run the focused test from a built source tree:

```powershell
.\Source\OutDir\x64\Debug\Tests.exe -Run NT_Win32LongPath
.\Source\OutDir\x86\Debug\Tests.exe -Run NT_Win32LongPath
.\Source\OutDir\x64\Debug\Tests.exe -Run NT_NativeRootDirectory
# Repository-required automatic suite:
.\Source\OutDir\x64\Debug\Tests.exe -Run
```

The recorded results validate the public RTL-based conversion against the tested
system binary; the historical results also cover the earlier dispatcher structure.
They do not certify execution on a pre-RS3 Windows 10 installation. No such OS was
available for these runs. The separate controlled-call harness injects allocation
failure and successively larger required capacities. The differential test does
not actively mutate the process current directory concurrently; RTL re-reads that
state on each normalization attempt, as in the native converter.

To create a distinct local package after building all eight configurations:

```powershell
nuget pack Source/KNSoft.MakeLifeEasier.nuspec -OutputDirectory Source/OutDir/Packages `
    -Version 1.0.12-beta.local.1 -NoPackageAnalysis
```
