#pragma once
#include "AreaSkillVisuals.h"
#include <Windows.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

// Presentation-only. Ground light is clipped to reached cells; airborne
// embellishments retain their own cell foot anchors and render behind units.
namespace AreaSkillEffectRenderer
{
    constexpr float TailSeconds = AreaSkillVisuals::Lifetime;
    constexpr float ContactFrameSeconds = AreaSkillVisuals::BurstFrameSeconds;
    constexpr int ContactFrames = 8;
    constexpr float ContactSeconds = AreaSkillVisuals::BurstSeconds;
    constexpr float FloorWidthRatio = 1.15f;
    constexpr float FloorHeightRatio = 0.66f;
    struct Options
    {
        BattleSkillArea::Kind kind = BattleSkillArea::Kind::Single;
        BattleActionTiming::ElementVisual element = BattleActionTiming::ElementVisual::Neutral;
        int originX = 0, originY = 0, tileSize = 48, offsetX = 0, offsetY = 0;
        float footRatio = 0.75f;
        COLORREF color = RGB(180,220,255);
        HBITMAP contactBitmap = nullptr;
        int bitmapWidth = 256, bitmapHeight = 128;
        COLORREF transparentColor = RGB(255,0,255);
        bool groundOnly = false;
    };
    constexpr bool IsCellVisible(float elapsed, float arrival)
    { return AreaSkillVisuals::Visible(elapsed-arrival); }
    constexpr int ContactFrame(float elapsed, float arrival)
    { return AreaSkillVisuals::BurstFrame(elapsed-arrival); }
    inline float EndSeconds(const std::vector<BattleSkillArea::Cell>& cells)
    {
        float last = 0.0f;
        for (const auto& cell : cells) last = (std::max)(last,cell.arrivalSeconds);
        return last + TailSeconds;
    }
    namespace Detail
    {
        inline int Round(float value) { return static_cast<int>(std::lround(value)); }
        inline COLORREF Color(AreaSkillVisuals::Color color) { return RGB(color.r,color.g,color.b); }
        constexpr unsigned int BandBit(float alpha)
        { const int band = AreaSkillVisuals::Band(alpha); return band > 0 ? 1u << band : 0u; }
        // One reusable surface per whole pass, not a bitmap/DC per particle.
        // Copying the backdrop before each alpha band preserves true opacity
        // for ordinary GDI geometry and color-key sprites without black fades.
        class Surface
        {
            HDC target = nullptr, layer = nullptr;
            HBITMAP bitmap = nullptr;
            HGDIOBJ previous = nullptr;
            RECT bounds{};
        public:
            Surface(HDC destination, RECT requested) : target(destination)
            {
                RECT clip{};
                if (GetClipBox(target,&clip) == NULLREGION || !IntersectRect(&bounds,&clip,&requested)) return;
                layer = CreateCompatibleDC(target);
                if (!layer) return;
                bitmap = CreateCompatibleBitmap(target,bounds.right-bounds.left,bounds.bottom-bounds.top);
                if (!bitmap) { DeleteDC(layer); layer=nullptr; return; }
                previous = SelectObject(layer,bitmap);
            }
            Surface(const Surface&) = delete;
            Surface& operator=(const Surface&) = delete;
            ~Surface()
            {
                if (layer && previous && previous != HGDI_ERROR) SelectObject(layer,previous);
                if (bitmap) DeleteObject(bitmap);
                if (layer) DeleteDC(layer);
            }
            template<class Pass> void Blend(float opacity, Pass pass)
            {
                if (!layer || !previous || previous == HGDI_ERROR || !(opacity > 0.0f)) return;
                const int width=bounds.right-bounds.left, height=bounds.bottom-bounds.top;
                if (!BitBlt(layer,0,0,width,height,target,bounds.left,bounds.top,SRCCOPY)) return;
                const int saved = SaveDC(layer);
                if (!saved) return;
                SetViewportOrgEx(layer,-bounds.left,-bounds.top,nullptr);
                pass(layer);
                RestoreDC(layer,saved);
                const BLENDFUNCTION blend{AC_SRC_OVER,0,static_cast<BYTE>(Round(AreaSkillVisuals::Clamp(opacity)*255.0f)),0};
                AlphaBlend(target,bounds.left,bounds.top,width,height,layer,0,0,width,height,blend);
            }
        };
        inline void EllipseShape(HDC dc,float x,float y,float width,float height,COLORREF color,bool filled,int stroke=1)
        {
            HPEN pen=CreatePen(PS_SOLID,stroke,color);
            HBRUSH brush=filled?CreateSolidBrush(color):nullptr;
            const auto oldPen=SelectObject(dc,pen);
            const auto oldBrush=SelectObject(dc,filled?brush:GetStockObject(NULL_BRUSH));
            Ellipse(dc,Round(x-width*0.5f),Round(y-height*0.5f),Round(x+width*0.5f),Round(y+height*0.5f));
            SelectObject(dc,oldBrush); SelectObject(dc,oldPen);
            if (brush) DeleteObject(brush);
            if (pen) DeleteObject(pen);
        }
        inline void Poly(HDC dc,const POINT* points,int count,COLORREF color,bool filled,int stroke=1)
        {
            HPEN pen=CreatePen(PS_SOLID,stroke,color);
            HBRUSH brush=filled?CreateSolidBrush(color):nullptr;
            const auto oldPen=SelectObject(dc,pen);
            const auto oldBrush=SelectObject(dc,filled?brush:GetStockObject(NULL_BRUSH));
            if (filled) Polygon(dc,points,count); else Polyline(dc,points,count);
            SelectObject(dc,oldBrush); SelectObject(dc,oldPen);
            if (brush) DeleteObject(brush);
            if (pen) DeleteObject(pen);
        }
        inline void Sprite(HDC dc,HDC scratch,int frame,float x,float y,float width,float height,const Options& options)
        {
            if (!scratch || scratch==dc || !options.contactBitmap || frame<0 || options.bitmapWidth!=256 || options.bitmapHeight!=128) return;
            TransparentBlt(dc,Round(x-width*0.5f),Round(y-height*0.5f),(std::max)(1,Round(width)),(std::max)(1,Round(height)),
                scratch,(frame%4)*64,(frame/4)*64,64,64,options.transparentColor);
        }
        inline void Particle(HDC dc,const AreaSkillVisuals::Particle& p,float footX,float footY,float tile,
            const AreaSkillVisuals::Palette& palette,int index)
        {
            const float x=footX+p.x*tile,y=footY+p.y*tile,r=(std::max)(1.0f,p.size*tile);
            const COLORREF color=Color(index%3==0?palette.bright:palette.accent);
            if (p.shape==AreaSkillVisuals::Shape::Droplet)
                EllipseShape(dc,x,y,r*1.3f,r*2.0f,color,true);
            else if (p.shape==AreaSkillVisuals::Shape::Star)
            {
                const POINT points[8]={{Round(x),Round(y-r*1.8f)},{Round(x+r*.32f),Round(y-r*.32f)},
                    {Round(x+r*1.4f),Round(y)},{Round(x+r*.32f),Round(y+r*.32f)},
                    {Round(x),Round(y+r*1.8f)},{Round(x-r*.32f),Round(y+r*.32f)},
                    {Round(x-r*1.4f),Round(y)},{Round(x-r*.32f),Round(y-r*.32f)}};
                Poly(dc,points,8,color,true);
            }
            else
            {
                const float along=p.shape==AreaSkillVisuals::Shape::Leaf?2.2f:1.4f;
                const POINT points[4]={{Round(x+p.dx*r*along),Round(y+p.dy*r*along)},
                    {Round(x-p.dy*r*.6f),Round(y+p.dx*r*.6f)},
                    {Round(x-p.dx*r*along),Round(y-p.dy*r*along)},
                    {Round(x+p.dy*r*.6f),Round(y-p.dx*r*.6f)}};
                Poly(dc,points,4,color,true);
            }
        }
        inline RECT Bounds(const std::vector<BattleSkillArea::Cell>& cells,float elapsed,const Options& options,bool air)
        {
            RECT bounds{}; bool first=true;
            for (const auto& cell:cells)
            {
                if (!IsCellVisible(elapsed,cell.arrivalSeconds)) continue;
                const int x=options.offsetX+cell.x*options.tileSize,y=options.offsetY+cell.y*options.tileSize;
                const int pad=air?options.tileSize:0;
                const RECT cellBounds{x-pad,y-pad*2,x+options.tileSize+pad,y+options.tileSize};
                if(first){bounds=cellBounds;first=false;}else UnionRect(&bounds,&bounds,&cellBounds);
            }
            return bounds;
        }
        inline void Air(HDC destination,HDC scratch,const std::vector<BattleSkillArea::Cell>& cells,
            float elapsed,const Options& options,bool front)
        {
            Surface surface(destination,Bounds(cells,elapsed,options,!front));
            const auto palette=AreaSkillVisuals::Colors(options.element);
            const float tile=static_cast<float>(options.tileSize);
            unsigned int bands=0;
            for(const auto& cell:cells)
            {
                const float age=elapsed-cell.arrivalSeconds;if(!AreaSkillVisuals::Visible(age))continue;
                const float strength=AreaSkillVisuals::CenterStrength(options.kind,cell.x,cell.y,options.originX,options.originY);
                const float variation=(AreaSkillVisuals::Seed(cell.x,cell.y,31)&255u)/255.0f;
                const auto state=AreaSkillVisuals::SampleCell(options.element,age,strength,variation);
                if(!front)
                {
                    if(state.frame>=0 && scratch && options.contactBitmap)bands|=BandBit(state.plumeAlpha);
                    if(options.element==BattleActionTiming::ElementVisual::Grass)bands|=BandBit(state.beamAlpha);
                    if(options.element==BattleActionTiming::ElementVisual::Light)
                        for(int segment=0;segment<AreaSkillVisuals::LightRaySegments;++segment)
                            bands|=BandBit(AreaSkillVisuals::SampleLightRay(cell.x,cell.y,age,strength,segment).alpha);
                }
                for(int i=0;i<AreaSkillVisuals::ParticleCount(options.element);++i)
                {
                    const auto p=AreaSkillVisuals::SampleParticle(options.element,cell.x,cell.y,i,age,strength);
                    if(p.front==front)bands|=BandBit(p.alpha);
                }
            }
            for(int band=1;band<=AreaSkillVisuals::AlphaBands;++band)
            {
                if(!(bands&(1u<<band)))continue;
                surface.Blend(band/static_cast<float>(AreaSkillVisuals::AlphaBands),[&](HDC dc)
                {
                    for(const auto& cell:cells)
                    {
                        const float age=elapsed-cell.arrivalSeconds;
                        if(!AreaSkillVisuals::Visible(age))continue;
                        const float strength=AreaSkillVisuals::CenterStrength(options.kind,cell.x,cell.y,options.originX,options.originY);
                        const float variation=(AreaSkillVisuals::Seed(cell.x,cell.y,31)&255u)/255.0f;
                        const auto state=AreaSkillVisuals::SampleCell(options.element,age,strength,variation);
                        const float x=options.offsetX+(cell.x+.5f)*tile,y=options.offsetY+(cell.y+options.footRatio)*tile;
                        if(!front)
                        {
                            if(AreaSkillVisuals::Band(state.plumeAlpha)==band)
                                Sprite(dc,scratch,state.frame,x,y-state.plumeHeight*tile*.43f,state.plumeWidth*tile,state.plumeHeight*tile,options);
                            if(options.element==BattleActionTiming::ElementVisual::Light)
                            {
                                for(int segment=0;segment<AreaSkillVisuals::LightRaySegments;++segment)
                                {
                                    const auto ray=AreaSkillVisuals::SampleLightRay(cell.x,cell.y,age,strength,segment);
                                    if(AreaSkillVisuals::Band(ray.alpha)!=band)continue;
                                    const POINT points[2]={{Round(x+ray.x0*tile),Round(y+ray.y0*tile)},
                                        {Round(x+ray.x1*tile),Round(y+ray.y1*tile)}};
                                    Poly(dc,points,2,Color(palette.bright),false,ray.width);
                                }
                            }
                            if(options.element==BattleActionTiming::ElementVisual::Grass && AreaSkillVisuals::Band(state.beamAlpha)==band)
                            {
                                const float h=state.plumeHeight*tile;
                                for(int side:{-1,1})
                                {
                                    const POINT vine[4]={{Round(x),Round(y)},{Round(x+side*tile*.18f),Round(y-h*.3f)},
                                        {Round(x-side*tile*.05f),Round(y-h*.58f)},{Round(x+side*tile*.12f),Round(y-h*.82f)}};
                                    Poly(dc,vine,4,Color(palette.accent),false,2);
                                }
                            }
                        }
                        for(int i=0;i<AreaSkillVisuals::ParticleCount(options.element);++i)
                        {
                            const auto particle=AreaSkillVisuals::SampleParticle(options.element,cell.x,cell.y,i,age,strength);
                            if(particle.front==front && AreaSkillVisuals::Band(particle.alpha)==band)
                                Particle(dc,particle,x,y,tile,palette,i);
                        }
                    }
                });
            }
        }
    }
    inline void Draw(HDC destination,HDC scratch,const std::vector<BattleSkillArea::Cell>& cells,
        float elapsed,const Options& options)
    {
        if(!destination || cells.empty() || options.tileSize<=0)return;
        if(scratch==destination)scratch=nullptr;
        HRGN reached=CreateRectRgn(0,0,0,0); if(!reached)return;
        bool any=false;
        for(const auto& cell:cells)
        {
            if(!IsCellVisible(elapsed,cell.arrivalSeconds))continue;
            const int x=options.offsetX+cell.x*options.tileSize,y=options.offsetY+cell.y*options.tileSize;
            HRGN rectangle=CreateRectRgn(x,y,x+options.tileSize,y+options.tileSize);
            if(rectangle){CombineRgn(reached,reached,rectangle,RGN_OR);DeleteObject(rectangle);any=true;}
        }
        if(!any){DeleteObject(reached);return;}
        const int saved=SaveDC(destination),savedScratch=scratch?SaveDC(scratch):0;
        if(!saved){if(savedScratch)RestoreDC(scratch,savedScratch);DeleteObject(reached);return;}
        if(savedScratch && options.contactBitmap)SelectObject(scratch,options.contactBitmap);
        HDC source = savedScratch && options.contactBitmap ? scratch : nullptr;
        if(ExtSelectClipRgn(destination,reached,RGN_AND)!=ERROR)
        {
            Detail::Surface surface(destination,Detail::Bounds(cells,elapsed,options,false));
            const float tile=static_cast<float>(options.tileSize);
            const auto palette=AreaSkillVisuals::Colors(options.element);
            unsigned int bands=0;
            for(const auto& cell:cells)
            {
                const float age=elapsed-cell.arrivalSeconds;if(!AreaSkillVisuals::Visible(age))continue;
                const float strength=AreaSkillVisuals::CenterStrength(options.kind,cell.x,cell.y,options.originX,options.originY);
                const auto state=AreaSkillVisuals::SampleCell(options.element,age,strength);
                bands|=Detail::BandBit(state.glowAlpha*.34f)|Detail::BandBit(state.glowAlpha*.62f)|
                    Detail::BandBit(state.glowAlpha*.90f)|Detail::BandBit(state.ringAlpha);
                if(state.frame>=0 && source)bands|=Detail::BandBit(state.spriteAlpha);
            }
            for(int band=1;band<=AreaSkillVisuals::AlphaBands;++band)
            {
                if(!(bands&(1u<<band)))continue;
                surface.Blend(band/static_cast<float>(AreaSkillVisuals::AlphaBands),[&](HDC dc)
                {
                    for(const auto& cell:cells)
                    {
                        const float age=elapsed-cell.arrivalSeconds;if(!AreaSkillVisuals::Visible(age))continue;
                        const float strength=AreaSkillVisuals::CenterStrength(options.kind,cell.x,cell.y,options.originX,options.originY);
                        const auto state=AreaSkillVisuals::SampleCell(options.element,age,strength);
                        const float x=options.offsetX+(cell.x+.5f)*tile,y=options.offsetY+(cell.y+options.footRatio-.08f)*tile;
                        for(int glow=0;glow<3;++glow)
                        {
                            const float scales[3]={1.0f,.73f,.42f},alphas[3]={.34f,.62f,.90f};
                            if(AreaSkillVisuals::Band(state.glowAlpha*alphas[glow])==band)
                                Detail::EllipseShape(dc,x,y,state.glowWidth*tile*scales[glow],state.glowHeight*tile*scales[glow],
                                    Detail::Color(glow==2?palette.bright:palette.main),true);
                        }
                        if(AreaSkillVisuals::Band(state.ringAlpha)==band)
                            Detail::EllipseShape(dc,x,y,state.ringWidth*tile,state.ringHeight*tile,Detail::Color(palette.accent),false,2);
                        if(AreaSkillVisuals::Band(state.spriteAlpha)==band)
                            Detail::Sprite(dc,source,state.frame,x,y,state.floorWidth*tile,state.floorHeight*tile,options);
                    }
                });
            }
        }
        RestoreDC(destination,saved);DeleteObject(reached);
        if(!options.groundOnly)Detail::Air(destination,source,cells,elapsed,options,false);
        if(savedScratch)RestoreDC(scratch,savedScratch);
    }
    inline void DrawFront(HDC destination,HDC scratch,const std::vector<BattleSkillArea::Cell>& cells,
        float elapsed,const Options& options)
    {
        if(!destination || cells.empty() || options.tileSize<=0 || options.groundOnly)return;
        Detail::Air(destination,scratch,cells,elapsed,options,true);
    }
    inline void DrawBodyContact(HDC destination,HDC scratch,HBITMAP bitmap,int x,int y,float age,COLORREF key)
    {
        if(!destination || !scratch || scratch==destination || !bitmap || age<0 || age>=AreaSkillVisuals::BodySeconds)return;
        constexpr int size=AreaSkillVisuals::BodyDrawSize;
        const RECT bounds{x-size/2-1,y-size/2-1,x+size/2+1,y+size/2+1};
        Detail::Surface surface(destination,bounds);
        const int saved=SaveDC(scratch);if(!saved)return;
        SelectObject(scratch,bitmap);
        surface.Blend(AreaSkillVisuals::BodyAlpha(age),[&](HDC dc)
        {
            const int frame=AreaSkillVisuals::BodyFrame(age);
            TransparentBlt(dc,x-size/2,y-size/2,size,size,scratch,(frame%4)*64,(frame/4)*64,64,64,key);
        });
        RestoreDC(scratch,saved);
    }
}
