#pragma once

#include <wrl/client.h>
#include <string>
#include <dxcapi.h>   // IDxcBlob 정의 

namespace GP
{
	/*
	* .hlsl 을 실행 시점에 DXC 로 컴파일해서 바이트코드(Blob)를 반환한다.
	* 실패 시 에러를 VS 출력창(OutputDebugString)에 찍고 nullptr 반환.
	* @param path   .hlsl 경로 — 작업 디렉터리($(ProjectDir)) 기준 상대경로
	* @param entry  진입 함수 이름 (보통 L"main")
	* @param target 셰이더 종류_모델 (L"vs_6_2" / L"ps_6_2" / L"cs_6_2")
	* @return 컴파일된 바이트코드 Blob. PSO Finalize 까지 살아있어야 함. 실패 시 nullptr
	*/
	Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(
		const std::wstring& path,
		const std::wstring& entry,
		const std::wstring& target);
}
