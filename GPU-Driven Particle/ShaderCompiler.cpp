#include "ShaderCompiler.h"

#include <Windows.h>
#include <dxcapi.h>

#pragma comment(lib, "dxcompiler.lib")

using Microsoft::WRL::ComPtr;

namespace GP
{
	ComPtr<IDxcBlob> CompileShader(const std::wstring& path, const std::wstring& entry, const std::wstring& target)
	{
		// 함수 로컬 static DXC 인스턴스
		static ComPtr<IDxcUtils> s_Utils;					// 파일 로드 유틸
		static ComPtr<IDxcCompiler3> s_Compiler;			// 컴파일러
		static ComPtr<IDxcIncludeHandler> s_IncludeHandler;	// #include 처리기
		if (!s_Compiler)
		{
			// 객체 생성
			DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&s_Utils));
			DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&s_Compiler));
			s_Utils->CreateDefaultIncludeHandler(&s_IncludeHandler); // #include "..." 처리용
		}

		// .hlsl 텍스트를 메모리로 읽음
		// 실패 시 출력창에 경로 출력
		ComPtr<IDxcBlobEncoding> source;
		if (FAILED(s_Utils->LoadFile(path.c_str(), nullptr, &source)))
		{
			OutputDebugStringW((L"[ShaderCompiler] 파일을 못 찾음: " + path + L"\n").c_str());
			return nullptr;
		}

		// 컴파일 인자 설정
		const wchar_t* args[] =
		{
			path.c_str(),           // 에러 메시지에 파일명이 나오게
			L"-E", entry.c_str(),	// Entry: 진입 함수 이름
			L"-T", target.c_str(),	// Target: 셰이더 모델
			L"-I", L".",            // include 검색 경로 (현재 작업 디렉터리)
			L"-Zi",                 // 디버그 정보 (PIX 에서 HLSL 소스가 보임)
			L"-Qembed_debug",       // 디버그 정보를 Blob에 내장 ?
		};

		// DxcBuffer - 컴파일할 대상 데이터(소스 코드, 텍스트, 또는 바이너리)의 메모리 위치와 크기를 정의
		DxcBuffer buffer{
			source->GetBufferPointer(), // hlsl 텍스트 메모리 주소
			source->GetBufferSize(),	// 텍스트 크기
			DXC_CP_ACP					// 인코딩(자동 감지)
		};
		ComPtr<IDxcResult> result;
		s_Compiler->Compile(
			&buffer,				// 컴파일 대상 데이터
			args, _countof(args),	// 어떻게 컴파일할지
			s_IncludeHandler.Get(),	// #include 만나면 누구한테 물어볼지
			IID_PPV_ARGS(&result)	// 결과를 어디에 받을지
		);

		// 에러/경고 텍스트 꺼내서 무조건 출력 (경고 확인 위함)
		ComPtr<IDxcBlobUtf8> errors;
		result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
		if (errors && errors->GetStringLength() > 0)
			OutputDebugStringA(errors->GetStringPointer());

		HRESULT status;
		result->GetStatus(&status);
		if (FAILED(status)) // 컴파일 실패
		{
			OutputDebugStringW((L"[ShaderCompiler] 컴파일 실패: " + path + L"\n").c_str());
			return nullptr;
		}

		// 성공 시 바이트코드 꺼냄
		ComPtr<IDxcBlob> blob;
		result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&blob), nullptr);
		return blob;
	}
}
