# HP EliteBook 845 G11 / Radeon 780M hardware evidence

Date recorded: 2026-08-03

This privacy-safe summary was extracted from the project owner's supplied Windows DxDiag report. The raw DxDiag was reviewed but not committed because it contains machine-specific identifiers that are unnecessary for compatibility tracking.

## System

- Manufacturer: HP
- Model: HP EliteBook 845 14-inch G11 Notebook PC
- Operating system: Windows 11 Enterprise 64-bit, build 26200
- Processor: AMD Ryzen 7 8840U with Radeon 780M Graphics
- Logical processors reported by DxDiag: 16
- System memory: 16,384 MB RAM; 15,646 MB available to the OS
- DirectX version: DirectX 12

## Graphics device used by the demo

- Adapter: AMD Radeon(TM) Graphics
- Architecture/product class: Radeon 780M integrated graphics in the Ryzen 7 8840U
- PCI vendor ID: `0x1002`
- PCI device ID: `0x1900`
- Device type: full device; integrated/hybrid graphics GPU
- Display memory: 8,240 MB
- Dedicated graphics memory: 417 MB
- Shared graphics memory: 7,822 MB
- Windows display driver: `32.0.11020.10002`
- Driver date: 2024-09-27
- Driver model: WDDM 3.2
- DDI version: 12
- Direct3D feature levels: `12_2`, `12_1`, `12_0`, `11_1`, `11_0`, and lower
- DxDiag device status: no problem

The machine also reported a USB display-only adapter for one external monitor. The Horde Lantern RT capability report selected candidate 0, `AMD Radeon(TM) Graphics`, as the Vulkan RT device; the USB display adapter was not the renderer.

## Vulkan project evidence

The project's generated capability report recorded:

- Backend: Vulkan
- RT mode: `RayTracingPipeline`
- Vulkan API: `1.3.280`
- Vulkan driver text: `2.0.302`
- `VK_KHR_acceleration_structure`: yes
- `VK_KHR_ray_tracing_pipeline`: yes
- `VK_KHR_ray_query`: yes
- `VK_KHR_buffer_device_address`: yes
- `VK_KHR_deferred_host_operations`: yes
- RT scene presented through the swapchain: yes

The corresponding full benchmark evidence is stored in [`horde-elitebook-845-g11-radeon-780m-benchmark-2026-08-03.json`](horde-elitebook-845-g11-radeon-780m-benchmark-2026-08-03.json).
