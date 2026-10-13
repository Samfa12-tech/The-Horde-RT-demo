#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>

namespace horde::scene::visibility
{
using Point3 = std::array<double, 3u>;
struct Bounds3 { Point3 minimum{}, maximum{}; };

// A retained opaque quad is a geometric witness, not an origin-domain closure
// certificate. Unsupported/moving/transparent geometry must fail closed.
class OpaqueQuadWitness
{
public:
    static std::optional<OpaqueQuadWitness> Build(
        const std::array<Point3, 6u>& triangles, std::uint8_t visibilityMask,
        std::uint64_t sourceGeneration, bool immutableOpaque)
    {
        if(!immutableOpaque || !visibilityMask || !sourceGeneration) return std::nullopt;
        for(const auto& p:triangles) for(double value:p)
            if(!std::isfinite(value) || std::abs(value)>1000000.0) return std::nullopt;
        // Deliberately restricted to actual axis-aligned rectangular surfaces.
        // A proxy AABB, oblique roof, cut polygon or almost-planar quad cannot
        // stand in for missing geometry. No tolerance fills a hole or seam.
        std::size_t axis=3;
        for(std::size_t a=0;a<3;++a) {
            if(std::all_of(triangles.begin(),triangles.end(),[&](const auto& p){return p[a]==triangles[0][a];})) {
                if(axis!=3) return std::nullopt; // degenerate line/point
                axis=a;
            }
        }
        if(axis==3) return std::nullopt;
        OpaqueQuadWitness result;
        result.axis_=axis;result.plane_=triangles[0][axis];
        result.u_=(axis+1)%3;result.v_=(axis+2)%3;
        for(unsigned i=0;i<2;++i) {
            const auto component=i?result.v_:result.u_;
            result.minimum_[i]=result.maximum_[i]=triangles[0][component];
            for(const auto& p:triangles) {
                result.minimum_[i]=std::min(result.minimum_[i],p[component]);
                result.maximum_[i]=std::max(result.maximum_[i],p[component]);
            }
            if(result.minimum_[i]>=result.maximum_[i]) return std::nullopt;
        }
        std::array<unsigned,6> corners{};
        for(unsigned i=0;i<6;++i) for(unsigned c=0;c<2;++c) {
            const auto component=c?result.v_:result.u_;
            if(triangles[i][component]==result.maximum_[c]) corners[i]|=1u<<c;
            else if(triangles[i][component]!=result.minimum_[c]) return std::nullopt;
        }
        for(unsigned triangle=0;triangle<2;++triangle) {
            const auto i=triangle*3;
            if(corners[i]==corners[i+1] || corners[i]==corners[i+2] || corners[i+1]==corners[i+2])
                return std::nullopt;
        }
        std::array<unsigned,3> shared{};unsigned count=0;
        for(unsigned a=0;a<3;++a) for(unsigned b=3;b<6;++b)
            if(corners[a]==corners[b]) shared[count++]=corners[a];
        // Exactly a shared diagonal and one distinct outer corner on each side
        // proves the two source triangles cover the rectangle without overlap.
        if(count!=2 || (shared[0]^shared[1])!=3) return std::nullopt;
        result.mask_=visibilityMask;result.generation_=sourceGeneration;
        return result;
    }
    bool CoversEverySegment(const Bounds3& origin, const Bounds3& target,
        std::uint8_t rayMask, std::uint64_t sourceGeneration, double guardMetres) const
    {
        if(!rayMask || !(mask_&rayMask) || !sourceGeneration || generation_!=sourceGeneration ||
           !Valid(origin) || !Valid(target) || !std::isfinite(guardMetres) || guardMetres<=0 ||
           minimum_[0]+guardMetres>=maximum_[0]-guardMetres ||
           minimum_[1]+guardMetres>=maximum_[1]-guardMetres) return false;
        const bool forward=origin.minimum[axis_]>plane_+guardMetres &&
                           target.maximum[axis_]<plane_-guardMetres;
        const bool reverse=target.minimum[axis_]>plane_+guardMetres &&
                           origin.maximum[axis_]<plane_-guardMetres;
        if(!forward && !reverse) return false;
        // Reject a guard which disappears at the source float coordinate scale.
        // This is a numeric floor, not a substitute for inflating domains by
        // the actual shader spawn offsets/tMin or proving hardware precision.
        double coordinateScale=std::max(1.0,std::abs(plane_));
        for(unsigned i=0;i<2;++i)
            coordinateScale=std::max({coordinateScale,std::abs(minimum_[i]),std::abs(maximum_[i])});
        for(unsigned i=0;i<3;++i) for(double value :
            {origin.minimum[i],origin.maximum[i],target.minimum[i],target.maximum[i]})
            coordinateScale=std::max(coordinateScale,std::abs(value));
        if(guardMetres<32.0*std::numeric_limits<float>::epsilon()*coordinateScale) return false;
        // The intersection coordinate is linear-fractional in each endpoint
        // coordinate separately. Its denominator keeps one strict sign because
        // the boxes lie on opposite sides, so extrema occur at box vertices.
        // Check all64 vertex pairs, not a sparse set of representative rays.
        for(unsigned a=0;a<8;++a) for(unsigned b=0;b<8;++b) {
            const auto start=Corner(origin,a),end=Corner(target,b);
            const double d0=start[axis_]-plane_,d1=end[axis_]-plane_;
            const double fraction=d0/(d0-d1);
            if(!std::isfinite(fraction) || fraction<=0 || fraction>=1) return false;
            for(unsigned i=0;i<2;++i) {
                const auto component=i?v_:u_;
                const double hit=start[component]+(end[component]-start[component])*fraction;
                if(!std::isfinite(hit) || hit<minimum_[i]+guardMetres || hit>maximum_[i]-guardMetres)
                    return false;
            }
        }
        return true;
    }
private:
    OpaqueQuadWitness()=default;
    static bool Valid(const Bounds3& b) {
        for(unsigned i=0;i<3;++i)
            if(!std::isfinite(b.minimum[i]) || !std::isfinite(b.maximum[i]) ||
               b.minimum[i]>b.maximum[i] || std::abs(b.minimum[i])>1000000.0 ||
               std::abs(b.maximum[i])>1000000.0) return false;
        return true;
    }
    static Point3 Corner(const Bounds3& b,unsigned bits) {
        return {{bits&1?b.maximum[0]:b.minimum[0],bits&2?b.maximum[1]:b.minimum[1],
                 bits&4?b.maximum[2]:b.minimum[2]}};
    }
    std::size_t axis_=0,u_=1,v_=2;
    double plane_=0;
    std::array<double,2> minimum_{},maximum_{};
    std::uint8_t mask_=0;
    std::uint64_t generation_=0;
};
}
