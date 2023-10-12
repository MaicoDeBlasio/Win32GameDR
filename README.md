Shaders in the application are embedded as precompiled headers.

All shaders target Shader Model 6.7 and are compiled offline with DXC compiler release version:

dxcompiler.dll: 1.7 - 1.7.2308.7 (69e54e290); dxil.dll: 1.7(101.7.2308.12)

The app compiles and runs successfully when NOT using payload access qualifiers, i.e. compiling raytracing shaders with flag `-disable-payload-qualifiers`.

The app will fail to compile if raytracing shaders with enabled (as the default in SM 6.7) payload access qualifiers are included.

This will cause an access violation exception in nvrtum64.dll.

To reproduce this, in _Game_Core.cpp_, comment *out* the following includes:
```
#include "Shaders/RaytracingShaderAO.hlsl.h"
#include "Shaders/RaytracingShaderColor.hlsl.h"
#include "Shaders/RaytracingShaderShadows.hlsl.h"
```

*Remove* comments from the following includes:
```
//#include "Shaders/RaytracingShaderAO_PAQ.hlsl.h"
//#include "Shaders/RaytracingShaderColor_PAQ.hlsl.h"
//#include "Shaders/RaytracingShaderShadows_PAQ.hlsl.h"
```
**Device specifications:**

Processor	12th Gen Intel(R) Core(TM) i9-12900H   2.90 GHz

Installed RAM	16.0 GB (15.7 GB usable)

System type	64-bit operating system, x64-based processor

**Windows specifications:**

Edition	Windows 11 Home
Version	22H2

OS build	22621.2428

Experience	Windows Feature Experience Pack 1000.22674.1000.0

**Graphics hardware specifications:**

NVIDIA GeForce RTX 3070 Laptop GPU
