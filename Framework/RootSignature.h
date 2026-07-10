//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
//

#pragma once

#include "pch.h"

class DescriptorCache;

// 매개변수 한 칸
class RootParameter
{
    friend class RootSignature;
public:

    RootParameter() 
    {
        m_RootParam.ParameterType = (D3D12_ROOT_PARAMETER_TYPE)0xFFFFFFFF;
    }

    ~RootParameter()
    {
        Clear();
    }

    void Clear()
    {
        if (m_RootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
            delete [] m_RootParam.DescriptorTable.pDescriptorRanges;

        m_RootParam.ParameterType = (D3D12_ROOT_PARAMETER_TYPE)0xFFFFFFFF;
    }

    void InitAsConstants( UINT Register, UINT NumDwords, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL, UINT Space = 0 )
    {
        m_RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        m_RootParam.ShaderVisibility = Visibility;
        m_RootParam.Constants.Num32BitValues = NumDwords;
        m_RootParam.Constants.ShaderRegister = Register;
        m_RootParam.Constants.RegisterSpace = Space;
    }

    void InitAsConstantBuffer( UINT Register, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL, UINT Space = 0 )
    {
        m_RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        m_RootParam.ShaderVisibility = Visibility;
        m_RootParam.Descriptor.ShaderRegister = Register;
        m_RootParam.Descriptor.RegisterSpace = Space;
    }

	/*
	* 루트 파라미터를 Root Descriptor로 설정
	* @param Register 연결할 t레지스터 번호
	* @param Visibility 이 파라미터를 볼 수 있는 셰이더 단계
	* @param Space 레지스터 공간 (보통 0)
	*/
    void InitAsBufferSRV( UINT Register, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL, UINT Space = 0 )
    {
        m_RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        m_RootParam.ShaderVisibility = Visibility;
        m_RootParam.Descriptor.ShaderRegister = Register;
        m_RootParam.Descriptor.RegisterSpace = Space;
    }

    void InitAsBufferUAV( UINT Register, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL, UINT Space = 0 )
    {
        m_RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
        m_RootParam.ShaderVisibility = Visibility;
        m_RootParam.Descriptor.ShaderRegister = Register;
        m_RootParam.Descriptor.RegisterSpace = Space;
    }

	/*
	* Table+Range를 한 번에 만드는 축약(Range 1개 = 같은 종류, 연속 레지스터 Count칸
	* 내부적으로 InitAsDescriptorTable(1) + SetTableRange(0)를 호출
	* 텍스처 SRV처럼 루트 디스크립터로 못 꽂는 리소스는 최소 이 방식이 필요
	* @param Type     종류 (SRV / UAV / CBV / SAMPLER)
	* @param Register 시작 레지스터 번호 (SRV면 t레지스터)
	* @param Count    시작 레지스터부터 연속 몇 개인지
	*/
    void InitAsDescriptorRange( D3D12_DESCRIPTOR_RANGE_TYPE Type, UINT Register, UINT Count, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL, UINT Space = 0 )
    {
        InitAsDescriptorTable(1, Visibility);
        SetTableRange(0, Type, Register, Count, Space);
    }

	/*
	* 이 슬롯을 디스크립터 테이블로 정의 - "Range 몇 개짜리 목록"인지 틀만 잡는다
	* 루트에는 힙 오프셋 1 DWORD만 실리고, 실제 디스크립터들은 힙에 산다
	* 주의: 레인지 배열을 new로 할당만 하고 내용은 안 채움(쓰레기값).
	*       반드시 SetTableRange로 RangeCount개를 전부 채운 뒤 Finalize
	* @param RangeCount 이 테이블을 구성하는 레인지 개수
	*/
    void InitAsDescriptorTable( UINT RangeCount, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL )
    {
        m_RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        m_RootParam.ShaderVisibility = Visibility;
        m_RootParam.DescriptorTable.NumDescriptorRanges = RangeCount;
        m_RootParam.DescriptorTable.pDescriptorRanges = new D3D12_DESCRIPTOR_RANGE[RangeCount];
    }

	/*
	* 테이블의 RangeIndex번째 Range 내용을 채운다
	* "어떤 종류를, 어느 레지스터부터, 몇 개"
	* 앞 Range가 끝난 지점부터 자동으로 이어붙음 (수동 offset 계산 불필요)
	* @param RangeIndex 채울 Range 번호 (0 ~ RangeCount-1)
	* @param Type       구획 종류 (SRV / UAV / CBV / SAMPLER)
	* @param Register   시작 레지스터 번호
	* @param Count      연속 개수 (Register부터 Count개)
	*/
    void SetTableRange( UINT RangeIndex, D3D12_DESCRIPTOR_RANGE_TYPE Type, UINT Register, UINT Count, UINT Space = 0 )
    {
        D3D12_DESCRIPTOR_RANGE* range = const_cast<D3D12_DESCRIPTOR_RANGE*>(m_RootParam.DescriptorTable.pDescriptorRanges + RangeIndex);
        range->RangeType = Type;
        range->NumDescriptors = Count;
        range->BaseShaderRegister = Register;
        range->RegisterSpace = Space;
        range->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    }

    const D3D12_ROOT_PARAMETER& operator() ( void ) const { return m_RootParam; }
        

protected:

    D3D12_ROOT_PARAMETER m_RootParam;
};

// Maximum 64 DWORDS divied up amongst all root parameters.
// Root constants = 1 DWORD * NumConstants
// Root descriptor (CBV, SRV, or UAV) = 2 DWORDs each
// Descriptor table pointer = 1 DWORD
// Static samplers = 0 DWORDS (compiled into shader)
// 매개변수 목록
class RootSignature
{
    friend class DynamicDescriptorHeap;

public:

