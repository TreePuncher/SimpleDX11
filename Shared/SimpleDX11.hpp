#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <tiny_obj_loader.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include <filesystem>

struct VShader
{
	ID3D11VertexShader* shader      = nullptr;
	ID3DBlob*           blob        = nullptr;
	char*               shaderFile  = nullptr;
	size_t              bufferSize  = 0;
	
	~VShader()
	{
		if (shader)
			shader->Release();

		if (blob)
			blob->Release();

		if (shaderFile)
			free(shaderFile);
	}

	void*   ByteCode()      const 
	{
		if(blob)
			return blob->GetBufferPointer();
		return shaderFile;
	}
	
	size_t  ByteCodeSize()  const 
	{ 
		if (blob)
			return blob->GetBufferSize(); 
		return bufferSize;
	}

	operator ID3D11VertexShader* () const { return shader; }
	operator ID3DBlob* ()           const { return blob; }
};

struct GShader
{
	ID3D11GeometryShader* shader = nullptr;

	~GShader()
	{
		if (shader)
			shader->Release();
	}

	operator ID3D11GeometryShader* () { return shader; }
};

struct PShader
{
	ID3D11PixelShader* shader = nullptr;

	~PShader()
	{
		if (shader)
			shader->Release();
	}

	operator ID3D11PixelShader* () { return shader; }
};

struct DX_Context
{
	DX_Context() = default;

					DX_Context  (const DX_Context&) = delete;
	DX_Context&     operator =  (const DX_Context&) = delete;

	DX_Context  (DX_Context&& rhs) : 
		swapChain   { std::exchange(rhs.swapChain, nullptr) },
		device      { std::exchange(rhs.device,  nullptr)    },
		context     { std::exchange(rhs.context, nullptr)   } {}

	DX_Context&     operator =  (DX_Context&& rhs)
	{
		swapChain   = std::exchange(rhs.swapChain, nullptr);
		device      = std::exchange(rhs.device,  nullptr);
		context     = std::exchange(rhs.context, nullptr);  
	}

	IDXGISwapChain*         swapChain;
	ID3D11Device*           device;
	ID3D11DeviceContext*    context = nullptr;

	ID3D11Buffer* CreateConstantBuffer(size_t constantBuffer)
	{
		ID3D11Buffer* constants;
		D3D11_BUFFER_DESC   bufferDesc;
		bufferDesc.ByteWidth            = (UINT)constantBuffer;
		bufferDesc.Usage                = D3D11_USAGE_DYNAMIC;
		bufferDesc.BindFlags            = D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER;
		bufferDesc.CPUAccessFlags       = D3D11_CPU_ACCESS_WRITE;
		bufferDesc.MiscFlags            = 0;
		bufferDesc.StructureByteStride  = 0;

		auto res = device->CreateBuffer(&bufferDesc, nullptr, &constants);

		return constants;
	}        
	
	ID3D11RenderTargetView* GetBackBufferView()
	{
		ID3D11Resource* currentBuffer;

		swapChain->GetBuffer(0, IID_PPV_ARGS(&currentBuffer));

		ID3D11RenderTargetView* renderTargetView;
		D3D11_RENDER_TARGET_VIEW_DESC desc;
		desc.ViewDimension      = D3D11_RTV_DIMENSION_TEXTURE2DMS;
		desc.Format             = DXGI_FORMAT_R16G16B16A16_FLOAT;
		device->CreateRenderTargetView(currentBuffer, &desc, &renderTargetView);

		return renderTargetView;
	}

	VShader  LoadVertexShader(LPCWCHAR file, const char* entryPoint)
	{
		ID3DBlob* blob = nullptr;
		auto hr1 = D3DCompileFromFile(file, nullptr, nullptr, entryPoint, "vs_5_0", D3DCOMPILE_DEBUG, 0, &blob, nullptr);

		ID3D11VertexShader* shader = nullptr;
		device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader);

