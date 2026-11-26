#include "Camera.h"
ENGINE_USE

Camera::Camera()
{
    Update_View();
    Update_Projection();
    Update_ViewProjection();
}

HRESULT Camera::Set_Pose(const DirectX::XMFLOAT3& Position, float Yaw, float Pitch) {
    if (!std::isfinite(Position.x) || !std::isfinite(Position.y) || !std::isfinite(Position.z) ||
        !std::isfinite(Yaw) || !std::isfinite(Pitch)) return E_INVALIDARG;
    
    // Prevent LookDir, UpDir be Paralleled
    constexpr float PitchLimit = XM_PIDIV2 - 0.01f;
    
    if (Pitch < -PitchLimit || Pitch > PitchLimit) return E_INVALIDARG;

    m_Position = Position; m_fYaw = Yaw; m_fPitch = Pitch;

    Update_View();
    Update_ViewProjection();
    
    return S_OK;
}

HRESULT Camera::Set_Perspective(float FovY, float AspectRatio, float NearZ, float FarZ) {
    if (!std::isfinite(FovY) || !std::isfinite(AspectRatio) || !std::isfinite(NearZ) || !std::isfinite(FarZ) ||
    FovY <= 0.f || FovY >= XM_PI || AspectRatio <= 0.f || NearZ <= 0.f || FarZ <= NearZ) return E_INVALIDARG;
    
    m_fFovY = FovY;
    m_fAspectRatio = AspectRatio;
    m_fNearZ = NearZ; m_fFarZ = FarZ;

    Update_Projection();
    Update_ViewProjection();
    
    return S_OK;
}

void Camera::Update_View() {
    const float CosPitch = std::cos(m_fPitch);

    const XMVECTOR Direction = XMVectorSet(std::sin(m_fYaw) * CosPitch, std::sin(m_fPitch), std::cos(m_fYaw) * CosPitch, 0.f);

    const XMVECTOR Up = XMVectorSet(0.f, 1.f, 0.f, 0.f);

    XMStoreFloat4x4(&m_View, XMMatrixLookToLH(XMLoadFloat3(&m_Position), Direction, Up));
}

void Camera::Update_Projection() {
    XMStoreFloat4x4(&m_Projection, XMMatrixPerspectiveFovLH(m_fFovY, m_fAspectRatio, m_fNearZ, m_fFarZ));
}

void Camera::Update_ViewProjection() {
    XMStoreFloat4x4(&m_ViewProjection, XMLoadFloat4x4(&m_View) * XMLoadFloat4x4(&m_Projection));
}
