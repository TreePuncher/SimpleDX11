#include "SimpleDX11.hpp"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include <cstdint>
#include <filesystem>
#include <fstream>

struct Drawable
{
    ~Drawable()
    {
        if (indexBuffer)
            indexBuffer->Release();

        indexBuffer = nullptr;
        indexCount = 0;
    }

    Drawable() = default;

    Drawable(ID3D11Buffer* in_buffer, size_t in_size) :
        indexBuffer{ in_buffer },
        indexCount{ in_size } {}

    Drawable(const Drawable& rhs) = delete;
    Drawable& operator = (const Drawable& rhs) = delete;

    Drawable(Drawable&& rhs) :
        indexBuffer{ std::exchange(rhs.indexBuffer, nullptr) },
        indexCount{ std::exchange(rhs.indexCount, 0) } { }

    Drawable& operator = (Drawable&& rhs)
    {
        indexBuffer = std::exchange(rhs.indexBuffer, nullptr);
        indexCount = std::exchange(rhs.indexCount, 0);
    }

    ID3D11Buffer* indexBuffer = nullptr;
    size_t          indexCount = 0;
};

int main(int argv, const char* argvs[])
{
    auto API = CreateDX(1024, 1024);

    // Load Obj file
    tinyobj::attrib_t                   attrib;
    std::vector<tinyobj::shape_t>       shapes;
    std::vector<tinyobj::material_t>    materials;
    std::string                         warn;
    std::string                         err;

    if (argv > 1)
    {
        std::ifstream   fileStream{ argvs[1] };
        if (!fileStream.is_open())
            return -1;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, &fileStream))
            return -1;
    }

    const auto& shape = shapes.front();
    auto vertexBuffer = API.CreateVertexBuffer(attrib.vertices.data(), attrib.vertices.size() * sizeof(float));

    std::vector<Drawable> drawables;

    for (const auto& shape : shapes)
    {
        std::vector<uint16_t> pointIndices;
        pointIndices.reserve(shape.mesh.indices.size());

        const auto end = shape.mesh.indices.size();
        for (size_t i = 0; i < end; i += 3)
        {
            pointIndices.push_back(shape.mesh.indices[i + 0].vertex_index);
            pointIndices.push_back(shape.mesh.indices[i + 2].vertex_index);
            pointIndices.push_back(shape.mesh.indices[i + 1].vertex_index);
        }

        auto indexBuffer = API.CreateIndexBuffer(pointIndices.data(), pointIndices.size() * sizeof(uint16_t));

        drawables.emplace_back(indexBuffer, pointIndices.size());
    }

    struct GPUPoint
    {
        float xyz[3];
    };


    // Initiate API
    auto constants      = API.CreateConstantBuffer(4096);

    auto vertexShader   = API.LoadCompiledVertexShader("VertexShader2.cso");
    auto geometryShader = API.LoadCompiledGeometryShader("GeometryShader2.cso");
    auto pixelShader    = API.LoadCompiledPixelShader("PixelShader2.cso");

    auto depthBuffer        = API.CreateDepthBuffer(1024, 1024);
    auto depthView          = API.CreateDeptStencilView(depthBuffer);

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,  0, 0, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    ID3D11InputLayout* inputLayout1 = nullptr;
    API.device->CreateInputLayout(layout, 1, vertexShader.ByteCode(), vertexShader.ByteCodeSize(), &inputLayout1);

    // Camera
    Camera viewpoint;
    viewpoint.aspectRatio = 1.0f;
    viewpoint.Translate(0, 5.0f, 7.5f);
    viewpoint.Pitch(-3.1415919f / 8.0f);

    // Begin loop
    auto before = std::chrono::high_resolution_clock::now();
    double t    = 0;
    double dt   = 0.0f;

    while (true)
    {
        MSG msg;

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        before = std::chrono::high_resolution_clock::now();

        auto renderTargetView = API.GetBackBufferView();

        struct {
            DirectX::XMMATRIX pvt;
        } constantValues{
            .pvt = DirectX::XMMatrixRotationY((float)t) * viewpoint.GetPV(),
        };

        API.UpdateDiscard(constants, constantValues);

        // Clear States
        API.context->ClearState();
        API.context->VSSetConstantBuffers(0, 1, &constants);
        API.context->PSSetConstantBuffers(0, 1, &constants);

        const float clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
        API.context->ClearRenderTargetView(renderTargetView, clearColor);
        API.context->ClearDepthStencilView(depthView, D3D11_CLEAR_DEPTH, 1.0, 0);

        D3D11_VIEWPORT  viewports = { 0 };
        viewports.Width     = 1024;
        viewports.Height    = 1024;
        viewports.MinDepth  = 0.0f;
        viewports.MaxDepth  = 1.0f;

        D3D11_RECT      rects = { 0 };
        rects.right     = 1024;
        rects.bottom    = 1024;

        const UINT offsets[] = { 0 };
        const UINT strides[] = { 12 };

        API.context->IASetInputLayout(inputLayout1);
        API.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        API.context->IASetVertexBuffers(0, 1, &vertexBuffer, strides, offsets);

        API.context->VSSetShader(vertexShader, nullptr, 0);
        API.context->GSSetShader(geometryShader, nullptr, 0);
        API.context->PSSetShader(pixelShader, nullptr, 0);

        API.context->RSSetViewports(1, &viewports);
        API.context->RSSetScissorRects(1, &rects);
        API.context->OMSetRenderTargets(1, &renderTargetView, depthView);

        // Draw drawables
        for (auto& shape : drawables)
        {
            API.context->IASetIndexBuffer(shape.indexBuffer, DXGI_FORMAT::DXGI_FORMAT_R16_UINT, 0);
            API.context->DrawIndexed((UINT)shape.indexCount, 0, 0);
        }

        // Present
        renderTargetView->Release();
        API.swapChain->Present(1, 0);

        const auto after    = std::chrono::high_resolution_clock::now();
        const auto duration = after - before;

        dt = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1,1>>> (duration).count();
        t += dt;
    }

	return 0;
}