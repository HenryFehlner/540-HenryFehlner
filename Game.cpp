#include "Game.h"
#include "Graphics.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"
#include "Window.h"
#include "BufferStructs.h"

#include <DirectXMath.h>

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
static float vsColorTint[4] = { 1.0f, 0.5f, 0.5f, 1.0f };
static float vsOffset[3] = { 0.25f, 0.0f, 0.0f };

// --------------------------------------------------------
// The constructor is called after the window and graphics API
// are initialized but before the game loop begins
// --------------------------------------------------------
Game::Game()
{
	// Helper methods for loading shaders, creating some basic
	// geometry to draw and some simple camera matrices.
	//  - You'll be expanding and/or replacing these later
	LoadShaders();

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
		Graphics::Context->PSSetShader(pixelShader.Get(), 0, 0);
	}

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

	// Hardcoded meshes
	{
		// Create points & colors for meshes
		XMFLOAT4 red = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
		XMFLOAT4 green = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
		XMFLOAT4 blue = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
		XMFLOAT4 black = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		XMFLOAT4 gray = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);

		// Triangle mesh
		Vertex triangleVertices[] =
		{
			{ XMFLOAT3(+0.0f, +0.5f, +0.0f), red },
			{ XMFLOAT3(+0.5f, -0.5f, +0.0f), blue },
			{ XMFLOAT3(-0.5f, -0.5f, +0.0f), green }
		};
		unsigned int triangleIndices[] = { 0, 1, 2 };
		//triangleMesh = std::make_shared<Mesh>(triangleVertices, std::size(triangleVertices), triangleIndices, std::size(triangleIndices));
		meshVec.push_back(std::make_shared<Mesh>(triangleVertices, std::size(triangleVertices), triangleIndices, std::size(triangleIndices), "Triangle"));

		// Quad mesh
		Vertex quadVertices[] =
		{
			{ XMFLOAT3(-0.25f - 0.5f, +0.0f + 0.5f, +0.0f), blue },		// Adding 0.5 to offset it, ugly but it works
			{ XMFLOAT3(+0.0f - 0.5f, +0.25f + 0.5f, +0.0f), red },
			{ XMFLOAT3(+0.0f - 0.5f, +0.0f + 0.5f, +0.0f), red },
			{ XMFLOAT3(-0.25f - 0.5f, +0.25f + 0.5f, +0.0f), blue }
		};
		unsigned int quadIndices[] = { 0, 1, 2,   0, 3, 1 };
		//quadMesh = std::make_shared<Mesh>(quadVertices, std::size(quadVertices), quadIndices, std::size(quadIndices));
		meshVec.push_back(std::make_shared<Mesh>(quadVertices, std::size(quadVertices), quadIndices, std::size(quadIndices), "Quad"));

		// Weird mesh
		Vertex weirdVertices[] =
		{
			{ XMFLOAT3(-0.25f + 0.5f, +0.0f + 0.5f, +0.0f), gray },
			{ XMFLOAT3(+0.25f + 0.5f, +0.125f + 0.5f, +0.0f), black },
			{ XMFLOAT3(+0.0f + 0.5f, +0.0f + 0.5f, +0.0f), gray },
			{ XMFLOAT3(-0.125f + 0.5f, +0.25f + 0.5f, +0.0f), black },

			{ XMFLOAT3(-0.25f + 0.5f, +0.0f + 0.5f, +0.0f), gray },
			{ XMFLOAT3(+0.25f + 0.5f, -0.125f + 0.5f, +0.0f), black },
			{ XMFLOAT3(+0.0f + 0.5f, +0.0f + 0.5f, +0.0f), gray },
			{ XMFLOAT3(-0.125f + 0.5f, -0.25f + 0.5f, +0.0f), black }
		};
		unsigned int weirdIndices[] = { 0, 1, 2,   0, 3, 1,   4, 6, 5,   4, 5, 7 };
		//weirdMesh = std::make_shared<Mesh>(weirdVertices, std::size(weirdVertices), weirdIndices, std::size(weirdIndices));
		meshVec.push_back(std::make_shared<Mesh>(weirdVertices, std::size(weirdVertices), weirdIndices, std::size(weirdIndices), "The weird one"));
	}

	// Constant buffer
	{
		// Describe buffer
		D3D11_BUFFER_DESC cbDesc = {};
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		unsigned int size = sizeof(VertexShaderData);
		size = (size + 15) / 16 * 16;
		cbDesc.ByteWidth = size;
		cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		cbDesc.Usage = D3D11_USAGE_DYNAMIC;

		// Create and bind buffer
		Graphics::Device->CreateBuffer(&cbDesc, 0, constBuffer.GetAddressOf());
		Graphics::Context->VSSetConstantBuffers(0, 1, constBuffer.GetAddressOf());
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

// --------------------------------------------------------
// Handle resizing to match the new window size
//  - Eventually, we'll want to update our 3D camera
// --------------------------------------------------------
void Game::OnResize()
{
	
}


// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	// Update ImGui
	ImGuiNewFrameUpdate(deltaTime);
	ImGuiBuildUI();

	// Example input checking: Quit if the escape key is pressed
	if (Input::KeyDown(VK_ESCAPE))
		Window::Quit();
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// Frame START
	// - These things should happen ONCE PER FRAME
	// - At the beginning of Game::Draw() before drawing *anything*
	{
		// Clear the back buffer (erase what's on screen) and depth buffer
		Graphics::Context->ClearRenderTargetView(Graphics::BackBufferRTV.Get(),	bgColor);
		Graphics::Context->ClearDepthStencilView(Graphics::DepthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	}
	
	// DRAW geometry
	//triangleMesh->Draw(deltaTime, totalTime);
	//quadMesh->Draw(deltaTime, totalTime);
	//weirdMesh->Draw(deltaTime, totalTime);
	for (unsigned int i = 0; i < meshVec.size(); ++i)
	{
		// Create vertex shader data
		VertexShaderData vsData;
		vsData.ColorTint = XMFLOAT4(vsColorTint[0], vsColorTint[1], vsColorTint[2], 1.0f);
		vsData.Offset = XMFLOAT3(vsOffset[0], vsOffset[1], vsOffset[2]);

		// Copy to the resource (map (lock) -> copy -> unmap (unlock))
		D3D11_MAPPED_SUBRESOURCE mappedBuffer = {};
		Graphics::Context->Map(constBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedBuffer);	// map
		memcpy(mappedBuffer.pData, &vsData, sizeof(vsData));	// copy
		Graphics::Context->Unmap(constBuffer.Get(), 0);		// unmap

		// Draw mesh
		meshVec[i]->Draw(deltaTime, totalTime);
	}

	// Draw ImGui
	ImGui::Render();	// Turns the frame's UI into tris to be rendered
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());	// Draw to the screen

	// Frame END
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

	// Mesh info tree
	if (ImGui::TreeNode("Meshes"))
	{
		for (unsigned int i = 0; i < meshVec.size(); ++i)
		{
			if (ImGui::TreeNode(meshVec[i]->GetName().c_str()))
			{
				ImGui::Text("Triangles:"); ImGui::SameLine(); ImGui::Text(std::to_string(meshVec[i]->GetIndexCount() / 3).c_str());
				ImGui::Text("Vertices:"); ImGui::SameLine(); ImGui::Text(std::to_string(meshVec[i]->GetVertexCount()).c_str());
				ImGui::Text("Indices:"); ImGui::SameLine(); ImGui::Text(std::to_string(meshVec[i]->GetIndexCount()).c_str());
				ImGui::TreePop();
			}
		}
		ImGui::TreePop();
	}

	// Vertex shader controls (temporary
	if (ImGui::TreeNode("Vertex shader controls"))
	{
		ImGui::ColorEdit4("Color tint", vsColorTint);
		ImGui::SliderFloat3("Offset", vsOffset, -1.0f, 1.0f);

		ImGui::TreePop();
	}

	// End ImGui creation
	ImGui::End();
}