#pragma once
#include "Engine_Default.h"
#include "Engine_Interface.h"

ENGINE_START
class ENGINE_DLL Camera : public Interface::EngineObject {
public:
    Camera();
    
public:
    HRESULT Set_Pose(const DirectX::XMFLOAT3& Position, float Yaw, float Pitch);

    HRESULT Set_Perspective(float FovY, float AspectRatio, float NearZ, float FarZ);

    const DirectX::XMFLOAT4X4& Get_ViewProjection() const noexcept { return m_ViewProjection; }
    
public:
    const DirectX::XMFLOAT3& Get_Position() const noexcept {
        return m_Position;
    }

    float Get_NearZ() const noexcept { return m_fNearZ; }
    float Get_FarZ() const noexcept { return m_fFarZ; }
    
private:
    void Update_View();
    void Update_Projection();
    void Update_ViewProjection();
    
private:
    DirectX::XMFLOAT3 m_Position{0.f, 0.f, -1.f};
    float m_fYaw = 0.f;
    float m_fPitch = 0.f;

    float m_fFovY = DirectX::XM_PIDIV4;
    float m_fAspectRatio = 16.f / 9.f;
    float m_fNearZ = 0.1f;
    float m_fFarZ = 1000.f;

    DirectX::XMFLOAT4X4 m_View{};
    DirectX::XMFLOAT4X4 m_Projection{};
    DirectX::XMFLOAT4X4 m_ViewProjection{};
};
ENGINE_END
