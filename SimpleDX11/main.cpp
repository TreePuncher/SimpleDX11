#include "SimpleDX11.hpp"

int main(int argv, const char* argvs[])
{
	auto API		= CreateDX(1024, 1024);
	auto constants	= API.CreateConstantBuffer(4096);

    auto vertexShader   = API.LoadCompiledVertexShader("VertexShader.cso");
    auto pixelShader    = API.LoadCompiledPixelShader("PixelShader.cso");

    struct GPUPoint
    {
        float xyz[3];
    };

    struct 
    {
        GPUPoint    p;
    }   slice[] = {
        { -1, -1, 0.5f },
        {  0,  1, 0.5f },
        {  1, -1, 0.5f },
    };

    auto depthBuffer        = API.CreateDepthBuffer(1024, 1024);
    auto depthView          = API.CreateDeptStencilView(depthBuffer);
    auto vertexBuffer       = API.CreateVertexBuffer(slice, sizeof(slice));

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,  0, 0, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    ID3D11InputLayout* inputLayout1 = nullptr;
    API.device->CreateInputLayout(layout, 1, vertexShader.ByteCode(), vertexShader.ByteCodeSize(), &inputLayout1);

    while (true)
    {
        MSG msg;

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        auto renderTargetView = API.GetBackBufferView();

        // Clear States
        API.context->ClearState();

        const float clearColor[4] = { 1.0f, 0.0f, 1.0f, 1.0f };
        API.context->ClearRenderTargetView(renderTargetView, clearColor);


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
        API.context->PSSetShader(pixelShader, nullptr, 0);

        API.context->RSSetViewports(1, &viewports);
        API.context->RSSetScissorRects(1, &rects);
        API.context->OMSetRenderTargets(1, &renderTargetView, nullptr);

        // Draw drawables
        API.context->Draw(3, 0);

        // Present
        renderTargetView->Release();
        API.swapChain->Present(1, 0);
    }

	return 0;
}