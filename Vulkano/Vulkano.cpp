#pragma once
#include <iostream>
#include <Windows.h>
#include "Render/Renderer.h"
#include "Render/RenderWindow.h"
#include "Render/Shader.h"

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)									
{
    FShaderCompiler::Get()->AddShader<FDefaultVertexShader>();
    FShaderCompiler::Get()->AddShader<FDefaultPixelShader>();
    FShaderCompiler::Get()->CompileShaders();
    
    FRenderWindow RenderWindow("Vulkano", 1920, 1080);
    RenderWindow.Init(hInstance);
    FRenderer Renderer;
    Renderer.Init(&RenderWindow);
    Renderer.RenderLoop();

    Renderer.Shutdown();
    RenderWindow.Shutdown();
    return 0;																						
}