		return { shader, blob };
	}

	GShader  LoadGeometryShader(LPCWCHAR file, const char* entryPoint)
	{
		ID3DBlob* blob = nullptr;
		auto hr1 = D3DCompileFromFile(file, nullptr, nullptr, entryPoint, "gs_5_0", D3DCOMPILE_DEBUG, 0, &blob, nullptr);

		ID3D11GeometryShader* shader = nullptr;
		device->CreateGeometryShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader);

		blob->Release();

		return { shader };
	}
   
	PShader  LoadPixelShader(LPCWCHAR file, const char* entryPoint)
	{
		ID3DBlob* blob = nullptr;
		auto hr1 = D3DCompileFromFile(file, nullptr, nullptr, entryPoint, "ps_5_0", 0, 0, &blob, nullptr);

		ID3D11PixelShader* shader = nullptr;
		device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader);

		blob->Release();

		return { shader };
	}

	GShader  LoadCompiledGeometryShader(const char* file)
	{
		const auto fileSize = std::filesystem::file_size(file);
		char* buffer = (char*)malloc(fileSize);

		FILE* f = nullptr;
		fopen_s(&f, file, "rb");
		auto readSize = fread(buffer, 1, fileSize, f);
		fclose(f);

		ID3D11GeometryShader* shader = nullptr;
		device->CreateGeometryShader(buffer, fileSize, nullptr, &shader);

		return { shader };
	}

	VShader  LoadCompiledVertexShader(const char* file)
	{
		const auto fileSize = std::filesystem::file_size(file);
		char* buffer = (char*)malloc(fileSize);

		FILE* f = nullptr;
		fopen_s(&f, file, "rb");
		auto readSize = fread(buffer, 1, fileSize, f);
		fclose(f);

		ID3D11VertexShader* shader = nullptr;
		device->CreateVertexShader(buffer, fileSize, nullptr, &shader);

		return { shader, nullptr, buffer, fileSize };
	}

	PShader  LoadCompiledPixelShader(const char* file)
	{
		const auto fileSize = std::filesystem::file_size(file);
		char* buffer = (char*)malloc(fileSize);

		FILE* f = nullptr;
		fopen_s(&f, file, "rb");
		auto readSize = fread(buffer, 1, fileSize, f);
		fclose(f);

		ID3D11PixelShader* shader = nullptr;
		device->CreatePixelShader(buffer, fileSize, nullptr, &shader);

		return { shader };
	}

	ID3D11Texture2D* CreateDepthBuffer(const size_t width, const size_t height)
	{
		ID3D11Texture2D* depthBuffer = nullptr;
		D3D11_TEXTURE2D_DESC desc = { 0 };
		desc.Format             = DXGI_FORMAT_D32_FLOAT;
		desc.ArraySize          = 1;
		desc.Height             = (UINT)height;
		desc.Width              = (UINT)width;
		desc.SampleDesc.Count   = 4;
		desc.SampleDesc.Quality = 0;
		desc.MipLevels          = 1;
		desc.BindFlags          = D3D10_BIND_DEPTH_STENCIL;

		device->CreateTexture2D(&desc, 0, &depthBuffer);

		return depthBuffer;
	}

	ID3D11DepthStencilView* CreateDeptStencilView(ID3D11Texture2D* resource)
	{
		ID3D11DepthStencilView* view = nullptr;

		D3D11_DEPTH_STENCIL_VIEW_DESC desc;
		desc.Format         = DXGI_FORMAT_D32_FLOAT;
		desc.ViewDimension  = D3D11_DSV_DIMENSION_TEXTURE2DMS;
		desc.Flags          = 0;
		device->CreateDepthStencilView(resource, &desc, &view);
		
		return view;
	}

	ID3D11Buffer* CreateVertexBuffer(void* buffer, const size_t byteSize)
	{
		D3D11_BUFFER_DESC   bufferDesc;
		bufferDesc.ByteWidth            = (UINT)byteSize;
		bufferDesc.Usage                = D3D11_USAGE_DEFAULT;
		bufferDesc.BindFlags            = D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.CPUAccessFlags       = 0;
		bufferDesc.MiscFlags            = 0;
		bufferDesc.StructureByteStride  = 0;

		D3D11_SUBRESOURCE_DATA initial;
		initial.pSysMem             = buffer;
		initial.SysMemPitch         = 0;
		initial.SysMemSlicePitch    = 0;

		ID3D11Buffer* vertexBuffer;
		auto res = device->CreateBuffer(&bufferDesc, &initial, &vertexBuffer);

		return vertexBuffer;
	}

	ID3D11Buffer* CreateIndexBuffer(void* buffer, const size_t byteSize)
	{

		D3D11_BUFFER_DESC   bufferDesc = { 0 };
		bufferDesc.ByteWidth            = (UINT)byteSize;
		bufferDesc.Usage                = D3D11_USAGE_DEFAULT;
		bufferDesc.BindFlags            = D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER;

		D3D11_SUBRESOURCE_DATA initial = { 0 };
		initial.pSysMem             = buffer;

		ID3D11Buffer* vertexBuffer;
		auto res = device->CreateBuffer(&bufferDesc, &initial, &vertexBuffer);

		return vertexBuffer;
	}

	ID3D11Texture2D* CreateTexture(size_t width, size_t height, DXGI_FORMAT format, void* initial)
	{
		D3D11_TEXTURE2D_DESC desc = { 0 };
		desc.Format             = format;
		desc.ArraySize          = 1;
		desc.BindFlags          = D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags     = 0;
		desc.Height             = (UINT)height;
		desc.Width              = (UINT)width;
		desc.SampleDesc         = { 1, 0 };
		desc.MiscFlags          = 0;
		desc.MipLevels          = 1;
		desc.Usage              = D3D11_USAGE_DEFAULT;

		D3D11_SUBRESOURCE_DATA sr = { 0 };
		sr.pSysMem      = initial;
		sr.SysMemPitch  = (UINT)(sizeof(float) * width);

		ID3D11Texture2D* texture;
		auto hr = device->CreateTexture2D(&desc, &sr, &texture);

		return texture;
	}

	ID3D11Texture3D* CreateTexture3D(size_t width, size_t height, size_t depth, DXGI_FORMAT format, void* initial)
	{
		D3D11_TEXTURE3D_DESC desc = { 0 };
		desc.Format             = format;
		desc.BindFlags          = D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags     = 0;
		desc.Height             = (UINT)height;
		desc.Width              = (UINT)width;
		desc.Depth              = (UINT)depth;
		desc.MiscFlags          = 0;
		desc.MipLevels          = 1;
		desc.Usage              = D3D11_USAGE_DEFAULT;

		D3D11_SUBRESOURCE_DATA sr = { 0 };
		sr.pSysMem          = initial;
		sr.SysMemPitch      = (UINT)(sizeof(float) * width);
		sr.SysMemSlicePitch = (UINT)(sizeof(float) * width * height);

		ID3D11Texture3D* texture;
		auto hr = device->CreateTexture3D(&desc, &sr, &texture);

		return texture;
	}

	template<typename TY>
	void UpdateDiscard(ID3D11Resource* buffer, const TY& values)
	{
		D3D11_MAPPED_SUBRESOURCE sr = { 0 };
		context->Map(buffer, 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &sr);
		memcpy(sr.pData, &values, sizeof(values));
		context->Unmap(buffer, 0);
	}
};

