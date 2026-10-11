#pragma once

#include "gameplay/simulation/DevelopmentSupportFixture.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <numeric>
#include <span>
#include <string>
#include <vector>

namespace horde::gameplay::simulation
{
// Immutable CPU authority built from the same world-space triangles admitted by
// the renderer. Owns no GPU handles. Publication/readiness stays with the zone
// owner; preparation alone must never switch live player support.
class TriangleSupportSurface
{
public:
    using Point=std::array<float,3>;
    struct Triangle { std::array<Point,3> points; SupportSurface surface=SupportSurface::Earth; };
    struct Sample
    {
        bool found=false, walkable=false;
        float worldY=0;
        Point normal{};
        SupportSurface surface=SupportSurface::Earth;
        std::uint32_t triangle=0;
        std::uint32_t testedTriangles=0;
    };
    static constexpr float kMaximumWalkableSlope=.65f;
    // Bounded intake capacity, not a renderer/quality budget or runtime growth.
    static constexpr std::size_t kMaximumTriangles=65536;
    static constexpr std::size_t kMaximumSweepIntervals=512;
    static std::shared_ptr<const TriangleSupportSurface> Build(std::span<const Triangle> input,
                                                              std::string& diagnostic)
    {
        if(input.empty() || input.size()>kMaximumTriangles) {
            diagnostic="Terrain support triangle count is outside bounded intake capacity.";return {};
        }
        auto result=std::shared_ptr<TriangleSupportSurface>(new TriangleSupportSurface);
        result->triangles_.assign(input.begin(),input.end());
        result->normals_.reserve(input.size());
        for(const auto& triangle:input) {
            for(const auto& p:triangle.points) for(float c:p)
                if(!std::isfinite(c)||std::abs(c)>1000000.f) {
                    diagnostic="Terrain support contains a nonfinite or unbounded coordinate.";return {};
                }
            if(triangle.surface!=SupportSurface::Stone && triangle.surface!=SupportSurface::Earth &&
               triangle.surface!=SupportSurface::Wood) {
                diagnostic="Terrain support surface classification is invalid.";return {};
            }
            const auto a=triangle.points[0],b=triangle.points[1],c=triangle.points[2];
            const double ux=double(b[0])-a[0],uy=double(b[1])-a[1],uz=double(b[2])-a[2],
                         vx=double(c[0])-a[0],vy=double(c[1])-a[1],vz=double(c[2])-a[2];
            const double nx=uy*vz-uz*vy,ny=uz*vx-ux*vz,nz=ux*vy-uy*vx;
            const double length=std::sqrt(nx*nx+ny*ny+nz*nz);
            if(!std::isfinite(length)||length<=1e-10) {
                diagnostic="Terrain support contains a degenerate triangle.";return {};
            }
            result->normals_.push_back({float(nx/length),float(ny/length),float(nz/length)});
        }
        result->order_.resize(input.size());
        std::iota(result->order_.begin(),result->order_.end(),0u);
        result->nodes_.reserve(NodeCount(input.size()));
        result->BuildNode(0,static_cast<std::uint32_t>(input.size()));
        diagnostic.clear();return result;
    }
    Sample Query(float x,float z,float minimumY=-1000000.f,float maximumY=1000000.f) const
    {
        Sample result;
        if(!std::isfinite(x)||!std::isfinite(z)||!std::isfinite(minimumY)||
           !std::isfinite(maximumY)||minimumY>maximumY) return result;
        QueryNode(0,x,z,minimumY,maximumY,result);return result;
    }
    // Ground-footprint sweep only. Authored obstacle/body-clearance tests remain
    // separate; this cannot certify capsule clearance under branches or roofs.
    bool GroundSweep(float fromX,float fromZ,float toX,float toZ,float supportY,
                     float radius,float maximumStep=.18f) const
    {
        if(!std::isfinite(fromX)||!std::isfinite(fromZ)||!std::isfinite(toX)||!std::isfinite(toZ)||
           !std::isfinite(supportY)||!std::isfinite(radius)||radius<=0||radius>2||
           !std::isfinite(maximumStep)||maximumStep<=0||maximumStep>1) return false;
        const float distance=std::hypot(toX-fromX,toZ-fromZ);
        if(!std::isfinite(distance)||distance>16) return false;
        // Nine support probes are the established footprint approximation, not
        // a full swept capsule. Each probe must now have continuous triangle
        // coverage: arbitrarily thin cracks cannot hide between time samples.
        for(int probe=-1;probe<8;++probe) {
            const float angle=float(probe)*.785398163397f;
            const float dx=probe<0?0:radius*std::cos(angle),dz=probe<0?0:radius*std::sin(angle);
            if(!ContinuousSupport(fromX+dx,fromZ+dz,toX+dx,toZ+dz,maximumStep)) return false;
        }
        const int steps=std::max(1,static_cast<int>(std::ceil(distance/.1f)));
        float previousY=supportY;
        for(int step=0;step<=steps;++step) {
            const float t=float(step)/float(steps),x=fromX+(toX-fromX)*t,z=fromZ+(toZ-fromZ)*t;
            const auto centre=Query(x,z);
            if(!centre.found||!centre.walkable||std::abs(centre.worldY-previousY)>maximumStep+.0001f) return false;
            for(int ring=0;ring<8;++ring) {
                const float angle=float(ring)*.785398163397f;
                const auto edge=Query(x+radius*std::cos(angle),z+radius*std::sin(angle));
                if(!edge.found||!edge.walkable||std::abs(edge.worldY-centre.worldY)>
                   radius*kMaximumWalkableSlope+.001f) return false;
            }
            previousY=centre.worldY;
        }
        return true;
    }
    std::size_t TriangleCount() const { return triangles_.size(); }
    std::size_t RetainedBytes() const {
        return triangles_.capacity()*sizeof(Triangle)+normals_.capacity()*sizeof(Point)+
               order_.capacity()*sizeof(std::uint32_t)+nodes_.capacity()*sizeof(Node);
    }
private:
    struct Interval { double begin=0,end=1,height=0,deltaHeight=0; };
    using Intervals=std::array<Interval,kMaximumSweepIntervals>;
    struct Node { Point minimum{},maximum{};std::uint32_t begin=0,count=0,left=0,right=0; };
    std::vector<Triangle> triangles_;
    std::vector<Point> normals_;
    std::vector<std::uint32_t> order_;
    std::vector<Node> nodes_;
    static std::size_t NodeCount(std::size_t triangles)
    {
        return triangles<=8?1:1+NodeCount(triangles/2)+NodeCount(triangles-triangles/2);
    }
    bool GatherIntervals(std::uint32_t index,double x,double z,double dx,double dz,
                         Intervals& intervals,std::size_t& count) const
    {
        const auto& node=nodes_[index];
        if(std::max(x,x+dx)<node.minimum[0]||std::min(x,x+dx)>node.maximum[0]||
           std::max(z,z+dz)<node.minimum[2]||std::min(z,z+dz)>node.maximum[2]) return true;
        if(!node.count) return GatherIntervals(node.left,x,z,dx,dz,intervals,count)&&
                               GatherIntervals(node.right,x,z,dx,dz,intervals,count);
        for(auto i=node.begin;i<node.begin+node.count;++i) {
            const auto id=order_[i];const auto n=normals_[id];
            if(n[1]<=1e-7f) continue;
            const auto& p=triangles_[id].points;const auto a=p[0],b=p[1],c=p[2];
            const double den=(double(b[2])-c[2])*(double(a[0])-c[0])+(double(c[0])-b[0])*(double(a[2])-c[2]);
            const double u=((double(b[2])-c[2])*(x-c[0])+(double(c[0])-b[0])*(z-c[2]))/den;
            const double v=((double(c[2])-a[2])*(x-c[0])+(double(a[0])-c[0])*(z-c[2]))/den;
            const double du=((double(b[2])-c[2])*dx+(double(c[0])-b[0])*dz)/den;
            const double dv=((double(c[2])-a[2])*dx+(double(a[0])-c[0])*dz)/den;
            Interval interval;
            bool intersects=true;
            for(const auto bary:std::array<std::array<double,2>,3>{{{{u,du}},{{v,dv}},{{1-u-v,-du-dv}}}}) {
                if(std::abs(bary[1])<1e-15) {if(bary[0]<-1e-10) intersects=false;}
                else if(bary[1]>0) interval.begin=std::max(interval.begin,-bary[0]/bary[1]);
                else interval.end=std::min(interval.end,-bary[0]/bary[1]);
            }
            if(!intersects||interval.begin>interval.end) continue;
            // Conservative for layered meshes: an intersected steep upward
            // face is rejected even if another floor might obscure it. This
            // support sweep is intended for the exported terrain heightfield.
            if(std::hypot(n[0],n[2])>kMaximumWalkableSlope*n[1]+1e-7f || count==intervals.size()) return false;
            interval.height=u*a[1]+v*b[1]+(1-u-v)*c[1];
            interval.deltaHeight=du*a[1]+dv*b[1]+(-du-dv)*c[1];
            intervals[count++]=interval;
        }
        return true;
    }
    bool ContinuousSupport(float x,float z,float toX,float toZ,float maximumStep) const
    {
        Intervals intervals;std::size_t count=0;
        if(!GatherIntervals(0,x,z,double(toX)-x,double(toZ)-z,intervals,count)||!count) return false;
        std::sort(intervals.begin(),intervals.begin()+count,[](const auto& a,const auto& b){
            return a.begin==b.begin?a.end<b.end:a.begin<b.begin;
        });
        double covered=0;
        for(std::size_t i=0;i<count;++i) {
            if(intervals[i].begin>covered+1e-9) return false;
            covered=std::max(covered,intervals[i].end);
        }
        if(covered<1-1e-9) return false;
        // Check both sides of every triangle boundary. A short raised sliver
        // must not evade the ordinary height samples either. Crossings of
        // overlapping linear planes are continuous; only endpoints can jump.
        for(std::size_t i=0;i<count;++i) for(double t:{intervals[i].begin,intervals[i].end}) {
            if(t<=0||t>=1) continue;
            double before=-std::numeric_limits<double>::infinity(),after=before;
            for(std::size_t j=0;j<count;++j) {
                const auto& span=intervals[j];const double y=span.height+t*span.deltaHeight;
                if(span.begin<t && span.end>=t-1e-9) before=std::max(before,y);
                if(span.begin<=t+1e-9 && span.end>t) after=std::max(after,y);
            }
            if(!std::isfinite(before)||!std::isfinite(after)||std::abs(after-before)>maximumStep+.0001) return false;
        }
        return true;
    }
    std::uint32_t BuildNode(std::uint32_t begin,std::uint32_t count)
    {
        Node node;node.begin=begin;node.count=count;
        node.minimum.fill(std::numeric_limits<float>::max());
        node.maximum.fill(std::numeric_limits<float>::lowest());
        for(auto i=begin;i<begin+count;++i) for(const auto& p:triangles_[order_[i]].points)
            for(unsigned k=0;k<3;++k) {node.minimum[k]=std::min(node.minimum[k],p[k]);node.maximum[k]=std::max(node.maximum[k],p[k]);}
        const auto index=static_cast<std::uint32_t>(nodes_.size());nodes_.push_back(node);
        if(count>8) {
            const unsigned axis=node.maximum[0]-node.minimum[0]>node.maximum[2]-node.minimum[2]?0:2;
            const auto middle=begin+count/2;
            const auto centroid=[&](std::uint32_t triangle) {
                const auto& p=triangles_[triangle].points;
                return double(p[0][axis])+p[1][axis]+p[2][axis];
            };
            std::nth_element(order_.begin()+begin,order_.begin()+middle,order_.begin()+begin+count,
                [&](auto a,auto b){const auto ca=centroid(a),cb=centroid(b);return ca==cb?a<b:ca<cb;});
            const auto left=BuildNode(begin,middle-begin),right=BuildNode(middle,begin+count-middle);
            nodes_[index].count=0;nodes_[index].left=left;nodes_[index].right=right;
        }
        return index;
    }
    void QueryNode(std::uint32_t index,float x,float z,float minY,float maxY,Sample& result) const
    {
        const auto& node=nodes_[index];
        if(x<node.minimum[0]||x>node.maximum[0]||z<node.minimum[2]||z>node.maximum[2]||
           node.maximum[1]<minY||node.minimum[1]>maxY) return;
        if(!node.count) {QueryNode(node.left,x,z,minY,maxY,result);QueryNode(node.right,x,z,minY,maxY,result);return;}
        for(auto i=node.begin;i<node.begin+node.count;++i) {
            const auto id=order_[i];const auto n=normals_[id];
            ++result.testedTriangles;
            if(n[1]<=1e-7f) continue; // Downward shell/vertical banks cannot be a floor.
            const auto& p=triangles_[id].points;const auto a=p[0],b=p[1],c=p[2];
            const double den=(double(b[2])-c[2])*(double(a[0])-c[0])+(double(c[0])-b[0])*(double(a[2])-c[2]);
            const double u=((double(b[2])-c[2])*(double(x)-c[0])+(double(c[0])-b[0])*(double(z)-c[2]))/den;
            const double v=((double(c[2])-a[2])*(double(x)-c[0])+(double(a[0])-c[0])*(double(z)-c[2]))/den;
            if(std::min({u,v,1-u-v}) < -1e-7) continue;
            const float y=float(u*a[1]+v*b[1]+(1-u-v)*c[1]);
            if(y<minY||y>maxY||(result.found&&(y<result.worldY||(y==result.worldY&&id>result.triangle)))) continue;
            result.found=true;result.worldY=y;result.normal=n;result.triangle=id;
            result.walkable=std::hypot(n[0],n[2])<=kMaximumWalkableSlope*n[1]+1e-7f;
            result.surface=triangles_[id].surface;
        }
    }
};
}
