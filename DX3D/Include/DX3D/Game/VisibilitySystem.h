#pragma once

#include <DX3D/Game/Entity.h>
#include <DX3D/Game/RenderComponentSystem.h>
#include <DX3D/Game/TransformSystem.h>
#include <DX3D/Graphics/Resources/ModelData.h>
#include <DX3D/Graphics/Resources/ModelGPU.h>
#include <DX3D/InputSystem/Camera.h>
#include <DX3D/Math/Frustrum.h>

#include <DirectXMath.h>
#include <unordered_map>
#include <vector>

namespace dx3d {

    struct RenderProxy {
        ModelGPU* model = nullptr;
        DirectX::XMFLOAT4X4 world{};
        AABB bounds{};
    };

    class VisibilitySystem {
    public:
        void update(TransformSystem& transforms, RenderComponentSystem& renderables, const Camera& camera) {
            const Vec3d cameraPosition = camera.getPosition();
            const bool cameraMoved = !m_hasLastCameraPosition ||
                cameraPosition.x != m_lastCameraPosition.x ||
                cameraPosition.y != m_lastCameraPosition.y ||
                cameraPosition.z != m_lastCameraPosition.z;
            m_lastCameraPosition = cameraPosition;
            m_hasLastCameraPosition = true;

            if (cameraMoved || transforms.hasStructuralChanges() || renderables.hasStructuralChanges()) {
                rebuildAll(transforms, renderables, camera);
            }
            else {
                updateDirty(transforms, renderables, camera);
                refreshProxyLists(camera);
            }

            transforms.clearDirtyTracking();
            renderables.clearDirtyTracking();
        }

        const std::vector<RenderProxy>& getVisibleProxies() const noexcept { return m_visibleProxies; }
        const std::vector<RenderProxy>& getShadowProxies() const noexcept { return m_shadowProxies; }

        void clear() {
            m_cachedProxies.clear();
            m_visibleProxies.clear();
            m_shadowProxies.clear();
        }

    private:
        struct CachedProxy {
            RenderProxy proxy{};
            bool visible = false;
            bool castsShadow = false;
        };

        static DirectX::XMFLOAT4X4 buildCameraRelativeWorld(const WorldTransform& wt, const Vec3d& cameraPosition) {
            using namespace DirectX;

            Vec3d relPos = wt.position - cameraPosition;
            XMMATRIX S = XMMatrixScaling(wt.scale.x, wt.scale.y, wt.scale.z);
            XMMATRIX R = XMMatrixRotationQuaternion(XMLoadFloat4(&wt.rotation));
            XMMATRIX T = XMMatrixTranslation(static_cast<float>(relPos.x), static_cast<float>(relPos.y), static_cast<float>(relPos.z));

            XMFLOAT4X4 world;
            XMStoreFloat4x4(&world, S * R * T);
            return world;
        }

        void rebuildAll(TransformSystem& transforms, RenderComponentSystem& renderables, const Camera& camera) {
            m_cachedProxies.clear();

            const auto& entities = renderables.getRawEntities();
            const auto& renderData = renderables.getRawData();

            for (size_t i = 0; i < entities.size(); ++i) {
                updateEntity(entities[i], renderData[i], transforms, camera);
            }

            refreshProxyLists(camera);
        }

        void updateDirty(TransformSystem& transforms, RenderComponentSystem& renderables, const Camera& camera) {
            for (Entity e : transforms.getDirtyEntities()) {
                if (!renderables.has(e)) {
                    m_cachedProxies.erase(e.id);
                    continue;
                }
                updateEntity(e, static_cast<const RenderComponentSystem&>(renderables).get(e), transforms, camera);
            }

            for (Entity e : renderables.getDirtyEntities()) {
                if (!renderables.has(e)) {
                    m_cachedProxies.erase(e.id);
                    continue;
                }
                updateEntity(e, static_cast<const RenderComponentSystem&>(renderables).get(e), transforms, camera);
            }
        }

        void updateEntity(Entity e, const RenderComponent& rc, TransformSystem& transforms, const Camera& camera) {
            if (!rc.model || !rc.visible || !transforms.has(e)) {
                m_cachedProxies.erase(e.id);
                return;
            }

            const DirectX::XMFLOAT4X4 world = buildCameraRelativeWorld(transforms.getWorld(e), camera.getPosition());
            RenderProxy proxy{};
            proxy.model = rc.model;
            proxy.world = world;
            proxy.bounds = rc.model->boundingBox.transform(world);

            m_cachedProxies[e.id] = CachedProxy{ proxy, rc.visible, rc.castsShadow };
        }

        void refreshProxyLists(const Camera& camera) {
            m_visibleProxies.clear();
            m_shadowProxies.clear();

            Frustum frustum;
            frustum.constructFromViewProj(camera.getViewMatrix(), camera.getProjectionMatrix());

            for (const auto& [entityId, cached] : m_cachedProxies) {
                if (cached.castsShadow) {
                    m_shadowProxies.push_back(cached.proxy);
                }

                if (cached.visible && frustum.checkAABB(cached.proxy.bounds)) {
                    m_visibleProxies.push_back(cached.proxy);
                }
            }
        }

        std::unordered_map<uint32_t, CachedProxy> m_cachedProxies;
        Vec3d m_lastCameraPosition{};
        bool m_hasLastCameraPosition = false;
        std::vector<RenderProxy> m_visibleProxies;
        std::vector<RenderProxy> m_shadowProxies;
    };
}