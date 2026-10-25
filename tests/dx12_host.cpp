#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <array>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cmath>
using Microsoft::WRL::ComPtr;
bool input_fixture=false,received_move=false,received_click=false;
void check(HRESULT hr,const char*name){if(FAILED(hr))throw std::runtime_error(std::string(name)+" HRESULT="+std::to_string(static_cast<unsigned>(hr)));}
#include "free_camera_scene.hpp"
LRESULT CALLBACK wndproc(HWND h,UINT m,WPARAM w,LPARAM l){if(m==WM_KEYDOWN||m==WM_LBUTTONDOWN)std::ofstream("host-input.txt",std::ios::app)<<m<<' '<<w<<'\n';if(m==WM_KEYDOWN&&w=='W')received_move=true;if(m==WM_LBUTTONDOWN)received_click=true;if(m==WM_CLOSE){DestroyWindow(h);return 0;}if(m==WM_DESTROY){PostQuitMessage(0);return 0;}return DefWindowProcW(h,m,w,l);}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,LPWSTR args,int){
    bool interactive=wcsstr(args,L"--smoke")==nullptr;
    const bool grid_test=wcsstr(args,L"--grid")!=nullptr, grid_moved=wcsstr(args,L"--moved")!=nullptr;
    input_fixture=wcsstr(args,L"--input")!=nullptr;
    const bool camera_test=wcsstr(args,L"--free-camera")!=nullptr;
    try{
        WNDCLASSW wc{};wc.hInstance=instance;wc.lpfnWndProc=wndproc;wc.lpszClassName=L"SketchDX12TestWindow";wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);RegisterClassW(&wc);
        HWND window=CreateWindowW(wc.lpszClassName,L"沉星草稿 DirectX 12 测试",WS_OVERLAPPEDWINDOW,80,80,1100,760,nullptr,nullptr,instance,nullptr);
        if(!window)throw std::runtime_error("CreateWindow failed");
        if(interactive)ShowWindow(window,SW_SHOW);
        ComPtr<IDXGIFactory4>factory;check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"CreateDXGIFactory1");
        ComPtr<ID3D12Device>device;check(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)),"D3D12CreateDevice");
        D3D12_COMMAND_QUEUE_DESC q{};q.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;ComPtr<ID3D12CommandQueue>queue;check(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)),"CreateCommandQueue");
        DXGI_SWAP_CHAIN_DESC1 sd{};sd.Width=960;sd.Height=640;sd.Format=DXGI_FORMAT_R8G8B8A8_UNORM;sd.SampleDesc.Count=1;sd.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;sd.BufferCount=2;sd.SwapEffect=DXGI_SWAP_EFFECT_FLIP_DISCARD;
        ComPtr<IDXGISwapChain1>chain1;check(factory->CreateSwapChainForHwnd(queue.Get(),window,&sd,nullptr,nullptr,&chain1),"CreateSwapChainForHwnd");ComPtr<IDXGISwapChain3>chain;check(chain1.As(&chain),"SwapChain3");
        D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;hd.NumDescriptors=2;ComPtr<ID3D12DescriptorHeap>heap;check(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&heap)),"RTV heap");
        std::array<ComPtr<ID3D12Resource>,2>buffers;auto first=heap->GetCPUDescriptorHandleForHeapStart();UINT step=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        auto acquire=[&]{for(UINT i=0;i<2;++i){check(chain->GetBuffer(i,IID_PPV_ARGS(&buffers[i])),"GetBuffer");D3D12_CPU_DESCRIPTOR_HANDLE h{first.ptr+i*step};device->CreateRenderTargetView(buffers[i].Get(),nullptr,h);}};acquire();
        ComPtr<ID3D12CommandAllocator>allocator;check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"allocator");
        ComPtr<ID3D12GraphicsCommandList>list;check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)),"list");check(list->Close(),"close");
        ComPtr<ID3D12Fence>fence;check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"fence");HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);UINT64 tick=0;
        auto wait=[&]{check(queue->Signal(fence.Get(),++tick),"signal");if(fence->GetCompletedValue()<tick){check(fence->SetEventOnCompletion(tick,event),"fence event");if(WaitForSingleObject(event,10000)!=WAIT_OBJECT_0)throw std::runtime_error("GPU timeout");}};
        auto barrier=[&](ID3D12Resource*resource,D3D12_RESOURCE_STATES a,D3D12_RESOURCE_STATES b){D3D12_RESOURCE_BARRIER r{};r.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;r.Transition.pResource=resource;r.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;r.Transition.StateBefore=a;r.Transition.StateAfter=b;list->ResourceBarrier(1,&r);};
        ComPtr<ID3D12Resource>camera_buffer,alternate_camera_buffer,gpu_camera_buffer,transform_buffer,transform_default;ComPtr<ID3D12DescriptorHeap>camera_heap;
        free_scene::Scene camera_scene;if(camera_test)camera_scene.init(device.Get());
        if(grid_test){
            D3D12_HEAP_PROPERTIES h{};h.Type=D3D12_HEAP_TYPE_UPLOAD;D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=1280;d.Height=1;d.DepthOrArraySize=1;d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            if(wcsstr(args,L"--larger-camera"))d.Width=1536;
            if(wcsstr(args,L"--default-camera"))d.Width=4096;
            if(wcsstr(args,L"--gpu-upload"))h.Type=static_cast<D3D12_HEAP_TYPE>(5);
            check(device->CreateCommittedResource(&h,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&camera_buffer)),"camera upload buffer");
            if(wcsstr(args,L"--late-descriptor"))check(device->CreateCommittedResource(&h,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&alternate_camera_buffer)),"alternate camera upload buffer");
            h.Type=D3D12_HEAP_TYPE_UPLOAD;
            if(wcsstr(args,L"--default-camera")){h.Type=D3D12_HEAP_TYPE_DEFAULT;check(device->CreateCommittedResource(&h,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&gpu_camera_buffer)),"default camera buffer");h.Type=D3D12_HEAP_TYPE_UPLOAD;}
            D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=2;
            check(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&camera_heap)),"camera CBV heap");
            d.Width=48*256;check(device->CreateCommittedResource(&h,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&transform_buffer)),"transform upload buffer");
            h.Type=D3D12_HEAP_TYPE_DEFAULT;check(device->CreateCommittedResource(&h,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&transform_default)),"transform GPU buffer");
        }
        unsigned last=0;bool stop=false;
        for(unsigned frame=0;!stop&&(interactive||frame<240);++frame){
            if(wcsstr(args,L"--camera-input")){
                if(frame==30||frame==106||frame==112||frame==130)PostMessageW(window,WM_KEYDOWN,'T',1);
                if(frame==31||frame==107||frame==113||frame==131)PostMessageW(window,WM_KEYUP,'T',1);
                if(frame==40)PostMessageW(window,WM_KEYDOWN,'W',1);
                if(frame==50)PostMessageW(window,WM_KEYUP,'W',1);
                if(frame==92)PostMessageW(window,WM_KEYDOWN,'W',1);
                if(frame==98)PostMessageW(window,WM_KEYUP,'W',1);
                if(frame==140)PostMessageW(window,WM_KEYDOWN,VK_ESCAPE,1);
                if(frame==141)PostMessageW(window,WM_KEYUP,VK_ESCAPE,1);
            }
            // Messages target only this test window; no desktop input/focus is changed.
            if(input_fixture){
                if(frame==75||frame==145)PostMessageW(window,WM_KEYDOWN,'W',1);
                if(frame==78||frame==148)PostMessageW(window,WM_KEYUP,'W',1);
                if(frame==85)PostMessageW(window,WM_KEYDOWN,'A',1);
                if(frame==88)PostMessageW(window,WM_KEYUP,'A',1);
                if(frame==95)PostMessageW(window,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(600,300));
                if(frame==98)PostMessageW(window,WM_LBUTTONUP,0,MAKELPARAM(600,300));
            }
            MSG msg;while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){if(msg.message==WM_QUIT)stop=true;TranslateMessage(&msg);DispatchMessageW(&msg);}if(stop)break;
            if(frame==120&&!interactive&&!wcsstr(args,L"--camera-cached")){wait();for(auto&b:buffers)b.Reset();check(chain->ResizeBuffers(2,1000,700,sd.Format,0),"ResizeBuffers");acquire();}
            if(grid_test){
                if(alternate_camera_buffer)camera_buffer.Swap(alternate_camera_buffer);
                float values[320]{};double cx=grid_moved?2:0,cy=grid_moved?1:0,height=500,zoom=grid_moved?65:50;
                double m[16]={zoom,0,0,-zoom*cx,0,zoom,0,-zoom*cy,0,0,-1000./999,1000*(height-1)/999,0,0,-1,height};
                double a[4][8]{};for(int r=0;r<4;++r)for(int c=0;c<4;++c){a[r][c]=m[r*4+c];a[r][c+4]=r==c;}
                for(int c=0;c<4;++c){int pivot=c;for(int r=c+1;r<4;++r)if(std::abs(a[r][c])>std::abs(a[pivot][c]))pivot=r;for(int j=0;j<8;++j)std::swap(a[c][j],a[pivot][j]);double d=a[c][c];for(int j=0;j<8;++j)a[c][j]/=d;for(int r=0;r<4;++r)if(r!=c){double d=a[r][c];for(int j=0;j<8;++j)a[r][j]-=d*a[c][j];}}
                for(int r=0;r<4;++r)for(int c=0;c<4;++c){values[r*4+c]=float(m[r*4+c]);values[16+r*4+c]=float(a[r][c+4]);}
                auto d=buffers[0]->GetDesc();values[208]=float(cx);values[209]=float(cy);values[210]=float(height);values[214]=-1;values[216]=1.f/float(d.Width);values[217]=1.f/float(d.Height);std::memcpy(values+225,&frame,4);
                if(camera_test)free_scene::camera(values,frame,unsigned(d.Width),unsigned(d.Height));
                // Deliberately create CBV before writing CPU bytes, as the game does.
                D3D12_CONSTANT_BUFFER_VIEW_DESC view{(gpu_camera_buffer?gpu_camera_buffer:camera_buffer)->GetGPUVirtualAddress(),UINT(camera_buffer->GetDesc().Width)};device->CreateConstantBufferView(&view,camera_heap->GetCPUDescriptorHandleForHeapStart());
                void *mapped=nullptr;D3D12_RANGE none{0,0};
                if(!(wcsstr(args,L"--camera-cached")&&frame>60&&frame<200)){
                    check(camera_buffer->Map(0,&none,&mapped),"camera Map");std::memcpy(mapped,values,sizeof(values));camera_buffer->Unmap(0,nullptr);
                }
                D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.ViewDimension=D3D12_SRV_DIMENSION_BUFFER;srv.Format=DXGI_FORMAT_UNKNOWN;srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Buffer.NumElements=256;srv.Buffer.StructureByteStride=48;
                auto handle=camera_heap->GetCPUDescriptorHandleForHeapStart();handle.ptr+=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);device->CreateShaderResourceView(transform_default.Get(),&srv,handle);
                check(transform_buffer->Map(0,&none,&mapped),"transform Map");float *transforms=static_cast<float*>(mapped);
                for(unsigned n=0;n<256;++n){float matrix[12]={1,0,0,float(n%16)+.5f,0,1,0,float(n/16)+.5f,0,0,1,0};std::memcpy(transforms+n*12,matrix,sizeof(matrix));}transform_buffer->Unmap(0,nullptr);
            }
            last=chain->GetCurrentBackBufferIndex();check(allocator->Reset(),"reset allocator");check(list->Reset(allocator.Get(),nullptr),"reset list");
            if(grid_test)list->CopyBufferRegion(transform_default.Get(),0,transform_buffer.Get(),0,48*256);
            if(gpu_camera_buffer){list->CopyBufferRegion(gpu_camera_buffer.Get(),0,camera_buffer.Get(),0,4096);barrier(gpu_camera_buffer.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);}
            barrier(buffers[last].Get(),D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_RENDER_TARGET);float clear[]={.035f,.06f,.09f,1};D3D12_CPU_DESCRIPTOR_HANDLE rtv{first.ptr+last*step};list->ClearRenderTargetView(rtv,clear,0,nullptr);
            if(camera_test){auto d=buffers[last]->GetDesc();camera_scene.draw(device.Get(),list.Get(),camera_heap->GetCPUDescriptorHandleForHeapStart(),rtv,unsigned(d.Width),unsigned(d.Height));}
            if(gpu_camera_buffer)barrier(gpu_camera_buffer.Get(),D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,D3D12_RESOURCE_STATE_COPY_DEST);
            barrier(buffers[last].Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PRESENT);check(list->Close(),"close frame");ID3D12CommandList*commands[]={list.Get()};queue->ExecuteCommandLists(1,commands);
            check(chain->Present(0,0),"Present");wait();Sleep(16);
            if(camera_test&&wcsstr(args,L"--camera-preview")==nullptr&&wcsstr(args,L"--camera-input")==nullptr&&(frame==55||frame==100||frame==140||frame==160||frame==180))camera_scene.verify(device.Get(),queue.Get(),buffers[last].Get(),camera_buffer.Get(),frame);
        }
        auto desc=buffers[last]->GetDesc();D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT64 bytes=0;device->GetCopyableFootprints(&desc,0,1,0,&footprint,nullptr,nullptr,&bytes);
        D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_READBACK;D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;rd.Width=bytes;rd.Height=1;rd.DepthOrArraySize=1;rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        ComPtr<ID3D12Resource>readback;check(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback)),"readback");
        allocator->Reset();list->Reset(allocator.Get(),nullptr);barrier(buffers[last].Get(),D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_COPY_SOURCE);
        D3D12_TEXTURE_COPY_LOCATION src{};src.pResource=buffers[last].Get();src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;D3D12_TEXTURE_COPY_LOCATION dst{};dst.pResource=readback.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=footprint;
        list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);barrier(buffers[last].Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_PRESENT);list->Close();ID3D12CommandList*cmd[]={list.Get()};queue->ExecuteCommandLists(1,cmd);wait();
        unsigned char*pixels=nullptr;D3D12_RANGE range{0,static_cast<SIZE_T>(bytes)};check(readback->Map(0,&range,reinterpret_cast<void**>(&pixels)),"map");
        BITMAPFILEHEADER bf{};BITMAPINFOHEADER bi{};bi.biSize=sizeof(bi);bi.biWidth=static_cast<LONG>(desc.Width);bi.biHeight=-static_cast<LONG>(desc.Height);bi.biPlanes=1;bi.biBitCount=32;bi.biCompression=BI_RGB;bf.bfType=0x4d42;bf.bfOffBits=sizeof(bf)+sizeof(bi);bf.bfSize=bf.bfOffBits+bi.biWidth*desc.Height*4;
        std::ofstream image("render.bmp",std::ios::binary);image.write(reinterpret_cast<char*>(&bf),sizeof(bf));image.write(reinterpret_cast<char*>(&bi),sizeof(bi));size_t bright=0;
        for(UINT y=0;y<desc.Height;++y){std::vector<unsigned char>row(desc.Width*4);for(UINT x=0;x<desc.Width;++x){auto p=pixels+y*footprint.Footprint.RowPitch+x*4;row[x*4]=p[2];row[x*4+1]=p[1];row[x*4+2]=p[0];row[x*4+3]=255;if(y>80&&y<desc.Height-90&&(p[0]>140||p[1]>140||p[2]>140))++bright;}image.write(reinterpret_cast<char*>(row.data()),row.size());}
        readback->Unmap(0,nullptr);CloseHandle(event);DestroyWindow(window);
        std::ofstream result("dx12-test-result.txt");result<<"D3D12 frames presented: 240\nResizeBuffers: "<<(wcsstr(args,L"--camera-cached")?"not exercised (cached buffer case)":"passed")<<"\nDevice removed reason: "<<device->GetDeviceRemovedReason()<<"\nBright pixels in draft area: "<<bright<<"\n";
        if(bright<1000)throw std::runtime_error("No in-game draft pixels detected");result<<"PASS: native addon rendered into swap-chain back buffer\n";
        if(input_fixture){if(!received_move||received_click)throw std::runtime_error("Input routing failed: game must receive W and must not receive drawing clicks");result<<"PASS: keyboard W reaches host while drawing mouse clicks are blocked\n";}
        return 0;
    }catch(const std::exception&e){std::ofstream("dx12-test-error.txt")<<e.what();return 1;}
}