    RootSignature( UINT NumRootParams = 0, UINT NumStaticSamplers = 0 ) : m_Finalized(FALSE), m_NumParameters(NumRootParams)
    {
        Reset(NumRootParams, NumStaticSamplers);
    }

    ~RootSignature()
    {
    }

    static void DestroyAll(void);

	/*
	* @NumRootParams 루트 파라미터(슬롯) 개수
	* @NumStaticSamplers 정적 샘플러 개수
	*/
    void Reset( UINT NumRootParams, UINT NumStaticSamplers = 0 )
    {
        if (NumRootParams > 0)
            m_ParamArray.reset(new RootParameter[NumRootParams]);
        else
            m_ParamArray = nullptr;
        m_NumParameters = NumRootParams;

        if (NumStaticSamplers > 0)
            m_SamplerArray.reset(new D3D12_STATIC_SAMPLER_DESC[NumStaticSamplers]);
        else
            m_SamplerArray = nullptr;
        m_NumSamplers = NumStaticSamplers;
        m_NumInitializedStaticSamplers = 0;
    }

    RootParameter& operator[] ( size_t EntryIndex )
    {
        ASSERT(EntryIndex < m_NumParameters);
        return m_ParamArray.get()[EntryIndex];
    }

    const RootParameter& operator[] ( size_t EntryIndex ) const
    {
        ASSERT(EntryIndex < m_NumParameters);
        return m_ParamArray.get()[EntryIndex];
    }

	/*
	* @param Register 샘플러 레지스터 번호
	* @param NonStaticSamplerDesc D3D12_SAMPLER_DESC
	* @param D3D12_SHADER_VISIBILITY 셰이더 어느 stage에서 보이는지
	*/
    void InitStaticSampler( UINT Register, const D3D12_SAMPLER_DESC& NonStaticSamplerDesc,
        D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL );

    void Finalize(const std::wstring& name, D3D12_ROOT_SIGNATURE_FLAGS Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);

    ID3D12RootSignature* GetSignature() const { return m_Signature; }

protected:

    BOOL m_Finalized;
    UINT m_NumParameters;
    UINT m_NumSamplers;
    UINT m_NumInitializedStaticSamplers;
    uint32_t m_DescriptorTableBitMap;		// One bit is set for root parameters that are non-sampler descriptor tables
    uint32_t m_SamplerTableBitMap;			// One bit is set for root parameters that are sampler descriptor tables
    uint32_t m_DescriptorTableSize[16];		// Non-sampler descriptor tables need to know their descriptor count
    std::unique_ptr<RootParameter[]> m_ParamArray;
    std::unique_ptr<D3D12_STATIC_SAMPLER_DESC[]> m_SamplerArray;
    ID3D12RootSignature* m_Signature;
};
