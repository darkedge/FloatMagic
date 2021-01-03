#pragma once
#include <Windows.h>

struct IDWriteFactory;
struct ID2D1DeviceContext;
struct IWICImagingFactory;

namespace svc
{
  void ProvideDWriteFactory(IDWriteFactory* pFactory);
  void ProvideD2D1DeviceContext(ID2D1DeviceContext* pContext);
  void ProvideWicFactory(IWICImagingFactory* pContext);
} // namespace svc
