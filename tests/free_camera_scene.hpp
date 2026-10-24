#pragma once
#include "../src/free_camera.hpp"
#include <d3dcompiler.h>
#include <string>
// Synthetic scene: checkerboard geometry uses the same b2 / descriptor-table
// camera route as the game. Readback checks the red center tile and a world
// path waypoint, rather than accepting a nonempty overlay as camera control.
namespace free_scene {
using Microsoft::WRL::ComPtr;
inline void camera(float*p,unsigned frame,unsigned width,unsigned height){
    using namespace sketch::free_camera;auto view=translation({0,0,-14});double scale=2.6;
    Mat proj={scale*height/width,0,0,0,0,scale,0,0,0,0,-1.0001,-.10001,0,0,-1,0},vp=mul(proj,view),inv;
    std::fill(p,p+320,0.f);store(p,vp);inverse(vp,inv);store(p+16,inv);store(p+160,view);store(p+192,proj);inverse(proj,inv);store(p+176,inv);
    p[210]=14;p[214]=-1;p[216]=1.f/width;p[217]=1.f/height;p[220]=.1f;p[221]=1000;uint32_t type=8;std::memcpy(p+225,&frame,4);std::memcpy(p+226,&type,4);
}
struct Scene {
    ComPtr<ID3D12RootSignature> root;
    ComPtr<ID3D12PipelineState> pipeline;
    ComPtr<ID3D12DescriptorHeap> heap,bulk_staging;
    UINT step=0;
    void init(ID3D12Device*d){
        D3D12_DESCRIPTOR_RANGE ranges[2]{};ranges[0]={D3D12_DESCRIPTOR_RANGE_TYPE_SRV,23,0,0,0};ranges[1]={D3D12_DESCRIPTOR_RANGE_TYPE_CBV,1,2,0,23};
        D3D12_ROOT_PARAMETER param{};param.ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;param.DescriptorTable={2,ranges};param.ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
        D3D12_ROOT_SIGNATURE_DESC desc{1,&param,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> blob,error;
        check(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"camera root serialize");check(d->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)),"camera root");
        const char*source=R"(
cbuffer Camera:register(b2){row_major float4x4 vp;};
struct V{float4 position:SV_Position;float3 color:COLOR;};
V vs(uint vertex:SV_VertexID,uint instance:SV_InstanceID){
 float2 corners[6]={float2(-.45,-.45),float2(.45,-.45),float2(-.45,.45),float2(-.45,.45),float2(.45,-.45),float2(.45,.45)};
 int x=int(instance%9)-4,y=int(instance/9)-4;V o;o.position=mul(vp,float4(float2(x,y)+corners[vertex],0,1));
 o.color=(x==0&&y==0)?float3(1,.05,.05):(((x+y)&1)?float3(.12,.28,.5):float3(.18,.38,.65));return o;}
float4 ps(V i):SV_Target{return float4(i.color,1);})";
        ComPtr<ID3DBlob> vs,ps;check(D3DCompile(source,std::strlen(source),nullptr,nullptr,nullptr,"vs","vs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&vs,&error),"camera VS");check(D3DCompile(source,std::strlen(source),nullptr,nullptr,nullptr,"ps","ps_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&ps,&error),"camera PS");
        D3D12_GRAPHICS_PIPELINE_STATE_DESC p{};p.pRootSignature=root.Get();p.VS={vs->GetBufferPointer(),vs->GetBufferSize()};p.PS={ps->GetBufferPointer(),ps->GetBufferSize()};p.SampleMask=UINT_MAX;
        p.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;p.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;p.RasterizerState.DepthClipEnable=TRUE;p.BlendState.RenderTarget[0].RenderTargetWriteMask=15;
        p.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;p.NumRenderTargets=1;p.RTVFormats[0]=DXGI_FORMAT_R8G8B8A8_UNORM;p.SampleDesc.Count=1;
        check(d->CreateGraphicsPipelineState(&p,IID_PPV_ARGS(&pipeline)),"camera pipeline");
        const bool bulk=wcsstr(GetCommandLineW(),L"--bulk-descriptors")!=nullptr;
        D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=bulk?5000:24;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;check(d->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&heap)),"camera shader heap");step=d->GetDescriptorHandleIncrementSize(hd.Type);
        if(bulk){hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_NONE;check(d->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&bulk_staging)),"bulk staging heap");}
        D3D12_SHADER_RESOURCE_VIEW_DESC null_view{};null_view.Format=DXGI_FORMAT_R32_FLOAT;null_view.ViewDimension=D3D12_SRV_DIMENSION_BUFFER;null_view.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;null_view.Buffer.NumElements=1;
        auto handle=heap->GetCPUDescriptorHandleForHeapStart();for(int i=0;i<23;++i){d->CreateShaderResourceView(nullptr,&null_view,handle);handle.ptr+=step;}
        if(bulk){handle=bulk_staging->GetCPUDescriptorHandleForHeapStart();for(int i=0;i<5000;++i){d->CreateShaderResourceView(nullptr,&null_view,handle);handle.ptr+=step;}}
    }
    void draw(ID3D12Device*d,ID3D12GraphicsCommandList*list,D3D12_CPU_DESCRIPTOR_HANDLE cbv,D3D12_CPU_DESCRIPTOR_HANDLE rtv,unsigned width,unsigned height){
        auto to=heap->GetCPUDescriptorHandleForHeapStart();to.ptr+=23*step;
        const bool late=wcsstr(GetCommandLineW(),L"--late-descriptor")!=nullptr;
        auto fill=[&]{if(bulk_staging){auto cpu=bulk_staging->GetCPUDescriptorHandleForHeapStart();cpu.ptr+=23*step;d->CopyDescriptorsSimple(1,cpu,cbv,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);d->CopyDescriptorsSimple(5000,heap->GetCPUDescriptorHandleForHeapStart(),bulk_staging->GetCPUDescriptorHandleForHeapStart(),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);}else d->CopyDescriptorsSimple(1,to,cbv,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);};
        if(!late)fill();
        list->SetPipelineState(pipeline.Get());list->SetGraphicsRootSignature(root.Get());ID3D12DescriptorHeap*h[]={heap.Get()};list->SetDescriptorHeaps(1,h);list->SetGraphicsRootDescriptorTable(0,heap->GetGPUDescriptorHandleForHeapStart());
        D3D12_VIEWPORT vp{0,0,float(width),float(height),0,1};D3D12_RECT sc{0,0,LONG(width),LONG(height)};list->RSSetViewports(1,&vp);list->RSSetScissorRects(1,&sc);list->OMSetRenderTargets(1,&rtv,FALSE,nullptr);list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);list->DrawInstanced(6,81,0,0);
        if(late)fill();
    }
    void verify(ID3D12Device*d,ID3D12CommandQueue*q,ID3D12Resource*back,ID3D12Resource*cb,unsigned frame){
        using namespace sketch;void*camera_data=nullptr;D3D12_RANGE cr{0,960};check(cb->Map(0,&cr,&camera_data),"camera verify Map");auto view=free_camera::decode(static_cast<float*>(camera_data));D3D12_RANGE none{0,0};cb->Unmap(0,&none);
        if(!view.valid)throw std::runtime_error("GPU camera constants invalid");bool changed=frame==100;if(changed&&grid::length(view.eye-grid::V{0,0,14})<1)throw std::runtime_error("Free camera did not affect submitted constants");if(!changed&&grid::length(view.eye-grid::V{0,0,14})>.01)throw std::runtime_error("Game camera was not restored");
        auto desc=back->GetDesc();D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};UINT64 bytes=0;d->GetCopyableFootprints(&desc,0,1,0,&fp,nullptr,nullptr,&bytes);
        D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_READBACK;D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;rd.Width=bytes;rd.Height=1;rd.DepthOrArraySize=1;rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;ComPtr<ID3D12Resource>read;
        check(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&read)),"camera readback");
        ComPtr<ID3D12CommandAllocator>a;ComPtr<ID3D12GraphicsCommandList>list;check(d->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&a)),"verify allocator");check(d->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,a.Get(),nullptr,IID_PPV_ARGS(&list)),"verify list");
        D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition.pResource=back;b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;b.Transition.StateBefore=D3D12_RESOURCE_STATE_PRESENT;b.Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_SOURCE;list->ResourceBarrier(1,&b);
        D3D12_TEXTURE_COPY_LOCATION from{};from.pResource=back;from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;D3D12_TEXTURE_COPY_LOCATION to{};to.pResource=read.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint=fp;list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        std::swap(b.Transition.StateBefore,b.Transition.StateAfter);list->ResourceBarrier(1,&b);check(list->Close(),"verify close");ID3D12CommandList*lists[]={list.Get()};q->ExecuteCommandLists(1,lists);
        ComPtr<ID3D12Fence>fence;check(d->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"verify fence");HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);check(q->Signal(fence.Get(),1),"verify signal");check(fence->SetEventOnCompletion(1,event),"verify event");if(WaitForSingleObject(event,10000)!=WAIT_OBJECT_0)throw std::runtime_error("verify GPU timeout");CloseHandle(event);
        unsigned char*pixels=nullptr;D3D12_RANGE rr{0,SIZE_T(bytes)};check(read->Map(0,&rr,reinterpret_cast<void**>(&pixels)),"verify pixels");
        grid::Camera camera{view.vp,view.inv,0,true,view.eye,view.forward};auto pixel=[&](grid::V world){double x,y;if(!camera.project(world,x,y)||x<0||x>=1||y<0||y>=1)throw std::runtime_error("Camera target left fixture frame");return pixels+unsigned(y*desc.Height)*fp.Footprint.RowPitch+unsigned(x*desc.Width)*4;};
        auto red=pixel({0,0,0});if(red[0]<200||red[1]>110||red[2]>110)throw std::runtime_error("Rendered geometry does not match modified camera projection");
        auto path=pixel({2,1,0});if(path[1]<180||path[2]<180)throw std::runtime_error("World path lost alignment with the rendered grid");
        read->Unmap(0,&none);std::ofstream("free-camera-render-results.txt",std::ios::app)<<"PASS frame "<<frame<<": GPU geometry, projected world path, "<<(changed?"pan/height/yaw/tilt/zoom":"game view")<<'\n';
    }
};
}