inline LRESULT CALLBACK WindowProcess(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hWnd, message, wParam, lParam);
}

inline void RegisterWindowClass( HINSTANCE hinst )
{
	// Register Window Class
	WNDCLASSEX wcex = {0};

	wcex.cbSize			= sizeof( wcex );
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= &WindowProcess;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hinst;
	wcex.hIcon			= LoadIcon( wcex.hInstance, IDI_APPLICATION );
	wcex.hCursor		= LoadCursor( nullptr, IDC_ARROW );
	wcex.hbrBackground	= (HBRUSH)( COLOR_WINDOW );
	wcex.lpszMenuName	= nullptr;
	wcex.lpszClassName	= L"RENDER_WINDOW";
	wcex.hIconSm		= LoadIcon( wcex.hInstance, IDI_APPLICATION );

	auto res = RegisterClassEx( &wcex );
	auto temp = 0;
}

inline DX_Context CreateDX(uint32_t width = 800, uint32_t height = 600)
{
	DX_Context out;

	D3D_FEATURE_LEVEL requested[] = { D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_1 };
	
	RegisterWindowClass(GetModuleHandle(0));

	auto windowHndl = CreateWindow(
		L"RENDER_WINDOW", 
		L"Debug Vis", 
		WS_OVERLAPPEDWINDOW | WS_SIZEBOX,
		0, 0, width, height, 0, 0, 0, nullptr);

	DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
	swapChainDesc.BufferCount                           = 3;
	swapChainDesc.BufferDesc.Width                      = width;
	swapChainDesc.BufferDesc.Height                     = height;
	swapChainDesc.BufferDesc.Format                     = DXGI_FORMAT_R16G16B16A16_FLOAT;
	swapChainDesc.BufferDesc.RefreshRate.Numerator      = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator    = 1;
	swapChainDesc.BufferDesc.Scaling                    = DXGI_MODE_SCALING_STRETCHED;
	swapChainDesc.BufferDesc.ScanlineOrdering           = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
	swapChainDesc.SampleDesc.Count                      = 4;
	swapChainDesc.SampleDesc.Quality                    = 0;
	swapChainDesc.BufferUsage                           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect                            = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Windowed                              = true;
	swapChainDesc.Flags                                 = 0;
	swapChainDesc.OutputWindow                          = windowHndl;

	IDXGISwapChain*         swapChain;
	ID3D11Device*           device;
	ID3D11DeviceContext*    context = nullptr;

	uint32_t flags = 0;

#ifdef _DEBUG
	flags = D3D11_CREATE_DEVICE_DEBUG;
#endif

	auto hres = D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		flags,
		requested,
		1,
		D3D11_SDK_VERSION,
		&swapChainDesc,
		&swapChain,
		&device,
		nullptr,
		&context);

	out.context     = context;
	out.device      = device;
	out.swapChain   = swapChain;


	ShowWindow(windowHndl, SW_SHOW);

	return out;
}

