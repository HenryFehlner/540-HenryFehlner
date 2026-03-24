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

		// Pixel shaders
		tintPixelShader = LoadPixelShader(L"PixelShader.cso");
		debugUVsPixelShader = LoadPixelShader(L"DebugUVsPS.cso");
		debugNormalsPixelShader = LoadPixelShader(L"DebugNormalsPS.cso");
		customPixelShader = LoadPixelShader(L"CustomPS.cso");
		combinedPixelShader = LoadPixelShader(L"CombinedPS.cso");
	}

	// Set initial graphics API state
	//  - These settings persist until we change them
	//  - Some of these, like the primitive topology & input layout, probably won't change
	//  - Others, like setting shaders, will need to be moved elsewhere later
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
		Graphics::Context->PSSetShader(tintPixelShader.Get(), 0, 0);
	}

	// Load textures
	{
		// Shader Resource Views
		// Loads image file and creates texture and srv
		DirectX::CreateWICTextureFromFile(
			Graphics::Device.Get(),
			Graphics::Context.Get(),
			FixPath(L"../../Assets/Textures/PavingStones2K/PavingStones138_2K-PNG_Color.png").c_str(),
			0,
			pavingStonesSrv.GetAddressOf());
		Graphics::Context->PSSetShaderResources(0, 1, pavingStonesSrv.GetAddressOf());

		// Graffiti texture
		DirectX::CreateWICTextureFromFile(
			Graphics::Device.Get(),
			Graphics::Context.Get(),
			FixPath(L"../../Assets/Textures/qqGraffiti.png").c_str(),
			0,
			graffitiSrv.GetAddressOf());
		Graphics::Context->PSSetShaderResources(1, 1, graffitiSrv.GetAddressOf());

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
	{
		tintPsMaterial = std::make_shared<Material>(
			XMFLOAT2(1.0f, 3.0f),
			XMFLOAT2(0.0f, 0.0f),
			XMFLOAT4(0.7f, 1.0f, 0.7f, 1.0f), 
			vertexShader, 
			tintPixelShader
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
		// Tint entities & custom shader entity
		entityVec.push_back(std::make_shared<Entity>(cubeMesh, combinedPsMaterial));
		entityVec.push_back(std::make_shared<Entity>(cylinderMesh, tintPsMaterial));
		entityVec.push_back(std::make_shared<Entity>(helixMesh, tintPsMaterial));
		entityVec.push_back(std::make_shared<Entity>(sphereMesh, customPsMaterial));
		entityVec.push_back(std::make_shared<Entity>(torusMesh, tintPsMaterial));
		entityVec.push_back(std::make_shared<Entity>(quadMesh, tintPsMaterial));
		entityVec.push_back(std::make_shared<Entity>(quadDoubleMesh, tintPsMaterial));

		// UV entities
		entityVec.push_back(std::make_shared<Entity>(cubeMesh, uvPsMaterial));
		entityVec.push_back(std::make_shared<Entity>(cylinderMesh, uvPsMaterial));
		entityVec.push_back(std::make_shared<Entity>(helixMesh, uvPsMaterial));
		entityVec.push_back(std::make_shared<Entity>(sphereMesh, uvPsMaterial));
		entityVec.push_back(std::make_shared<Entity>(torusMesh, uvPsMaterial));
		entityVec.push_back(std::make_shared<Entity>(quadMesh, uvPsMaterial));
		entityVec.push_back(std::make_shared<Entity>(quadDoubleMesh, uvPsMaterial));

		// Normals entities
		entityVec.push_back(std::make_shared<Entity>(cubeMesh, normalsPsMaterial));
		entityVec.push_back(std::make_shared<Entity>(cylinderMesh, normalsPsMaterial));
		entityVec.push_back(std::make_shared<Entity>(helixMesh, normalsPsMaterial));
		entityVec.push_back(std::make_shared<Entity>(sphereMesh, normalsPsMaterial));
		entityVec.push_back(std::make_shared<Entity>(torusMesh, normalsPsMaterial));
		entityVec.push_back(std::make_shared<Entity>(quadMesh, normalsPsMaterial));
		entityVec.push_back(std::make_shared<Entity>(quadDoubleMesh, normalsPsMaterial));


		// Offset entities
		for (UINT i = 0; i < entityVec.size(); ++i)
		{
			float yPos = 0.0f;

			if (i >= 7)
				yPos = 2.5;
			if (i >= 14)
				yPos = 2.5 * 2.0;

			entityVec[i]->GetTransform().SetPosition(XMFLOAT3((i % 7) * 2.5f, yPos, 0.0f));
		}
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

	D3D11_INPUT_ELEMENT_DESC inputElements[3] = {};

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

	// Create the input layout, verifying our description against actual shader code
	Graphics::Device->CreateInputLayout(
		inputElements,							// An array of descriptions
		3,										// How many elements in that array?
		vertexShaderBlob->GetBufferPointer(),	// Pointer to the code of a shader that uses this layout
		vertexShaderBlob->GetBufferSize(),		// Size of the shader code that uses this layout
		tempInputLayout.GetAddressOf());			// Address of the resulting ID3D11InputLayout pointer

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
		Graphics::Context->ClearRenderTargetView(Graphics::BackBufferRTV.Get(),	bgColor);
		Graphics::Context->ClearDepthStencilView(Graphics::DepthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	}
	
	// Draw geometry
	//for (unsigned int i = 0; i < meshVec.size(); ++i)
	for (unsigned int i = 0; i < entityVec.size(); ++i)
	{
		// Set vertex shader data
		VertexShaderData vsData{};
		vsData.WorldMatrix = entityVec[i]->GetTransform().GetWorldMatrix();
		vsData.ViewMatrix = cameraVec[activeCameraIndex]->GetViewMatrix();
		vsData.ProjectionMatrix = cameraVec[activeCameraIndex]->GetProjectionMatrix();

		FillAndBindNextConstantBuffer(
			&vsData,
			sizeof(VertexShaderData),
			D3D11_VERTEX_SHADER,
			0);

		// Set pixel shader data
		PixelShaderData psData{};
		psData.UVScale = entityVec[i]->GetMaterial()->GetUVScale();
		psData.UVOffset = entityVec[i]->GetMaterial()->GetUVOffset();
		psData.ColorTint = entityVec[i]->GetMaterial()->GetColorTint();
		psData.TotalTime = totalTime;

		FillAndBindNextConstantBuffer(
			&psData,
			sizeof(PixelShaderData),
			D3D11_PIXEL_SHADER,
			0);

		// Bind textures and samplers to pipeline
		entityVec[i]->GetMaterial()->BindTexturesAndSamplers();

		// Draw entity
		entityVec[i]->Draw(deltaTime, totalTime);
	}

	// Draw ImGui
	ImGui::Render();	// Turns the frame's UI into tris to be rendered
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());	// Draw to the screen

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
		ImGui::ColorEdit4("BG Color", bgColor);

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
				float scaleArray[2] = {uvScale.x, uvScale.y};
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
				ImGui::ColorEdit4(std::string("Color Tint##" + std::to_string(i)).c_str(), tintArray);
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

	// End ImGui creation
	ImGui::End();
}

/*
// --------------------------------------------------------
// Loads shaders from compiled shader object (.cso) files
// and also created the Input Layout that describes our
// vertex data to the rendering pipeline.
// - Input Layout creation is done here because it must
//    be verified against vertex shader byte code
// - We'll have that byte code already loaded below
// --------------------------------------------------------
void Game::LoadShaders()
{
	// BLOBs (or Binary Large OBjects) for reading raw data from external files
	// - This is a simplified way of handling big chunks of external data
	// - Literally just a big array of bytes read from a file
	ID3DBlob* pixelShaderBlob;
	ID3DBlob* vertexShaderBlob;

	// Loading shaders
	//  - Visual Studio will compile our shaders at build time
	//  - They are saved as .cso (Compiled Shader Object) files
	//  - We need to load them when the application starts
	{
		// Read our compiled shader code files into blobs
		// - Essentially just "open the file and plop its contents here"
		// - Uses the custom FixPath() helper from Helpers.h to ensure relative paths
		// - Note the "L" before the string - this tells the compiler the string uses wide characters
		D3DReadFileToBlob(FixPath(L"PixelShader.cso").c_str(), &pixelShaderBlob);
		D3DReadFileToBlob(FixPath(L"VertexShader.cso").c_str(), &vertexShaderBlob);

		// Create the actual Direct3D shaders on the GPU
		Graphics::Device->CreatePixelShader(
			pixelShaderBlob->GetBufferPointer(),	// Pointer to blob's contents
			pixelShaderBlob->GetBufferSize(),		// How big is that data?
			0,										// No classes in this shader
			pixelShader.GetAddressOf());			// Address of the ID3D11PixelShader pointer

		Graphics::Device->CreateVertexShader(
			vertexShaderBlob->GetBufferPointer(),	// Get a pointer to the blob's contents
			vertexShaderBlob->GetBufferSize(),		// How big is that data?
			0,										// No classes in this shader
			vertexShader.GetAddressOf());			// The address of the ID3D11VertexShader pointer
	}

	// Create an input layout
	//  - This describes the layout of data sent to a vertex shader
	//  - In other words, it describes how to interpret data (numbers) in a vertex buffer
	//  - Doing this NOW because it requires a vertex shader's byte code to verify against!
	//  - Luckily, we already have that loaded (the vertex shader blob above)
	{
		D3D11_INPUT_ELEMENT_DESC inputElements[2] = {};

		// Set up the first element - a position, which is 3 float values
		inputElements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;				// Most formats are described as color channels; really it just means "Three 32-bit floats"
		inputElements[0].SemanticName = "POSITION";							// This is "POSITION" - needs to match the semantics in our vertex shader input!
		inputElements[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;	// How far into the vertex is this?  Assume it's after the previous element

		// Set up the second element - a color, which is 4 more float values
		inputElements[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;			// 4x 32-bit floats
		inputElements[1].SemanticName = "COLOR";							// Match our vertex shader input!
		inputElements[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;	// After the previous element

		// Create the input layout, verifying our description against actual shader code
		Graphics::Device->CreateInputLayout(
			inputElements,							// An array of descriptions
			2,										// How many elements in that array?
			vertexShaderBlob->GetBufferPointer(),	// Pointer to the code of a shader that uses this layout
			vertexShaderBlob->GetBufferSize(),		// Size of the shader code that uses this layout
			inputLayout.GetAddressOf());			// Address of the resulting ID3D11InputLayout pointer
	}
}
*/

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