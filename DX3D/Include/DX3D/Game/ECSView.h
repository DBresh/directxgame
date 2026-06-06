#pragma once
#include <DX3D/Game/Entity.h>

namespace dx3d {

    // A lightweight view over two sparse set systems.
    template <typename SysA, typename SysB>
    class ECSView2 {
    public:
        ECSView2(const SysA& a, const SysB& b) : m_sysA(a), m_sysB(b) {}

        // Calls func(Entity, const CompA&, const CompB&) for matches
        template <typename Func>
        void each(Func&& func) const {
            // Iterate the smallest dense set for cache locality
            if (m_sysA.getRawEntities().size() <= m_sysB.getRawEntities().size()) {
                for (Entity e : m_sysA.getRawEntities()) {
                    if (m_sysB.has(e)) {
                        func(e, m_sysA.get(e), m_sysB.get(e));
                    }
                }
            }
            else {
                for (Entity e : m_sysB.getRawEntities()) {
                    if (m_sysA.has(e)) {
                        // Keep component parameter order predictable (SysA, SysB)
                        func(e, m_sysA.get(e), m_sysB.get(e));
                    }
                }
            }
        }

    private:
        const SysA& m_sysA;
        const SysB& m_sysB;
    };

    template <typename SysA, typename SysB>
    ECSView2<SysA, SysB> make_view(const SysA& a, const SysB& b) {
        return ECSView2<SysA, SysB>(a, b);
    }
}