struct Camera
{
	DirectX::XMVECTOR p = DirectX::XMVectorSet(0, 0, 0, 0);
	DirectX::XMVECTOR q = DirectX::XMVectorSet(0, 0, 0, 1);

	float fov           = 3.1415919f / 4;
	float aspectRatio   = 800.0f / 600.0f;

	DirectX::XMMATRIX GetPerspective() const noexcept
	{
		const auto p = DirectX::XMMatrixPerspectiveFovRH(
						fov, aspectRatio, 0.01f, 100.0f);

		return DirectX::XMMatrixTranspose(p);
	}

	DirectX::XMMATRIX GetView() const noexcept
	{
		const auto translate    = DirectX::XMMatrixTranslationFromVector(p);
		const auto rotate       = DirectX::XMMatrixRotationQuaternion(q);

		return DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(
				nullptr,
				rotate * translate));
	}

	DirectX::XMMATRIX GetPV() const noexcept
	{
		auto p = GetPerspective();
		auto v = GetView();

		return DirectX::XMMatrixTranspose(p * v);
	}

	void Yaw(float a)
	{
	   const auto y = DirectX::XMQuaternionRotationAxis(
				DirectX::XMVectorSet(0, 1, 0, 0),
				a);

	   q = DirectX::XMQuaternionMultiply(q, y);
	}

	void Pitch(float a)
	{
		const auto y = DirectX::XMQuaternionRotationAxis(
			DirectX::XMVectorSet(1, 0, 0, 0),
			a);

		q = DirectX::XMQuaternionMultiply(q, y);
	}

	void Roll(float a)
	{
		const auto y = DirectX::XMQuaternionRotationAxis(
			DirectX::XMVectorSet(0, 0, 1, 0),
			a);

		q = DirectX::XMQuaternionMultiply(q, y);
	}

	void Translate(float x, float y, float z)
	{
		p = DirectX::XMVectorAdd(
				p, 
				DirectX::XMVectorSet(x, y, z, 0));
	}

	void Translate(DirectX::XMVECTOR xyz)
	{
		p = DirectX::XMVectorAdd(
			p,
			xyz);
	}
};