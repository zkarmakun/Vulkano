#pragma once
#include <iostream>
#include <Windows.h>
#include "Render/Renderer.h"
#include "Render/RenderWindow.h"
#include "Render/Shader.h"
#include "Render/VulkanInterface.h"

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)									
{
    // Initialize vulkan
    FVulkan::CreateVulkanInstance("Vulkano");
#ifdef _DEBUG
    FVulkan::CreateVulkanDebugLayer();
#endif
    FVulkan::CreateVulkanDevice(hInstance);

    // Compile all default shaders
    FShaderCompiler::Get()->AddShader<FDefaultVertexShader>();
    FShaderCompiler::Get()->AddShader<FDefaultPixelShader>();
    FShaderCompiler::Get()->CompileShaders();

    // Create window and attach renderer
    FRenderWindow RenderWindow("Vulkano", 1920, 1080);
    RenderWindow.Init(hInstance);
    FRenderer Renderer;
    Renderer.Init(&RenderWindow);

    // Draw me papu!
    Renderer.RenderLoop();

    // Party is over
    Renderer.Shutdown();
    RenderWindow.Shutdown();

#ifdef _DEBUG
    FVulkan::DestroyVulkanDebugLayer();
#endif
    FVulkan::ExitVulkan();
    return 1;																						
}
