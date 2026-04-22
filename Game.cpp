#include "Game.h"
#include "Graphics.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"
#include "Window.h"
#include "BufferStructs.h"

#include <DirectXMath.h>
#include <WICTextureLoader.h>

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// For the DirectX Math library
using namespace DirectX;

// ImGui variables
static float bgColor[4] = { 0.4f, 0.6f, 0.75f, 0.0f };
static int sliderVal = 0;
static float dragVal = 0.0;
const int listItems[5] = { 10, 20, 30, 40, 50 };
static float vsColorTint[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
static float vsOffset[3] = { 0.25f, 0.0f, 0.0f };

// --------------------------------------------------------
// The constructor is called after the window and graphics API
// are initialized but before the game loop begins
// --------------------------------------------------------
Game::Game()
{
	// ImGui setup
	{
		// ImGui init
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui_ImplWin32_Init(Window::Handle());
		ImGui_ImplDX11_Init(Graphics::Device.Get(), Graphics::Context.Get());

		// Pick a style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsLight();
		//ImGui::StyleColorsClassic();
	}

	// Constant buffers
	{
		// Get D3D11.1 context
		Graphics::Context->QueryInterface<ID3D11DeviceContext1>(Context1.GetAddressOf());

		// Start at zero
		cbHeapOffsetInBytes = 0;

		// Set buffer size
		cbHeapSizeInBytes = 1000 * 256;
		cbHeapSizeInBytes = (cbHeapSizeInBytes + 255) / 256 * 256;	// 256 bit alignment

		// Describe and create buffer
		D3D11_BUFFER_DESC heapCbDesc = {};
		heapCbDesc.ByteWidth = cbHeapSizeInBytes;
		heapCbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		heapCbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		heapCbDesc.Usage = D3D11_USAGE_DYNAMIC;
		Graphics::Device->CreateBuffer(&heapCbDesc, 0, ConstantBufferHeap.GetAddressOf());
	}

	// Camera setup
	{
		camera1 = std::make_shared<Camera>
			(0.0f, 0.0f, -3.0f,
				Window::AspectRatio(), 1.5708f,
				0.01f, 500.0f,
				2.5f, 0.01f,
				true);
		cameraVec.push_back(camera1);

		camera2 = std::make_shared<Camera>
			(-0.2f, 0.5f, -2.0f,
				Window::AspectRatio(), 0.4f,
				0.01f, 500.0f,
				1.0f, 0.01f,
				true);
		cameraVec.push_back(camera2);

		// Set active camera
		activeCameraIndex = 0;
	}

	// Load shaders
	{
		// Vertex shaders
		vertexShader = LoadVertexShader(L"VertexShader.cso");
		skyVertexShader = LoadVertexShader(L"SkyVertexShader.cso");
		shadowVertexShader = LoadVertexShader(L"ShadowMapVS.cso");

		// Pixel shaders
		basicPixelShader = LoadPixelShader(L"PixelShader.cso");
		debugUVsPixelShader = LoadPixelShader(L"DebugUVsPS.cso");
		debugNormalsPixelShader = LoadPixelShader(L"DebugNormalsPS.cso");
		customPixelShader = LoadPixelShader(L"CustomPS.cso");
		combinedPixelShader = LoadPixelShader(L"CombinedPS.cso");
		skyPixelShader = LoadPixelShader(L"SkyPixelShader.cso");
	}

	// Set initial graphics API state
	{
		// Tell the input assembler (IA) stage of the pipeline what kind of
		// geometric primitives (points, lines or triangles) we want to draw.  
		// Essentially: "What kind of shape should the GPU draw with our vertices?"
		Graphics::Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Ensure the pipeline knows how to interpret all the numbers stored in
		// the vertex buffer. For this course, all of your vertices will probably
		// have the same layout, so we can just set this once at startup.
		Graphics::Context->IASetInputLayout(inputLayout.Get());

		// Set the active vertex and pixel shaders
		//  - Once you start applying different shaders to different objects,
		//    these calls will need to happen multiple times per frame
		Graphics::Context->VSSetShader(vertexShader.Get(), 0, 0);
		Graphics::Context->PSSetShader(basicPixelShader.Get(), 0, 0);
	}

	// Set up lighting
	{
		// White directional in -Y, +Z direction
		lights[0] = {};
		lights[0].Type = LIGHT_TYPE_DIRECTIONAL;
		lights[0].Direction = XMFLOAT3(0.0f, -1.0f, 1.0f);
		lights[0].Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
		lights[0].Intensity = 1.0f;

		// Red directional in +X, -Y direction
		lights[1] = {};
		lights[1].Type = LIGHT_TYPE_DIRECTIONAL;
		lights[1].Direction = XMFLOAT3(1.0f, -1.0f, 0.0f);
		lights[1].Color = XMFLOAT3(1.0f, 0.0f, 0.0f);
		lights[1].Intensity = 1.0f;
		
		// Blue directional in -X, -Y direction
		lights[2] = {};
		lights[2].Type = LIGHT_TYPE_DIRECTIONAL;
		lights[2].Direction = XMFLOAT3(-1.0f, -1.0f, 0.0f);
		lights[2].Color = XMFLOAT3(0.0f, 0.0f, 1.0f);
		lights[2].Intensity = 1.0f;

		//// White point positioned inside the helix
		//lights[3] = {};
		//lights[3].Type = LIGHT_TYPE_POINT;
		//lights[3].Position = XMFLOAT3(5.0f, 0.0f, 0.0f);
		//lights[3].Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
		//lights[3].Intensity = 5.0f;
		//lights[3].Range = 8.0f;
		//
		//// Green spot positioned above the one-sided quad
		//lights[4] = {};
		//lights[4].Type = LIGHT_TYPE_SPOT;
		//lights[4].Position = XMFLOAT3(12.5f, 2.0f, 0.0f);
		//lights[4].Direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
		//lights[4].Color = XMFLOAT3(0.0f, 1.0f, 0.0f);
		//lights[4].SpotInnerAngle = 0.2f;
		//lights[4].SpotOuterAngle = 0.4f;
		//lights[4].Intensity = 2.0f;
		//lights[4].Range = 12.0f;
	}

	// Load textures
	{
		// Shader Resource Views
		// Loads image file and creates texture and srv

		// Paving stones texture
		{
			DirectX::CreateWICTextureFromFile(
				Graphics::Device.Get(),
				Graphics::Context.Get(),
				FixPath(L"../../Assets/Textures/PavingStones2K/PavingStones138_2K-PNG_Color.png").c_str(),
				//FixPath(L"../../Assets/Textures/Skies/ColdSunset/front.png").c_str(),
				0,
				pavingStonesSrv.GetAddressOf());
			Graphics::Context->PSSetShaderResources(0, 1, pavingStonesSrv.GetAddressOf());

			// Paving stones normal map
			DirectX::CreateWICTextureFromFile(
				Graphics::Device.Get(),
				Graphics::Context.Get(),
				FixPath(L"../../Assets/Textures/PavingStones2K/PavingStones138_2K-PNG_NormalDX.png").c_str(),
				0,
				pavingStonesNormalsSrv.GetAddressOf());
			Graphics::Context->PSSetShaderResources(1, 1, pavingStonesNormalsSrv.GetAddressOf());
		}

		// Graffiti texture
		DirectX::CreateWICTextureFromFile(
			Graphics::Device.Get(),
			Graphics::Context.Get(),
			FixPath(L"../../Assets/Textures/qqGraffiti.png").c_str(),
			0,
			graffitiSrv.GetAddressOf());
		Graphics::Context->PSSetShaderResources(2, 1, graffitiSrv.GetAddressOf());

		// Flat normals texture
		DirectX::CreateWICTextureFromFile(
			Graphics::Device.Get(),
			Graphics::Context.Get(),
			FixPath(L"../../Assets/Textures/flat_normals.png").c_str(),
			0,
			flatNormalsSrv.GetAddressOf());
		Graphics::Context->PSSetShaderResources(3, 1, flatNormalsSrv.GetAddressOf());

		// Rusted metal texture
		{
			DirectX::CreateWICTextureFromFile(
				Graphics::Device.Get(),
				Graphics::Context.Get(),
				FixPath(L"../../Assets/Textures/RustedMetal2K/Metal053B_2K-PNG_Color.png").c_str(),
				0,
				rustedMetalAlbedoSrv.GetAddressOf());
			Graphics::Context->PSSetShaderResources(4, 1, rustedMetalAlbedoSrv.GetAddressOf());

			DirectX::CreateWICTextureFromFile(
				Graphics::Device.Get(),
				Graphics::Context.Get(),
				FixPath(L"../../Assets/Textures/RustedMetal2K/Metal053B_2K-PNG_NormalDX.png").c_str(),
				0,
				rustedMetalNormalsSrv.GetAddressOf());
			Graphics::Context->PSSetShaderResources(5, 1, rustedMetalNormalsSrv.GetAddressOf());

			DirectX::CreateWICTextureFromFile(
				Graphics::Device.Get(),
				Graphics::Context.Get(),
				FixPath(L"../../Assets/Textures/RustedMetal2K/Metal053B_2K-PNG_Roughness.png").c_str(),
					0,
					rustedMetalRoughnessSrv.GetAddressOf());
					Graphics::Context->PSSetShaderResources(6, 1, rustedMetalRoughnessSrv.GetAddressOf());

					DirectX::CreateWICTextureFromFile(
						Graphics::Device.Get(),
						Graphics::Context.Get(),
						FixPath(L"../../Assets/Textures/RustedMetal2K/Metal053B_2K-PNG_Metalness.png").c_str(),
						0,
						rustedMetalMetalnessSrv.GetAddressOf());
					Graphics::Context->PSSetShaderResources(7, 1, rustedMetalMetalnessSrv.GetAddressOf());
		}

		// Metal panel texture
		{
			DirectX::CreateWICTextureFromFile(
				Graphics::Device.Get(),
				Graphics::Context.Get(),
				FixPath(L"../../Assets/Textures/MetalPanels/rusted-panels_albedo.png").c_str(),
				0,
				metalPanelAlbedoSrv.GetAddressOf());
			Graphics::Context->PSSetShaderResources(8, 1, metalPanelAlbedoSrv.GetAddressOf());

			DirectX::CreateWICTextureFromFile(
				Graphics::Device.Get(),
				Graphics::Context.Get(),
				FixPath(L"../../Assets/Textures/MetalPanels/rusted-panels_normal-ogl.png").c_str(),
				0,
				metalPanelNormalsSrv.GetAddressOf());
			Graphics::Context->PSSetShaderResources(9, 1, metalPanelNormalsSrv.GetAddressOf());

			DirectX::CreateWICTextureFromFile(
				Graphics::Device.Get(),
				Graphics::Context.Get(),
				FixPath(L"../../Assets/Textures/MetalPanels/rusted-panels_roughness.png").c_str(),
				0,
				metalPanelRoughnessSrv.GetAddressOf());
			Graphics::Context->PSSetShaderResources(10, 1, metalPanelRoughnessSrv.GetAddressOf());

			DirectX::CreateWICTextureFromFile(
				Graphics::Device.Get(),
				Graphics::Context.Get(),
				FixPath(L"../../Assets/Textures/MetalPanels/rusted-panels_metallic.png").c_str(),
				0,
				metalPanelMetalnessSrv.GetAddressOf());
			Graphics::Context->PSSetShaderResources(11, 1, metalPanelMetalnessSrv.GetAddressOf());
		}

		// Wood texture
		{
			DirectX::CreateWICTextureFromFile(
				Graphics::Device.Get(),
				Graphics::Context.Get(),
				FixPath(L"../../Assets/Textures/Wood/wood_albedo.png").c_str(),
				0,
				woodAlbedoSrv.GetAddressOf());
			Graphics::Context->PSSetShaderResources(12, 1, woodAlbedoSrv.GetAddressOf());

			DirectX::CreateWICTextureFromFile(
				Graphics::Device.Get(),
				Graphics::Context.Get(),
				FixPath(L"../../Assets/Textures/Wood/wood_normals.png").c_str(),
				0,
				woodNormalsSrv.GetAddressOf());
			Graphics::Context->PSSetShaderResources(13, 1, woodNormalsSrv.GetAddressOf());

			DirectX::CreateWICTextureFromFile(
				Graphics::Device.Get(),
				Graphics::Context.Get(),
				FixPath(L"../../Assets/Textures/Wood/wood_roughness.png").c_str(),
				0,
				woodRoughnessSrv.GetAddressOf());
			Graphics::Context->PSSetShaderResources(14, 1, woodRoughnessSrv.GetAddressOf());

			DirectX::CreateWICTextureFromFile(
				Graphics::Device.Get(),
				Graphics::Context.Get(),
				FixPath(L"../../Assets/Textures/Wood/wood_metal.png").c_str(),
				0,
				woodMetalnessSrv.GetAddressOf());
			Graphics::Context->PSSetShaderResources(15, 1, woodMetalnessSrv.GetAddressOf());
		}

		// Texture Sampler State
		// Describe texture sampler
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
		samplerDesc.MaxAnisotropy = 8;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		// Bind to pipeline
		Graphics::Device->CreateSamplerState(&samplerDesc, texSampler.GetAddressOf());
		Graphics::Context->PSSetSamplers(0, 1, texSampler.GetAddressOf());
	}

	// Create materials
	{}
	{
		pavingStonesMat = std::make_shared<Material>(
			XMFLOAT2(0.8f, 0.8f),
			XMFLOAT2(0.0f, 0.0f),
			XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
			vertexShader,
			basicPixelShader
		);
		pavingStonesMat->AddTextureSRV(0, pavingStonesSrv.Get());
		pavingStonesMat->AddTextureSRV(1, pavingStonesNormalsSrv.Get());
		//basicPsMaterial->AddTextureSRV(1, flatNormalsSrv.Get());
		pavingStonesMat->AddSampler(0, texSampler.Get());
		materialVec.push_back(pavingStonesMat);

		rustedMetalMat = std::make_shared<Material>(
			XMFLOAT2(0.8f, 0.8f),
			XMFLOAT2(0.0f, 0.0f),
			XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
			vertexShader,
			basicPixelShader
		);
		rustedMetalMat->AddTextureSRV(0, rustedMetalAlbedoSrv.Get());
		rustedMetalMat->AddTextureSRV(1, rustedMetalNormalsSrv.Get());
		rustedMetalMat->AddTextureSRV(2, rustedMetalRoughnessSrv.Get());
		rustedMetalMat->AddTextureSRV(3, rustedMetalMetalnessSrv.Get());
		rustedMetalMat->AddSampler(0, texSampler.Get());
		materialVec.push_back(rustedMetalMat);

		metalPanelMat = std::make_shared<Material>(
			XMFLOAT2(2.0f, 2.0f),
			XMFLOAT2(0.0f, 0.0f),
			XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
			vertexShader,
			basicPixelShader
		);
		metalPanelMat->AddTextureSRV(0, metalPanelAlbedoSrv.Get());
		metalPanelMat->AddTextureSRV(1, metalPanelNormalsSrv.Get());
		metalPanelMat->AddTextureSRV(2, metalPanelRoughnessSrv.Get());
		metalPanelMat->AddTextureSRV(3, metalPanelMetalnessSrv.Get());
		metalPanelMat->AddSampler(0, texSampler.Get());
		materialVec.push_back(metalPanelMat);

		woodMat = std::make_shared<Material>(
			XMFLOAT2(2.0f, 2.0f),
			XMFLOAT2(0.0f, 0.0f),
			XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
			vertexShader,
			basicPixelShader
		);
		woodMat->AddTextureSRV(0, woodAlbedoSrv.Get());
		woodMat->AddTextureSRV(1, woodNormalsSrv.Get());
		woodMat->AddTextureSRV(2, woodRoughnessSrv.Get());
		woodMat->AddTextureSRV(3, woodMetalnessSrv.Get());
		woodMat->AddSampler(0, texSampler.Get());
		materialVec.push_back(woodMat);

		tintPsMaterial = std::make_shared<Material>(
			XMFLOAT2(1.0f, 3.0f),
			XMFLOAT2(0.0f, 0.0f),
			XMFLOAT4(0.7f, 1.0f, 0.7f, 1.0f),
			vertexShader,
			basicPixelShader
		);
		tintPsMaterial->AddTextureSRV(0, pavingStonesSrv.Get());
		tintPsMaterial->AddSampler(0, texSampler.Get());
		materialVec.push_back(tintPsMaterial);

		uvPsMaterial = std::make_shared<Material>(
			XMFLOAT2(1.0f, 1.0f),
			XMFLOAT2(0.0f, 0.0f),
			XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f),
			vertexShader,
			debugUVsPixelShader);
		uvPsMaterial->AddSampler(0, texSampler.Get());
		materialVec.push_back(uvPsMaterial);

		normalsPsMaterial = std::make_shared<Material>(
			XMFLOAT2(1.0f, 1.0f),
			XMFLOAT2(0.0f, 0.0f),
			XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f),
			vertexShader,
			debugNormalsPixelShader);
		normalsPsMaterial->AddSampler(0, texSampler.Get());
		materialVec.push_back(normalsPsMaterial);

		customPsMaterial = std::make_shared<Material>(
			XMFLOAT2(1.0f, 1.0f),
			XMFLOAT2(0.0f, 0.0f),
			XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f),
			vertexShader,
			customPixelShader);
		customPsMaterial->AddSampler(0, texSampler.Get());
		materialVec.push_back(customPsMaterial);

		combinedPsMaterial = std::make_shared<Material>(
			XMFLOAT2(1.0f, 1.0f),
			XMFLOAT2(0.0f, 0.0f),
			XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
			vertexShader,
			combinedPixelShader);
		combinedPsMaterial->AddTextureSRV(0, pavingStonesSrv.Get());
		combinedPsMaterial->AddTextureSRV(1, graffitiSrv.Get());
		combinedPsMaterial->AddSampler(0, texSampler.Get());
		materialVec.push_back(combinedPsMaterial);
	}

	// Create meshes
	{}
	{
		cubeMesh = std::make_shared<Mesh>(FixPath("../../Assets/Meshes/cube.obj").c_str());
		meshVec.push_back(cubeMesh);
		cylinderMesh = std::make_shared<Mesh>(FixPath("../../Assets/Meshes/cylinder.obj").c_str());
		meshVec.push_back(cylinderMesh);
		helixMesh = std::make_shared<Mesh>(FixPath("../../Assets/Meshes/helix.obj").c_str());
		meshVec.push_back(helixMesh);
		sphereMesh = std::make_shared<Mesh>(FixPath("../../Assets/Meshes/sphere.obj").c_str());
		meshVec.push_back(sphereMesh);
		torusMesh = std::make_shared<Mesh>(FixPath("../../Assets/Meshes/torus.obj").c_str());
		meshVec.push_back(torusMesh);
		quadMesh = std::make_shared<Mesh>(FixPath("../../Assets/Meshes/quad.obj").c_str());
		meshVec.push_back(quadMesh);
		quadDoubleMesh = std::make_shared<Mesh>(FixPath("../../Assets/Meshes/quad_double_sided.obj").c_str());
		meshVec.push_back(quadDoubleMesh);
	}

	// Create entities
	{
		entityVec.push_back(std::make_shared<Entity>(cubeMesh, metalPanelMat));
		entityVec.push_back(std::make_shared<Entity>(cylinderMesh, woodMat));
		entityVec.push_back(std::make_shared<Entity>(helixMesh, metalPanelMat));
		entityVec.push_back(std::make_shared<Entity>(sphereMesh, rustedMetalMat));
		entityVec.push_back(std::make_shared<Entity>(torusMesh, woodMat));

		// Offset entities
		for (UINT i = 0; i < entityVec.size(); ++i)
		{
			entityVec[i]->GetTransform().SetPosition(XMFLOAT3(i * 2.5f, 0.0f, 0.0f));
		}

		// Create floor plane
		entityVec.push_back(std::make_shared<Entity>(quadDoubleMesh, woodMat));
		entityVec[entityVec.size() - 1]->GetTransform().SetPosition(XMFLOAT3(5.0f, -2.0f, 0.0f));
		entityVec[entityVec.size() - 1]->GetTransform().SetScale(XMFLOAT3(10.0f, 1.0f, 10.0f));
	}

	// Create skybox
	{
		skybox = std::make_shared<Skybox>(cubeMesh, texSampler.Get(), skyVertexShader.Get(), skyPixelShader.Get(),
			L"../../Assets/Textures/Skies/ColdSunset/right.png",
			L"../../Assets/Textures/Skies/ColdSunset/left.png",
			L"../../Assets/Textures/Skies/ColdSunset/up.png",
			L"../../Assets/Textures/Skies/ColdSunset/down.png",
			L"../../Assets/Textures/Skies/ColdSunset/front.png",
			L"../../Assets/Textures/Skies/ColdSunset/back.png");
	}

	// Shadows
	{
		// Set Resolution
		shadowMapRes = 1024;

		// Create GPU resource
		D3D11_TEXTURE2D_DESC shadowDesc = {};
		shadowDesc.Width = shadowMapRes;
		shadowDesc.Height = shadowMapRes;
		shadowDesc.ArraySize = 1;
		shadowDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		shadowDesc.CPUAccessFlags = 0;
		shadowDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		shadowDesc.MipLevels = 1;
		shadowDesc.MiscFlags = 0;
		shadowDesc.SampleDesc.Count = 1;
		shadowDesc.SampleDesc.Quality = 0;
		shadowDesc.Usage = D3D11_USAGE_DEFAULT;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> shadowTexture;
		Graphics::Device->CreateTexture2D(&shadowDesc, 0, shadowTexture.GetAddressOf());

		// Create depth/stencil view
		D3D11_DEPTH_STENCIL_VIEW_DESC shadowDSDesc = {};
		shadowDSDesc.Format = DXGI_FORMAT_D32_FLOAT;
		shadowDSDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		shadowDSDesc.Texture2D.MipSlice = 0;
		Graphics::Device->CreateDepthStencilView(
			shadowTexture.Get(),
			&shadowDSDesc,
			shadowDSV.GetAddressOf());

		// Create shadow map SRV
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		Graphics::Device->CreateShaderResourceView(
			shadowTexture.Get(),
			&srvDesc,
			shadowSRV.GetAddressOf());

		// Shadow rasterizer
		D3D11_RASTERIZER_DESC shadowRastDesc = {};
		shadowRastDesc.FillMode = D3D11_FILL_SOLID;
		shadowRastDesc.CullMode = D3D11_CULL_BACK;
		shadowRastDesc.DepthClipEnable = false;
		shadowRastDesc.DepthBias = 1000;
		shadowRastDesc.SlopeScaledDepthBias = 1.0f;
		Graphics::Device->CreateRasterizerState(&shadowRastDesc, &shadowRasterizer);

		// Shadow sampler
		D3D11_SAMPLER_DESC shadowSamplerDesc = {};
		shadowSamplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
		shadowSamplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
		shadowSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
		shadowSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
		shadowSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
		shadowSamplerDesc.BorderColor[0] = 1.0f;
		Graphics::Device->CreateSamplerState(&shadowSamplerDesc, &shadowSampler);

		// Light view matrix
		shadowCastingLightIndex = 0;
		CreateShadowViewMat();

		// Light projection matrix
		lightProjectionSize = 15.0;
		CreateShadowProjMat();
	}
}


// --------------------------------------------------------
// Clean up memory or objects created by this class
// 
// Note: Using smart pointers means there probably won't
//       be much to manually clean up here!
// --------------------------------------------------------
Game::~Game()
{
	// ImGui clean up
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

Microsoft::WRL::ComPtr<ID3D11VertexShader> Game::LoadVertexShader(std::wstring shaderPath)
{
	// Temporary vertex shader pointer
	Microsoft::WRL::ComPtr<ID3D11VertexShader> tempVertexShader;

	// Read compiled shader code file into blob
	ID3DBlob* vertexShaderBlob;
	D3DReadFileToBlob(FixPath(shaderPath).c_str(), &vertexShaderBlob);

	// Create the actual Direct3D shader on the GPU
	Graphics::Device->CreateVertexShader(
		vertexShaderBlob->GetBufferPointer(),	// Get a pointer to the blob's contents
		vertexShaderBlob->GetBufferSize(),		// How big is that data?
		0,										// No classes in this shader
		tempVertexShader.GetAddressOf());			// The address of the ID3D11VertexShader pointer

	// Create input layout
	inputLayout = LoadInputLayout(vertexShaderBlob);

	return tempVertexShader;
}

Microsoft::WRL::ComPtr<ID3D11PixelShader> Game::LoadPixelShader(std::wstring shaderPath)
{
	// Temporary pixel shader pointer
	Microsoft::WRL::ComPtr<ID3D11PixelShader> tempPixelShader;

	// Read compiled shader code file into blob
	ID3DBlob* pixelShaderBlob;
	D3DReadFileToBlob(FixPath(shaderPath).c_str(), &pixelShaderBlob);

	// Create the actual Direct3D shader on the GPU
	Graphics::Device->CreatePixelShader(
		pixelShaderBlob->GetBufferPointer(),	// Pointer to blob's contents
		pixelShaderBlob->GetBufferSize(),		// How big is that data?
		0,										// No classes in this shader
		tempPixelShader.GetAddressOf());			// Address of the ID3D11PixelShader pointer

	return tempPixelShader;
}

Microsoft::WRL::ComPtr<ID3D11InputLayout> Game::LoadInputLayout(ID3DBlob* vertexShaderBlob)
{
	// Temporary input layout pointer
	Microsoft::WRL::ComPtr<ID3D11InputLayout> tempInputLayout;

	D3D11_INPUT_ELEMENT_DESC inputElements[4] = {};

	// Set up the first element - a position, which is 3 float values
	inputElements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;				// Most formats are described as color channels; really it just means "Three 32-bit floats"
	inputElements[0].SemanticName = "POSITION";							// This is "POSITION" - needs to match the semantics in our vertex shader input!
	inputElements[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;	// How far into the vertex is this?  Assume it's after the previous element

	// Set up the second element - a UV, a 2d coordinate
	inputElements[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElements[1].SemanticName = "TEXCOORD";
	inputElements[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;

	// Set up the third element - a normal, 3 float values
	inputElements[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElements[2].SemanticName = "NORMAL";
	inputElements[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;

	// Set up tangent element - 3 floats
	inputElements[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElements[3].SemanticName = "TANGENT";
	inputElements[3].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;

	// Create the input layout, verifying our description against actual shader code
	Graphics::Device->CreateInputLayout(
		inputElements,							// An array of descriptions
		4,										// How many elements in that array?
		vertexShaderBlob->GetBufferPointer(),	// Pointer to the code of a shader that uses this layout
		vertexShaderBlob->GetBufferSize(),		// Size of the shader code that uses this layout
		tempInputLayout.GetAddressOf());		// Address of the resulting ID3D11InputLayout pointer

	return tempInputLayout;
}

// --------------------------------------------------------
// Handle resizing to match the new window size
//  - Eventually, we'll want to update our 3D camera
// --------------------------------------------------------
void Game::OnResize()
{
	for (unsigned int i = 0; i < cameraVec.size(); ++i)
	{
		cameraVec[i]->UpdateProjectionMatrix(Window::AspectRatio());
	}
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	// Example input checking: Quit if the escape key is pressed
	if (Input::KeyDown(VK_ESCAPE))
		Window::Quit();

	// Update ImGui
	ImGuiNewFrameUpdate(deltaTime);
	ImGuiBuildUI();

	// Update Camera
	cameraVec[activeCameraIndex]->Update(deltaTime);

	// Rotate entities
	for (unsigned int i = 0; i < entityVec.size() - 1; ++i)
	{
		entityVec[i]->GetTransform().Rotate(0.0f, 0.3f * deltaTime, 0.0f);
	}
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// Frame start
	// - These things should happen ONCE PER FRAME
	// - At the beginning of Game::Draw() before drawing *anything*
	{
		// Clear the back buffer (erase what's on screen) and depth buffer
		Graphics::Context->ClearRenderTargetView(Graphics::BackBufferRTV.Get(), bgColor);
		Graphics::Context->ClearDepthStencilView(Graphics::DepthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	}

	// Shadow map render
	{
		// Clear shadow map
		Graphics::Context->ClearDepthStencilView(shadowDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

		// Enable rasterizer state
		Graphics::Context->RSSetState(shadowRasterizer.Get());

		// Set up output merger
		ID3D11RenderTargetView* nullRTV{};
		Graphics::Context->OMSetRenderTargets(1, &nullRTV, shadowDSV.Get());

		// Deactivate pixel shader
		Graphics::Context->PSSetShader(0, 0, 0);

		// Change viewport
		D3D11_VIEWPORT viewport = {};
		viewport.Width = (float)shadowMapRes;
		viewport.Height = (float)shadowMapRes;
		viewport.MaxDepth = 1.0f;
		Graphics::Context->RSSetViewports(1, &viewport);

		// Render entities
		Graphics::Context->VSSetShader(shadowVertexShader.Get(), 0, 0);

		// Vertex shader data
		struct ShadowVSData
		{
			DirectX::XMFLOAT4X4 world;
			DirectX::XMFLOAT4X4 view;
			DirectX::XMFLOAT4X4 proj;
		};

		ShadowVSData shadowVsData = {};
		shadowVsData.view = lightViewMatrix;
		shadowVsData.proj = lightProjectionMatrix;

		// Loop and draw all entities
		for (auto& entity : entityVec)
		{
			shadowVsData.world = entity->GetTransform().GetWorldMatrix();
			FillAndBindNextConstantBuffer(
				&shadowVsData,
				sizeof(ShadowVSData),
				D3D11_VERTEX_SHADER,
				0);

			entity->DrawNoMaterial(deltaTime);
		}

		// Reset pipeline
		viewport.Width = (float)Window::Width();
		viewport.Height = (float)Window::Height();
		Graphics::Context->RSSetViewports(1, &viewport);
		Graphics::Context->OMSetRenderTargets(
			1,
			Graphics::BackBufferRTV.GetAddressOf(),
			Graphics::DepthBufferDSV.Get());

		// Reset rasterizer state
		Graphics::Context->RSSetState(0);
	}

	// Draw geometry
	for(auto& entity : entityVec)
	{
		// Set vertex shader data
		VertexShaderData vsData{};
		vsData.WorldMatrix = entity->GetTransform().GetWorldMatrix();
		vsData.WorldInverseTransposeMatrix = entity->GetTransform().GetWorldInverseTransposeMatrix();
		vsData.ViewMatrix = cameraVec[activeCameraIndex]->GetViewMatrix();
		vsData.ProjectionMatrix = cameraVec[activeCameraIndex]->GetProjectionMatrix();
		vsData.LightViewMatrix = lightViewMatrix;
		vsData.LightProjectionMatrix = lightProjectionMatrix;

		FillAndBindNextConstantBuffer(
			&vsData,
			sizeof(VertexShaderData),
			D3D11_VERTEX_SHADER,
			0);

		// Set pixel shader data
		PixelShaderData psData{};
		psData.UVScale = entity->GetMaterial()->GetUVScale();
		psData.UVOffset = entity->GetMaterial()->GetUVOffset();
		psData.ColorTint = entity->GetMaterial()->GetColorTint();
		psData.CameraPosition = cameraVec[activeCameraIndex]->GetPosition();
		psData.TotalTime = totalTime;
		for (unsigned int i = 0; i < 5; ++i)
		{
			psData.Lights[i] = lights[i];
		}

		FillAndBindNextConstantBuffer(
			&psData,
			sizeof(PixelShaderData),
			D3D11_PIXEL_SHADER,
			0);

		// Bind textures and samplers to pipeline
		entity->GetMaterial()->BindTexturesAndSamplers();

		// Bind shadow map to pipeline
		Graphics::Context->PSSetShaderResources(entity->GetMaterial()->GetTextureCount(), 1, shadowSRV.GetAddressOf());
		Graphics::Context->PSSetSamplers(1, 1, shadowSampler.GetAddressOf());

		// Draw entity
		entity->Draw(deltaTime);

		// Unbind shadow map from shader resources (can't be depth map and texture simultaneously)
		ID3D11ShaderResourceView* nullSRVs[64] = {};
		Graphics::Context->PSSetShaderResources(entity->GetMaterial()->GetTextureCount(), 64, nullSRVs);
	}

	// Draw skybox
	{
		// Set skybox data and bind cbuffer
		SkyVertexShaderData vsData = {};
		vsData.ViewMatrix = cameraVec[activeCameraIndex]->GetViewMatrix();
		vsData.ProjectionMatrix = cameraVec[activeCameraIndex]->GetProjectionMatrix();

		FillAndBindNextConstantBuffer(
			&vsData, 
			sizeof(SkyVertexShaderData), 
			D3D11_VERTEX_SHADER, 
			0);

		// Draw
		skybox->Draw(totalTime);
	}

	// Draw ImGui
	{
		ImGui::Render();	// Turns the frame's UI into tris to be rendered
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());	// Draw to the screen
	}

	// Frame end
	// - These should happen exactly ONCE PER FRAME
	// - At the very end of the frame (after drawing *everything*)
	{
		// Present at the end of the frame
		bool vsync = Graphics::VsyncState();
		Graphics::SwapChain->Present(
			vsync ? 1 : 0,
			vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING);

		// Re-bind back buffer and depth buffer after presenting
		Graphics::Context->OMSetRenderTargets(
			1,
			Graphics::BackBufferRTV.GetAddressOf(),
			Graphics::DepthBufferDSV.Get());
	}
}

void Game::FillAndBindNextConstantBuffer(
	void* data,
	unsigned int dataSizeInBytes,
	D3D11_SHADER_TYPE shaderType,
	unsigned int registerSlot)
{
	// Reserve space adhering to the 256 byte chunk requirement
	unsigned int reservationSize = (dataSizeInBytes + 255) / 256 * 256;

	// Loop to beginning of buffer in necessary
	if (cbHeapOffsetInBytes + reservationSize >= cbHeapSizeInBytes)
	{
		cbHeapOffsetInBytes = 0;
	}

	// Map the buffer
	D3D11_MAPPED_SUBRESOURCE map{};
	Graphics::Context->Map(
		ConstantBufferHeap.Get(),
		0,
		D3D11_MAP_WRITE_NO_OVERWRITE,
		0,
		&map);

	// Memcpy to write into the buffer
	void* uploadAddress = reinterpret_cast<void*>((UINT64)map.pData + cbHeapOffsetInBytes);
	memcpy(uploadAddress, data, dataSizeInBytes);

	// Unmap
	Graphics::Context->Unmap(ConstantBufferHeap.Get(), 0);

	// Calculate binding offsets as 16 byte constants
	unsigned int firstConstant = cbHeapOffsetInBytes / 16;
	unsigned int numConstants = reservationSize / 16;

	// Bind to proper pipeline stage
	switch (shaderType)
	{
	case D3D11_VERTEX_SHADER:
		Context1->VSSetConstantBuffers1(
			registerSlot,
			1,
			ConstantBufferHeap.GetAddressOf(),
			&firstConstant,
			&numConstants);
		break;
	case D3D11_PIXEL_SHADER:
		Context1->PSSetConstantBuffers1(
			registerSlot,
			1,
			ConstantBufferHeap.GetAddressOf(),
			&firstConstant,
			&numConstants);
		break;
	}

	// Offset for the next call
	cbHeapOffsetInBytes += reservationSize;
}

void Game::CreateShadowViewMat()
{
	DirectX::XMVECTOR lightDirection = XMLoadFloat3(&lights[shadowCastingLightIndex].Direction);
	DirectX::XMMATRIX lightView = XMMatrixLookToLH(
		-lightDirection * 20,		// Position backing up 20 units from origin
		lightDirection,				// Light direction
		XMVectorSet(0, 1, 0, 0));	// Up world vector

	DirectX::XMStoreFloat4x4(&lightViewMatrix, lightView);
}

void Game::CreateShadowProjMat()
{
	DirectX::XMMATRIX lightProj = XMMatrixOrthographicLH(
		lightProjectionSize,
		lightProjectionSize,
		1.0f,
		100.0f);

	DirectX::XMStoreFloat4x4(&lightProjectionMatrix, lightProj);
}

// Pass ImGui new frame information at the start of update
void Game::ImGuiNewFrameUpdate(float deltaTime)
{
	// Give fresh data to ImGui
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = deltaTime;
	io.DisplaySize.x = (float)Window::Width();
	io.DisplaySize.y = (float)Window::Height();

	//Reset the frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// Determine new input capture
	Input::SetKeyboardCapture(io.WantCaptureKeyboard);
	Input::SetMouseCapture(io.WantCaptureMouse);

	// Show demo window
	//ImGui::ShowDemoWindow();
}

// Build the ImGui UI, called after new frame data is passed to ImGui
void Game::ImGuiBuildUI()
{
	// Begin a new window
	ImGui::Begin("Inspector");

	// App Info
	if (ImGui::TreeNode("App Details"))
	{
		// Show fps
		ImGui::Text("Framerate: %f fps", ImGui::GetIO().Framerate);

		// Show window size
		ImGui::Text("Window Size: %dx%dpx", Window::Width(), Window::Height());

		// BG color picker
		ImGui::ColorEdit4("BG Color", bgColor, ImGuiColorEditFlags_NoInputs);

		ImGui::TreePop();
	}

	// Camera info (not meant to be edited in the GUI)
	if (ImGui::TreeNode("Camera Controls"))
	{
		// Get amount of cameras
		size_t cameraCount = cameraVec.size();

		// Create cycle button
		if (ImGui::Button("Next Camera"))
		{
			if (activeCameraIndex >= cameraCount - 1) { activeCameraIndex = 0; }
			else { activeCameraIndex++; }
		}

		// Camera index label
		ImGui::SameLine();
		ImGui::Text(std::string("Active Camera: " + std::to_string(activeCameraIndex)).c_str());

		// Position
		XMFLOAT3 cameraPos = cameraVec[activeCameraIndex]->GetPosition();
		float positionData[3] = { cameraPos.x, cameraPos.y, cameraPos.z };
		ImGui::DragFloat3("Position", positionData, 0.0f);

		// FOV
		float cameraFov = cameraVec[activeCameraIndex]->GetFov();
		ImGui::DragFloat("FOV Angle", &cameraFov, 0.0f);

		ImGui::TreePop();
	}

	// Material controls
	if (ImGui::TreeNode("Material Controls"))
	{
		for (unsigned int i = 0; i < materialVec.size(); ++i)
		{
			if (ImGui::TreeNode(std::string("Material " + std::to_string(i + 1)).c_str()))
			{
				// UV scale
				XMFLOAT2 uvScale = materialVec[i]->GetUVScale();
				float scaleArray[2] = { uvScale.x, uvScale.y };
				ImGui::DragFloat2(std::string("UV Scale##" + std::to_string(i)).c_str(), scaleArray, 0.01f, 0.1f, 10.0f);
				materialVec[i]->SetUVScale(XMFLOAT2(scaleArray));

				// UV offset
				XMFLOAT2 uvOffset = materialVec[i]->GetUVOffset();
				float offsetArray[2] = { uvOffset.x, uvOffset.y };
				ImGui::DragFloat2(std::string("UV Offset##" + std::to_string(i)).c_str(), offsetArray, 0.01f, -5.0f, 5.0f);
				materialVec[i]->SetUVOffset(XMFLOAT2(offsetArray));

				// Color tint
				XMFLOAT4 colorTint = materialVec[i]->GetColorTint();
				float tintArray[4] = { colorTint.x, colorTint.y, colorTint.z, colorTint.w };
				ImGui::ColorEdit4(std::string("Color Tint##" + std::to_string(i)).c_str(), tintArray, ImGuiColorEditFlags_NoInputs);
				materialVec[i]->SetColorTint(XMFLOAT4(tintArray));

				// Display textures
				for (unsigned int j = 0; j < materialVec[i]->GetTextureCount(); ++j)
				{
					ImTextureID textId = (unsigned long long)materialVec[i]->GetTextureSRVs()[j].Get();
					ImGui::Image(textId, ImVec2(50, 50));
					ImGui::SameLine();
				}
				ImGui::NewLine();

				ImGui::TreePop();
			}
		}
		ImGui::TreePop();
	}

	// Mesh info
	if (ImGui::TreeNode("Mesh Info"))
	{
		for (unsigned int i = 0; i < meshVec.size(); ++i)
		{
			if (ImGui::TreeNode(std::string("Mesh " + std::to_string(i + 1)).c_str()))
			{
				ImGui::Text("Triangles:"); ImGui::SameLine(); ImGui::Text(std::to_string(meshVec[i]->GetIndexCount() / 3).c_str());
				ImGui::Text("Vertices:"); ImGui::SameLine(); ImGui::Text(std::to_string(meshVec[i]->GetVertexCount()).c_str());
				ImGui::Text("Indices:"); ImGui::SameLine(); ImGui::Text(std::to_string(meshVec[i]->GetIndexCount()).c_str());
				ImGui::TreePop();
			}
		}
		ImGui::TreePop();
	}

	// Entity controls
	if (ImGui::TreeNode("Entity Controls"))
	{
		for (unsigned int i = 0; i < entityVec.size(); ++i)
		{
			if (ImGui::TreeNode(std::string("Entity " + std::to_string(i + 1)).c_str()))
			{
				// Name
				ImGui::Text(("Name: " + entityVec[i]->GetMesh()->GetName()).c_str());

				// Position
				XMFLOAT3 position = entityVec[i]->GetTransform().GetPosition();
				float posArray[3] = { position.x, position.y, position.z };
				ImGui::DragFloat3(std::string("Position##" + std::to_string(i)).c_str(), posArray, 0.01f, -10.0f, 10.0f);
				entityVec[i]->GetTransform().SetPosition(posArray[0], posArray[1], posArray[2]);

				// Rotation
				XMFLOAT3 rotation = entityVec[i]->GetTransform().GetPitchYawRoll();
				float rotArray[3] = { rotation.x, rotation.y, rotation.z };
				ImGui::DragFloat3(std::string("Rotation##" + std::to_string(i)).c_str(), rotArray, 0.01f, -10.0f, 10.0f);
				entityVec[i]->GetTransform().SetRotation(rotArray[0], rotArray[1], rotArray[2]);

				// Scale
				XMFLOAT3 scale = entityVec[i]->GetTransform().GetScale();
				float scArray[3] = { scale.x, scale.y, scale.z };
				ImGui::DragFloat3(std::string("Scale##" + std::to_string(i)).c_str(), scArray, 0.01f, 0.0f, 10.0f);
				entityVec[i]->GetTransform().SetScale(scArray[0], scArray[1], scArray[2]);

				ImGui::TreePop();
			}
		}

		ImGui::TreePop();
	}

	// Light controls
	if (ImGui::TreeNode("Light Controls"))
	{
		// Light controls
		for (unsigned int i = 0; i < 5; ++i)
		{
			if (ImGui::TreeNode(std::string("Light " + std::to_string(i + 1)).c_str()))
			{
				// Type
				int type = lights[i].Type;
				ImGui::Text(std::string("Type: " + std::to_string(type)).c_str());

				// Color
				float colorTemp[3] = { lights[i].Color.x, lights[i].Color.y, lights[i].Color.z };
				ImGui::ColorEdit3(std::string("Color##" + std::to_string(i)).c_str(), colorTemp, ImGuiColorEditFlags_NoInputs);
				lights[i].Color = XMFLOAT3(colorTemp);

				// Intensity
				float intensityTemp = lights[i].Intensity;
				ImGui::DragFloat(std::string("Intensity##" + std::to_string(i)).c_str(), &intensityTemp, 0.1f, 0.0f, 20.0f);
				lights[i].Intensity = intensityTemp;

				ImGui::TreePop();
			}
		}

		ImGui::TreePop();
	}

	// Shadow map
	if (ImGui::TreeNode("Shadow Map"))
	{
		ImGui::Text("Change Resolution");

		ImGui::Image(shadowSRV.Get(), ImVec2(256, 256));

		ImGui::TreePop();
	}

	// End ImGui creation
	ImGui::End();
}

//// Translation
//XMMATRIX trMat = XMMatrixTranslation((float)sin(totalTime), 0, 0);
//
//// Scale
//float scale = (float)sin(totalTime * 3.7f) * 0.5f + 1.0f;
//XMMATRIX scMat = XMMatrixScaling(scale, scale, scale);
//
//// Rotation
//XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(0, 0, totalTime * 0.1f);
//
//// Build world matrix
//XMMATRIX worldMat = scMat * rotMat * trMat;	// order matters here
//
//// Store result
//XMStoreFloat4x4(&vsData.Matrix, worldMat);

//// DXMath demo
//// Create storage types
//XMFLOAT3 position(1, 2, 3);
//XMFLOAT3 offset(4, 5, 6);
//
//// Load into math types
//XMVECTOR posVec = XMLoadFloat3(&position);
//XMVECTOR offVec = XMLoadFloat3(&offset);
//
//// Do some math
//posVec = XMVectorAdd(posVec, offVec);
//posVec *= 5;
//
//// Store back in storage type
//XMStoreFloat3(&position, posVec);